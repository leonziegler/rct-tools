/*
 * RctRosBridge.h
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

namespace rct {

class RctRosBridge;

class TransformWrapper: public Transform {
public:
	TransformWrapper(const Transform& t, bool isStatic): Transform(t), isStatic(isStatic) {
	}
	bool isStatic;
};

class Handler: public TransformListener {
public:
	typedef boost::shared_ptr<Handler> Ptr;
	Handler(RctRosBridge* parent): parent(parent) {
	}
	virtual ~Handler() {
	}
	void newTransformAvailable(const Transform& transform, bool isStatic);
	bool hasTransforms();
	TransformWrapper nextTransform();
private:
	RctRosBridge* parent;
	boost::mutex mutexHandler;
	std::vector<TransformWrapper> transforms;
};

class RctRosBridge {
public:
	RctRosBridge(const std::string &name = "rct-ros-bridge");
	virtual ~RctRosBridge();

	void run();
	void interrupt();
	void notify();
private:
	Transformer::Ptr transformerRsb;
	TransformCommunicator::Ptr commRos;
	Handler::Ptr rosHandler;
	Handler::Ptr rsbHandler;
	bool interrupted;

	boost::condition_variable cond;
	boost::mutex mutex;

	static log4cxx::LoggerPtr logger;
};

} /* namespace rct */
