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

#include <boost/program_options.hpp>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <log4cxx/log4cxx.h>
#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
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
	("info", "info mode");

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
	} else {
		Logger::getRootLogger()->setLevel(Level::getWarn());
	}

	try {

		map<string, string> remappings;
		ros::init(argc, argv, "rctrosbridge");

		bridge = new rct::RctRosBridge("rctrosbridge");

		// register signal SIGINT and signal handler
		signal(SIGINT, signalHandler);

		// block
		bridge->run();

	} catch (std::exception &e) {
		cerr << "Error:\n  " << e.what() << "\n" << endl;
		return 1;
	}

	return 0;
}

namespace rct {

log4cxx::LoggerPtr RctRosBridge::logger = log4cxx::Logger::getLogger("rct.RctRosBridge");

RctRosBridge::RctRosBridge(const string &name) :
		interrupted(false) {

	TransformerConfig configRsb;
	configRsb.setCommType(TransformerConfig::RSB);

	rosHandler = Handler::Ptr(new Handler(this));
	rsbHandler = Handler::Ptr(new Handler(this));

	transformerRsb = getTransformerFactory().createTransformer(name, rsbHandler, configRsb);

	TransformerConfig configRos;
	configRos.setCommType(TransformerConfig::ROS);
	commRos = TransformCommRos::Ptr(new TransformCommRos(name, configRos.getCacheTime(), rosHandler));
	commRos->init(configRos);
}

void RctRosBridge::notify() {
	boost::mutex::scoped_lock lock(mutex);
	cond.notify_all();
}

void RctRosBridge::run() {

	// run until interrupted
	while (!interrupted) {
		boost::mutex::scoped_lock lock(mutex);
		// wait for notification
		cond.wait(lock);
		LOG4CXX_DEBUG(logger, "notified");
		if (bridge) {
			while (rsbHandler->hasTransforms()) {
				LOG4CXX_DEBUG(logger, "rsb handler has transforms");
				TransformWrapper t = rsbHandler->nextTransform();
				if (t.getAuthority() != transformerRsb->getAuthorityName()) {
					TransformType type = STATIC;
					if (!t.isStatic) {
						type = DYNAMIC;
					}
					t.setAuthority(string("rct:") + t.getAuthority());
					commRos->sendTransform(t, type);
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
					transformerRsb->sendTransform(t, type);
				} else {
					LOG4CXX_TRACE(logger,
							"skip bridging of transform from ros to rsb because own authority: " << t.getAuthority());
				}
			}
		}
		LOG4CXX_TRACE(logger, "loop done");
	}
	LOG4CXX_TRACE(logger, "interrupted");
}
void RctRosBridge::interrupt() {
	interrupted = true;
	notify();
}

RctRosBridge::~RctRosBridge() {
}

void Handler::newTransformAvailable(const Transform& transform, bool isStatic) {
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
