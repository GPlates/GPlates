/* $Id: PythonConsoleDialog.h 11875 2011-06-28 00:56:10Z mchin $ */

/**
 * \file 
 * $Revision: 11875 $
 * $Date: 2011-06-28 10:56:10 +1000 (Tue, 28 Jun 2011) $ 
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_PYTHONINITFAILEDDIALOG_H
#define GPLATES_QTWIDGETS_PYTHONINITFAILEDDIALOG_H

#include "PythonInitFailedDialogUi.h"


namespace GPlatesQtWidgets
{

	class PythonInitFailedDialog
		: public QDialog,
		  public Ui_PythonInitFailedDialog
	{
	public:
		PythonInitFailedDialog(
				QWidget *parent_ = NULL);

		bool
		show_again()
		{
			return !show_again_button->isChecked();
		}
	protected:
		
		void
		assemble_message();

		QString d_html_page;
	};
}


#endif  // GPLATES_QTWIDGETS_PYTHONCONSOLEDIALOG_H
