/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2016 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_OPENPROJECTRELATIVEORABSOLUTEDIALOG_H
#define GPLATES_QTWIDGETS_OPENPROJECTRELATIVEORABSOLUTEDIALOG_H

#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QStringList>

#include "OpenProjectRelativeOrAbsoluteDialogUi.h"


namespace GPlatesQtWidgets
{
	/**
	 * This dialog pops up if the user loads a project file that has moved since it was saved and
	 * where some of the data files (referenced by project) exist relative to both the new
	 * and the original project locations.
	 *
	 * The dialog asks whether to load data files relative to the moved or original locations.
	 */
	class OpenProjectRelativeOrAbsoluteDialog : 
			public QDialog,
			protected Ui_OpenProjectRelativeOrAbsoluteDialog
	{
		Q_OBJECT
		
	public:

		enum Result
		{
			ABORT_OPEN,
			OPEN_ABSOLUTE,
			OPEN_RELATIVE
		};


		explicit
		OpenProjectRelativeOrAbsoluteDialog(
				QWidget *parent_ = NULL);

		virtual
		~OpenProjectRelativeOrAbsoluteDialog()
		{  }


		/**
		 * Set the absolute and relative file paths to be displayed in the dialog.
		 */
		void
		set_file_paths(
				QStringList existing_absolute_file_paths,
				QStringList missing_absolute_file_paths,
				QStringList existing_relative_file_paths,
				QStringList missing_relative_file_paths);

	private Q_SLOTS:

		void
		open_absolute();

		void
		open_relative();

		void
		abort_open();
	};
}

#endif  // GPLATES_QTWIDGETS_OPENPROJECTRELATIVEORABSOLUTEDIALOG_H
