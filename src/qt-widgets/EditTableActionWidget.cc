/* $Id: EditTableActionWidget.cc 6234 2009-07-08 11:44:34Z rwatson $ */

/**
 * \file 
 * $Revision: 6234 $
 * $Date: 2009-07-08 13:44:34 +0200 (on, 08 jul 2009) $ 
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

#include "EditTableWidget.h"
#include "EditTableActionWidget.h"


GPlatesQtWidgets::EditTableActionWidget::EditTableActionWidget(
		GPlatesQtWidgets::EditTableWidget *table_widget,
		QWidget *parent_):
	QWidget(parent_),
	d_table_widget_ptr(table_widget)
{
	setupUi(this);

	// Set up slots for each button
	QObject::connect(button_insert_above, SIGNAL(clicked()), this, SLOT(insert_row_above()));
	QObject::connect(button_insert_below, SIGNAL(clicked()), this, SLOT(insert_row_below()));
	QObject::connect(button_delete, SIGNAL(clicked()), this, SLOT(delete_row()));

}


void
GPlatesQtWidgets::EditTableActionWidget::insert_row_above()
{
	d_table_widget_ptr->handle_insert_row_above(this);
}

void
GPlatesQtWidgets::EditTableActionWidget::insert_row_below()
{
	d_table_widget_ptr->handle_insert_row_below(this);
}

void
GPlatesQtWidgets::EditTableActionWidget::delete_row()
{
	d_table_widget_ptr->handle_delete_row(this);
}
