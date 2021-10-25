/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#include "FeatureCollectionFileFormatConfigurations.h"


const std::string GPlatesFileIO::FeatureCollectionFileFormat::OGRConfiguration::FEATURE_COLLECTION_TAG(
		"model_to_attribute_mapping");


GPlatesFileIO::FeatureCollectionFileFormat::OGRConfiguration::model_to_attribute_map_type &
GPlatesFileIO::FeatureCollectionFileFormat::OGRConfiguration::get_model_to_attribute_map(
		GPlatesModel::FeatureCollectionHandle &feature_collection)
{
	// Look for the model-to-attribute-map tag in the feature collection.
	boost::any &model_to_attribute_map_tag = feature_collection.tags()[FEATURE_COLLECTION_TAG];

	// If first time tag added then set it to an empty model-to-attribute map.
	if (model_to_attribute_map_tag.empty())
	{
		model_to_attribute_map_tag = model_to_attribute_map_type();
	}

	return boost::any_cast<model_to_attribute_map_type &>(model_to_attribute_map_tag);
}

boost::optional<GPlatesPropertyValues::SpatialReferenceSystem::non_null_ptr_to_const_type>
GPlatesFileIO::FeatureCollectionFileFormat::OGRConfiguration::get_original_file_srs() const
{
	return d_original_file_srs;
}

void
GPlatesFileIO::FeatureCollectionFileFormat::OGRConfiguration::set_original_file_srs(
		const GPlatesPropertyValues::SpatialReferenceSystem::non_null_ptr_to_const_type &srs)
{
	d_original_file_srs.reset(srs);
}
