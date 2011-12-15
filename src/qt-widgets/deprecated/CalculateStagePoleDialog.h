/**
 * \file 
 * $Revision: 9477 $
 * $Date: 2010-08-24 05:36:12 +0200 (ti, 24 aug 2010) $ 
 * 
 * Copyright (C) 2011 Geological Survey of Norway
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

#ifndef GPLATES_QTWIDGETS_CALCULATESTAGEPOLEDIALOG_H
#define GPLATES_QTWIDGETS_CALCULATESTAGEPOLEDIALOG_H

#include <QDialog>

#include "maths/LatLonPoint.h"
#include "CalculateStagePoleDialogUi.h"

namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesQtWidgets
{
	class SmallCircleWidget;

	class CalculateStagePoleDialog:
		public QDialog,
		protected Ui_CalculateStagePoleDialog
	{
	
	Q_OBJECT

	public:
		CalculateStagePoleDialog(
			SmallCircleWidget *small_circle_widget,
			GPlatesAppLogic::ApplicationState &application_state,
			QWidget *parent);

	private slots:

		void
		handle_calculate();

		void
		handle_use();

	private:

		SmallCircleWidget *d_small_circle_widget_ptr;
		GPlatesAppLogic::ApplicationState &d_application_state;

		GPlatesMaths::LatLonPoint d_centre;
	};
}

#endif // GPLATES_QTWIDGETS_CALCULATESTAGEPOLEDIALOG_H