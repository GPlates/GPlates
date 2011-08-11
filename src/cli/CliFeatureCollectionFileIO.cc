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

#include "CliFeatureCollectionFileIO.h"
#include "CliInvalidOptionValue.h"

#include "file-io/ReadErrorAccumulation.h"


namespace
{
	/**
	 * Removes filename extension.
	 *
	 * Also removes '.*.gz' if found to support removing '.gpml.gz'.
	 */
	QString
	remove_filename_extension(
			const QString &filename)
	{
		const int ext_index = filename.lastIndexOf('.');
		if (ext_index < 0)
		{
			// No extension found.
			return filename;
		}

		// Remove extension.
		QString output_basename = filename.left(ext_index);

		// If the extension is "gz" then we have a "gpml.gz" extension so
		// remove the "gpml" part as well.
		if (filename.mid(ext_index + 1) == "gz")
		{
			const int next_ext_index = output_basename.lastIndexOf('.');
			if (next_ext_index >= 0)
			{
				output_basename = output_basename.left(next_ext_index);
			}
		}

		return output_basename;
	}


	void
	prepend_filename_prefix(
			QString &filename,
			const QString &filename_prefix)
	{
		filename.prepend(filename_prefix);
	}


	void
	append_filename_suffix(
			QString &filename,
			const QString &filename_suffix)
	{
		filename.append(filename_suffix);
	}


	void
	append_filename_extension(
			QString &filename,
			const QString &filename_extension)
	{
		filename.append('.').append(filename_extension);
	}
}


const std::string GPlatesCli::FeatureCollectionFileIO::SAVE_FILE_TYPE_GPML = "gpml";
const std::string GPlatesCli::FeatureCollectionFileIO::SAVE_FILE_TYPE_GPMLZ = "compressed-gpml";
const std::string GPlatesCli::FeatureCollectionFileIO::SAVE_FILE_TYPE_PLATES_LINE = "plates4-line";
const std::string GPlatesCli::FeatureCollectionFileIO::SAVE_FILE_TYPE_PLATES_ROTATION = "plates4-rotation";
const std::string GPlatesCli::FeatureCollectionFileIO::SAVE_FILE_TYPE_SHAPEFILE = "shapefile";
const std::string GPlatesCli::FeatureCollectionFileIO::SAVE_FILE_TYPE_GMT = "gmt";
const std::string GPlatesCli::FeatureCollectionFileIO::SAVE_FILE_TYPE_GMAP = "vgp";

GPlatesCli::FeatureCollectionFileIO::FeatureCollectionFileIO(
		GPlatesModel::ModelInterface &model,
		const boost::program_options::variables_map &command_line_variables) :
	d_model(model),
	d_file_format_registry(),
	d_command_line_variables(&command_line_variables)
{
	register_default_file_formats(d_file_format_registry, d_model);
}


GPlatesCli::FeatureCollectionFileIO::feature_collection_file_seq_type
GPlatesCli::FeatureCollectionFileIO::load_files(
		const std::string &option_name)
{
	if (!d_command_line_variables->count(option_name))
	{
		throw RequiredOptionNotPresent(
				GPLATES_EXCEPTION_SOURCE, option_name.c_str());
	}

	// Get the feature collection filenames.
	const std::vector<std::string> filenames =
			(*d_command_line_variables)[option_name].as< std::vector<std::string> >();

	feature_collection_file_seq_type feature_collection_file_seq;
	load_feature_collections(filenames, feature_collection_file_seq);

	return feature_collection_file_seq;
}


void
GPlatesCli::FeatureCollectionFileIO::load_feature_collections(
		const std::vector<std::string> &filenames,
		feature_collection_file_seq_type &files)
{
	// Load each feature collection file.
	std::vector<std::string>::const_iterator filename_iter = filenames.begin();
	std::vector<std::string>::const_iterator filename_end = filenames.end();
	for ( ; filename_iter != filename_end; ++filename_iter)
	{
		// Read the feature collection from the file.
		const QString filename(filename_iter->c_str());
		const GPlatesFileIO::FileInfo file_info(filename);

		// Create a file with an empty feature collection.
		GPlatesFileIO::File::non_null_ptr_type file = GPlatesFileIO::File::create_file(file_info);

		// Read new features from the file into the feature collection.
		// Both the filename and target feature collection are in 'file_ref'.
		GPlatesFileIO::ReadErrorAccumulation read_errors;
		d_file_format_registry.read_feature_collection(file->get_reference(), read_errors);

		files.push_back(file);
	}
}


