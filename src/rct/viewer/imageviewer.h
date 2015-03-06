#pragma once

#include <QMainWindow>

class QAction;
class QLabel;
class QMenu;
class QScrollArea;
class QScrollBar;

class ImageViewer: public QMainWindow {
	Q_OBJECT

public:
	ImageViewer();
	virtual ~ImageViewer();
	void open(const QString &file);
	bool showImage(const QImage &image);

private slots:
	void zoomIn();
	void zoomOut();
	void normalSize();
	void fitToWindow();
	void about();

private:
	void createActions();
	void createMenus();
	void updateActions();
	void scaleImage(double factor);
	void scaleImageTo(double scale);
	void adjustScrollBar(QScrollBar *scrollBar, double factor);
	void wheelEvent(QWheelEvent * event);

	QLabel *imageLabel;
	QScrollArea *scrollArea;
	double scaleFactor;

	QAction *exitAct;
	QAction *zoomInAct;
	QAction *zoomOutAct;
	QAction *normalSizeAct;
	QAction *fitToWindowAct;
	QAction *aboutAct;
	QAction *aboutQtAct;

	QMenu *fileMenu;
	QMenu *viewMenu;
	QMenu *helpMenu;
};
