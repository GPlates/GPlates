/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_EDITGEOMETRYACTIONWIDGET_H
#define GPLATES_QTWIDGETS_EDITGEOMETRYACTIONWIDGET_H

#include <QWidget>
#include "ApplicationState.h"
#include "EditGeometryActionWidgetUi.h"

namespace GPlatesQtWidgets
{
	class EditGeometryWidget;
	
	
	class EditGeometryActionWidget:
			public QWidget, 
			protected Ui_EditGeometryActionWidget
	{
		Q_OBJECT
		
	public:
		explicit
		EditGeometryActionWidget(
				EditGeometryWidget &geometry_widget,
				QWidget *parent_ = NULL);
		
		/**
		 * Note that since we are adding these ActionWidgets with a QWidget parent,
		 * and then setting them as a cell widget inside the list-of-points QTableWidget,
		 * Qt will kindly manage the memory for us. Of course, that's not documented
		 * anywhere, which is why I tested it with this destructor.
		 */
		virtual
		~EditGeometryActionWidget()
		{ 
		}
			
	public slots:
		
		void
		insert_point_above();
		
		void
		insert_point_below();
		
		void
		delete_point();
		
	private:
	
		EditGeometryWidget *d_geometry_widget_ptr;
		
	};
}

#endif  // GPLATES_QTWIDGETS_EDITGEOMETRYACTIONWIDGET_H