void
GPlatesCli::FeatureCollectionFileIO::extract_feature_collections(
		std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &feature_collections,
		FeatureCollectionFileIO::feature_collection_file_seq_type &files)
{
	// Load each feature collection file.
	FeatureCollectionFileIO::feature_collection_file_seq_type::iterator file_iter = files.begin();
	FeatureCollectionFileIO::feature_collection_file_seq_type::iterator file_end = files.end();
	for ( ; file_iter != file_end; ++file_iter)
	{
		feature_collections.push_back((*file_iter)->get_reference().get_feature_collection());
	}
}


void
GPlatesCli::FeatureCollectionFileIO::save_file(
		const GPlatesFileIO::FileInfo &file_info,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	d_file_format_registry.write_feature_collection(feature_collection, file_info);
}


GPlatesFileIO::FeatureCollectionFileFormat::Format
GPlatesCli::FeatureCollectionFileIO::get_save_file_format(
		const std::string &save_file_type)
{
	if (save_file_type == SAVE_FILE_TYPE_GPML)
	{
		return GPlatesFileIO::FeatureCollectionFileFormat::GPML;
	}
	else if (save_file_type == SAVE_FILE_TYPE_GPMLZ)
	{
		return GPlatesFileIO::FeatureCollectionFileFormat::GPMLZ;
	}
	else if (save_file_type == SAVE_FILE_TYPE_PLATES_LINE)
	{
		return GPlatesFileIO::FeatureCollectionFileFormat::PLATES4_LINE;
	}
	else if (save_file_type == SAVE_FILE_TYPE_PLATES_ROTATION)
	{
		return GPlatesFileIO::FeatureCollectionFileFormat::PLATES4_ROTATION;
	}
	else if (save_file_type == SAVE_FILE_TYPE_SHAPEFILE)
	{
		return GPlatesFileIO::FeatureCollectionFileFormat::SHAPEFILE;
	}
	else if (save_file_type == SAVE_FILE_TYPE_GMT)
	{
		return GPlatesFileIO::FeatureCollectionFileFormat::WRITE_ONLY_XY_GMT;
	}
	else if (save_file_type == SAVE_FILE_TYPE_GMAP)
	{
		return GPlatesFileIO::FeatureCollectionFileFormat::GMAP;
	}

	throw GPlatesCli::InvalidOptionValue(
			GPLATES_EXCEPTION_SOURCE,
			save_file_type.c_str());
}


GPlatesFileIO::FileInfo
GPlatesCli::FeatureCollectionFileIO::get_save_file_info(
		const GPlatesFileIO::FileInfo &file_info,
		GPlatesFileIO::FeatureCollectionFileFormat::Format save_file_format,
		const QString &filename_prefix,
		const QString &filename_suffix)
{
	//
	// Generate the output filename.
	//
	QString output_filename = file_info.get_qfileinfo().filePath();
	output_filename = remove_filename_extension(output_filename);
	prepend_filename_prefix(output_filename, filename_prefix);
	append_filename_suffix(output_filename, filename_suffix);
	append_filename_extension(
			output_filename,
			d_file_format_registry.get_primary_filename_extension(save_file_format));

	return GPlatesFileIO::FileInfo(output_filename);
}


GPlatesFileIO::FileInfo
GPlatesCli::FeatureCollectionFileIO::get_save_file_info(
		const QString &filename_no_extension,
		GPlatesFileIO::FeatureCollectionFileFormat::Format save_file_format)
{
	//
	// Generate the output filename.
	//
	QString output_filename(filename_no_extension);
	append_filename_extension(
			output_filename,
			d_file_format_registry.get_primary_filename_extension(save_file_format));

	return GPlatesFileIO::FileInfo(output_filename);
}
