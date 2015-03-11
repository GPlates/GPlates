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

#include "File.h"

#include "FileInfo.h"

#include "model/Model.h"


GPlatesFileIO::File::non_null_ptr_type
GPlatesFileIO::File::create_file(
		const FileInfo &file_info,
		const GPlatesModel::FeatureCollectionHandle::non_null_ptr_type &feature_collection,
		boost::optional<FeatureCollectionFileFormat::Configuration::shared_ptr_to_const_type> file_configuration)
{
	return non_null_ptr_type(new File(feature_collection, file_info, file_configuration));
}


GPlatesFileIO::File::Reference::non_null_ptr_type
GPlatesFileIO::File::create_file_reference(
		const FileInfo &file_info,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection,
		boost::optional<FeatureCollectionFileFormat::Configuration::shared_ptr_to_const_type> file_configuration)
{
	return File::Reference::non_null_ptr_type(new File::Reference(feature_collection, file_info, file_configuration));
}


GPlatesFileIO::File::File(
		const GPlatesModel::FeatureCollectionHandle::non_null_ptr_type &feature_collection,
		const FileInfo &file_info,
		boost::optional<FeatureCollectionFileFormat::Configuration::shared_ptr_to_const_type> file_configuration) :
	d_file(new Reference(feature_collection->reference(), file_info, file_configuration)),
	d_feature_collection_handle(feature_collection)
{
}


GPlatesFileIO::File::Reference::non_null_ptr_type
GPlatesFileIO::File::add_feature_collection_to_model(
		GPlatesModel::ModelInterface &model)
{
	// If we've already added the feature collection handle to the model then
	// return our internal File which should now contain a weak reference to the
	// feature collection handle stored in the model.
	if (!d_feature_collection_handle)
	{
		return d_file;
	}

	// Add the feature collection handle to the model.
	model->root()->add(d_feature_collection_handle.get());

	// Now that we've added the feature collection handle to the model we should
	// release our ownership of it.
	d_feature_collection_handle = boost::none;

	return d_file;
}


GPlatesFileIO::File::Reference::Reference(
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection,
		const FileInfo &file_info,
		boost::optional<FeatureCollectionFileFormat::Configuration::shared_ptr_to_const_type> file_configuration) :
	d_feature_collection(feature_collection),
	d_file_info(file_info),
	d_file_configuration(file_configuration)
{
}
