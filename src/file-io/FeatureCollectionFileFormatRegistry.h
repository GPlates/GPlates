/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#ifndef GPLATES_FILE_IO_FEATURECOLLECTIONFILEFORMATREGISTRY_H
#define GPLATES_FILE_IO_FEATURECOLLECTIONFILEFORMATREGISTRY_H

#include <map>
#include <vector>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <QFileInfo>
#include <QString>

#include "FeatureCollectionFileFormat.h"
#include "FeatureCollectionFileFormatClassify.h"
#include "File.h"

#include "model/FeatureCollectionHandle.h"
#include "model/FeatureVisitor.h"
#include "model/ModelInterface.h"


namespace GPlatesFileIO
{
	class FileInfo;
	struct ReadErrorAccumulation;

	namespace FeatureCollectionFileFormat
	{
		/**
		 * Base class for specifying configuration options (such as for reading and/or writing a
		 * feature collection from/to a file).
		 *
		 * If a file format requires specialised options then create a derived
		 * @a Configuration class for it and register that with @a Registry.
		 */
		class Configuration
		{
		public:
			//! Typedef for a shared pointer to const @a Configuration.
			typedef boost::shared_ptr<const Configuration> shared_ptr_to_const_type;
			//! Typedef for a shared pointer to @a Configuration.
			typedef boost::shared_ptr<Configuration> shared_ptr_type;

			//! Creates a non-derived @a Configuration.
			static
			shared_ptr_type
			create(
					Format file_format)
			{
				return shared_ptr_type(new Configuration(file_format));
			}

			virtual
			~Configuration()
			{  }

			//! Returns the file format of this configuration.
			Format
			get_file_format() const
			{
				return d_file_format;
			}

		protected:
			Format d_file_format;

			explicit
			Configuration(
					Format file_format) :
				d_file_format(file_format)
			{  }
		};


