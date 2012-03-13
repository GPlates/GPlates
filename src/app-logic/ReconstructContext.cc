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

#include "utils/Profile.h"


GPlatesAppLogic::ReconstructContext::ReconstructContext(
		const ReconstructMethodRegistry &reconstruct_method_registry,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
				reconstructable_feature_collections)
{
	reassign_reconstruct_methods_to_features(
			reconstruct_method_registry,
			reconstructable_feature_collections);
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
			// If no reconstruct method can reconstruct the current feature then skip the feature.
			// We could default to the 'BY_PLATE_ID' reconstruct method but ignoring the feature
			// helps to ensure that features that shouldn't be reconstructed using the
			// ReconstructContext framework are excluded - such as topological features that need
			// to be handled by a different framework.
			// NOTE: Previously this defaulted to 'BY_PLATE_ID' but this picked up topological
			// features that, although they had no geometry and hence no reconstructed geometry,
			// they still showed up as a ReconstructContext::ReconstructedFeature (eg, in the
			// data mining co-registration list of seed features).
			// The 'BY_PLATE_ID' reconstruct method is very lenient so it should be able pick up
			// pretty much anything that has a geometry to be reconstructed.
			const boost::optional<ReconstructMethod::Type> reconstruct_method_type =
					reconstruct_method_registry.get_reconstruct_method_type(feature_ref);
			if (!reconstruct_method_type)
			{
				continue;
			}

			// Create a new reconstruct method if one hasn't already been.
			if (reconstruct_method_indices.find(reconstruct_method_type.get()) ==
				reconstruct_method_indices.end())
			{
				const ReconstructMethodInterface::non_null_ptr_type reconstruct_method =
						reconstruct_method_registry.get_reconstruct_method(reconstruct_method_type.get());

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


GPlatesAppLogic::ReconstructHandle::type
GPlatesAppLogic::ReconstructContext::reconstruct(
		std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &reconstructed_feature_geometries,
		const ReconstructParams &reconstruct_params,
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		const double &reconstruction_time)
{
	//PROFILE_BLOCK("ReconstructContext::reconstruct: ReconstructedFeatureGeometry's");

	// Get the next global reconstruct handle - it'll be stored in each RFG.
	const ReconstructHandle::type reconstruct_handle = ReconstructHandle::get_next_reconstruct_handle();

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
					reconstruct_handle,
					reconstruct_params,
					reconstruction_tree_creator,
					reconstruction_time);
		}
	}

	return reconstruct_handle;
}


GPlatesAppLogic::ReconstructHandle::type
GPlatesAppLogic::ReconstructContext::reconstruct(
		std::vector<Reconstruction> &reconstructions,
		const ReconstructParams &reconstruct_params,
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		const double &reconstruction_time)
{
	//PROFILE_BLOCK("ReconstructContext::reconstruct: Reconstruction's");

	// Since we're mapping RFGs to geometry property handles we need to ensure
	// that the handles have been assigned.
	if (!have_assigned_geometry_property_handles())
	{
		assign_geometry_property_handles();
	}

	// Get the next global reconstruct handle - it'll be stored in each RFG.
	const ReconstructHandle::type reconstruct_handle = ReconstructHandle::get_next_reconstruct_handle();

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
					reconstruct_handle,
					reconstruct_params,
					reconstruction_tree_creator,
					reconstruction_time);

			// Convert the reconstructed feature geometries to reconstructions for the current feature.
			get_feature_reconstructions(
					reconstructions,
					reconstruct_method_feature,
					reconstructed_feature_geometries);
		}
	}

	return reconstruct_handle;
}


GPlatesAppLogic::ReconstructHandle::type
GPlatesAppLogic::ReconstructContext::reconstruct(
		std::vector<ReconstructedFeature> &reconstructed_features,
		const ReconstructParams &reconstruct_params,
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		const double &reconstruction_time)
{
	//PROFILE_BLOCK("ReconstructContext::reconstruct: ReconstructedFeature's");

	// Since we're mapping RFGs to geometry property handles we need to ensure
	// that the handles have been assigned.
	if (!have_assigned_geometry_property_handles())
	{
		assign_geometry_property_handles();
	}

	// Get the next global reconstruct handle - it'll be stored in each RFG.
	const ReconstructHandle::type reconstruct_handle = ReconstructHandle::get_next_reconstruct_handle();

	// Optimisation: Count the number of features so we can size the caller's array to avoid
	// unnecessary copying/re-allocation as we add features to it.
	unsigned int num_features = 0;
	BOOST_FOREACH(
			const ReconstructMethodFeatures &reconstruct_method_features,
			d_reconstruct_method_features_seq)
	{
		num_features += reconstruct_method_features.features.size();
	}
	// Avoid reallocations (note that 'ReconstructedFeature' contains a std::vector data member
	// itself which might also need to be deallocated/reallocated).
	reconstructed_features.reserve(reconstructed_features.size() + num_features);

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
					reconstruct_handle,
					reconstruct_params,
					reconstruction_tree_creator,
					reconstruction_time);

			// Add a reconstructed feature objects to the caller's sequence.
			reconstructed_features.push_back(ReconstructedFeature(feature_ref));
			ReconstructedFeature &reconstructed_feature = reconstructed_features.back();

			// Convert the reconstructed feature geometries to reconstructions for the current feature.
			get_feature_reconstructions(
					// Note: Add to the reconstructed feature instead of a global sequence of
					// reconstructions like other 'reconstruct' methods...
					reconstructed_feature.d_reconstructions,
					reconstruct_method_feature,
					reconstructed_feature_geometries);
		}
	}

	return reconstruct_handle;
}


void
GPlatesAppLogic::ReconstructContext::get_feature_reconstructions(
		std::vector<Reconstruction> &reconstructions,
		const ReconstructMethodFeature &reconstruct_method_feature,
		const std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &reconstructed_feature_geometries)
{
	// Nothing to do if there are no reconstructed feature geometries.
	if (reconstructed_feature_geometries.empty())
	{
		return;
	}

	// Optimisation to size geometries array (most likely one since most features have one geometry).
	reconstructions.reserve(reconstructed_feature_geometries.size());

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
