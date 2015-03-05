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

class ParserResult {
public:
	TransformerConfig config;
	std::vector<Transform> transforms;
};

class Parser {
public:
	typedef boost::shared_ptr<Parser> Ptr;
	virtual bool canParse(const std::string& file) = 0;
	virtual ParserResult parse(const std::string& file) = 0;
	virtual ~Parser() {
	}
};

}  // namespace rct
