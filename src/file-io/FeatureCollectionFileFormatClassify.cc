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
#include "app-logic/ExtractRasterFeatureProperties.h"
#include "app-logic/ExtractScalarField3DFeatureProperties.h"
#include "app-logic/ReconstructMethodRegistry.h"
#include "app-logic/TopologyUtils.h"

#include "feature-visitors/TotalReconstructionSequencePlateIdFinder.h"

#include <boost/foreach.hpp>

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
					const GPlatesAppLogic::ReconstructMethodRegistry &reconstruct_method_registry,
					const std::vector<GPlatesAppLogic::ReconstructMethod::Type> &reconstruct_methods)
			{
				// Check the reconstruction method types that can reconstruct the feature.
				BOOST_FOREACH(GPlatesAppLogic::ReconstructMethod::Type reconstruct_method, reconstruct_methods)
				{
					if (!classifications.test(reconstruct_method)) // Only test if not classified already...
					{
						// Check if the feature is can be reconstructed with the current reconstruct method type.
						if (reconstruct_method_registry.can_reconstruct_feature(reconstruct_method, feature))
						{
							classifications.set(reconstruct_method);
							// Each feature has one reconstruct method so no need to test the
							// remaining reconstruct methods for *this* feature.
							break;
						}
					}
				}

				// Check if the feature is a raster.
				if (!classifications.test(RASTER)) // Only test if not classified already...
				{
					if (GPlatesAppLogic::is_raster_feature(feature))
					{
						classifications.set(RASTER);
					}
				}

				// Check if the feature is a scalar field.
				if (!classifications.test(SCALAR_FIELD_3D)) // Only test if not classified already...
				{
					if (GPlatesAppLogic::is_scalar_field_3d_feature(feature))
					{
						classifications.set(SCALAR_FIELD_3D);
					}
				}

				// Check if the feature is topological.
				if (!classifications.test(TOPOLOGICAL)) // Only test if not classified already...
				{
					if (GPlatesAppLogic::TopologyUtils::is_topological_geometry_feature(feature))
					{
						classifications.set(TOPOLOGICAL);
					}
				}

				// Check if the feature is a reconstruction features.
				if (!classifications.test(RECONSTRUCTION)) // Only test if not classified already...
				{
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
}


GPlatesFileIO::FeatureCollectionFileFormat::classifications_type
GPlatesFileIO::FeatureCollectionFileFormat::classify(
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection,
		const GPlatesAppLogic::ReconstructMethodRegistry &reconstruct_method_registry)
{
	classifications_type classification;

	const std::vector<GPlatesAppLogic::ReconstructMethod::Type> registered_reconstruct_methods =
			reconstruct_method_registry.get_registered_reconstruct_methods();

	// Iterate through the features in the feature collection.
	GPlatesModel::FeatureCollectionHandle::const_iterator feature_iter = feature_collection->begin();
	GPlatesModel::FeatureCollectionHandle::const_iterator feature_end = feature_collection->end();
	for ( ; feature_iter != feature_end; ++feature_iter)
	{
		get_feature_classification(
				classification,
				(*feature_iter)->reference(),
				reconstruct_method_registry,
				registered_reconstruct_methods);
	}

	return classification;
}


GPlatesFileIO::FeatureCollectionFileFormat::classifications_type
GPlatesFileIO::FeatureCollectionFileFormat::classify(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature,
		const GPlatesAppLogic::ReconstructMethodRegistry &reconstruct_method_registry)
{
	classifications_type classification;

	get_feature_classification(
			classification,
			feature,
			reconstruct_method_registry,
			reconstruct_method_registry.get_registered_reconstruct_methods());

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

	const std::vector<GPlatesAppLogic::ReconstructMethod::Type> registered_reconstruct_methods =
			reconstruct_method_registry.get_registered_reconstruct_methods();

	// Iterate through the features in the feature collection.
	GPlatesModel::FeatureCollectionHandle::iterator feature_iter = feature_collection->begin();
	GPlatesModel::FeatureCollectionHandle::iterator feature_end = feature_collection->end();
	for ( ; feature_iter != feature_end; ++feature_iter)
	{
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref = (*feature_iter)->reference();

		classifications_type classification;
		get_feature_classification(
				classification,
				feature_ref,
				reconstruct_method_registry,
				registered_reconstruct_methods);

		// If the feature has the required classification then add it to the caller's sequence.
		if (classification_predicate(classification))
		{
			found_features.push_back(feature_ref);
			found = true;
		}
	}

	return found;
}
