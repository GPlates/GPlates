/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#include <cstddef> // std::size_t
#include <boost/foreach.hpp>

#include "ReconstructContext.h"

#include "ReconstructMethodRegistry.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"


GPlatesAppLogic::ReconstructContext::ReconstructContext(
		const ReconstructMethodRegistry &reconstruct_method_registry,
		const ReconstructParams &reconstruct_params,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
				reconstructable_feature_collections) :
	d_reconstruct_params(reconstruct_params)
{
	reassign_reconstruct_methods_to_features(
			reconstruct_method_registry,
			reconstructable_feature_collections);
}


void
GPlatesAppLogic::ReconstructContext::set_reconstruct_params(
		const ReconstructParams &reconstruct_params)
{
	d_reconstruct_params = reconstruct_params;
}


void
GPlatesAppLogic::ReconstructContext::reassign_reconstruct_methods_to_features(
		const ReconstructMethodRegistry &reconstruct_method_registry,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
				reconstructable_feature_collections)
{
	//
	// First clear the previous state including removing all features.
	//

	d_reconstruct_method_features_seq.clear();
	d_cached_present_day_geometries = boost::none;


	//
	// Find out which reconstruct method to use for each feature and associate them.
	//

	// Typedef for mapping reconstruct method type to the object used to generate RFGs.
	typedef std::map<ReconstructMethod::Type, unsigned int> reconstruct_method_indices_type;

	// Maps reconstruction method types to indices into 'd_reconstruct_method_features_seq'.
	reconstruct_method_indices_type reconstruct_method_indices;
	d_reconstruct_method_features_seq.reserve(ReconstructMethod::NUM_TYPES);

	// Iterate over the feature collections.
	BOOST_FOREACH(
			const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection_ref,
			reconstructable_feature_collections)
	{
		// Iterate over the features in the current feature collection.
		GPlatesModel::FeatureCollectionHandle::iterator features_iter = feature_collection_ref->begin();
		GPlatesModel::FeatureCollectionHandle::iterator features_end = feature_collection_ref->end();
		for ( ; features_iter != features_end; ++features_iter)
		{
			const GPlatesModel::FeatureHandle::weak_ref feature_ref = (*features_iter)->reference();

			// See if any reconstruct methods can reconstruct the current feature.
			boost::optional<ReconstructMethod::Type> reconstruct_method_type =
					reconstruct_method_registry.get_reconstruct_method_type(feature_ref);

			if (!reconstruct_method_type)
			{
				// If no reconstruct method can reconstruct the current feature then
				// default to the by-plate-id reconstruct method.
				reconstruct_method_type = ReconstructMethod::BY_PLATE_ID;
			}

			// Create a new reconstruct method if one hasn't already been.
			if (reconstruct_method_indices.find(reconstruct_method_type.get()) ==
				reconstruct_method_indices.end())
			{
				const ReconstructMethodInterface::non_null_ptr_type reconstruct_method =
						reconstruct_method_registry.get_reconstruct_method(
								reconstruct_method_type.get());

				reconstruct_method_indices[reconstruct_method_type.get()] =
						d_reconstruct_method_features_seq.size();

				// Add the new reconstruct method.
				d_reconstruct_method_features_seq.push_back(
						ReconstructMethodFeatures(reconstruct_method));
			}

			// Add the current feature to be reconstructed by the reconstruction method.
			const unsigned int reconstruct_method_index =
					reconstruct_method_indices[reconstruct_method_type.get()];
			d_reconstruct_method_features_seq[reconstruct_method_index]
					.features.push_back(ReconstructMethodFeature(feature_ref));
		}
	}
}


const std::vector<GPlatesAppLogic::ReconstructContext::geometry_type> &
GPlatesAppLogic::ReconstructContext::get_present_day_geometries()
{
	if (!have_assigned_geometry_property_handles())
	{
		assign_geometry_property_handles();

		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				d_cached_present_day_geometries,
				GPLATES_ASSERTION_SOURCE);
	}

	return d_cached_present_day_geometries.get();
}


void
GPlatesAppLogic::ReconstructContext::reconstruct(
		std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &reconstructed_feature_geometries,
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		const double &reconstruction_time)
{
	// Iterate over the reconstruct method features.
	BOOST_FOREACH(
			const ReconstructMethodFeatures &reconstruct_method_features,
			d_reconstruct_method_features_seq)
	{
		ReconstructMethodInterface &reconstruct_method = *reconstruct_method_features.reconstruct_method;

		// Iterate over the features that are to be reconstructed using the current method.
		std::vector<ReconstructMethodFeature>::const_iterator features_iter =
				reconstruct_method_features.features.begin();
		const std::vector<ReconstructMethodFeature>::const_iterator features_end =
				reconstruct_method_features.features.end();
		for ( ; features_iter != features_end; ++features_iter)
		{
			const ReconstructMethodFeature &reconstruct_method_feature = *features_iter;
			const GPlatesModel::FeatureHandle::weak_ref &feature_ref = reconstruct_method_feature.feature;

			// Reconstruct the current feature.
			reconstruct_method.reconstruct_feature(
					reconstructed_feature_geometries,
					feature_ref,
					d_reconstruct_params,
					reconstruction_tree_creator,
					reconstruction_time);
		}
	}
}


