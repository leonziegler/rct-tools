/*
 * RctEcho.cpp
 *
 *  Created on: Dec 23, 2014
 *      Author: leon
 */
#include <rct/rctConfig.h>
#include <rct/TransformerFactory.h>
#include <rct/Transformer.h>
#include <rct/TransformerConfig.h>
#include <rct/impl/TransformerTF2.h>
#include <rct/impl/TransformCommRsb.h>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/progress.hpp>
#include <log4cxx/log4cxx.h>
#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/consoleappender.h>
#include <log4cxx/patternlayout.h>
#include "rct-tools-config.h"
#ifdef POPPLERQT4_FOUND
#include <poppler-qt4.h>
#endif
#ifdef POPPLERQT5_FOUND
#include <poppler-qt5.h>
#include <QtCore>
#include <QApplication>
#include "viewer/imageviewer.h"
#endif
#include <fstream>
#include <iostream>
#include <stdio.h>

using namespace boost::program_options;
using namespace std;
using namespace log4cxx;

void printHelp(int argc, char **argv, options_description desc) {
	cout << "Usage:\n  " << argv[0] << " [options]\n"
			<< endl;
	cout << desc << endl;
	cout << "This will print all transforms as a graph." << endl;
}

int main(int argc, char **argv) {

	options_description desc("Allowed options");
	variables_map vm;

	desc.add_options()("help,h", "produce help message") // help
	("debug", "debug mode") //debug
	("trace", "trace mode") //trace
	("duration", value<double>(), "time waiting for transforms (seconds)") // duration
	("info", "info mode");

	store(command_line_parser(argc, argv).options(desc).run(), vm);
	notify(vm);

	if (vm.count("help")) {
		cout << 0 << endl;
		printHelp(argc, argv, desc);
		return 0;
	}

	LayoutPtr pattern(new PatternLayout("%r [%t] %-5p %c - %m%n"));
	AppenderPtr appender(new ConsoleAppender(pattern));
	Logger::getRootLogger()->addAppender(appender);

	if (vm.count("debug")) {
		Logger::getRootLogger()->setLevel(Level::getDebug());
	} else if (vm.count("trace")) {
		Logger::getRootLogger()->setLevel(Level::getTrace());
	} else if (vm.count("info")) {
		Logger::getRootLogger()->setLevel(Level::getInfo());
	} else {
		Logger::getRootLogger()->setLevel(Level::getWarn());
	}

	double seconds = 5.0;
	if (vm.count("duration")) {
		seconds = vm["duration"].as<double>();
	}

	rct::TransformerConfig config;
	rct::TransformerCore::Ptr core = rct::TransformerTF2::Ptr(new rct::TransformerTF2(config.getCacheTime()));
	rct::TransformCommRsb::Ptr comm(new rct::TransformCommRsb("view", config.getCacheTime(), core));
	comm->init(config);
	rct::Transformer::Ptr transformer(new rct::Transformer(core, comm, config));

	cout << "collecting transforms for " << seconds << " sec" << endl;
	usleep(seconds * 1000000.0);

	string dotStr = core->allFramesAsDot();

	if (dotStr.empty()) {
		cerr << "no transforms found" << endl;
		return 1;
	}

	stringstream tmpss;
	tmpss << "/tmp/rct" << getuid() << "/";
	string tmpStr = tmpss.str();
	string tmpSrcStr = tmpStr + "frames.gv";
	string tmpTgtStr = tmpStr + "frames.pdf";

	boost::filesystem::create_directory(tmpStr);

	ofstream dotFile;
	dotFile.open(tmpSrcStr.c_str());
	if (!dotFile.is_open()) {
		cerr << "ERROR generating pdf. Cannot write to " << tmpSrcStr << endl;
		return 1;
	}
	dotFile << dotStr;
	dotFile.close();

	string command = "dot";
	command += " -Tpdf ";
	command += tmpSrcStr;
	command += " -o ";
	command += tmpTgtStr;

	FILE* pipe = popen(command.c_str(), "r");
	if (!pipe) {
		cerr << "ERROR generating pdf" << endl;
		return 1;
	}
	char buffer[128];
	std::string result = "";
	while(!feof(pipe)) {
		if(fgets(buffer, 128, pipe) != NULL)
			result += buffer;
	}
	int exitCode = pclose(pipe)/256;

	if (exitCode == 0) {
#ifdef POPPLERQT4_FOUND
		cout << "frames.pdf generated" << endl;
#else
#ifdef POPPLERQT5_FOUND
	QString filename = QString::fromStdString(tmpTgtStr);
	Poppler::Document* document = Poppler::Document::load(filename);
	if (!document || document->isLocked()) {
		cerr << "can not load " << tmpTgtStr << endl;
		delete document;
		return 1;
	}
	// Access page of the PDF file
	Poppler::Page* pdfPage = document->page(0);  // Document starts at page 0
	if (pdfPage == 0) {
		cerr << "can not load " << tmpTgtStr << endl;
		return 1;
	}
	// Generate a QImage of the rendered page
	QImage image = pdfPage->renderToImage(300,300);
	if (image.isNull()) {
		cerr << "can not render " << tmpTgtStr << endl;
	  return 1;
	}

	QApplication app(argc, argv);
	QCoreApplication::setOrganizationName("Bielefeld University, CITEC, Central Lab Facilities");
	QCoreApplication::setOrganizationDomain("clf.cit-ec.uni-bielefeld.de");
	QCoreApplication::setApplicationName("rct-view");
	ImageViewer imageViewer;
	imageViewer.showImage(image);
	imageViewer.show();
	return app.exec();

	// after the usage, the page must be deleted
	delete pdfPage;
#else
		boost::filesystem::copy_file(tmpTgtStr,"frames.pdf",boost::filesystem::copy_option::overwrite_if_exists);
		cout << "frames.pdf generated" << endl;
#endif
#endif
	} else {
		cout << "An error occured. Is graphviz installed?" << endl;
	}

	return 0;
}

