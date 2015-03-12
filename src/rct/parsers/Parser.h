/*
 * Parser.h
 *
 *  Created on: Mar 5, 2015
 *      Author: lziegler
 */

#pragma once

#include <rct/Transform.h>
#include <rct/TransformerConfig.h>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace rct {

class ParserResultTransforms {
public:
	TransformerConfig config;
	std::vector<Transform> transforms;
};

class ParserResultMessage {
public:
	std::string parent;
	std::string child;
	std::string authority;
	std::string scope;
};

class Parser {
public:
	typedef boost::shared_ptr<Parser> Ptr;
	virtual bool canParse(const std::string& file) = 0;
	virtual ParserResultTransforms parseStaticTransforms(const std::string& file) = 0;
	virtual std::vector<std::string> parseConvertScopes(const std::string& file) = 0;
	virtual std::vector<ParserResultMessage> parseConvertMessages(const std::string& file) = 0;
	virtual ~Parser() {
	}
};

}  // namespace rct