void
GPlatesAppLogic::ReconstructContext::reconstruct(
		std::vector<Reconstruction> &reconstructions,
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		const double &reconstruction_time)
{
	// Since we're mapping RFGs to geometry property handles we need to ensure
	// that the handles have been assigned.
	if (!have_assigned_geometry_property_handles())
	{
		assign_geometry_property_handles();
	}

	// Iterate over the reconstruct method features.
	BOOST_FOREACH(
			const ReconstructMethodFeatures &reconstruct_method_features,
			d_reconstruct_method_features_seq)
	{
		ReconstructMethodInterface &reconstruct_method = *reconstruct_method_features.reconstruct_method;

		// Iterate over the features that are to be reconstructed using the current method.
		std::vector<ReconstructMethodFeature>::const_iterator features_iter =
				reconstruct_method_features.features.begin();
		const std::vector<ReconstructMethodFeature>::const_iterator features_end =
				reconstruct_method_features.features.end();
		for ( ; features_iter != features_end; ++features_iter)
		{
			const ReconstructMethodFeature &reconstruct_method_feature = *features_iter;
			const GPlatesModel::FeatureHandle::weak_ref &feature_ref = reconstruct_method_feature.feature;

			// Reconstruct the current feature.
			std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> reconstructed_feature_geometries;
			reconstruct_method.reconstruct_feature(
					reconstructed_feature_geometries,
					feature_ref,
					d_reconstruct_params,
					reconstruction_tree_creator,
					reconstruction_time);

			// Iterate over the reconstructed feature geometries and determine the
			// geometry property handle of each one.
			std::vector<ReconstructedFeatureGeometry::non_null_ptr_type>::const_iterator rfg_iter =
					reconstructed_feature_geometries.begin();
			std::vector<ReconstructedFeatureGeometry::non_null_ptr_type>::const_iterator rfg_end =
					reconstructed_feature_geometries.end();
			for ( ; rfg_iter != rfg_end; ++rfg_iter)
			{
				const ReconstructedFeatureGeometry::non_null_ptr_type &rfg = *rfg_iter;
				const GPlatesModel::FeatureHandle::iterator rfg_geometry_property = rfg->property();

				// Iterate over the geometry properties we've previously obtained for the
				// current feature and find which one corresponds to the current RFG.
				ReconstructMethodFeature::geometry_property_to_handle_seq_type::const_iterator
						geometry_property_handle_iter =
								reconstruct_method_feature.geometry_property_to_handle_seq.begin();
				const ReconstructMethodFeature::geometry_property_to_handle_seq_type::const_iterator
						geometry_property_handle_end =
								reconstruct_method_feature.geometry_property_to_handle_seq.end();
				for ( ; geometry_property_handle_iter != geometry_property_handle_end; ++geometry_property_handle_iter)
				{
					if (geometry_property_handle_iter->property_iterator == rfg_geometry_property)
					{
						// Add the RFG and its associated geometry property handle to the caller's sequence.
						reconstructions.push_back(
								Reconstruction(
										rfg,
										geometry_property_handle_iter->geometry_property_handle));

						// Continue to the next RFG.
						break;
					}
				}
			}
		}
	}
}


void
GPlatesAppLogic::ReconstructContext::assign_geometry_property_handles()
{
	d_cached_present_day_geometries = std::vector<geometry_type>();

	// Iterate over the reconstruct method features.
	BOOST_FOREACH(
			ReconstructMethodFeatures &reconstruct_method_features,
			d_reconstruct_method_features_seq)
	{
		ReconstructMethodInterface &reconstruct_method = *reconstruct_method_features.reconstruct_method;

		// Iterate over the features of the current reconstruct method.
		std::vector<ReconstructMethodFeature>::iterator features_iter =
				reconstruct_method_features.features.begin();
		const std::vector<ReconstructMethodFeature>::iterator features_end =
				reconstruct_method_features.features.end();
		for ( ; features_iter != features_end; ++features_iter)
		{
			ReconstructMethodFeature &reconstruct_method_feature = *features_iter;
			const GPlatesModel::FeatureHandle::weak_ref &feature_ref = reconstruct_method_feature.feature;

			// Get the present day geometries for the current feature.
			// Note: There should be one for each geometry property that can be reconstructed.
			std::vector<ReconstructMethodInterface::Geometry> present_day_geometries;
			reconstruct_method.get_present_day_geometries(present_day_geometries, feature_ref);

			// Iterate over the present day geometries.
			std::vector<ReconstructMethodInterface::Geometry>::const_iterator present_day_geometry_iter =
					present_day_geometries.begin();
			std::vector<ReconstructMethodInterface::Geometry>::const_iterator present_day_geometry_end =
					present_day_geometries.end();
			for ( ; present_day_geometry_iter != present_day_geometry_end; ++present_day_geometry_iter)
			{
				const geometry_property_handle_type geometry_property_handle =
						d_cached_present_day_geometries->size();

				const ReconstructMethodFeature::GeometryPropertyToHandle geometry_property_to_handle =
				{
					present_day_geometry_iter->property_iterator,
					geometry_property_handle
				};

				reconstruct_method_feature.geometry_property_to_handle_seq.push_back(geometry_property_to_handle);
				d_cached_present_day_geometries->push_back(present_day_geometry_iter->geometry);
			}
		}
	}
}
