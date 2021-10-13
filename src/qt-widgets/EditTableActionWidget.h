/* $Id: EditTableActionWidget.h 6856 2009-10-16 11:32:59Z rwatson $ */

/**
 * \file 
 * $Revision: 6856 $
 * $Date: 2009-10-16 13:32:59 +0200 (fr, 16 okt 2009) $ 
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
 
#ifndef GPLATES_QTWIDGETS_EDITTABLEACTIONWIDGET_H
#define GPLATES_QTWIDGETS_EDITTABLEACTIONWIDGET_H

#include <QWidget>
#include "app-logic/ApplicationState.h"
#include "EditTableActionWidgetUi.h"

namespace GPlatesQtWidgets
{
	class EditTableWidget;
	
	
	class EditTableActionWidget:
			public QWidget, 
			protected Ui_EditTableActionWidget
	{
		Q_OBJECT
		
	public:
		explicit
		EditTableActionWidget(
				EditTableWidget *table_widget,
				QWidget *parent_ = NULL);
		
		/**
		 * Note that since we are adding these ActionWidgets with a QWidget parent,
		 * and then setting them as a cell widget inside the list-of-points QTableWidget,
		 * Qt will kindly manage the memory for us. Of course, that's not documented
		 * anywhere, which is why I tested it with this destructor.
		 */
		virtual
		~EditTableActionWidget()
		{ 
		}
			
	public slots:
		
		void
		insert_row_above();
		
		void
		insert_row_below();
		
		void
		delete_row();
		
	private:
	
		EditTableWidget *d_table_widget_ptr;
		
	};
}

#endif  // GPLATES_QTWIDGETS_EDITTABLEACTIONWIDGET_H
