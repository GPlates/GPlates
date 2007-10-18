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
 
#ifndef GPLATES_GUI_ABOUTDIALOG_H
#define GPLATES_GUI_ABOUTDIALOG_H

#include <QDialog>
#include "AboutDialogUi.h"


namespace GPlatesQtWidgets
{
	class ViewportWindow;


	class AboutDialog: 
			public QDialog,
			protected Ui_AboutDialog 
	{
		Q_OBJECT
		
	public:
		explicit
		AboutDialog(
				ViewportWindow &viewport,
				QWidget *parent_ = NULL);

		virtual
		~AboutDialog()
		{  }
	private:
		ViewportWindow *d_viewport_ptr;
	};
}

#endif  // GPLATES_GUI_ABOUTDIALOG_H
