/*
 * ParserXML.h
 *
 *  Created on: Mar 5, 2015
 *      Author: lziegler
 */

#pragma once

#include "Parser.h"

#include <rsc/logging/Logger.h>

namespace rct {

class ParserXML: public rct::Parser {
public:
	typedef boost::shared_ptr<ParserXML> Ptr;
	ParserXML();
	virtual ~ParserXML();

	virtual bool canParse(const std::string& file);
	virtual ParserResult parse(const std::string& file);

private:
	static rsc::logging::LoggerPtr logger;
};

}  // namespace rct

