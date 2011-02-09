/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
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
 
#ifndef GPLATES_QTWIDGETS_PYTHONREADLINEDIALOG_H
#define GPLATES_QTWIDGETS_PYTHONREADLINEDIALOG_H

#include <QDialog>
#include <QString>
#include <QPoint>

#include "PythonReadlineDialogUi.h"


namespace GPlatesQtWidgets
{
	class PythonReadlineDialog : 
			public QDialog,
			protected Ui_PythonReadlineDialog 
	{
	public:

		explicit
		PythonReadlineDialog(
				QWidget *parent_ = NULL);

		/**
		 * Opens this dialog as modal and returns the string that the user enters.
		 */
		QString
		get_line(
				const QString &prompt);

	private:

		QPoint d_pos;
	};
}

#endif  // GPLATES_QTWIDGETS_PYTHONREADLINEDIALOG_H
