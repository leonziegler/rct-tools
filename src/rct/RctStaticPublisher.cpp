/*
 * RctStaticPublisher.cpp
 *
 *  Created on: Dec 16, 2014
 *      Author: leon
 */

#include "RctStaticPublisher.h"
#include "parsers/ParserINI.h"
#include "parsers/ParserXML.h"
#include <rct/rctConfig.h>

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

rct::RctStaticPublisher *publisher;

void signalHandler(int signum) {
	cout << "Interrupt signal (" << signum << ") received." << endl;
	publisher->interrupt();
}

int main(int argc, char **argv) {
	options_description desc("Allowed options");
	variables_map vm;

	desc.add_options()("help,h", "produce help message") // help
	("config,c", value<string>(), "a single config file") // config file
	("bridge", "rsb/ros bride mode") //bridge
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

	if (!vm.count("dir") && !vm.count("config")) {
		cerr << "\nERROR: either --dir or --config must be set!" << endl
				<< endl;
		cout << desc << endl;
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

		publisher = new rct::RctStaticPublisher(vm["config"].as<string>(), vm.count("bridge"));

		publisher->addParser(rct::ParserINI::Ptr(new rct::ParserINI()));
		publisher->addParser(rct::ParserXML::Ptr(new rct::ParserXML()));

		// register signal SIGINT and signal handler
		signal(SIGINT, signalHandler);

		// block
		publisher->run();

	} catch (std::exception &e) {
		cerr << "Error:\n  " << e.what() << "\n" << endl;
		return 1;
	}

	return 0;
}

namespace rct {

log4cxx::LoggerPtr RctStaticPublisher::logger = log4cxx::Logger::getLogger("rct.RctStaticPublisher");

RctStaticPublisher::RctStaticPublisher(const string &configFile, bool bridge) :
		configFile(configFile), bridge(bridge), interrupted(false) {

	TransformerConfig configRsb;
	configRsb.setCommType(TransformerConfig::RSB);

	if (bridge) {
#ifdef RCT_HAVE_ROS
		rosHandler = Handler::Ptr(new Handler(this));
		rsbHandler = Handler::Ptr(new Handler(this));

		transformerRsb = getTransformerFactory().createTransformer(rsbHandler, configRsb);

		TransformerConfig configRos;
		configRos.setCommType(TransformerConfig::ROS);
		commRos = TransformCommRos::Ptr(new TransformCommRos(configRos.getCacheTime(), rosHandler));
#else
		throw TransformerFactoryException("Can not activate bridge mode, because ROS implementation is not present!");
#endif
	} else {
		transformerRsb = getTransformerFactory().createTransformer(configRsb);
	}
}

void RctStaticPublisher::notify() {
	boost::mutex::scoped_lock lock(mutex);
	cond.notify_all();
}

void RctStaticPublisher::run() {

	LOG4CXX_DEBUG(logger, "reading config file: " << configFile)
	ParserResult result;
	vector<Parser::Ptr>::iterator it;
	for (it = parsers.begin(); it != parsers.end(); ++it) {
		Parser::Ptr p = *it;
		if (p->canParse(configFile)){
			result =p->parse(configFile);
			break;
		}
	}

	if (result.transforms.empty()) {
		LOG4CXX_ERROR(logger, "no transforms to publish")
	} else {
		transformerRsb->sendTransform(result.transforms, true);
	}

	// run until interrupted
	while (!interrupted) {
		boost::mutex::scoped_lock lock(mutex);
		// wait for notification
		cond.wait(lock);
		LOG4CXX_DEBUG(logger, "notified");
		if (bridge) {
			while(rsbHandler->hasTransforms()) {
				TransformWrapper t = rsbHandler->nextTransform();
				if (t.getAuthority() != transformerRsb->getAuthorityName()) {
					commRos->sendTransform(t, t.isStatic);
				} else {
					LOG4CXX_TRACE(logger, "skip bridging of transform from rsb to ros because own authority: " << t.getAuthority());
				}
			}
			while(rosHandler->hasTransforms()) {
				TransformWrapper t = rosHandler->nextTransform();
				if (t.getAuthority() != commRos->getAuthorityName()) {
					transformerRsb->sendTransform(t, t.isStatic);
				} else {
					LOG4CXX_TRACE(logger, "skip bridging of transform from ros to rsb because own authority: " << t.getAuthority());
				}
			}
		}
	}
	LOG4CXX_DEBUG(logger, "interrupted");
}
void RctStaticPublisher::interrupt() {
	interrupted = true;
	notify();
}
void RctStaticPublisher::addParser(const Parser::Ptr &p) {
	parsers.push_back(p);
}
RctStaticPublisher::~RctStaticPublisher() {
}

void Handler::newTransformAvailable(const Transform& transform, bool isStatic) {
	boost::mutex::scoped_lock lock(mutex);
	TransformWrapper w(transform, isStatic);
	transforms.push_back(w);
	parent->notify();
}
bool Handler::hasTransforms() {
	boost::mutex::scoped_lock lock(mutex);
	return !transforms.empty();
}

TransformWrapper Handler::nextTransform() {
	if (!hasTransforms()) {
		throw std::range_error("no transforms available");
	}
	boost::mutex::scoped_lock lock(mutex);
	TransformWrapper ret = *transforms.begin();
	transforms.erase(transforms.begin());
	return ret;
}

} /* namespace rct */
