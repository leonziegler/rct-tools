/*
 * ParserXML.cpp
 *
 *  Created on: Mar 5, 2015
 *      Author: lziegler
 */

#include "ParserXML.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <rsc/logging/Logger.h>

using namespace std;
using namespace boost::property_tree;

namespace rct {

rsc::logging::LoggerPtr ParserXML::logger = rsc::logging::Logger::getLogger("rct.ParserXML");

ParserXML::ParserXML() {

}

ParserXML::~ParserXML() {
}

bool ParserXML::canParse(const string& file) {
	ptree pt;
	try {
		xml_parser::read_xml(file, pt);
	} catch (xml_parser_error &e) {
		RSCTRACE(logger, "cannot parse " << file << ". Reason: " << e.what());
	}
	RSCDEBUG(logger, "can parse " << file << ": " << !pt.empty());
	return !pt.empty();
}

vector<string> ParserXML::parseConvertScopes(const string& file) {
	ptree pt;
	xml_parser::read_xml(file, pt);

	RSCDEBUG(logger, "parse: " << file);

	vector<string> scopes;
	BOOST_FOREACH(ptree::value_type const& v, pt.get_child("rct.scopes") ) {
		scopes.push_back(v.second.data());
	}
	return scopes;
}
ParserResultTransforms ParserXML::parseStaticTransforms(const string& file) {
	ptree pt;
	xml_parser::read_xml(file, pt);

	RSCDEBUG(logger, "parse: " << file);

	vector<Transform> transforms;
	BOOST_FOREACH(ptree::value_type const& v, pt.get_child("rct.transforms") ) {
		RSCTRACE(logger, "section: transform");
		ptree ptTransform = v.second;
		ptree ptTranslation = v.second.get_child("translation").begin()->second;
		ptree ptRotation = v.second.get_child("rotation").begin()->second;

		string parent = ptTransform.get<string>("<xmlattr>.parent");
		string child = ptTransform.get<string>("<xmlattr>.child");
		RSCTRACE(logger, "parent: " << parent << "  child: " << child);
		boost::optional<double> tx = ptTranslation.get_optional<double>(
				boost::property_tree::path("x", '/'));
		boost::optional<double> ty = ptTranslation.get_optional<double>(
				boost::property_tree::path("y", '/'));
		boost::optional<double> tz = ptTranslation.get_optional<double>(
				boost::property_tree::path("z", '/'));
		RSCTRACE(logger, "x:" << tx << "  y:" << ty << "  z:" << tz);
		boost::optional<double> yaw = ptRotation.get_optional<double>(
				boost::property_tree::path("yaw", '/'));
		boost::optional<double> pitch = ptRotation.get_optional<double>(
				boost::property_tree::path("pitch", '/'));
		boost::optional<double> roll = ptRotation.get_optional<double>(
				boost::property_tree::path("roll", '/'));
		boost::optional<double> qw = ptRotation.get_optional<double>(
				boost::property_tree::path("qw", '/'));
		boost::optional<double> qx = ptRotation.get_optional<double>(
				boost::property_tree::path("qx", '/'));
		boost::optional<double> qy = ptRotation.get_optional<double>(
				boost::property_tree::path("qy", '/'));
		boost::optional<double> qz = ptRotation.get_optional<double>(
				boost::property_tree::path("qz", '/'));
		RSCTRACE(logger, "yaw:" << yaw << "  pitch:" << pitch << "  roll:" << roll);
		RSCTRACE(logger, "qw:" << qw << "  qx:" << qx << "  qy:" << qy << "  qz:" << qz);

		if (!tx || !ty || !tz) {
			stringstream ss;
			ss << "Error parsing transforms. ";
			ss << "Section \"rct.transforms\" has incomplete translation. ";
			ss << "Required: (translation.x, translation.y, translation.z)";
			throw ptree_error(ss.str());
		}

		Eigen::Vector3d translation(tx.get(), ty.get(), tz.get());

		if (yaw && pitch && roll) {
			if (qw || qx || qy || qz) {
				stringstream ss;
				ss << "Error parsing transforms. ";
				ss << "Section \"rct.transforms\" has arbitrary rotation declarations. ";
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
				ss << "Section \"rct.transforms\" has arbitrary rotation declarations. ";
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
			ss << "Section \"rct.transforms\" has incomplete rotation. ";
			ss << "Required: (rotation.yaw, rotation.pitch, rotation.roll)";
			ss << " OR (rotation.qw, rotation.qx, rotation.qy, rotation.qz)";
			throw ptree_error(ss.str());
		}
	}

	TransformerConfig config;
	boost::optional<ptree& > ptCoreIt = pt.get_child_optional("rct.core");
	if (ptCoreIt) {
		boost::optional<ptree& > ptCachetimeIt = ptCoreIt.get().get_child_optional("cachetime");
		if (ptCachetimeIt) {
			ptree ptCachetime = ptCachetimeIt.get().front().second;
			boost::optional<int> cachetime = ptCachetime.get_optional<int>(boost::property_tree::path("value", '/'));
			RSCTRACE(logger, "cachetime:" << cachetime);
			if (cachetime) {
				config.setCacheTime(boost::posix_time::time_duration(0, 0, cachetime.get()));
			}
		} else {
			RSCTRACE(logger, "no cachetime");
		}
	}

	ParserResultTransforms results;
	results.transforms = transforms;
	results.config = config;
	return results;
}

vector<ParserResultMessage> ParserXML::parseConvertMessages(const string& file) {
	ptree pt;
	xml_parser::read_xml(file, pt);

	RSCDEBUG(logger, "parse: " << file);

	vector<ParserResultMessage> messages;
	BOOST_FOREACH(ptree::value_type const& v, pt.get_child("rct.messages") ) {
		RSCTRACE(logger, "section: message");
		ptree ptMessage = v.second;
		ParserResultMessage msg;
		msg.parent = ptMessage.get<string>("<xmlattr>.parent");
		msg.child = ptMessage.get<string>("<xmlattr>.child");
		msg.authority = ptMessage.get<string>("<xmlattr>.authority");
		msg.scope = ptMessage.get<string>("<xmlattr>.scope");
		messages.push_back(msg);
	}

	return messages;
}

}  // namespace rct

