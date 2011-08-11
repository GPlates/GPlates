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

#include "FeatureCollectionFileFormatClassify.h"

#include "FeatureCollectionFileFormat.h"

#include "app-logic/AppLogicUtils.h"
#include "app-logic/ReconstructMethodRegistry.h"
#include "app-logic/TopologyUtils.h"

#include "feature-visitors/TotalReconstructionSequencePlateIdFinder.h"


namespace GPlatesFileIO
{
	namespace FeatureCollectionFileFormat
	{
		namespace
		{
			/**
			 * Used to test a single ClassificationType from a classifications_type.
			 */
			class Classify
			{
			public:
				Classify(
						ClassificationType classification_) :
					classification(classification_)
				{  }

				bool
				operator()(
						const classifications_type &
								classifications) const
				{
					return classifications.test(classification);
				}

			private:
				ClassificationType classification;
			};


			/**
			 * Extracts and returns the feature classification of the specified feature.
			 */
			void
			get_feature_classification(
					classifications_type &classifications,
					const GPlatesModel::FeatureHandle::const_weak_ref &feature,
					const GPlatesAppLogic::ReconstructMethodRegistry &reconstruct_method_registry)
			{
				// Check if the feature is topological.
				if (GPlatesAppLogic::TopologyUtils::is_topological_closed_plate_boundary_feature(*feature.handle_ptr()) ||
					GPlatesAppLogic::TopologyUtils::is_topological_network_feature(*feature.handle_ptr()))
				{
					classifications.set(TOPOLOGICAL);
				}

				// Check if the feature is reconstructable - and if so record its reconstruct method type.
				const boost::optional<GPlatesAppLogic::ReconstructMethod::Type> reconstruction_method_type =
						reconstruct_method_registry.get_reconstruct_method_type(feature);
				if (reconstruction_method_type)
				{
					classifications.set(reconstruction_method_type.get());
				}

				// Check if the feature is a reconstruction features.
				GPlatesFeatureVisitors::TotalReconstructionSequencePlateIdFinder reconstruction_sequence_finder;
				reconstruction_sequence_finder.visit_feature(feature);
				if (reconstruction_sequence_finder.fixed_ref_frame_plate_id() ||
					reconstruction_sequence_finder.moving_ref_frame_plate_id())
				{
					classifications.set(RECONSTRUCTION);
				}
			}
		}
	}
}


GPlatesFileIO::FeatureCollectionFileFormat::classifications_type
GPlatesFileIO::FeatureCollectionFileFormat::classify(
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection,
		const GPlatesAppLogic::ReconstructMethodRegistry &reconstruct_method_registry)
{
	classifications_type classification;

	// Iterate through the features in the feature collection.
	GPlatesModel::FeatureCollectionHandle::const_iterator feature_iter = feature_collection->begin();
	GPlatesModel::FeatureCollectionHandle::const_iterator feature_end = feature_collection->end();
	for ( ; feature_iter != feature_end; ++feature_iter)
	{
		get_feature_classification(classification, (*feature_iter)->reference(), reconstruct_method_registry);
	}

	return classification;
}


GPlatesFileIO::FeatureCollectionFileFormat::classifications_type
GPlatesFileIO::FeatureCollectionFileFormat::classify(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature,
		const GPlatesAppLogic::ReconstructMethodRegistry &reconstruct_method_registry)
{
	classifications_type classification;

	get_feature_classification(classification, feature, reconstruct_method_registry);

	return classification;
}


bool
GPlatesFileIO::FeatureCollectionFileFormat::find_classified_features(
		std::vector<GPlatesModel::FeatureHandle::weak_ref> &found_features,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection,
		const GPlatesAppLogic::ReconstructMethodRegistry &reconstruct_method_registry,
		ClassificationType classification)
{
	return find_classified_features(
			found_features,
			feature_collection,
			reconstruct_method_registry,
			Classify(classification));
}


bool
GPlatesFileIO::FeatureCollectionFileFormat::find_classified_features(
		std::vector<GPlatesModel::FeatureHandle::weak_ref> &found_features,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection,
		const GPlatesAppLogic::ReconstructMethodRegistry &reconstruct_method_registry,
		const classification_predicate_type &classification_predicate)
{
	if (!feature_collection.is_valid())
	{
		return false;
	}

	bool found = false;

	// Iterate through the features in the feature collection.
	GPlatesModel::FeatureCollectionHandle::iterator feature_iter = feature_collection->begin();
	GPlatesModel::FeatureCollectionHandle::iterator feature_end = feature_collection->end();
	for ( ; feature_iter != feature_end; ++feature_iter)
	{
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref = (*feature_iter)->reference();

		classifications_type classification;
		get_feature_classification(classification, feature_ref, reconstruct_method_registry);

		// If the feature has the required classification then add it to the caller's sequence.
		if (classification_predicate(classification))
		{
			found_features.push_back(feature_ref);
			found = true;
		}
	}

	return found;
}
