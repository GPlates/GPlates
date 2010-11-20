/* $Id: PanMap.h 8194 2010-04-26 16:24:42Z rwatson $ */

/**
 * \file 
 * $Revision: 8194 $
 * $Date: 2010-04-26 18:24:42 +0200 (ma, 26 apr 2010) $ 
 * 
 * Copyright (C)  2010 Geological Survey of Norway
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

#ifndef GPLATES_QTWIDGETS_CREATESMALLCIRCLEDIALOG_H
#define GPLATES_QTWIDGETS_CREATESMALLCIRCLEDIALOG_H

#include <QDialog>

#include "maths/SmallCircle.h"
#include "SmallCircleManager.h"
#include "CreateSmallCircleDialogUi.h"

namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesQtWidgets
{
	class CreateSmallCircleDialog:
		public QDialog,
		protected Ui_CreateSmallCircleDialog
	{
		Q_OBJECT

	public:
		CreateSmallCircleDialog(
			SmallCircleManager *small_circle_manager,
			GPlatesAppLogic::ApplicationState &application_state,
			QWidget *parent);

		void
		init();

	signals:
	
	void
	circle_added();

	private slots:
		
		void
		handle_stage_pole_checkbox_state();

		void
		handle_calculate();

		void
		handle_add();

		void
		handle_single_changed(
			bool state);

		void
		handle_multiple_changed(
			bool state);

		void
		handle_multiple_circle_fields_changed();


	private:

		void
		set_multiple_circle_field_colours(
			const QColor &color);

		void
		highlight_invalid_radius_fields();

		SmallCircleManager *d_small_circle_manager_ptr;

		GPlatesAppLogic::ApplicationState &d_application_state;
	};
}

#endif // GPLATES_QTWIDGETS_CREATESMALLCIRCLEDIALOG_H
