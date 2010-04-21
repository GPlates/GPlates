/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2010 Geological Survey of Norway
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
 
#ifndef GPLATES_QTWIDGETS_SETVGPVISIBILITYDIALOG_H
#define GPLATES_QTWIDGETS_SETVGPVISIBILITYDIALOG_H

#include <QWidget>

#include "SetVGPVisibilityDialogUi.h"

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	/**
	 * Dialog to view and modify the ViewState's VGPRenderSettings.
	 */
	class SetVGPVisibilityDialog:
			public QDialog,
			protected Ui_SetVGPVisibilityDialog
	{
		Q_OBJECT
	public:

		explicit
		SetVGPVisibilityDialog(
			GPlatesPresentation::ViewState &view_state_,
			QWidget *parent_ = NULL);
			
			
	signals:
		
	
	private:

		void
		set_initial_state();
		
		void
		setup_connections();
		
	private slots:
		
		
		void
		handle_always_visible();
		
		void
		handle_time_window();
		
		void
		handle_delta_t();
		
		void
		handle_distant_past(
			bool state);
		
		void
		handle_distant_future(
			bool state);
		
		void
		handle_apply();
		
	private:

		GPlatesPresentation::ViewState *d_view_state_ptr;

	};
}

#endif // GPLATES_QTWIDGETS_SETVGPVISIBILITYDIALOG_H
 