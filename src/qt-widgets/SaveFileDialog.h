/* $Id$ */

/**
 * @file
 * Contains definition of SaveFileDialog.
 *
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2009, 2010, 2011 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_SAVEFILEDIALOG_H
#define GPLATES_QTWIDGETS_SAVEFILEDIALOG_H

#include <vector>
#include <boost/optional.hpp>
#include <boost/scoped_ptr.hpp>
#include <QWidget>

#include "FileDialogFilter.h"


namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	// Forward declaration.
	namespace SaveFileDialogInternals
	{
		class SaveFileDialogImpl;
	}

	/**
	 * SaveFileDialog retrieves a file name for saving from the user. Use this in
	 * preference to QFileDialog.
	 *
	 * SaveFileDialog wraps around the Qt save file dialog, adding support for:
	 *  - Remembering the last file location chosen
	 *  - Setting a default prefix depending on which filter the user has chosen
	 *  - Choosing the right options for each operating system
	 */
	class SaveFileDialog
	{
	public:

		/**
		 * Typedef for a sequence of FileDialogFilter.
		 */
		typedef std::vector<FileDialogFilter> filter_list_type;

		~SaveFileDialog();

		/**
		 * Constructs a SaveFileDialog.
		 *
		 * NOTE: This class is not derived from QObject. You will need to manage this
		 * object's lifetime yourself; it is not automatically destroyed by @a parent.
		 *
		 * @param parent The parent window for the dialog box.
		 * @param caption The dialog box's caption.
		 * @param filters A vector of filter descriptions
		 */
		explicit
		SaveFileDialog(
				QWidget *parent,
				const QString &caption,
				const filter_list_type &filters,
				GPlatesPresentation::ViewState &view_state);

		/**
		 * Gets a file name from the user.
		 * @param selected_filter If not NULL, the full text of the selected filter is
		 * returned in this variable. This is useful if there are multiple filters
		 * that target the same file extension (e.g. difference ways to do CSV).
		 * @returns boost::none if the user clicked cancel.
		 */
		boost::optional<QString>
		get_file_name(
				QString *selected_filter = NULL);

		/**
		 * Changes the filters used by the dialog box.
		 * @param filters The same as the filters parameter in the constructor.
		 */
		void
		set_filters(
				const filter_list_type &filters);

		/**
		 * Selects a file in the dialog box.
		 * @param file_path The path to the file to be selected.
		 */
		void
		select_file(
				const QString &file_path);

	private:

		boost::scoped_ptr<SaveFileDialogInternals::SaveFileDialogImpl> d_impl;
	};
}

#endif  // GPLATES_QTWIDGETS_SAVEFILEDIALOG_H

