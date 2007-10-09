/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2006 The University of Sydney, Australia
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 *
 * GPlates is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
 
#ifndef CUBEWIDGET_H
#define CUBEWIDGET_H

#include <QtGui/QApplication>
#include <QtCore/QTimer>
#include <QtOpenGL/qgl.h>

namespace GPlatesQtWidgets
{
	class CubeWidget :
			public QGLWidget
	{
		Q_OBJECT
	
	public:
		CubeWidget(QWidget *parent = 0);
	
	protected:
		virtual void initializeGL();
		virtual void resizeGL(int width, int height);
		virtual void paintGL();
		virtual void timeOut();
	
	protected slots:
		virtual void timeOutSlot();
	
	private:
		QTimer *timer;
		GLfloat rtri, rquad;
	};
}

#endif
