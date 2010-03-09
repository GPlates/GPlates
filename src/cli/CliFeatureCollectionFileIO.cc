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

#include "file-io/FeatureCollectionReaderWriter.h"
#include "file-io/ReadErrorAccumulation.h"


GPlatesCli::FeatureCollectionFileIO::FeatureCollectionFileIO(
		GPlatesModel::ModelInterface &model,
		const boost::program_options::variables_map &command_line_variables) :
	d_model(model),
	d_command_line_variables(&command_line_variables)
{
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
		GPlatesFileIO::ReadErrorAccumulation read_errors;

		// Read the feature collection from the file.
		const QString filename(filename_iter->c_str());
		const GPlatesFileIO::FileInfo file_info(filename);
		GPlatesFileIO::File::shared_ref file =
				GPlatesFileIO::read_feature_collection(file_info, d_model, read_errors);

		files.push_back(file);
	}
}


void
GPlatesCli::extract_feature_collections(
		std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &feature_collections,
		FeatureCollectionFileIO::feature_collection_file_seq_type &files)
{
	// Load each feature collection file.
	FeatureCollectionFileIO::feature_collection_file_seq_type::iterator file_iter = files.begin();
	FeatureCollectionFileIO::feature_collection_file_seq_type::iterator file_end = files.end();
	for ( ; file_iter != file_end; ++file_iter)
	{
		feature_collections.push_back((*file_iter)->get_feature_collection());
	}
}
