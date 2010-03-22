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

#include "AppLogicUtils.h"

#include "feature-visitors/FeatureClassifier.h"

#include "file-io/FeatureCollectionFileFormat.h"


namespace
{
	/**
	 * Used to test a single ClassificationType from a classifications_type.
	 */
	class Classify
	{
	public:
		Classify(
				GPlatesAppLogic::ClassifyFeatureCollection::ClassificationType classification_) :
			classification(classification_)
		{  }

		bool
		operator()(
				const GPlatesAppLogic::ClassifyFeatureCollection::classifications_type &
						classifications) const
		{
			return classifications.test(classification);
		}

	private:
		GPlatesAppLogic::ClassifyFeatureCollection::ClassificationType classification;
	};


	/**
	 * Extracts and returns the feature classification from @a classifier.
	 */
	GPlatesAppLogic::ClassifyFeatureCollection::classifications_type
	get_classification(
			const GPlatesFeatureVisitors::FeatureClassifier &classifier)
	{
		GPlatesAppLogic::ClassifyFeatureCollection::classifications_type classifications;

		// Check if the feature collection contains reconstructable features.
		if (classifier.reconstructable_feature_count() > 0)
		{
			classifications.set(GPlatesAppLogic::ClassifyFeatureCollection::RECONSTRUCTABLE);
		}

		// Check if the feature collection contains reconstruction features.
		if (classifier.reconstruction_feature_count() > 0)
		{
			classifications.set(GPlatesAppLogic::ClassifyFeatureCollection::RECONSTRUCTION);
		}

		// Check if the feature collection contains instantaneous features.
		if (classifier.instantaneous_feature_count() > 0)
		{
			classifications.set(GPlatesAppLogic::ClassifyFeatureCollection::INSTANTANEOUS);
		}

		return classifications;
	}
}


GPlatesAppLogic::ClassifyFeatureCollection::classifications_type
GPlatesAppLogic::ClassifyFeatureCollection::classify_feature_collection(
		const GPlatesFileIO::File &file)
{
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
	GPlatesFeatureVisitors::FeatureClassifier classifier;

	// Visit the feature collection with the classifier.
	AppLogicUtils::visit_feature_collection(feature_collection, classifier);

	// Get the classifications from the classifier.
	const classifications_type classifications = get_classification(classifier);

	return classifications;
}


bool
GPlatesAppLogic::ClassifyFeatureCollection::find_classified_features(
		std::vector<GPlatesModel::FeatureHandle::weak_ref> &found_features,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection,
		ClassificationType classification)
{
	return find_classified_features(
			found_features,
			feature_collection,
			Classify(classification));
}


bool
GPlatesAppLogic::ClassifyFeatureCollection::find_classified_features(
		std::vector<GPlatesModel::FeatureHandle::weak_ref> &found_features,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection,
		const classification_predicate_type &classification_predicate)
{
	if (!feature_collection.is_valid())
	{
		return false;
	}

	GPlatesFeatureVisitors::FeatureClassifier feature_classifier;
	bool found = false;

	// Iterate through the features in the feature collection.
	GPlatesModel::FeatureCollectionHandle::children_iterator feature_iter =
			feature_collection->children_begin();
	GPlatesModel::FeatureCollectionHandle::children_iterator feature_end =
			feature_collection->children_end();
	for ( ; feature_iter != feature_end; ++feature_iter)
	{
		// Reset the classifier and visit a new feature to be classified.
		feature_classifier.reset();
		if (feature_classifier.visit_feature(feature_iter))
		{
			// Get the classifications from the classifier.
			const classifications_type classifications = get_classification(feature_classifier);

			// If the feature has the required classification then add it to the caller's sequence.
			if (classification_predicate(classifications))
			{
				// We've already checked that the feature iterator is valid when we called
				// 'visit_feature()' above.
				const GPlatesModel::FeatureHandle::weak_ref &feature_ref =
						(*feature_iter)->reference();

				found_features.push_back(feature_ref);
				found = true;
			}
		}
	}

	return found;
}
