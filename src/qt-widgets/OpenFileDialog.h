/* $Id: SaveFileDialog.h 10510 2010-12-11 02:25:34Z elau $ */

/**
 * @file
 * Contains definition of OpenFileDialog.
 *
 * $Revision: 10510 $
 * $Date: 2010-12-11 13:25:34 +1100 (Sat, 11 Dec 2010) $ 
 * 
 * Copyright (C) 2010, 2011 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_OPENFILEDIALOG_H
#define GPLATES_QTWIDGETS_OPENFILEDIALOG_H

#include <QWidget>
#include <QString>
#include <QStringList>

#include "FileDialogFilter.h"

#include "presentation/ViewState.h"


namespace GPlatesQtWidgets
{
	class OpenFileDialog
	{
	public:

		/**
		 * Constructs an OpenFileDialog with a sequence of @a FileDialogFilter specified by
		 * @a filters_begin and @a filters_end.
		 */
		template<typename Iterator>
		OpenFileDialog(
				QWidget *parent,
				const QString &caption,
				Iterator filters_begin,
				Iterator filters_end,
				GPlatesPresentation::ViewState &view_state) :
			d_parent(parent),
			d_caption(caption),
			d_last_open_directory(view_state.get_last_open_directory()),
			d_filter(FileDialogFilter::create_filter_string(filters_begin, filters_end))
		{  }

		/**
		 * Constructs an OpenFileDialog with a preformatted @a filter, which should
		 * look something like:
		 *     "Text Documents (*.txt *.foo);;All Files (*)"
		 */
		OpenFileDialog(
				QWidget *parent,
				const QString &caption,
				const QString &filter,
				GPlatesPresentation::ViewState &view_state);

		/**
		 * Prompts the user to select one file name and returns it.
		 * If the user clicks cancel, returns the empty string.
		 */
		QString
		get_open_file_name();

		/**
		 * Prompts the user to select at least one file name and returns them in a
		 * list. If the user clicks cancel, returns an empty list.
		 */
		QStringList
		get_open_file_names();

	private:

		QWidget *d_parent;
		QString d_caption;
		QString &d_last_open_directory;
		QString d_filter;
		QString d_selected_filter;
	};
}

#endif  // GPLATES_QTWIDGETS_OPENFILEDIALOG_H

