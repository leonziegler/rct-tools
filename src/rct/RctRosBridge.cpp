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
#include <rsc/logging/Logger.h>
#include <rsc/logging/OptionBasedConfigurator.h>
#include <rsc/logging/LoggerFactory.h>
#include <iostream>
#include <csignal>

using namespace boost::program_options;
using namespace boost::filesystem;
using namespace std;
using namespace rsc::logging;

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
	("log-prop,l", value<string>(), "logging properties file");

	store(parse_command_line(argc, argv, desc), vm);
	notify(vm);

	if (vm.count("help")) {
		cout << "Usage:\n  " << argv[0] << " [options]\n" << endl;
		cout << desc << endl;
		return 0;
	}
	Logger::getLogger("")->setLevel(Logger::LEVEL_WARN);
	if (vm.count("debug")) {
		Logger::getLogger("rct")->setLevel(Logger::LEVEL_DEBUG);
	} else if (vm.count("trace")) {
		Logger::getLogger("rct")->setLevel(Logger::LEVEL_TRACE);
	} else if (vm.count("info")) {
		Logger::getLogger("rct")->setLevel(Logger::LEVEL_INFO);
	} else if (vm.count("log-prop")) {
		string properties = vm["log-prop"].as<string>();
		cout << "Using logging properties: " << properties << endl;
		LoggerFactory::getInstance().reconfigureFromFile(properties);
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

rsc::logging::LoggerPtr RctRosBridge::logger = rsc::logging::Logger::getLogger("rct.RctRosBridge");

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

	RSCINFO(logger, "start running");

	// run until interrupted
	while (!interrupted && ros::ok()) {
		boost::mutex::scoped_lock lock(mutex);
		// wait for notification
		RSCTRACE(logger, "wait");
		cond.wait(lock);
		RSCTRACE(logger, "notified");
		if (bridge) {
			while (rsbHandler->hasTransforms()) {
				RSCDEBUG(logger, "rsb handler has transforms");
				TransformWrapper t = rsbHandler->nextTransform();
				if (t.getAuthority() != commRsb->getAuthorityName()) {
					TransformType type = STATIC;
					if (!t.isStatic) {
						type = DYNAMIC;
						RSCDEBUG(logger, "publish dynamic transform " << t);
					} else {
						RSCDEBUG(logger, "publish static transform " << t);
					}
					t.setAuthority(string("rct:") + t.getAuthority());
					try {
						commRos->sendTransform(t, type);
					} catch (std::exception& e) {
						RSCTRACE(logger, "Error sending transform. Reason: " << e.what());
					}
				} else {
					RSCTRACE(logger,
							"skip bridging of transform from rsb to ros because own authority: " << t.getAuthority());
				}
			}
			while (rosHandler->hasTransforms()) {
				RSCDEBUG(logger, "ros handler has transforms");
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
						RSCTRACE(logger, "Error sending transform. Reason: " << e.what());
					}
				} else {
					RSCTRACE(logger,
							"skip bridging of transform from ros to rsb because own authority: " << t.getAuthority());
				}
			}
		}
		RSCTRACE(logger, "loop done");
	}
	RSCWARN(logger, "interrupted");
	RSCINFO(logger, "shutdown");
	RSCTRACE(logger, "shutdown rsb communicator");
	commRsb->shutdown();
	RSCTRACE(logger, "shutdown ros communicator");
	commRos->shutdown();

	if (!ros::ok()) {
		RSCWARN(logger, "Shutdown request received from ROS");
		cerr << "Shutdown request received from ROS" << endl;
		return 1;
	}
	RSCINFO(logger, "done");
	return 0;
}
void RctRosBridge::interrupt() {
	interrupted = true;
	notify();
}

RctRosBridge::~RctRosBridge() {
}

void Handler::newTransformAvailable(const Transform& transform, bool isStatic) {
	RSCTRACE(logger, "newTransformAvailable()");
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
