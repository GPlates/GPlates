/* $Id: SaveFileDialog.h 10506 2010-12-11 00:10:52Z elau $ */

/**
 * @file
 *
 * $Revision: 10506 $
 * $Date: 2010-12-11 11:10:52 +1100 (Sat, 11 Dec 2010) $ 
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
 
#ifndef GPLATES_QTWIDGETS_FILEDIALOGFILTER_H
#define GPLATES_QTWIDGETS_FILEDIALOGFILTER_H

#include <boost/optional.hpp>
#include <boost/foreach.hpp>
#include <vector>
#include <QString>
#include <QStringList>


namespace GPlatesQtWidgets
{
	/**
	 * FileDialogFilter encapsulates one file dialog filter entry, that has a
	 * description and a number of file extensions, the first of which is taken to
	 * be the "default" extension for that filter entry.
	 *
	 * If a filter has no file extensions, it is treated as though it were a filter
	 * to match all files "(*)".
	 */
	class FileDialogFilter
	{
	public:

		explicit
		FileDialogFilter(
				const QString &description) :
			d_description(description)
		{  }

		explicit
		FileDialogFilter(
				const QString &description,
				const QString &extension) :
			d_description(description),
			d_extensions(1, extension)
		{  }

		template<typename Iterator>
		explicit
		FileDialogFilter(
				const QString &description,
				Iterator extensions_begin,
				Iterator extensions_end) :
			d_description(description),
			d_extensions(extensions_begin, extensions_end)
		{  }

		/**
		 * Adds @a extension to this filter.
		 *
		 * Note that @a extension should just be the extension, e.g. "gpml" instead of
		 * ".gpml" or "*.gpml".
		 */
		void
		add_extension(
				const QString &extension)
		{
			d_cached_filter_string = boost::none;
			d_extensions.push_back(extension);
		}

		const QString &
		get_description() const
		{
			return d_description;
		}

		const std::vector<QString> &
		get_extensions() const
		{
			return d_extensions;
		}

		/**
		 * Returns the filter as a string that can be used with open and save file dialogs.
		 */
		QString
		create_filter_string() const
		{
			if (d_cached_filter_string)
			{
				return *d_cached_filter_string;
			}

			QStringList starred_extensions;
			BOOST_FOREACH(const QString extension, d_extensions)
			{
				starred_extensions.push_back("*." + extension);
			}
			QString joined_extensions = starred_extensions.isEmpty() ? "*" : starred_extensions.join(" ");

			QString result = d_description + " (" + joined_extensions + ")";
			d_cached_filter_string = result;
			return result;
		}


		/**
		 * Creates the sequence of filters as a string that can be used with open and
		 * save file dialogs.
		 */
		template<typename Iterator>
		static
		QString
		create_filter_string(
				Iterator filters_begin,
				Iterator filters_end)
		{
			QStringList filter_strings;
			for ( ; filters_begin != filters_end; ++filters_begin)
			{
				filter_strings.push_back(filters_begin->create_filter_string());
			}
			return filter_strings.join(";;");
		}

	private:

		QString d_description;
		std::vector<QString> d_extensions;
		mutable boost::optional<QString> d_cached_filter_string;
	};
}

#endif  // GPLATES_QTWIDGETS_FILEDIALOGFILTER_H