		/**
		 * Stores information concerning feature collection file formats and reading/writing to/from them.
		 */
		class Registry :
				public boost::noncopyable
		{
		public:
			/**
			 * Convenience typedef for a function that determines if a specified file format is recognised for reading.
			 *
			 * The function takes the following arguments:
			 * - A 'QFileInfo' representing a filename.
			 * - A 'QString' that is the registered filename extension for the file format.
			 *
			 * Returns true if the file format is recognised.
			 */
			typedef boost::function<bool (const QFileInfo&, const QString &)> is_read_file_format_function_type;

			/**
			 * Convenience typedef for a function that determines if a specified file format is recognised for writing.
			 *
			 * The function takes the following arguments:
			 * - A 'QFileInfo' representing a filename.
			 * - A 'QString' that is the registered filename extension for the file format.
			 *
			 * Returns true if the file format is recognised.
			 */
			typedef boost::function<bool (const QFileInfo&, const QString &)> is_write_file_format_function_type;

			/**
			 * Convenience typedef for a function that reads a feature collection from a file.
			 *
			 * The function takes the following arguments:
			 * - A reference to the file to contain the feature collection that is read,
			 * - A structure to report read errors to.
			 * - Options to use when reading from feature collection.
			 */
			typedef boost::function<
					void (
							const File::Reference &,
							ReadErrorAccumulation &,
							const Configuration::shared_ptr_to_const_type &)>
									read_feature_collection_function_type;

			/**
			 * Convenience typedef for a function that creates a feature visitor that writes features to a file.
			 *
			 * The function takes the following arguments:
			 * - A feature collection,
			 * - The file to write the feature collection to,
			 * - Options to use when writing the feature collection.
			 *
			 * Returns the create feature visitor.
			 */
			typedef boost::function<
					boost::shared_ptr<GPlatesModel::ConstFeatureVisitor> (
							const GPlatesModel::FeatureCollectionHandle::const_weak_ref &,
							const FileInfo &,
							const Configuration::shared_ptr_to_const_type &)>
									create_feature_collection_writer_function_type;


			/**
			 * Stores information about the given @a file_format.
			 *
			 * @param short_description a very short description of the file format (eg, "ESRI shapefile").
			 * @param filename_extensions the filename extensions of the file format.
			 *        Note that there must be at least one extension and the first one is the primary one.
			 * @param is_file_format_function used to recognise a file format.
			 * @param read_feature_collection_function used to read a feature collection.
			 *        Is boost::none if file format does not support reading feature collections.
			 * @param write_feature_collection_function used to write a feature collection.
			 *        Is boost::none if file format does not support writing feature collections.
			 * @param default_configuration the default configuration to use when reading/writing a feature collection.
			 *        Is boost::none if file format does not have configuration options.
			 */
			void
			register_file_format(
					Format file_format,
					const QString &short_description,
					const std::vector<QString> &filename_extensions,
					const classifications_type &feature_classification,
					const is_read_file_format_function_type &is_read_file_format_function_type,
					const is_write_file_format_function_type &is_write_file_format_function_type,
					const boost::optional<read_feature_collection_function_type> &read_feature_collection_function,
					const boost::optional<create_feature_collection_writer_function_type> &create_feature_collection_writer_function,
					const boost::optional<Configuration::shared_ptr_to_const_type> &default_configuration);

			/**
			 * Unregisters the specified file format.
			 */
			void
			unregister_file_format(
					Format file_format);


			/**
			 * Returns a list of all registered file formats.
			 */
			std::vector<Format>
			get_registered_file_formats() const;


			/**
			 * Returns a short description of the specified @a file_format.
			 *
			 * The description is short enough to be used in file dialogs.
			 * For example, "ESRI shapefile".
			 *
			 * @throws FileFormatNotSupportedException if file format is not registered.
			 */
			const QString &
			get_short_description(
					Format file_format) const;


			/**
			 * Returns the primary filename extension associated with @a file_format.
			 *
			 * For example, "gpml" or "rot" - note that the first '.' is removed.
			 * An example of a double-barreled extension is "gpml.gz".
			 *
			 * @throws FileFormatNotSupportedException if file format is not registered.
			 */
			QString
			get_primary_filename_extension(
					Format file_format) const;

			/**
			 * Returns the primary and alternative filename extensions associated with @a file_format.
			 *
			 * Note that only the primary extension is returned if the specified file format only has one extension.
			 *
			 * @throws FileFormatNotSupportedException if file format is not registered.
			 */
			const std::vector<QString> &
			get_all_filename_extensions(
					Format file_format) const;


			/**
			 * Returns the classification of features that the specified file format can read/write.
			 *
			 * This can be used, for example, to determine which file formats are available for saving.
			 */
			classifications_type
			get_feature_classification(
					Format file_format) const;


			/**
			 * Returns true if the specified file format supports *reading* feature collections from files.
			 *
			 * This is determined by looking at the filename extension and/or the file contents.
			 *
			 * @throws FileFormatNotSupportedException if file format is not registered.
			 */
			bool
			does_file_format_support_reading(
					Format file_format) const;

			/**
			 * Determine the feature collection file format that supports *reading* from the file described by @a file_info.
			 *
			 * Returns boost::none if a recognised file format does not support reading (eg, only writing).
			 *
			 * This is determined by looking at the filename extension and/or the file contents.
			 */
			boost::optional<Format>
			get_read_file_format(
					const QFileInfo& file_info) const;


			/**
			 * Returns true if the specified file format supports *writing* feature collections to files.
			 *
			 * Also note that this differs from @a does_file_format_support_reading in that the file contents
			 * cannot be read to help determine the file format (because the file is being written to).
			 * Hence the determination is purely based on the filename (extension).
			 *
			 * @throws FileFormatNotSupportedException if file format is not registered.
			 */
			bool
			does_file_format_support_writing(
					Format file_format) const;

			/**
			 * Determine the feature collection file format that supports *writing* to the file described by @a file_info.
			 *
			 * Returns boost::none if a recognised file format does not support writing (eg, only reading).
			 * 
			 * Also note that this differs from @a get_read_file_format in that the file contents
			 * cannot be read to help determine the file format (because the file is being written to).
			 * Hence the determination is purely based on the filename (extension).
			 */
			boost::optional<Format>
			get_write_file_format(
					const QFileInfo& file_info) const;


			/**
			 * Reads features from file @a file_ref into the file's feature collection.
			 *
			 * @param file_ref both filename of file to read from and feature collection to store in.
			 * @param read_errors to contain errors reading file.
			 * @param configuration determines the file format and any options to use when reading
			 *        from the file - if none are provided then the file format is determined from
			 *        the filename of @a file_ref and the default configuration registered for that
			 *        file format is used.
			 *
			 * @throws FileFormatNotSupportedException if file format does not support reading
			 * (eg, only writing) or file format is not registered.
			 */
			void
			read_feature_collection(
					const File::Reference &file_ref,
					ReadErrorAccumulation &read_errors,
					boost::optional<Configuration::shared_ptr_to_const_type> configuration = boost::none) const;

			/**
			 * Writes features from the specified feature collection to the specified output file.
			 *
			 * @param feature_collection the feature collection that will be written.
			 * @param file_info output file that feature collection is written to.
			 * @param configuration options used when writing to the file - if none are provided
			 *        then the default configuration registered for the file format is used.
			 * @param configuration determines the file format and any options to use when writing
			 *        to the file - if none are provided then the file format is determined from
			 *        the filename of @a file_ref and the default configuration registered for that
			 *        file format is used.
			 *
			 * @throws ErrorOpeningFileForWritingException if file is not writable.
			 * @throws FileFormatNotSupportedException if file format does not support writing
			 * (eg, only reading) or file format is not registered.
			 */
			void
			write_feature_collection(
					const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection,
					const FileInfo &file_info,
					boost::optional<Configuration::shared_ptr_to_const_type> configuration = boost::none) const;


			/**
			 * Returns the default configuration options for the specified file format.
			 *
			 * If a base @a Configuration instance is returned then the file format has no configuration options.
			 * Some file formats don't really need any options.
			 *
			 * @throws FileFormatNotSupportedException if file format no been registered.
			 */
			const Configuration::shared_ptr_to_const_type &
			get_default_configuration(
					Format file_format) const;

			/**
			 * Sets the default configuration options for the specified file format.
			 *
			 * @throws FileFormatNotSupportedException if file format no been registered.
			 */
			void
			set_default_configuration(
					Format file_format,
					const Configuration::shared_ptr_to_const_type &default_configuration);

		private:
			struct FileFormatInfo
			{
				FileFormatInfo(
						const QString &short_description_,
						const std::vector<QString> &filename_extensions_,
						const classifications_type &feature_classification_,
						const is_read_file_format_function_type &is_read_file_format_function_,
						const is_write_file_format_function_type &is_write_file_format_function_,
						const boost::optional<read_feature_collection_function_type> &read_feature_collection_function_,
						const boost::optional<create_feature_collection_writer_function_type> &create_feature_collection_writer_function_,
						const Configuration::shared_ptr_to_const_type &default_configuration_) :
					short_description(short_description_),
					filename_extensions(filename_extensions_),
					feature_classification(feature_classification_),
					is_read_file_format_function(is_read_file_format_function_),
					is_write_file_format_function(is_write_file_format_function_),
					read_feature_collection_function(read_feature_collection_function_),
					create_feature_collection_writer_function(create_feature_collection_writer_function_),
					default_configuration(default_configuration_)
				{  }

				QString short_description;
				std::vector<QString> filename_extensions;
				classifications_type feature_classification;
				is_read_file_format_function_type is_read_file_format_function;
				is_write_file_format_function_type is_write_file_format_function;
				boost::optional<read_feature_collection_function_type> read_feature_collection_function;
				boost::optional<create_feature_collection_writer_function_type> create_feature_collection_writer_function;
				Configuration::shared_ptr_to_const_type default_configuration;
			};

			typedef std::map<Format, FileFormatInfo> file_format_info_map_type;


			GPlatesModel::ModelInterface d_model;

			/**
			 * Stores a struct of information for each file format.
			 */
			file_format_info_map_type d_file_format_info_map;


			//! Returns file format info for specified file format, otherwise throws @a FileFormatNotSupportedException.
			const FileFormatInfo &
			get_file_format_info(
					Format file_format) const;

			//! Returns file format info for specified file format, otherwise throws @a FileFormatNotSupportedException.
			FileFormatInfo &
			get_file_format_info(
					Format file_format);
		};


		/**
		 * Registers information about the default feature collection file formats with the given @a registry.
		 */
		void
		register_default_file_formats(
				Registry &registry,
				GPlatesModel::ModelInterface &model);
	}
}

#endif // GPLATES_FILE_IO_FEATURECOLLECTIONFILEFORMATREGISTRY_H
