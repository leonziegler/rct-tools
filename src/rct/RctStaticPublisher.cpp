/*
 * RctStaticPublisher.cpp
 *
 *  Created on: Dec 16, 2014
 *      Author: leon
 */

#include "parsers/ParserINI.h"
#include "parsers/ParserXML.h"
#include <rct/rct.h>

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
using namespace rct;

bool running = true;

void signalHandler(int signum) {
	cout << "Interrupt signal (" << signum << ") received." << endl;
	running = false;
}

int main(int argc, char **argv) {
	options_description desc("Allowed options");
	variables_map vm;

	desc.add_options()("help,h", "produce help message") // help
	("config,c", value<string>(), "a single config file") // config file
	("name,n", value<string>(), "name for this instance") // config file
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

	if (!vm.count("config")) {
		cerr << "\nERROR: --config must be set!" << endl
				<< endl;
		cout << desc << endl;
	}
	string name = "static-publisher";
	if (vm.count("name")) {
		name = vm["name"].as<string>();
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
	string configFile = vm["config"].as<string>();
	log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("rct.RctStaticPublisher");

	// register signal SIGINT and signal handler
	signal(SIGINT, signalHandler);

	std::vector<Parser::Ptr> parsers;
	parsers.push_back(rct::ParserINI::Ptr(new rct::ParserINI()));
	parsers.push_back(rct::ParserXML::Ptr(new rct::ParserXML()));

	try {

		TransformPublisher::Ptr publisher = getTransformerFactory().createTransformPublisher(name);

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
			cout << "successfully started" << endl;
			publisher->sendTransform(result.transforms, rct::STATIC);
		}

		// run until interrupted
		while (running) {
			sleep(1);
		}
		LOG4CXX_DEBUG(logger, "interrupted");

	} catch (std::exception &e) {
		cerr << "Error:\n  " << e.what() << "\n" << endl;
		return 1;
	}
	cout << "done" << endl;
	return 0;
}
