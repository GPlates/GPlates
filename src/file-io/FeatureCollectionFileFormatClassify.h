/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009, 2011 The University of Sydney, Australia
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

#ifndef GPLATES_FILE_IO_FEATURECOLLECTIONFILEFORMATCLASSIFY_H
#define GPLATES_FILE_IO_FEATURECOLLECTIONFILEFORMATCLASSIFY_H

#include <bitset>
#include <boost/function.hpp>

#include "app-logic/ReconstructMethodType.h"

#include "file-io/File.h"

#include "model/FeatureCollectionHandle.h"
#include "model/FeatureHandle.h"


namespace GPlatesAppLogic
{
	class ReconstructMethodRegistry;
}

namespace GPlatesFileIO
{
	namespace FeatureCollectionFileFormat
	{
		/**
		 * The types in which a feature collection can be classified for file reading/writing.
		 *
		 * Some file formats will only be able to read/write a subset of these classifications.
		 */
		enum ClassificationType
		{
			//
			// NOTE: The first values of this enumeration map to the enumeration values in
			// GPlatesAppLogic::ReconstructMethod::Type.
			//

			/**
			 * Rasters features contain image data.
			 *
			 * These can be reconstructed by, unlike the other reconstructable features, these
			 * features require other reconstructable features (polygons) to reconstruct them..
			 *
			 * Even though they are not explicitly defined, the first values of this
			 * enumeration are taken to be all of the members of the enumeration
			 * GPlatesAppLogic::ReconstructMethod::Type. As such, the first explicitly defined
			 * member of this enumeration must have the value of
			 * GPlatesAppLogic::ReconstructMethod::NUM_TYPES to avoid conflict between values.
			 */
			RASTER = static_cast<unsigned int>(GPlatesAppLogic::ReconstructMethod::NUM_TYPES), // See note above.

			/**
			 * Scalar coverage features contain a geometry and a scalar value per point in geometry.
			 */
			SCALAR_COVERAGE,

			/**
			 * Scalar field features contain scalar volume data.
			 */
			SCALAR_FIELD_3D,

			/**
			 * Topological features contain topological geometry that references other feature geometries.
			 *
			 * These are thought as as resolvable instead of reconstructable because they are not
			 * strictly reconstructed (just resolve the reconstructions of referenced feature geometries).
			 */
			TOPOLOGICAL,

			/**
			 * Reconstruction features have 'fixedReferenceFrame' and 'movingReferenceFrame' plate ids and
			 * are used to rotate other features.
			 */
			RECONSTRUCTION,

			NUM_CLASSIFICATION_TYPES // Must be the last enum.
		};


		/**
		 * A std::bitset for testing multiple classification types for
		 * a single feature collection.
		 *
		 * Test with the enums values in @a ClassificationType using classifications_type::test().
		 */
		typedef std::bitset<NUM_CLASSIFICATION_TYPES> classifications_type;

		/**
		 * Returns true if either classification intersects the other.
		 */
		inline
		bool
		intersect(
				const classifications_type &classifications1,
				const classifications_type &classifications2)
		{
			const classifications_type common = (classifications1 & classifications2);
			return common.any();
		}


		/**
		 * Returns the classification type(s) of @a feature_collection.
		 */
		classifications_type
		classify(
				const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection,
				const GPlatesAppLogic::ReconstructMethodRegistry &reconstruct_method_registry);

		/**
		 * Returns the classification type(s) of @a feature.
		 */
		classifications_type
		classify(
				const GPlatesModel::FeatureHandle::const_weak_ref &feature,
				const GPlatesAppLogic::ReconstructMethodRegistry &reconstruct_method_registry);


		/**
		 * Typedef for a predicate function that accepts a @a classifications_type
		 * as it argument and returns a bool.
		 */
		typedef boost::function<bool (const classifications_type &)> classification_predicate_type;

		/**
		 * Finds features in @a feature_collection that contain the
		 * classification @a classification.
		 *
		 * Appends any matching features to @a found_features.
		 *
		 * Returns false if no matching features are found.
		 */
		bool
		find_classified_features(
				std::vector<GPlatesModel::FeatureHandle::weak_ref> &found_features,
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection,
				const GPlatesAppLogic::ReconstructMethodRegistry &reconstruct_method_registry,
				ClassificationType classification);

		/**
		 * Finds features in @a feature_collection that match the
		 * classification predicate @a classification_predicate.
		 *
		 * Appends any matching features to @a found_features.
		 *
		 * Returns false if no matching features are found.
		 */
		bool
		find_classified_features(
				std::vector<GPlatesModel::FeatureHandle::weak_ref> &found_features,
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection,
				const GPlatesAppLogic::ReconstructMethodRegistry &reconstruct_method_registry,
				const classification_predicate_type &classification_predicate);
	}
}

#endif // GPLATES_FILE_IO_FEATURECOLLECTIONFILEFORMATCLASSIFY_H
