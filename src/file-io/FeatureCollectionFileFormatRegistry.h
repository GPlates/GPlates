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
#include "FeatureCollectionFileFormatConfiguration.h"
#include "File.h"

#include "model/FeatureCollectionHandle.h"
#include "model/FeatureVisitor.h"
#include "model/ModelInterface.h"


namespace GPlatesModel
{
	class Gpgim;
}

namespace GPlatesFileIO
{
	class FileInfo;
	struct ReadErrorAccumulation;

	namespace FeatureCollectionFileFormat
	{
		/**
		 * Stores information concerning feature collection file formats and reading/writing to/from them.
		 */
		class Registry :
				public boost::noncopyable
		{
		public:
			/**
			 * Convenience typedef for a function that determines if a specified file format is recognised.
			 *
			 * The function takes the following arguments:
			 * - A 'QFileInfo' representing a filename.
			 * - A 'QString' that is the registered filename extension for the file format.
			 *
			 * Returns true if the file format is recognised.
			 */
			typedef boost::function<bool (const QFileInfo&, const QString &)> is_file_format_function_type;

			/**
			 * Convenience typedef for a function that reads a feature collection from a file.
			 *
			 * The function takes the following arguments:
			 * - A reference to the file to contain the feature collection that is read,
			 * - A structure to report read errors to.
			 * - A reference to an unsaved changes flag (for changes made just after reading file).
			 */
			typedef boost::function<
					void (
							File::Reference &,
							ReadErrorAccumulation &,
							bool &)>
									read_feature_collection_function_type;

			/**
			 * Convenience typedef for a function that creates a feature visitor that writes features to a file.
			 *
			 * The function takes the following arguments:
			 * - A reference to the file containing the feature collection to write,
			 *
			 * Returns the create feature visitor.
			 */
			typedef boost::function<
					boost::shared_ptr<GPlatesModel::ConstFeatureVisitor> (
							File::Reference &)>
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
			 * @param create_feature_collection_writer_function used to write a feature collection.
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
					const is_file_format_function_type &is_file_format_function,
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
			get_all_filename_extensions_for_format(
					Format file_format) const;

			/**
			 * Returns the primary and alternative filename extensions associated with all registered file formats.
			 */
			void
			get_all_filename_extensions(
					std::vector<QString> &filename_extensions) const;


			/**
			 * Returns the classification of features that the specified file format can read/write.
			 *
			 * This can be used, for example, to determine which file formats are available for saving.
			 */
			classifications_type
			get_feature_classification(
					Format file_format) const;


			/**
			 * Determine the feature collection file format of the file described by @a file_info.
			 *
			 * Returns boost::none if no file formats are recognised.
			 *
			 * This is determined by looking at the filename extension (and/or the file contents
			 * if the file exists).
			 */
			boost::optional<Format>
			get_file_format(
					const QFileInfo& file_info) const;


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
			 * Reads features from file @a file_ref into the file's feature collection.
			 *
			 * @param file_ref both filename of file to read from and feature collection to store in.
			 * @param read_errors to contain errors reading file.
			 * @param contains_unsaved_changes if specified then returns true if changes were made
			 *        to one or more features after reading from file (eg, to conform to GPGIM).
			 *
			 * @throws FileFormatNotSupportedException if file format does not support reading
			 * (eg, only writing) or file format is not registered.
			 */
			void
			read_feature_collection(
					File::Reference &file_ref,
					ReadErrorAccumulation &read_errors,
					boost::optional<bool &> contains_unsaved_changes = boost::none) const;

			/**
			 * Writes features to the specified file @a file_ref.
			 *
			 * @param file_ref both filename of file to write to and feature collection to write.
			 *
			 * @throws ErrorOpeningFileForWritingException if file is not writable.
			 * @throws FileFormatNotSupportedException if file format does not support writing
			 * (eg, only reading) or file format is not registered.
			 */
			void
			write_feature_collection(
					File::Reference &file_ref) const;


			/**
			 * Returns the default configuration options for the specified file format.
			 *
			 * If there is no default then boost::none is returned.
			 *
			 * If a base @a Configuration instance is returned then the file format has no configuration options.
			 * Some file formats don't really need any options.
			 *
			 * @throws FileFormatNotSupportedException if file format no been registered.
			 */
			const boost::optional<Configuration::shared_ptr_to_const_type> &
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
						const is_file_format_function_type &is_file_format_function_,
						const boost::optional<read_feature_collection_function_type> &read_feature_collection_function_,
						const boost::optional<create_feature_collection_writer_function_type> &create_feature_collection_writer_function_,
						const boost::optional<Configuration::shared_ptr_to_const_type> &default_configuration_) :
					short_description(short_description_),
					filename_extensions(filename_extensions_),
					feature_classification(feature_classification_),
					is_file_format_function(is_file_format_function_),
					read_feature_collection_function(read_feature_collection_function_),
					create_feature_collection_writer_function(create_feature_collection_writer_function_),
					default_configuration(default_configuration_)
				{  }

				QString short_description;
				std::vector<QString> filename_extensions;
				classifications_type feature_classification;
				is_file_format_function_type is_file_format_function;
				boost::optional<read_feature_collection_function_type> read_feature_collection_function;
				boost::optional<create_feature_collection_writer_function_type> create_feature_collection_writer_function;
				boost::optional<Configuration::shared_ptr_to_const_type> default_configuration;
			};

			typedef std::map<Format, FileFormatInfo> file_format_info_map_type;


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
				GPlatesModel::ModelInterface model,
				const GPlatesModel::Gpgim &gpgim);
	}
}

#endif // GPLATES_FILE_IO_FEATURECOLLECTIONFILEFORMATREGISTRY_H
