/* $Id$ */

/**
 * \file 
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

#ifndef GPLATES_FILE_IO_FILE_H
#define GPLATES_FILE_IO_FILE_H

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

#include "FeatureCollectionFileFormat.h"
#include "FileInfo.h"

#include "model/FeatureCollectionHandle.h"
#include "model/FeatureCollectionHandleUnloader.h"


namespace GPlatesFileIO
{
	/**
	 * Wrapper to associate a file information with a feature collection loaded from a file and
	 * manage unloading of feature collection.
	 */
	class File :
			private boost::noncopyable
	{
	public:
		/**
		 * Typedef for a shared reference to a file.
		 *
		 * For example:
		 *   File::shared_ref file = File::create_loaded_file(feature_collection, file_info);
		 *   file->get_feature_collection();
		 *   ...
		 *   File file2 = file;
		 *   file2->get_feature_collection();
		 *   ...
		 *   // NOTE: when 'file' and 'file2' both go out of scope the feature collection
		 *   // is unloaded.
		 */
		typedef boost::shared_ptr<File> shared_ref;

		/**
		 * Same as @a shared_ref except references a const @a File.
		 */
		typedef boost::shared_ptr<const File> const_shared_ref;


		/**
		 * Copies a @a shared_ref to a @a const_shared_ref such that
		 * both reference the same @a File object correctly.
		 */
		static
		const_shared_ref
		get_const_shared_ref(
				const shared_ref &ref)
		{
			return boost::static_pointer_cast<const File>(ref);
		}


		/**
		 * Create a file from a lifetime-managed feature collection that was not loaded
		 * from a file.
		 *
		 * A utility function that handles the creation of a @a File object
		 * and wrapping it up in a @a shared_ref.
		 *
		 * The file format is set to UNKNOWN.
		 *
		 * The GPlatesModel::FeatureCollectionHandleUnloader forces us to share the
		 * responsibility of unloading the feature collection - this is based on the
		 * assumption that the feature collection was created to be saved to a file and
		 * we are now responsible for managing its lifetime.
		 */
		static
		shared_ref
		create_empty_file(
				const GPlatesModel::FeatureCollectionHandleUnloader::shared_ref &feature_collection);


		/**
		 * Create a file from a lifetime-managed feature collection and a @a FileInfo
		 * that represents a file that has just been loaded.
		 *
		 * @a file_info should represent the file that @a feature_collection was loaded from.
		 *
		 * A utility function that handles the creation of a @a File object
		 * and wrapping it up in a @a shared_ref.
		 *
		 * The file format is determined from the file extension in @a file_info.
		 *
		 * The GPlatesModel::FeatureCollectionHandleUnloader forces us to share the
		 * responsibility of unloading the feature collection - this is based on the
		 * assumption that the feature collection was created from a file and we
		 * are now responsible for managing its lifetime.
		 */
		static
		shared_ref
		create_loaded_file(
				const GPlatesModel::FeatureCollectionHandleUnloader::shared_ref &feature_collection,
				const FileInfo &file_info);


		/**
		 * Create a file that shares the same lifetime-managed feature collection as @a file
		 * but has a new association with @a file_info which will be used to save the
		 * feature collection (with a new filename for example).
		 *
		 * NOTE: The file format is *not* determined from the file extension in @a file_info -
		 * instead it remains the same as that in the original file @a file.
		 *
		 * Both @a file and the newly created @a File share lifetime management of the
		 * feature collection contained in @a file.
		 */
		static
		shared_ref
		create_save_file(
				const File &file,
				const FileInfo &file_info);


		/**
		 * Returns const reference to feature collection.
		 *
		 * NOTE: If returned feature collection reference outlives all @a shared_ref references
		 * then it will get invalidated since the feature collection will be unloaded.
		 */
		GPlatesModel::FeatureCollectionHandle::const_weak_ref
		get_feature_collection() const
		{
			return GPlatesModel::FeatureCollectionHandle::get_const_weak_ref(
					d_feature_collection->get_feature_collection());
		}


		/**
		 * Returns non-const reference to feature collection.
		 *
		 * NOTE: If returned feature collection reference outlives all @a shared_ref references
		 * then it will get invalidated since the feature collection will be unloaded.
		 */
		GPlatesModel::FeatureCollectionHandle::weak_ref
		get_feature_collection()
		{
			return d_feature_collection->get_feature_collection();
		}


		/**
		 * Returns const reference to feature collection.
		 *
		 * NOTE: If returned feature collection reference outlives all @a shared_ref references
		 * then it will get invalidated since the feature collection will be unloaded.
		 */
		GPlatesModel::FeatureCollectionHandle::const_weak_ref
		get_const_feature_collection() const
		{
			return GPlatesModel::FeatureCollectionHandle::get_const_weak_ref(
					d_feature_collection->get_feature_collection());
		}


		/**
		 * Returns const reference to @a FileInfo object associated with this file.
		 *
		 * If created using @a create_loaded_file then contains information about
		 * the file that the feature collection was loaded from.
		 * If created using @a create_save_file then contains information about
		 * the file that the feature collection will be saved to (eg, if the
		 * user saves file with a different filename).
		 */
		const FileInfo &
		get_file_info() const
		{
			return d_file_info;
		}


		/**
		 * Returns the file format associated with this file when it was loaded.
		 *
		 * Returns FeatureCollectionFileFormat::UNKNOWN if this was created with
		 * @a create_empty_file or if the file format was not recognised.
		 *
		 * NOTE: If this file was created with @a create_save_file then the file format
		 * *still* represents the file format when the file was loaded.
		 * This is so the file format accurately reflects the feature collection (see
		 * comment below about modifying FileInfo for more detail).
		 */
		FeatureCollectionFileFormat::Format
		get_loaded_file_format() const
		{
			return d_loaded_file_format;
		}


		//
		// Note: setting or modifying the internal @a FileInfo is not allowed.
		//
		// This is to keep the feature collection and @a FileInfo in sync - although
		// it's still possible to alter the feature collection under the hood so
		// this situation needs to be dealt with better - in fact the whole concept of
		// @a File should be scrutinised - it is really an intermediate solution
		// until a better design involving the model is devised - FIXME.
		//
		// If you wish to modify the @a FileInfo then use @a create_save_file to create
		// another @a File using the same feature collection but with a different @a FileInfo.
		//
		// An example of getting out of sync is classifying the feature collection as
		// reconstructable or reconstruction which could be done by first looking at the
		// file format in FileInfo (currently based on the filename extension) and if
		// it was loaded from a PLATES rotation file for example then we know it can
		// only be reconstruction features. If the file format allows both types only then
		// does it look at the features in the collection itself.
		//
		// Code that does this sort of thing should use @a get_loaded_file_format instead
		// of looking at the @a FileInfo since the @a File object may have been created with
		// @a create_save_file and therefore have a different @a FileInfo that does not
		// reflect the classification of the feature collection.
		//

	private:
		//! The lifetime-managed feature collection.
		GPlatesModel::FeatureCollectionHandleUnloader::shared_ref d_feature_collection;

		//! Information about the file that the feature collection was loaded from or saved to.
		FileInfo d_file_info;

		//! The format of the file that the feature collection was loaded from.
		FeatureCollectionFileFormat::Format d_loaded_file_format;


		/**
		 * Constructor.
		 *
		 * Is private so that only way to create is via @a create_loaded_file or @a create_save_file.
		 */
		File(
				const GPlatesModel::FeatureCollectionHandleUnloader::shared_ref &feature_collection,
				const FileInfo &file_info,
				const FeatureCollectionFileFormat::Format file_format);
	};
}

#endif // GPLATES_FILE_IO_FILE_H
