/* $Id$ */

/**
 * @file 
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007, 2008 The University of Sydney, Australia
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

#ifndef GPLATES_FILEIO_FILEINFO_H
#define GPLATES_FILEIO_FILEINFO_H

#include <QString>
#include <QFileInfo>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include "model/ConstFeatureVisitor.h"
#include "model/FeatureCollectionHandle.h"
#include "FileFormat.h"

namespace GPlatesFileIO {
	
	/**
	 * Holds information associated with a loaded file.
	 *
	 * FileInfo is used to keep track of information relating to a file that
	 * has been loaded into GPlates.  It keeps track of basic file information
	 * with a QFileInfo object, but it also keeps track of the file's data format, 
	 * it's line-ending style, and the FeatureCollectionHandle containing the
	 * features that were loaded from the file.
	 */
	class FileInfo 
	{
	public:
		// XXX: these belongs elsewhere
		enum LineEndingStyle
		{
			NL, CRNL, CR 
		};

		// FIXME: Factor this into FileFormat, perhaps. Or maybe not, since we can only
		// really detect 'XML' and 'GZIP' instead of 'GPML' and 'GPML.GZ'.
		// See FileInfo::identify_by_magic_number();
		enum FileMagic
		{
			UNKNOWN, XML, GZIP
		};

		/**
		 * Construct a FileInfo for the given file_name.
		 * The file_name does not have to exist.
		 *
		 * No FeatureCollection is assigned to the FileInfo yet,
		 * use set_feature_collection() to do so.
		 */
		explicit
		FileInfo(
				QString file_name):
			d_file_info(file_name)
		{ }

		/**
		 * Construct a FileInfo without any file associated with
		 * it. This may seem a little weird, but is necessary
		 * to handle the concept of the user creating a new "empty"
		 * FeatureCollection within GPlates, and enabling it to
		 * show up in the list of active reconstructable files
		 * (so that it can be displayed on the globe, and saved via
		 * the ManageFeatureCollections dialog).
		 *
		 * No FeatureCollection is assigned to the FileInfo yet,
		 * use set_feature_collection() to do so.
		 */
		FileInfo():
			d_file_info()
		{ }


		/**
		 * Returns a string that can be used by the GUI to identify
		 * this file. Usually, this will simply be the file name -
		 * however, if a FileInfo has been created without any file
		 * association (e.g. a blank FeatureCollection created during
		 * digitisation), it will simply return "New Feature Collection".
		 */
		QString
		get_display_name(
				bool use_absolute_path_name) const;


		const QFileInfo &
		get_qfileinfo() const
		{
			return d_file_info;
		}


		boost::optional<GPlatesModel::FeatureCollectionHandle::weak_ref>
		get_feature_collection() 
		{ 
			return d_feature_collection; 
		}


		boost::optional<GPlatesModel::FeatureCollectionHandle::const_weak_ref>
		get_feature_collection() const
		{
			boost::optional<GPlatesModel::FeatureCollectionHandle::const_weak_ref> res;
			if (d_feature_collection) {
				res = GPlatesModel::FeatureCollectionHandle::get_const_weak_ref(*d_feature_collection);
			}
			return res;
		}


		void
		set_feature_collection(
				GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection)
		{
			d_feature_collection = feature_collection;
		}


		boost::shared_ptr< GPlatesModel::ConstFeatureVisitor >
		get_writer() const;


		bool
		is_writable() const
		{
			QFileInfo dir(get_qfileinfo().path());
			return dir.permission(QFile::WriteUser);
		}
		
		/**
		 * Briefly opens the file for reading, and attempts to identify the
		 * format of the file by it's magic number.
		 *
		 * This function can and will throw ErrorOpeningFileForReadingException.
		 */
		FileMagic
		identify_by_magic_number() const;

	private:
#if 0
		FileInfo(
				QFileInfo file_info,
				FileFormat file_format,
				LineEndingStyle line_ending,
				GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection) :
			d_file_info(file_info), d_file_format(file_format),
			d_line_ending(line_ending), d_feature_collection(feature_collection) { }
#endif

		QFileInfo d_file_info;
		//FileFormat d_file_format;
		//LineEndingStyle d_line_ending;
		boost::optional<GPlatesModel::FeatureCollectionHandle::weak_ref> d_feature_collection;
	};
}

#endif // GPLATES_FILEIO_FILEINFO_H
