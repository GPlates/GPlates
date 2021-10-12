/* $Id$ */

/**
 * @file
 * Contains definition of SaveFileDialog.
 *
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
 
#ifndef GPLATES_QTWIDGETS_SAVEFILEDIALOG_H
#define GPLATES_QTWIDGETS_SAVEFILEDIALOG_H

#include <map>
#include <vector>
#include <utility>
#include <boost/optional.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <QObject>
#include <QWidget>
#include <QString>
#include <QStringList>
#include <QFileDialog>

namespace GPlatesQtWidgets
{
	/**
	 * SaveFileDialog wraps around the Qt save file dialog, adding support for:
	 *  - Remembering the last file location chosen
	 *  - Setting a default prefix depending on which filter the user has chosen
	 *  - Choosing the right options for each operating system
	 */
	class SaveFileDialog
	{
	public:

		/**
		 * Gets a file name from the user.
		 * @returns boost::none if the user clicked cancel.
		 */
		virtual
		boost::optional<QString>
		get_file_name() = 0;

		/**
		 * Changes the filters used by the dialog box.
		 * @param filters The same as the filters parameter in the constructor.
		 * @param default_filter The same as the default_filter parameter in the constructor.
		 */
		virtual
		void
		set_filters(
				const std::vector<std::pair<QString, QString> > &filters,
				unsigned int default_filter) = 0;

		/**
		 * Changes the filters used by the dialog box, and keeps the last used filter
		 * as default, if it is still there.
		 * @param filters The same as the filters parameter in the constructor.
		 */
		virtual
		void
		set_filters(
				const std::vector<std::pair<QString, QString> > &filters) = 0;

		/**
		 * Selects a file in the dialog box.
		 * @param file The path to the file to be selected.
		 */
		virtual
		void
		select_file(
				const QString &file_path) = 0;

		//! Destructor
		virtual
		~SaveFileDialog();
		
		/**
		 * Returns a pointer to a platform-specific instance of SaveFileDialog.
		 * @param parent The parent window for the dialog box.
		 * @param caption The dialog box's caption.
		 * @param filters A vector of filter descriptions. Each filter description
		 * is a pair of strings; the first is the text displayed in the dialog box
		 * for that filter; the second is the default file extension for that filter.
		 * The default file extension is ignored on Windows, which derives the
		 * default file extension from the text displayed (the first string).
		 * @param default_filter The index of the filter to be selected by default.
		 * If the value is out of range, the first filter is used.
		 */
		static
		boost::shared_ptr<SaveFileDialog> get_save_file_dialog(
				QWidget *parent,
				const QString &caption,
				const std::vector<std::pair<QString, QString> > &filters,
				unsigned int default_filter = 0);

	};

	/**
	 * Implementation of SaveFileDialog for Windows
	 */
	class WindowsSaveFileDialog :
		public SaveFileDialog
	{
	public:

		WindowsSaveFileDialog(
				QWidget *parent_,
				const QString &caption,
				const std::vector<std::pair<QString, QString> > &filters,
				unsigned int default_filter = 0);

		boost::optional<QString>
		get_file_name();

		void
		set_filters(
				const std::vector<std::pair<QString, QString> > &filters,
				unsigned int default_filter);

		void
		set_filters(
				const std::vector<std::pair<QString, QString> > &filters);

		void
		select_file(
				const QString &file_path);

	private:

		QWidget *d_parent_ptr;
		QString d_caption;
		QString d_filters;
		QString d_last_file_name;
		QString d_last_filter;

		/** Maps file extension to filter text */
		std::map<QString, QString> d_filter_map_ext_to_text;

	};

	/**
	 * Implementation of SaveFileDialog for platforms other than Windows
	 */
	class OtherSaveFileDialog :
		public QObject,
		public SaveFileDialog
	{
		Q_OBJECT
			
	public:

		OtherSaveFileDialog(
				QWidget *parent_,
				const QString &caption,
				const std::vector<std::pair<QString, QString> > &filters,
				unsigned int default_filter = 0);

		boost::optional<QString>
		get_file_name();

		void
		set_filters(
				const std::vector<std::pair<QString, QString> > &filters,
				unsigned int default_filter);

		void
		set_filters(
				const std::vector<std::pair<QString, QString> > &filters);

		void
		select_file(
				const QString &file_path);

	private slots:

		void
		handle_filter_changed();

	private:

		boost::scoped_ptr<QFileDialog> d_file_dialog_ptr;

		/** Maps filter text to file extension */
		std::map<QString, QString> d_filter_map_text_to_ext;

		/** Maps file extension to filter text */
		std::map<QString, QString> d_filter_map_ext_to_text;

	};

}

#endif  // GPLATES_QTWIDGETS_SAVEFILEDIALOG_H

