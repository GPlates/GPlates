/* $Id$ */

/**
 * @file 
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007, 2008, 2009 The University of Sydney, Australia
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

#include <QFileInfo>
#include <QMap>
#include <QString>
#include <boost/optional.hpp>

#include "model/FeatureCollectionHandle.h"


namespace GPlatesFileIO
{
	/**
	 * Holds information associated with a loaded file.
	 *
	 * FileInfo is used to keep track of information relating to a file that has been loaded
	 * into GPlates.  It keeps track of basic file information using a QFileInfo object, but it
	 * also keeps track of the file's data format, its line-ending style, and the
	 * feature-collection which contains the features that were loaded from the file.
	 *
	 * JC says that FileInfo was not ever intended to act as a handle to a feature-collection,
	 * instead being more like "a filename string but with some extra methods".  As a result,
	 * it's copied and modified "all over the place".  Hence, when a FileInfo instance is
	 * destroyed, it will @strong not unload its associated feature-collection from the
	 * feature-store automatically.  If the feature-collection should be unloaded, this must be
	 * performed manually.
	 */
	class FileInfo 
	{
	public:
		// XXX: these belong elsewhere
		enum LineEndingStyle
		{
			NL, CRNL, CR 
		};


		/**
		 * Construct a FileInfo for the given @a file_name.
		 * The @a file_name does not have to exist.
		 *
		 * No FeatureCollection is assigned to the FileInfo yet;
		 * use set_feature_collection() to do so.
		 */
		explicit
		FileInfo(
				const QString &file_name):
			d_file_info(file_name)
		{  }


		/**
		 * Construct a FileInfo without any associated file.
		 *
		 * This may seem a little weird, but is necessary to enable the user to create a
		 * new empty FeatureCollection within GPlates and have it show up in the list of
		 * active reconstructable files (so that it can be displayed on the globe, and
		 * saved via the ManageFeatureCollections dialog).
		 *
		 * No FeatureCollection is assigned to the FileInfo yet;
		 * use set_feature_collection() to do so.
		 */
		FileInfo():
			d_file_info()
		{  }


		/**
		 * Destructor.
		 *
		 * Note that when a FileInfo instance is destroyed, it will @strong not unload its
		 * associated feature-collection from the feature-store automatically (read the
		 * class comment for the reasons).  If the feature-collection should be unloaded,
		 * this must be performed manually.
		 */
		~FileInfo();


		/**
		 * Return a string that can be used by the GUI to identify this file.
		 *
		 * Usually, this will simply be the file name -- however, if a FileInfo has been
		 * created without any file association (e.g. a blank FeatureCollection created
		 * during digitisation), it will simply return "New Feature Collection".
		 */
		const QString
		get_display_name(
				bool use_absolute_path_name) const;


		const QFileInfo &
		get_qfileinfo() const
		{
			return d_file_info;
		}


		/**
		 * FIXME:  Is it intended to be possible to modify the feature-collection weak-ref
		 * through this function?
		 */
		const boost::optional<GPlatesModel::FeatureCollectionHandle::weak_ref>
		get_feature_collection() 
		{ 
			return d_feature_collection; 
		}


		const boost::optional<GPlatesModel::FeatureCollectionHandle::const_weak_ref>
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
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
		{
			d_feature_collection = feature_collection;
		}


		/**
		 * Unload the feature-collection which is associated with this FileInfo instance
		 * (if there is one).
		 *
		 * If there is no associated feature-collection, or if the feature-collection
		 * weak-ref is invalid (perhaps the feature-collection was already unloaded?), this
		 * will be a no-op.
		 *
		 * Return true if a feature-collection was unloaded, false otherwise.
		 */
		bool
		unload_feature_collection();


		const QMap< QString,QString >& 
		get_model_to_shapefile_map() const
		{
			return d_model_to_shapefile_map;
		}

		void
		set_model_to_shapefile_map(
			QMap< QString, QString > model_to_shapefile_map) const
		{
			d_model_to_shapefile_map = model_to_shapefile_map;
		}

	private:
#if 0
		FileInfo(
				const QFileInfo &file_info,
				const FileFormat &file_format,
				const LineEndingStyle &line_ending,
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection):
			d_file_info(file_info),
			d_file_format(file_format),
			d_line_ending(line_ending),
			d_feature_collection(feature_collection)
		{  }
#endif

		QFileInfo d_file_info;
		//FileFormat d_file_format;
		//LineEndingStyle d_line_ending;
		boost::optional<GPlatesModel::FeatureCollectionHandle::weak_ref> d_feature_collection;

		/**
		 * A map of model property names to shapefile attributes names.                                                                     
		 */
		mutable QMap< QString,QString > d_model_to_shapefile_map;
	};

	//@{
	/**
	 * Returns 'true' if specified file is writable.
	 *
	 * @param file_info file to test for writability.
	 */
	bool
	is_writable(
			const QFileInfo &file_info);


	inline
	bool
	is_writable(
			const FileInfo &file_info)
	{
		return is_writable(file_info.get_qfileinfo());
	}
	//@}

}

#endif // GPLATES_FILEIO_FILEINFO_H
