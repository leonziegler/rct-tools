/*
 * RctRosBridge.cpp
 *
 *  Created on: Dec 16, 2014
 *      Author: leon
 */

#include <rct/rctConfig.h>
#include <rct/impl/TransformCommRsb.h>

#include "parsers/ParserINI.h"
#include "parsers/ParserXML.h"
#include <rct/rct.h>

#include <boost/program_options.hpp>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <rsc/logging/Logger.h>
#include <rsc/logging/LoggerFactory.h>
#include <rsc/misc/SignalWaiter.h>
#include <iostream>
#include <csignal>

#include <rsb/Handler.h>
#include <rsb/Listener.h>
#include <rsb/MetaData.h>
#include <rsb/Factory.h>
#include <rsb/converter/ProtocolBufferConverter.h>
#include <rsb/converter/Repository.h>

#include <rst/geometry/Pose.pb.h>

using namespace boost::program_options;
using namespace boost::filesystem;
using namespace std;
using namespace rsc::logging;
using namespace rsb;
using namespace rsb::converter;
using namespace rst::geometry;
using namespace rct;

TransformPublisher::Ptr publisher;
map<Scope, ParserResultMessage> messageMapping;
LoggerPtr logger = Logger::getLogger("rct.RctFromRst");

Converter<string>::Ptr converterPose;

void handlePose(boost::shared_ptr<Pose> p, ParserResultMessage message, boost::uint64_t time_usec) {

	boost::posix_time::ptime time(boost::gregorian::date(1970,1,1));
	time += boost::posix_time::microseconds(time_usec);

	// read data
	double x = p->translation().x();
	double y = p->translation().y();
	double z = p->translation().z();
	double qw =p->rotation().qw();
	double qx =p->rotation().qx();
	double qy =p->rotation().qy();
	double qz =p->rotation().qz();

	// create a transform
	Eigen::Vector3d translation(x,y,z);
	Eigen::Quaterniond rotation(qw,qx,qy,qz);
	Eigen::Affine3d affine = Eigen::Affine3d().fromPositionOrientationScale(translation, rotation,
						Eigen::Vector3d::Ones());
	rct::Transform transform(affine, message.parent, message.child, time);
	transform.setAuthority(message.authority);

	// publish the transform
	publisher->sendTransform(transform, rct::DYNAMIC);
}

void handleEvent(EventPtr e) {

	// find message for source
	Scope scope = e->getScope();
	boost::uint64_t time_usec = e->getMetaData().getCreateTime();
	ParserResultMessage message;
	map<Scope, ParserResultMessage>::iterator msgIt;
	for (msgIt = messageMapping.begin(); msgIt != messageMapping.end(); ++msgIt) {
		if (msgIt->first == scope || msgIt->first.isSuperScopeOf(scope)) {
			message = msgIt->second;
			break;
		}
	}
	if (message.scope.empty()) {
		RSCERROR(logger, "No known message configuration for scope: " << scope);
		return;
	}

	if (e->getType() == converterPose->getDataType()) {
		handlePose(boost::static_pointer_cast<Pose>(e->getData()), message, time_usec);
	}
}

void createMessageMapping(vector<ParserResultMessage> messages) {
	vector<ParserResultMessage>::iterator msgIt;
	for (msgIt = messages.begin(); msgIt != messages.end(); ++msgIt) {
		messageMapping[Scope(msgIt->scope)] = *msgIt;
	}
}

void parse(const string &configFile) {

	std::vector<Parser::Ptr> parsers;
	parsers.push_back(rct::ParserINI::Ptr(new rct::ParserINI()));
	parsers.push_back(rct::ParserXML::Ptr(new rct::ParserXML()));

	RSCDEBUG(logger, "reading config file: " << configFile)
	vector<ParserResultMessage> result;
	vector<Parser::Ptr>::iterator it;
	for (it = parsers.begin(); it != parsers.end(); ++it) {
		Parser::Ptr p = *it;
		if (p->canParse(configFile)) {
			result = p->parseConvertMessages(configFile);
			break;
		}
	}

	createMessageMapping(result);
}

int main(int argc, char **argv) {
	options_description desc("Allowed options");
	variables_map vm;

	desc.add_options()("help,h", "produce help message") // help
	("config,c", value<string>(), "a single config file") // config file
	("debug", "debug mode") //debug
	("trace", "trace mode") //trace
	("info", "info mode")("log-prop,l", value<string>(), "logging properties");

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

	string configFile = vm["config"].as<string>();
	string name = "rst-converter";
	if (vm.count("name")) {
		name = vm["name"].as<string>();
	}

	rsc::misc::initSignalWaiter();

	try {

		parse(configFile);
		publisher = getTransformerFactory().createTransformPublisher(name);

		converterPose = Converter<string>::Ptr(new ProtocolBufferConverter<Pose>());
		converterRepository<string>()->registerConverter(converterPose);

		vector<ListenerPtr> listeners;
		Factory& factory = getFactory();
		map<Scope, ParserResultMessage>::iterator msgIt;
		for (msgIt = messageMapping.begin(); msgIt != messageMapping.end(); ++msgIt) {
			Scope scope = msgIt->first;
			ParserResultMessage msg = msgIt->second;
			cout << msg.parent << msg.child << msg.scope << msg.authority << endl;
			ListenerPtr listener = factory.createListener(scope);
			listener->addHandler(HandlerPtr(new EventFunctionHandler(&handleEvent)));
			listeners.push_back(listener);
		}

		return rsc::misc::suggestedExitCode(rsc::misc::waitForSignal());

	} catch (std::exception &e) {
		cerr << "Error:\n  " << e.what() << "\n" << endl;
		return 1;
	}
}
