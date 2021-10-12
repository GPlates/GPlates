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

#include "ClassifyFeatureCollection.h"

#include "feature-visitors/FeatureCollectionClassifier.h"

#include "file-io/FeatureCollectionFileFormat.h"


GPlatesAppLogic::ClassifyFeatureCollection::classifications_type
GPlatesAppLogic::ClassifyFeatureCollection::classify_feature_collection(
		const GPlatesFileIO::File &file)
{
	GPlatesFeatureVisitors::FeatureCollectionClassifier classifier;

	// First try classifying by the file type.
	// This is because certain file types are known to contain only one type of feature.
	const GPlatesFileIO::FeatureCollectionFileFormat::Format file_format =
			file.get_loaded_file_format();

	// Classify the feature collection
	switch (file_format)
	{
	case GPlatesFileIO::FeatureCollectionFileFormat::GPML:
	case GPlatesFileIO::FeatureCollectionFileFormat::GPML_GZ:
		// GPML format files can contain both reconstructable features and
		// reconstruction trees. Inspect the features to find out which.
		return classify_feature_collection(
				file.get_feature_collection());

	case GPlatesFileIO::FeatureCollectionFileFormat::PLATES4_LINE:
		// PLATES line format files only contain reconstructable features.
		return classifications_type().set(RECONSTRUCTABLE);

	case GPlatesFileIO::FeatureCollectionFileFormat::PLATES4_ROTATION:
		// PLATES rotation format files only contain reconstruction features.
		return classifications_type().set(RECONSTRUCTION);

	case GPlatesFileIO::FeatureCollectionFileFormat::SHAPEFILE:
		// Shapefiles only contain reconstructable features.
		return classifications_type().set(RECONSTRUCTABLE);

	case GPlatesFileIO::FeatureCollectionFileFormat::UNKNOWN:
	default:
		break;
	}

	// We don't know the file type.
	// It could be that the user hasn't saved the file yet so we don't know the file extension.
	// Look at the features to classify the feature collection.
	return classify_feature_collection(file.get_feature_collection());
}


GPlatesAppLogic::ClassifyFeatureCollection::classifications_type
GPlatesAppLogic::ClassifyFeatureCollection::classify_feature_collection(
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection)
{
	classifications_type classifications;
	GPlatesFeatureVisitors::FeatureCollectionClassifier classifier;

	classifier.scan_feature_collection(feature_collection);
	
	// Check if the feature collection contains reconstructable features.
	//
	// We have to be a little cautious in testing for reconstructable
	// features, as if we don't enable them for reconstruction,
	// GPlates just won't bother displaying them. So the present
	// behaviour sets the reconstructable flag IFF (there are
	// reconstructable features OR there is no rotation data present).
	// Having 0 on both counts means that Something Is Amiss and
	// we should not rule out 'reconstructable' as an option.
	if (classifier.reconstructable_feature_count() > 0 ||
		classifier.reconstruction_feature_count() == 0)
	{
		classifications.set(RECONSTRUCTABLE);
	}

	// Check if the file_it contains reconstruction features.
	//
	// In testing for reconstruction features, we can be a little
	// more relaxed, because the set of features which make up
	// a reconstruction tree are smaller and easier to identify.
	// In this case, if we can't identify them, it would be pointless
	// attempting to reconstruct data with them anyway.
	if (classifier.reconstruction_feature_count() > 0)
	{
		classifications.set(RECONSTRUCTION);
	}

	return classifications;
}
