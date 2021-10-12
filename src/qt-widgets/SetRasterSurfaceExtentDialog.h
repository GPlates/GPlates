/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008 Geological Survey of Norway
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
 
#ifndef GPLATES_QTWIDGETS_SETRASTERSURFACEEXTENTDIALOG_H
#define GPLATES_QTWIDGETS_SETRASTERSURFACEEXTENTDIALOG_H

#include <QDialog>
#include "SetRasterSurfaceExtentDialogUi.h"

namespace GPlatesGui
{
	class Texture;
}

namespace GPlatesQtWidgets
{

	class InformationDialog;
	class ViewportWindow;

	class SetRasterSurfaceExtentDialog:
		public QDialog,
		protected Ui_SetRasterSurfaceExtentDialog
	{
		Q_OBJECT

	public:

		explicit
		SetRasterSurfaceExtentDialog(
			ViewportWindow &viewport_window,
			QWidget *parent_ = NULL);

		virtual
		~SetRasterSurfaceExtentDialog()
		{ }

		const QRectF 
		extent() const
		{
			return d_extent;
		}

	public slots:


		void
		accept();

		void
		handle_cancel();

		void
		handle_reset_to_default_fields();

	private:

		ViewportWindow *d_viewport_window_ptr;

		QRectF d_extent;

		InformationDialog *d_help_dialog;

		static const QString s_help_dialog_text;
		static const QString s_help_dialog_title;

	};
}

#endif // GPLATES_QTWIDGETS_SETRASTERSURFACEEXTENTDIALOG_H
