/* $Id$ */


/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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
 
#ifndef GPLATES_CLI_CLILOADFEATURECOLLECTIONS_H
#define GPLATES_CLI_CLILOADFEATURECOLLECTIONS_H

#include <string>
#include <vector>
#include <boost/program_options/variables_map.hpp>
#include <QString>

#include "CliRequiredOptionNotPresent.h"

#include "file-io/FeatureCollectionFileFormat.h"
#include "file-io/File.h"

#include "model/FeatureCollectionHandle.h"
#include "model/ModelInterface.h"


namespace GPlatesCli
{
	/**
	 * This is a replacement for the FeatureCollectionFileIO in namespace GPlatesAppLogic
	 * in that it doesn't require a @a FeatureCollectionFileState.
	 * This is one level lower in that it works directly with feature collections and
	 * their containing @a File objects which is more suitable for the command-line interface.
	 */
	class FeatureCollectionFileIO
	{
	public:
		/**
		 * Typedef for a sequence of files each containing a feature collection.
		 */
		typedef std::vector<GPlatesFileIO::File::shared_ref> feature_collection_file_seq_type;


		/**
		 * @a model will be used to create feature collections and @a command_line_variables
		 * will be used to search for filenames specified on the command-line.
		 */
		FeatureCollectionFileIO(
				GPlatesModel::ModelInterface &model,
				const boost::program_options::variables_map &command_line_variables);


		/**
		 * Load feature collection files using filenames specified via the
		 * command-line option @a option_name.
		 *
		 * The returned sequence of files contain and manage memory of the
		 * feature collections contained within.
		 * The feature collections will exist as long as the returned file sequence exists.
		 */
		feature_collection_file_seq_type
		load_files(
				const std::string &option_name);


		/**
		 * Extracts the feature collections from their containing @a File objects.
		 *
		 * Extracted feature collections are appended to @a feature_collections.
		 */
		static
		void
		extract_feature_collections(
				std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &feature_collections,
				FeatureCollectionFileIO::feature_collection_file_seq_type &files);


		/**
		 * Write the feature collection associated to a file described by @a file_info.
		 */
		static
		void
		save_file(
				const GPlatesFileIO::FileInfo &file_info,
				const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection);


		//
		// Values specified by user on command-line for the save file type.
		//
		static const std::string SAVE_FILE_TYPE_GPML;
		static const std::string SAVE_FILE_TYPE_GPML_GZ;
		static const std::string SAVE_FILE_TYPE_PLATES_LINE;
		static const std::string SAVE_FILE_TYPE_PLATES_ROTATION;
		static const std::string SAVE_FILE_TYPE_SHAPEFILE;
		static const std::string SAVE_FILE_TYPE_GMT;
		static const std::string SAVE_FILE_TYPE_GMAP;

		/**
		 * Returns the save filename by changing the extension of @a file_info using
		 * the save file format of @a save_file_format.
		 */
		static
		GPlatesFileIO::FeatureCollectionFileFormat::Format
		get_save_file_format(
				const std::string &save_file_type);

		/**
		 * Returns the save filename by appending the filename extension determined by
		 * @a save_file_format to @a filename_no_extension.
		 */
		static
		GPlatesFileIO::FileInfo
		get_save_file_info(
				const QString &filename_no_extension,
				GPlatesFileIO::FeatureCollectionFileFormat::Format save_file_format);

		/**
		 * Returns the save filename by appending the filename extension determined by
		 * @a save_file_type to @a filename_no_extension.
		 */
		static
		GPlatesFileIO::FileInfo
		get_save_file_info(
				const QString &filename_no_extension,
				const std::string &save_file_type)
		{
			return get_save_file_info(
					filename_no_extension,
					get_save_file_format(save_file_type));
		}

		/**
		 * Returns the save filename by changing the extension of @a file_info using
		 * the save file format of @a save_file_format.
		 */
		static
		GPlatesFileIO::FileInfo
		get_save_file_info(
				const GPlatesFileIO::FileInfo &file_info,
				GPlatesFileIO::FeatureCollectionFileFormat::Format save_file_format,
				const QString &filename_prefix = "",
				const QString &filename_suffix = "");

		/**
		 * Returns the save filename by changing the extension of @a file_info using
		 * the save file format of @a save_file_format.
		 */
		static
		GPlatesFileIO::FileInfo
		get_save_file_info(
				const GPlatesFileIO::FileInfo &file_info,
				const std::string &save_file_type,
				const QString &filename_prefix = "",
				const QString &filename_suffix = "")
		{
			return get_save_file_info(
					file_info,
					get_save_file_format(save_file_type),
					filename_prefix,
					filename_suffix);
		}

	private:
		/**
		 * Used to create feature collections when loading files.
		 */
		GPlatesModel::ModelInterface d_model;

		/**
		 * The command-line variables are stored here.
		 */
		const boost::program_options::variables_map *d_command_line_variables;


		void
		load_feature_collections(
				const std::vector<std::string> &filenames,
				feature_collection_file_seq_type &files);
	};
}

#endif // GPLATES_CLI_CLILOADFEATURECOLLECTIONS_H
