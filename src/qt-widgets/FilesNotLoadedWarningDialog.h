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
 
#ifndef GPLATES_QTWIDGETS_FILESNOTLOADEDWARNINGDIALOG_H
#define GPLATES_QTWIDGETS_FILESNOTLOADEDWARNINGDIALOG_H

#include <QDialog>

#include "FilesNotLoadedWarningDialogUi.h"


namespace GPlatesQtWidgets
{
	/**
	 * This dialog is the one which pops up when files were not loaded (during session/project restore).
	 */
	class FilesNotLoadedWarningDialog: 
			public QDialog,
			protected Ui_FilesNotLoadedWarningDialog 
	{
		Q_OBJECT
		
	public:

		explicit
		FilesNotLoadedWarningDialog(
				QWidget *parent_ = NULL):
			QDialog(parent_, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint)
		{
			setupUi(this);
		}

		virtual
		~FilesNotLoadedWarningDialog()
		{  }
		
		/**
		 * Changes the list of filenames displayed in the dialog.
		 */
		void
		set_filename_list(
				QStringList filenames)
		{
			list_files->clear();
			list_files->addItems(filenames);
		}
	};
}

#endif  // GPLATES_QTWIDGETS_FILESNOTLOADEDWARNINGDIALOG_H
