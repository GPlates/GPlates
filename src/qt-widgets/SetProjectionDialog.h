/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008, 2009 Geological Survey of Norway
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
 
#ifndef GPLATES_QTWIDGETS_SETPROJECTIONDIALOG_H
#define GPLATES_QTWIDGETS_SETPROJECTIONDIALOG_H

#include <QDialog>

#include "SetProjectionDialogUi.h"

#include "GPlatesDialog.h"

#include "gui/ViewportProjection.h"


namespace GPlatesQtWidgets
{
	class SetProjectionDialog: 
			public GPlatesDialog,
			protected Ui_SetProjectionDialog
	{
		Q_OBJECT
		
	public:
		
		SetProjectionDialog(
				QWidget *parent_ = NULL);

		virtual
		~SetProjectionDialog()
		{  }

		/**
		 * Call this prior to displaying the dialog, so we can set the widgets to their
		 * appropriate states.
		 */
		void
		setup(
				const GPlatesGui::ViewportProjection &viewport_projection)
		{
			set_projection(viewport_projection);
		}

		void
		set_projection(
				const GPlatesGui::ViewportProjection &viewport_projection);

		void
		set_map_central_meridian(
				double map_central_meridian);


		GPlatesGui::ViewportProjection::projection_type
		get_projection_type() const;

		double
		get_map_central_meridian();

	private Q_SLOTS:

		/**
		 * Disables the central_meridian spinbox when the Orthographic projection is selected. 
		 */
		void
		update_central_meridian_status();
	};

}

#endif  // GPLATES_QTWIDGETS_SETPROJECTIONDIALOG_H
