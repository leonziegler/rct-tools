/*
 * ParserINI.h
 *
 *  Created on: Mar 5, 2015
 *      Author: lziegler
 */

#pragma once

#include "Parser.h"

#include <rsc/logging/Logger.h>

namespace rct {

class ParserINI: public rct::Parser {
public:
	typedef boost::shared_ptr<ParserINI> Ptr;
	ParserINI();
	virtual ~ParserINI();

	virtual bool canParse(const std::string& file);
	virtual ParserResultTransforms parseStaticTransforms(const std::string& file);
	virtual std::vector<std::string> parseConvertScopes(const std::string& file);
	virtual std::vector<ParserResultMessage> parseConvertMessages(const std::string& file);

private:
	static rsc::logging::LoggerPtr logger;
};

}  // namespace rct
