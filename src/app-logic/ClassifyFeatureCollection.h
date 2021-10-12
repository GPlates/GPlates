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

#ifndef GPLATES_APP_LOGIC_CLASSIFYFEATURECOLLECTION_H
#define GPLATES_APP_LOGIC_CLASSIFYFEATURECOLLECTION_H

#include <bitset>
#include <boost/function.hpp>

#include "file-io/File.h"

#include "model/FeatureCollectionHandle.h"
#include "model/FeatureHandle.h"


namespace GPlatesAppLogic
{
	namespace ClassifyFeatureCollection
	{
		/**
		 * The types in which a feature collection can be classified.
		 */
		enum ClassificationType
		{
			/**
			 * Reconstructable features have a 'reconstructionPlateId' plate id.
			 */
			RECONSTRUCTABLE,
			/**
			 * Reconstruction features have 'fixedReferenceFrame' and
			 * 'movingReferenceFrame' plate ids.
			 */
			RECONSTRUCTION,
			/**
			 * Instantaneous features have a 'reconstructedPlateId' plate id.
			 */
			INSTANTANEOUS,

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
		 * Returns classification type(s) of feature collection in @a file.
		 *
		 * First looks at the file format and if classification can be determined
		 * from that then the feature collection is not inspected.
		 * Otherwise the features in the feature collection are inspected to determine
		 * classification.
		 */
		classifications_type
		classify_feature_collection(
				const GPlatesFileIO::File &file);


		/**
		 * Returns the classification type(s) of @a feature_collection.
		 */
		classifications_type
		classify_feature_collection(
				const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection);


		//! Returns true if found a reconstructable feature.
		inline
		bool
		found_reconstructable_feature(
				const classifications_type &classification)
		{
			return classification.test(RECONSTRUCTABLE);
		}

		//! Returns true if found a reconstruction feature.
		inline
		bool
		found_reconstruction_feature(
				const classifications_type &classification)
		{
			return classification.test(RECONSTRUCTION);
		}

		//! Returns true if found an instantaneous feature.
		inline
		bool
		found_instantaneous_feature(
				const classifications_type &classification)
		{
			return classification.test(INSTANTANEOUS);
		}

		/**
		 * Returns true if found a feature that probably has geometry in it.
		 *
		 * We have to be a little cautious in testing for features that
		 * can be displayed in GPlates. So a feature collection is considered displayable
		 * if and only if there are reconstructable features OR there is no
		 * rotation data present. Having zero on both counts means that
		 * something is wrong and we should not rule out 'displayble' as an option.
		 *
		 * FIXME: Probably better to explicitly search for geometry in the feature(s)
		 * rather than look at the types of plate ids stored in the feature(s).
		 * And where do instantaneous features fit into this?
		 */
		inline
		bool
		found_geometry_feature(
				const classifications_type &classification)
		{
			return found_reconstructable_feature(classification) ||
				!found_reconstruction_feature(classification);
		}


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
				const classification_predicate_type &classification_predicate);
	}
}

#endif // GPLATES_APP_LOGIC_CLASSIFYFEATURECOLLECTION_H
