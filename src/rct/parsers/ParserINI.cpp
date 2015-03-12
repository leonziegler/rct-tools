/*
 * ParserINI.cpp
 *
 *  Created on: Mar 5, 2015
 *      Author: lziegler
 */

#include "ParserINI.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace boost::property_tree;

namespace rct {

rsc::logging::LoggerPtr ParserINI::logger = rsc::logging::Logger::getLogger("rct.ParserINI");

ParserINI::ParserINI() {
}

ParserINI::~ParserINI() {
}

bool ParserINI::canParse(const string& file) {
	ptree pt;
	try {
		ini_parser::read_ini(file, pt);
	} catch (ini_parser_error &e) {
		if (boost::algorithm::ends_with(file, ".ini") || boost::algorithm::ends_with(file, ".conf")) {
			RSCWARN(logger, "cannot parse " << file << ". Reason: " << e.what());
		} else {
			RSCDEBUG(logger, "cannot parse " << file << ". Reason: " << e.what());
		}
	}
	RSCDEBUG(logger, "can parse " << file << ": " << !pt.empty());
	return !pt.empty();
}

vector<string>  ParserINI::parseConvertScopes(const string& file) {
	ptree pt;
	ini_parser::read_ini(file, pt);

	RSCDEBUG(logger, "parse: " << file);
	vector<string> scopes;

	BOOST_FOREACH(ptree::value_type const& v, pt.get_child("scopes") ) {
		RSCDEBUG(logger, v.first << " -> " << v.second.data());
		if (boost::algorithm::contains(v.first, "scope")) {
			scopes.push_back(v.second.data());
		}
	}

	return scopes;
}

ParserResultTransforms ParserINI::parseStaticTransforms(const string& file) {
	ptree pt;
	ini_parser::read_ini(file, pt);

	RSCDEBUG(logger, "parse: " << file);

	vector<Transform> transforms;
	ptree::const_iterator itTrans;
	for (itTrans = pt.begin(); itTrans != pt.end(); ++itTrans) {
		if (!boost::algorithm::starts_with(itTrans->first, "transform")) {
			continue;
		}
		string section = itTrans->first;
		ptree ptTransform = itTrans->second;

		string parent = ptTransform.get<string>("parent");
		string child = ptTransform.get<string>("child");
		boost::optional<double> tx = ptTransform.get_optional<double>(boost::property_tree::path("translation.x", '/'));
		boost::optional<double> ty = ptTransform.get_optional<double>(boost::property_tree::path("translation.y", '/'));
		boost::optional<double> tz = ptTransform.get_optional<double>(boost::property_tree::path("translation.z", '/'));
		boost::optional<double> yaw =   ptTransform.get_optional<double>(boost::property_tree::path("rotation.yaw", '/'));
		boost::optional<double> pitch = ptTransform.get_optional<double>(boost::property_tree::path("rotation.pitch", '/'));
		boost::optional<double> roll =  ptTransform.get_optional<double>(boost::property_tree::path("rotation.roll", '/'));
		boost::optional<double> qw = ptTransform.get_optional<double>(boost::property_tree::path("rotation.qw", '/'));
		boost::optional<double> qx = ptTransform.get_optional<double>(boost::property_tree::path("rotation.qx", '/'));
		boost::optional<double> qy = ptTransform.get_optional<double>(boost::property_tree::path("rotation.qy", '/'));
		boost::optional<double> qz = ptTransform.get_optional<double>(boost::property_tree::path("rotation.qz", '/'));

		if (!tx || !ty || !tz) {
			stringstream ss;
			ss << "Error parsing transforms. ";
			ss << "Section \"" << section << "\" has incomplete translation. ";
			ss << "Required: (translation.x, translation.y, translation.z)";
			throw ptree_error(ss.str());
		}

		Eigen::Vector3d translation(tx.get(), ty.get(), tz.get());

		if (yaw && pitch && roll) {
			if (qw || qx || qy || qz) {
				stringstream ss;
				ss << "Error parsing transforms. ";
				ss << "Section \"" << section << "\" has arbitrary rotation declarations. ";
				ss << "Use either yaw/pitch/roll or quaternion. ";
				throw ptree_error(ss.str());
			}
			Eigen::AngleAxisd rollAngle(roll.get(), Eigen::Vector3d::UnitZ());
			Eigen::AngleAxisd yawAngle(yaw.get(), Eigen::Vector3d::UnitY());
			Eigen::AngleAxisd pitchAngle(pitch.get(), Eigen::Vector3d::UnitX());
			Eigen::Quaterniond r = rollAngle * yawAngle * pitchAngle;
			Eigen::Affine3d a = Eigen::Affine3d().fromPositionOrientationScale(translation, r,
								Eigen::Vector3d::Ones());
			Transform t(a, parent, child, boost::posix_time::microsec_clock::universal_time());
			RSCDEBUG(logger, "parsed transform: " << t);
			transforms.push_back(t);
			continue;

		} else if (qw && qx && qy && qz) {
			if (yaw || pitch || roll) {
				stringstream ss;
				ss << "Error parsing transforms. ";
				ss << "Section \"" << section << "\" has arbitrary rotation declarations. ";
				ss << "Use either yaw/pitch/roll or quaternion. ";
				throw ptree_error(ss.str());
			}
			Eigen::Quaterniond r(qw.get(), qx.get(), qy.get(), qz.get());
			Eigen::Affine3d a = Eigen::Affine3d().fromPositionOrientationScale(translation, r,
								Eigen::Vector3d::Ones());
			Transform t(a, parent, child, boost::posix_time::microsec_clock::universal_time());
			RSCDEBUG(logger, "parsed transform: " << t);
			transforms.push_back(t);
			continue;
		} else {
			stringstream ss;
			ss << "Error parsing transforms. ";
			ss << "Section \"" << itTrans->first << "\" has incomplete rotation. ";
			ss << "Required: (rotation.yaw, rotation.pitch, rotation.roll)";
			ss << " OR (rotation.qw, rotation.qx, rotation.qy, rotation.qz)";
			throw ptree_error(ss.str());
		}
	}

	ParserResultTransforms results;
	results.transforms = transforms;
	return results;
}

vector<ParserResultMessage> ParserINI::parseConvertMessages(const string& file) {

	vector<ParserResultMessage> messages;
	ptree pt;
	ini_parser::read_ini(file, pt);

	RSCDEBUG(logger, "parse: " << file);

	ptree::const_iterator itTrans;
	for (itTrans = pt.begin(); itTrans != pt.end(); ++itTrans) {
		if (!boost::algorithm::starts_with(itTrans->first, "message")) {
			continue;
		}
		string section = itTrans->first;
		ptree ptMessage = itTrans->second;
		ParserResultMessage msg;

		msg.parent = ptMessage.get<string>("parent");
		msg.child = ptMessage.get<string>("child");
		msg.authority = ptMessage.get<string>("authority");
		msg.scope = ptMessage.get<string>("scope");
		messages.push_back(msg);
	}

	return messages;
}

}  // namespace rct
