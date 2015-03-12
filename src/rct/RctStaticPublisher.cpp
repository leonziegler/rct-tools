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
#include <rsc/logging/Logger.h>
#include <iostream>
#include <csignal>

using namespace boost::program_options;
using namespace boost::filesystem;
using namespace std;
using namespace rsc::logging;
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

	Logger::getLogger("")->setLevel(Logger::LEVEL_WARN);
	if (vm.count("debug")) {
		Logger::getLogger("rct")->setLevel(Logger::LEVEL_DEBUG);
	} else if (vm.count("trace")) {
		Logger::getLogger("rct")->setLevel(Logger::LEVEL_TRACE);
	} else if (vm.count("info")) {
		Logger::getLogger("rct")->setLevel(Logger::LEVEL_INFO);
	}


	string configFile = vm["config"].as<string>();
	LoggerPtr logger = Logger::getLogger("rct.RctStaticPublisher");

	// register signal SIGINT and signal handler
	signal(SIGINT, signalHandler);

	std::vector<Parser::Ptr> parsers;
	parsers.push_back(rct::ParserINI::Ptr(new rct::ParserINI()));
	parsers.push_back(rct::ParserXML::Ptr(new rct::ParserXML()));

	try {

		TransformPublisher::Ptr publisher = getTransformerFactory().createTransformPublisher(name);

		RSCDEBUG(logger, "reading config file: " << configFile)
		ParserResultTransforms result;
		vector<Parser::Ptr>::iterator it;
		for (it = parsers.begin(); it != parsers.end(); ++it) {
			Parser::Ptr p = *it;
			if (p->canParse(configFile)){
				result =p->parseStaticTransforms(configFile);
				break;
			}
		}

		if (result.transforms.empty()) {
			RSCERROR(logger, "no transforms to publish")
		} else {
			cout << "successfully started" << endl;
			publisher->sendTransform(result.transforms, rct::STATIC);
		}

		// run until interrupted
		while (running) {
			sleep(1);
		}
		RSCDEBUG(logger, "interrupted");

	} catch (std::exception &e) {
		cerr << "Error:\n  " << e.what() << "\n" << endl;
		return 1;
	}
	cout << "done" << endl;
	return 0;
}
