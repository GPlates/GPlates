#ifndef CUBEWIDGET_H
#define CUBEWIDGET_H

#include <QtGui/QApplication>
#include <QtCore/QTimer>
#include <QtOpenGL/qgl.h>

class CubeWidget : public QGLWidget {
	Q_OBJECT
	
public:
	CubeWidget(QWidget *widget_parent = 0);
	
protected:
	virtual void initializeGL();
	virtual void resizeGL(int new_width, int new_height);
	virtual void paintGL();
		
	virtual void timeOut();
	
protected slots:
	virtual void timeOutSlot();
	
private:
	QTimer *timer;
	GLfloat rtri, rquad;
};

#endif
