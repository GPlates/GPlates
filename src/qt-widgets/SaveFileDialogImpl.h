/* $Id$ */

/**
 * @file
 *
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
 * (some code in this file was originally part of "SaveFileDialog.h")
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
 
#ifndef GPLATES_QTWIDGETS_SAVEFILEDIALOGIMPL_H
#define GPLATES_QTWIDGETS_SAVEFILEDIALOGIMPL_H

#include <boost/scoped_ptr.hpp>
#include <map>
#include <QFileDialog>

#include "SaveFileDialog.h"


namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	namespace SaveFileDialogInternals
	{
		class SaveFileDialogImpl
		{
		public:

			typedef SaveFileDialog::filter_list_type filter_list_type;

			virtual
			boost::optional<QString>
			get_file_name(
					QString *selected_filter) = 0;

			virtual
			void
			set_filters(
					const filter_list_type &filters) = 0;

			virtual
			void
			select_file(
					const QString &file_path) = 0;

			virtual
			~SaveFileDialogImpl()
			{  }
		};


		/**
		 * Implementation of SaveFileDialog that uses the native dialog.
		 *
		 * This is used on Windows and Mac OS X. On Windows, the native dialog differs
		 * significantly from the Qt dialog in visual appearance. On Mac OS X, the Qt
		 * dialog fails to update the file's file extension when the user selects a new
		 * file format in the combobox; see http://trac.gplates.org/ticket/288.
		 */
		class NativeSaveFileDialog :
				public SaveFileDialogImpl
		{
		public:

			NativeSaveFileDialog(
					QWidget *parent_,
					const QString &caption,
					const filter_list_type &filters,
					GPlatesPresentation::ViewState &view_state);

			boost::optional<QString>
			get_file_name(
					QString *selected_filter);

			void
			set_filters(
					const filter_list_type &filters);

			void
			select_file(
					const QString &file_path);

		private:

			QWidget *d_parent_ptr;
			QString d_caption;
			QString d_filters;
			GPlatesPresentation::ViewState &d_view_state;
			QString d_last_file_name;

			/**
			 * Maps file extension to filter text
			 */
			std::map<QString, QString> d_filter_map_ext_to_text;
		};


		/**
		 * Implementation of SaveFileDialog that uses the Qt dialog.
		 *
		 * This is used on Linux. This uses the Qt dialog instead of the native dialog,
		 * which is used for Windows and Mac OS X. This is because the GTK File Chooser
		 * exhibits behaviour that can be considered suboptimal in relation to filters.
		 * When the user changes the selected filter, the file extension in the text
		 * edit is not changed. It is possible to determine which filter was selected.
		 * But suppose we corrected foo.dat to foo.gpml because the GPML filter was
		 * selected; the warning to the user about overwriting an existing file would
		 * be shown for foo.dat instead, which is problematic if we intend to save it
		 * to foo.gpml.
		 *
		 * GIMP uses the GTK File Chooser and its dialog exhibits this behaviour.
		 * Inkscape also uses the GTK File Chooser, but its dialog has an additional
		 * checkbox (enabled by default) that causes the file extension to be updated
		 * automatically when the user changes the filter. One can therefore conclude
		 * that others also consider the default behaviour of the GTK File Chooser to
		 * be suboptimal; but since we are unable to modify its behaviour via Qt, we
		 * must therefore resort to Qt's own dialog.
		 */
		class QtSaveFileDialog :
				public QObject,
				public SaveFileDialogImpl
		{
			Q_OBJECT
				
		public:

			QtSaveFileDialog(
					QWidget *parent_,
					const QString &caption,
					const filter_list_type &filters,
					GPlatesPresentation::ViewState &view_state);

			boost::optional<QString>
			get_file_name(
					QString *selected_filter);

			void
			set_filters(
					const filter_list_type &filters);

			void
			select_file(
					const QString &file_path);

		private Q_SLOTS:

			void
			handle_filter_changed();

		private:

			GPlatesPresentation::ViewState &d_view_state;
			boost::scoped_ptr<QFileDialog> d_file_dialog_ptr;

			/**
			 * Maps filter text to file extension
			 */
			std::map<QString, QString> d_filter_map_text_to_ext;

			/**
			 * Maps file extension to filter text
			 */
			std::map<QString, QString> d_filter_map_ext_to_text;
		};
	}
}

#endif  // GPLATES_QTWIDGETS_SAVEFILEDIALOGIMPL_H

