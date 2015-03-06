#include <QtGui>
#include <QMessageBox>
#include <QMenuBar>
#include <QScrollBar>
#include <QWheelEvent>
#include <QLabel>
#include <QAction>
#include <QMenu>
#include <QScrollArea>

#include <iostream>

#include "imageviewer.h"

using namespace std;

class DisableWheelFilter: public QObject {
public:
	bool eventFilter(QObject* o, QEvent* evt) {
		if (evt->type() == QEvent::Wheel) {
			evt->ignore();
			return true;
		}
		return QObject::eventFilter(o, evt);
	}
};
DisableWheelFilter* filter = new DisableWheelFilter();

ImageViewer::ImageViewer() {
	imageLabel = new QLabel(this);
	imageLabel->setBackgroundRole(QPalette::Base);
	imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
	imageLabel->setScaledContents(true);

	scrollArea = new QScrollArea(this);
	scrollArea->setBackgroundRole(QPalette::Dark);
	scrollArea->setWidget(imageLabel);
	scrollArea->viewport()->installEventFilter(filter);
	setCentralWidget(scrollArea);

	createActions();
	createMenus();

	setWindowTitle(tr("rct-view"));

	QSettings settings;
	setGeometry(settings.value("main/windowgeometry", QRect(0, 0, 500, 400)).toRect());
	scaleFactor = settings.value("main/scaleFactor", 1.0).toDouble();
}

ImageViewer::~ImageViewer() {
	QSettings settings;
	settings.setValue("main/windowgeometry", geometry());
	settings.setValue("main/scaleFactor", scaleFactor);
}

void ImageViewer::open(const QString &fileName) {
	if (!fileName.isEmpty()) {
		QImage image(fileName);
		if (!showImage(image)) {
			QMessageBox::information(this, tr("Image Viewer"), tr("Cannot load %1.").arg(fileName));
		}
	}
}

bool ImageViewer::showImage(const QImage &image) {
	if (image.isNull()) {
		return false;
	}
	imageLabel->setPixmap(QPixmap::fromImage(image));

	fitToWindowAct->setEnabled(true);
	updateActions();

	if (!fitToWindowAct->isChecked())
		imageLabel->adjustSize();

	fitToWindow();
	scaleImageTo(scaleFactor);
	return true;
}

void ImageViewer::zoomIn() {
	scaleImage(1.25);
}

void ImageViewer::zoomOut() {
	scaleImage(0.8);
}

void ImageViewer::normalSize() {
	imageLabel->adjustSize();
	scaleFactor = 1.0;
}

void ImageViewer::fitToWindow() {
	bool fitToWindow = fitToWindowAct->isChecked();
	scrollArea->setWidgetResizable(fitToWindow);
	//if (!fitToWindow) {
	//	normalSize();
	//}
	updateActions();
}

void ImageViewer::about() {
	QMessageBox::about(this, tr("About Image Viewer"),
			tr("<p>The <b>Image Viewer</b> example shows how to combine QLabel "
					"and QScrollArea to display an image. QLabel is typically used "
					"for displaying a text, but it can also display an image. "
					"QScrollArea provides a scrolling view around another widget. "
					"If the child widget exceeds the size of the frame, QScrollArea "
					"automatically provides scroll bars. </p><p>The example "
					"demonstrates how QLabel's ability to scale its contents "
					"(QLabel::scaledContents), and QScrollArea's ability to "
					"automatically resize its contents "
					"(QScrollArea::widgetResizable), can be used to implement "
					"zooming and scaling features. </p><p>In addition the example "
					"shows how to use QPainter to print an image.</p>"));
}

void ImageViewer::createActions() {
	exitAct = new QAction(tr("E&xit"), this);
	exitAct->setShortcut(tr("Ctrl+Q"));
	connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));

	zoomInAct = new QAction(tr("Zoom &In (25%)"), this);
	zoomInAct->setShortcut(tr("Ctrl++"));
	zoomInAct->setEnabled(false);
	connect(zoomInAct, SIGNAL(triggered()), this, SLOT(zoomIn()));

	zoomOutAct = new QAction(tr("Zoom &Out (25%)"), this);
	zoomOutAct->setShortcut(tr("Ctrl+-"));
	zoomOutAct->setEnabled(false);
	connect(zoomOutAct, SIGNAL(triggered()), this, SLOT(zoomOut()));

	normalSizeAct = new QAction(tr("&Normal Size"), this);
	normalSizeAct->setShortcut(tr("Ctrl+S"));
	normalSizeAct->setEnabled(false);
	connect(normalSizeAct, SIGNAL(triggered()), this, SLOT(normalSize()));

	fitToWindowAct = new QAction(tr("&Fit to Window"), this);
	fitToWindowAct->setEnabled(false);
	fitToWindowAct->setCheckable(true);
	fitToWindowAct->setShortcut(tr("Ctrl+F"));
	connect(fitToWindowAct, SIGNAL(triggered()), this, SLOT(fitToWindow()));

	aboutAct = new QAction(tr("&About"), this);
	connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));

	aboutQtAct = new QAction(tr("About &Qt"), this);
	connect(aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
}

void ImageViewer::createMenus() {
	fileMenu = new QMenu(tr("&File"), this);
	fileMenu->addAction(exitAct);

	viewMenu = new QMenu(tr("&View"), this);
	viewMenu->addAction(zoomInAct);
	viewMenu->addAction(zoomOutAct);
	viewMenu->addAction(normalSizeAct);
	viewMenu->addSeparator();
	viewMenu->addAction(fitToWindowAct);

	helpMenu = new QMenu(tr("&Help"), this);
	helpMenu->addAction(aboutAct);
	helpMenu->addAction(aboutQtAct);

	menuBar()->addMenu(fileMenu);
	menuBar()->addMenu(viewMenu);
	menuBar()->addMenu(helpMenu);
}

void ImageViewer::updateActions() {
	zoomInAct->setEnabled(!fitToWindowAct->isChecked());
	zoomOutAct->setEnabled(!fitToWindowAct->isChecked());
	normalSizeAct->setEnabled(!fitToWindowAct->isChecked());
}

void ImageViewer::scaleImage(double factor) {
	scaleFactor *= factor;
	scaleImageTo(scaleFactor);
	adjustScrollBar(scrollArea->horizontalScrollBar(), factor);
	adjustScrollBar(scrollArea->verticalScrollBar(), factor);
}

void ImageViewer::scaleImageTo(double scale) {
	Q_ASSERT(imageLabel->pixmap());
	imageLabel->resize(scale * imageLabel->pixmap()->size());

	adjustScrollBar(scrollArea->horizontalScrollBar(), scale);
	adjustScrollBar(scrollArea->verticalScrollBar(), scale);

	zoomInAct->setEnabled(scale < 3.0);
	zoomOutAct->setEnabled(scale > 0.333);
}

void ImageViewer::adjustScrollBar(QScrollBar *scrollBar, double factor) {
	scrollBar->setValue(
			int(factor * scrollBar->value() + ((factor - 1) * scrollBar->pageStep() / 2)));
}

void ImageViewer::wheelEvent(QWheelEvent * event) {
	event->accept();
	if (event->delta() > 0) {
		scaleImage(0.9);
	} else {
		scaleImage(1.125);
	}
}
