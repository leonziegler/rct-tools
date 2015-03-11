/*
 * RctRosBridge.cpp
 *
 *  Created on: Dec 16, 2014
 *      Author: leon
 */

#include "RctRosBridge.h"
#include <rct/rctConfig.h>
#include <rct/impl/TransformCommRsb.h>
#include <rct/impl/TransformCommRos.h>
#include <rct/impl/TransformerTF2.h>

#include <boost/program_options.hpp>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <log4cxx/log4cxx.h>
#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/consoleappender.h>
#include <log4cxx/patternlayout.h>
#include <iostream>
#include <csignal>

using namespace boost::program_options;
using namespace boost::filesystem;
using namespace std;
using namespace log4cxx;

rct::RctRosBridge *bridge;

void signalHandler(int signum) {
	cout << "Interrupt signal (" << signum << ") received." << endl;
	bridge->interrupt();
}

int main(int argc, char **argv) {
	options_description desc("Allowed options");
	variables_map vm;

	desc.add_options()("help,h", "produce help message") // help
	("debug", "debug mode") //debug
	("trace", "trace mode") //trace
	("info", "info mode")
	("log-prop,l", value<string>(), "logging properties (log4cxx)");

	store(parse_command_line(argc, argv, desc), vm);
	notify(vm);

	if (vm.count("help")) {
		cout << "Usage:\n  " << argv[0] << " [options]\n" << endl;
		cout << desc << endl;
		return 0;
	}

	LayoutPtr pattern(new PatternLayout("%r [%t] %-5p %c - %m%n"));
	AppenderPtr appender(new ConsoleAppender(pattern));
	Logger::getRootLogger()->addAppender(appender);

	if (vm.count("debug")) {
		Logger::getRootLogger()->setLevel(Level::getDebug());
	} else if (vm.count("trace")) {
		Logger::getRootLogger()->setLevel(Level::getTrace());
	} else if (vm.count("info")) {
		Logger::getRootLogger()->setLevel(Level::getInfo());
	} else if (vm.count("log-prop")) {
		string properties = vm["log-prop"].as<string>();
		cout << "Using logging properties: " << properties << endl;
		PropertyConfigurator::configure(properties);
	} else {
		Logger::getRootLogger()->setLevel(Level::getWarn());
	}

	try {

		map<string, string> remappings;
		ros::init(argc, argv, "rctrosbridge");

		bridge = new rct::RctRosBridge("rctrosbridge");

		// register signal SIGINT and signal handler
		signal(SIGINT, signalHandler);

		ros::AsyncSpinner spinner(4);
		spinner.start();

		cout << "successfully started" << endl;

		// block
		bool ret = bridge->run();

		if (!ret) {
			cout << "done" << endl;
		}
		return ret;

	} catch (std::exception &e) {
		cerr << "Error:\n  " << e.what() << "\n" << endl;
		return 1;
	}
}

namespace rct {

log4cxx::LoggerPtr RctRosBridge::logger = log4cxx::Logger::getLogger("rct.RctRosBridge");

RctRosBridge::RctRosBridge(const string &name, bool rosLegacyMode, long rosLegacyIntervalMSec) :
		interrupted(false) {

	rosHandler = Handler::Ptr(new Handler(this, "Ros"));
	rsbHandler = Handler::Ptr(new Handler(this, "Rsb"));

	TransformerConfig configRsb;
	configRsb.setCommType(TransformerConfig::RSB);
	commRsb = TransformCommRsb::Ptr(new TransformCommRsb(name, rsbHandler));
	commRsb->init(configRsb);

	TransformerConfig configRos;
	configRos.setCommType(TransformerConfig::ROS);
	commRos = TransformCommRos::Ptr(new TransformCommRos(name, configRos.getCacheTime(), rosHandler, rosLegacyMode, rosLegacyIntervalMSec));
	commRos->init(configRos);
}

void RctRosBridge::notify() {
	boost::mutex::scoped_lock lock(mutex);
	cond.notify_all();
}

bool RctRosBridge::run() {

	LOG4CXX_INFO(logger, "start running");

	// run until interrupted
	while (!interrupted && ros::ok()) {
		boost::mutex::scoped_lock lock(mutex);
		// wait for notification
		LOG4CXX_TRACE(logger, "wait");
		cond.wait(lock);
		LOG4CXX_TRACE(logger, "notified");
		if (bridge) {
			while (rsbHandler->hasTransforms()) {
				LOG4CXX_DEBUG(logger, "rsb handler has transforms");
				TransformWrapper t = rsbHandler->nextTransform();
				if (t.getAuthority() != commRsb->getAuthorityName()) {
					TransformType type = STATIC;
					if (!t.isStatic) {
						type = DYNAMIC;
						LOG4CXX_DEBUG(logger, "publish dynamic transform " << t);
					} else {
						LOG4CXX_DEBUG(logger, "publish static transform " << t);
					}
					t.setAuthority(string("rct:") + t.getAuthority());
					try {
						commRos->sendTransform(t, type);
					} catch (std::exception& e) {
						LOG4CXX_TRACE(logger, "Error sending transform. Reason: " << e.what());
					}
				} else {
					LOG4CXX_TRACE(logger,
							"skip bridging of transform from rsb to ros because own authority: " << t.getAuthority());
				}
			}
			while (rosHandler->hasTransforms()) {
				LOG4CXX_DEBUG(logger, "ros handler has transforms");
				TransformWrapper t = rosHandler->nextTransform();
				if (t.getAuthority() != commRos->getAuthorityName()) {
					TransformType type = STATIC;
					if (!t.isStatic) {
						type = DYNAMIC;
					}
					t.setAuthority(string("ros:") + t.getAuthority());
					try {
						commRsb->sendTransform(t, type);
					} catch (std::exception& e) {
						LOG4CXX_TRACE(logger, "Error sending transform. Reason: " << e.what());
					}
				} else {
					LOG4CXX_TRACE(logger,
							"skip bridging of transform from ros to rsb because own authority: " << t.getAuthority());
				}
			}
		}
		LOG4CXX_TRACE(logger, "loop done");
	}
	LOG4CXX_WARN(logger, "interrupted");
	LOG4CXX_INFO(logger, "shutdown");
	LOG4CXX_TRACE(logger, "shutdown rsb communicator");
	commRsb->shutdown();
	LOG4CXX_TRACE(logger, "shutdown ros communicator");
	commRos->shutdown();

	if (!ros::ok()) {
		LOG4CXX_WARN(logger, "Shutdown request received from ROS");
		cerr << "Shutdown request received from ROS" << endl;
		return 1;
	}
	LOG4CXX_INFO(logger, "done");
	return 0;
}
void RctRosBridge::interrupt() {
	interrupted = true;
	notify();
}

RctRosBridge::~RctRosBridge() {
}

void Handler::newTransformAvailable(const Transform& transform, bool isStatic) {
	LOG4CXX_TRACE(logger, "newTransformAvailable()");
	{
		boost::mutex::scoped_lock lock(mutexHandler);
		TransformWrapper w(transform, isStatic);
		transforms.push_back(w);
	}
	parent->notify();
}
bool Handler::hasTransforms() {
	boost::mutex::scoped_lock lock(mutexHandler);
	return !transforms.empty();
}

TransformWrapper Handler::nextTransform() {
	if (!hasTransforms()) {
		throw std::range_error("no transforms available");
	}
	boost::mutex::scoped_lock lock(mutexHandler);
	TransformWrapper ret = *transforms.begin();
	transforms.erase(transforms.begin());
	return ret;
}

} /* namespace rct */
