#include <QtGui/QApplication>
#include "cube-qt-demo/CubeWidget.h"

int main(int argc, char* argv[])
{
	QApplication app(argc, argv);

	CubeWidget* widget = new CubeWidget();

	// QApplication::setMainWidget(widget); 

	widget->show();

	return app.exec();
}

