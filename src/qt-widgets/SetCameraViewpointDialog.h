/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2007 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_SETCAMERAVIEWPOINTDIALOG_H
#define GPLATES_QTWIDGETS_SETCAMERAVIEWPOINTDIALOG_H

#include <QDialog>
#include "SetCameraViewpointDialogUi.h"

namespace GPlatesQtWidgets
{
	class ViewportWindow;
	
	class SetCameraViewpointDialog: 
			public QDialog,
			protected Ui_SetCameraViewpointDialog 
	{
		Q_OBJECT
		
	public:
		explicit
		SetCameraViewpointDialog(
				ViewportWindow &viewport_window,
				QWidget *parent_ = NULL);
		
		void
		set_lat_lon(
				const double &lat,
				const double &lon);
		
		const double
		latitude() const
		{
			return spinbox_latitude->value();
		}

		const double
		longitude() const
		{
			return spinbox_longitude->value();
		}
		
	private:

		ViewportWindow *d_viewport_window_ptr;

	};
}

#endif  // GPLATES_QTWIDGETS_SETCAMERAVIEWPOINTDIALOG_H
