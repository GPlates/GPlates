/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_INSERTIONPOINTWIDGET_H
#define GPLATES_QTWIDGETS_INSERTIONPOINTWIDGET_H

#include <QWidget>
#include <QAction>

#include "ui_InsertionPointWidgetUi.h"


namespace GPlatesQtWidgets
{
	/**
	 * Lightweight Qt widget to display the 'Insertion Point' arrow
	 * plus cancel button.
	 */
	class InsertionPointWidget:
			public QWidget, 
			protected Ui_InsertionPointWidget
	{
		Q_OBJECT
		
	public:
		explicit
		InsertionPointWidget(
				QAction &action,
				QWidget *parent_ = NULL):
			QWidget(parent_)
		{
			setupUi(this);
			toolbutton->setDefaultAction(&action);
		}

	};
}

#endif  // GPLATES_QTWIDGETS_INSERTIONPOINTWIDGET_H
