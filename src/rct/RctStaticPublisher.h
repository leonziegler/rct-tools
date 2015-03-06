/*
 * RctStaticPublisher.h
 *
 *  Created on: Dec 16, 2014
 *      Author: leon
 */

#pragma once
#include <string>
#include <vector>
#include <rct/TransformerFactory.h>
#include <boost/thread.hpp>
#include <log4cxx/logger.h>

#include "parsers/Parser.h"

namespace rct {

class RctStaticPublisher;

class TransformWrapper: public Transform {
public:
	TransformWrapper(const Transform& t, bool isStatic): Transform(t), isStatic(isStatic) {
	}
	bool isStatic;
};

class Handler: public TransformListener {
public:
	typedef boost::shared_ptr<Handler> Ptr;
	Handler(RctStaticPublisher* parent): parent(parent) {
	}
	virtual ~Handler() {
	}
	void newTransformAvailable(const Transform& transform, bool isStatic);
	bool hasTransforms();
	TransformWrapper nextTransform();
private:
	RctStaticPublisher* parent;
	boost::mutex mutex;
	std::vector<TransformWrapper> transforms;
};

class RctStaticPublisher {
public:
	RctStaticPublisher(const std::string &configFile, const std::string &name = "rct-static-publisher", bool bridge = false);
	virtual ~RctStaticPublisher();

	void run();
	void interrupt();
	void notify();
	void addParser(const Parser::Ptr &p);
private:
	std::string configFile;
	Transformer::Ptr transformerRsb;
	TransformCommunicator::Ptr commRos;
	Handler::Ptr rosHandler;
	Handler::Ptr rsbHandler;
	bool bridge;
	bool interrupted;

	boost::condition_variable cond;
	boost::mutex mutex;

	std::vector<Parser::Ptr> parsers;

	static log4cxx::LoggerPtr logger;
};

} /* namespace rct */
