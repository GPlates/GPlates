/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
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

#include <boost/optional.hpp>

#include "FeatureCollectionFileFormatConfiguration.h"
#include "FileInfo.h"

#include "model/FeatureCollectionHandle.h"
#include "model/ModelInterface.h"

#include "utils/ReferenceCount.h"


namespace GPlatesFileIO
{
	/**
	 * A wrapper around a file that owns a feature collection (that has not been added to the model).
	 */
	class File :
			public GPlatesUtils::ReferenceCount<File>
	{
	public:
		/**
		 * Interface to get file information associated with a feature collection loaded from or
		 * saved to a file.
		 */
		class Reference :
				public GPlatesUtils::ReferenceCount<Reference>
		{
		public:
			//! A convenience typedef for a non-null intrusive pointer to a non-const @a Reference.
			typedef GPlatesUtils::non_null_intrusive_ptr<Reference> non_null_ptr_type;


			/**
			 * Returns 'const' reference to feature collection.
			 */
			GPlatesModel::FeatureCollectionHandle::const_weak_ref
			get_feature_collection() const
			{
				return d_feature_collection;
			}

			/**
			 * Returns 'non-const' reference to feature collection.
			 */
			GPlatesModel::FeatureCollectionHandle::weak_ref
			get_feature_collection()
			{
				return d_feature_collection;
			}


			/**
			 * Returns const reference to @a FileInfo object associated with this file.
			 */
			const FileInfo &
			get_file_info() const
			{
				return d_file_info;
			}


			/**
			 * Returns the file configuration.
			 *
			 * This is used when reading/writing the feature collection from/to a file.
			 * If boost::none is returned then the default file configuration for this file's format
			 * is used when reading/writing the file.
			 */
			const boost::optional<FeatureCollectionFileFormat::Configuration::shared_ptr_to_const_type> &
			get_file_configuration() const
			{
				return d_file_configuration;
			}


			/**
			 * Modifies the file information and the file configuration (read/write options).
			 *
			 * This is useful when you want to save the file with a different filename or change the
			 * read/write options.
			 */
			void
			set_file_info(
					const FileInfo &file_info,
					boost::optional<FeatureCollectionFileFormat::Configuration::shared_ptr_to_const_type> file_configuration = boost::none)
			{
				d_file_info = file_info;
				d_file_configuration = file_configuration;
			}

		private:
			/**
			 * Weak reference to a feature collection handle.
			 *
			 * The feature collection handle could be stored in the model or in @a File
			 * as a 'FeatureCollectionHandle::non_null_ptr_type'.
			 */
			GPlatesModel::FeatureCollectionHandle::weak_ref d_feature_collection;

			//! Information about the file that the feature collection was loaded from or saved to.
			FileInfo d_file_info;

			//! Optional file configuration (used when reading and writing feature collection from/to file).
			boost::optional<FeatureCollectionFileFormat::Configuration::shared_ptr_to_const_type> d_file_configuration;


			/**
			 * Constructor.
			 *
			 * Is private so that only way to create is using friend class @a File.
			 */
			Reference(
					const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection,
					const FileInfo &file_info,
					boost::optional<FeatureCollectionFileFormat::Configuration::shared_ptr_to_const_type> file_configuration);

			friend class File;
		};


		//! A convenience typedef for a non-null intrusive pointer to a non-const @a File.
		typedef GPlatesUtils::non_null_intrusive_ptr<File> non_null_ptr_type;


		/**
		 * Create a @a File object with feature collection @a feature_collection.
		 *
		 * Note that this does not load/save the feature collection from/to an actual
		 * file in the file system.
		 *
		 * @a file_info should represent the file that @a feature_collection was read from
		 * (or will be saved to) or it should represent (if @a feature_collection is empty)
		 * the file that you will subsequently read new features from in order to populate
		 * the internal feature collection (which will initially be empty).
		 * Note: In order to populate the internal feature collection pass the File::Reference
		 * of the returned @a File to a feature collection file reader.
		 *
		 * The default value for @a file_info represents no filename.
		 *
		 * The default value for @a feature_collection is a new empty feature collection.
		 *
		 * @a file_configuration determines the file format and any options to use when reading/writing
		 *        the file - if not provided then the file format is determined from @a file_info
		 *        and the file configuration used is the one registered for its file format.
		 *
		 * The internal feature_collection is a non_null_ptr_type and has not been added
		 * to the model so the returned @a File is now responsible for managing its lifetime
		 * until @a add_feature_collection_to_model is called to transfer ownership to the model.
		 */
		static
		File::non_null_ptr_type
		create_file(
				const FileInfo &file_info = FileInfo(),
				const GPlatesModel::FeatureCollectionHandle::non_null_ptr_type &
						feature_collection = GPlatesModel::FeatureCollectionHandle::create(),
				boost::optional<FeatureCollectionFileFormat::Configuration::shared_ptr_to_const_type>
						file_configuration = boost::none);


		/**
		 * Create a @a Reference object with feature collection @a feature_collection.
		 *
		 * Note that this references an existing feature collection and does not create a new one.
		 *
		 * @a file_info should represent the file that @a feature_collection was read from
		 * (or will be saved to).
		 */
		static
		File::Reference::non_null_ptr_type
		create_file_reference(
				const FileInfo &file_info,
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection,
				boost::optional<FeatureCollectionFileFormat::Configuration::shared_ptr_to_const_type>
						file_configuration = boost::none);


		/**
		 * Returns a reference to a usable file interface.
		 */
		const Reference &
		get_reference() const
		{
			return *d_file;
		}


		/**
		 * Returns a reference to a usable file interface.
		 */
		Reference &
		get_reference()
		{
			return *d_file;
		}


		/**
		 * Adds the feature collection contained within to @a model.
		 *
		 * Ownership of the internal feature collection is *transferred* to @a model.
		 *
		 * The returned @a Reference is also referenced (shared) internally so the @a get_reference
		 * can still be used.
		 *
		 * The returned reference can now be used instead of 'this' and 'this' be can destroyed
		 * as it's no longer needed.
		 */
		Reference::non_null_ptr_type
		add_feature_collection_to_model(
				GPlatesModel::ModelInterface &model);

	private:
		Reference::non_null_ptr_type d_file;

		/**
		 * The feature collection handle before it is added, if ever, to the model.
		 * It is optional so we can release it when we add it to the model since
		 * the model will then 'own' it.
		 */
		boost::optional<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type> d_feature_collection_handle;


		/**
		 * Constructor.
		 *
		 * Is private so that only way to create is via @a create_file.
		 */
		File(
				const GPlatesModel::FeatureCollectionHandle::non_null_ptr_type &feature_collection,
				const FileInfo &file_info,
				boost::optional<FeatureCollectionFileFormat::Configuration::shared_ptr_to_const_type> file_configuration);
	};
}

#endif // GPLATES_FILE_IO_FILE_H
