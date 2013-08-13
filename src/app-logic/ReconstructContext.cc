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

#include <algorithm>
#include <cstddef> // std::size_t
#include <boost/bind.hpp>
#include <boost/foreach.hpp>

#include "ReconstructContext.h"

#include "ReconstructMethodRegistry.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "utils/Profile.h"


namespace GPlatesAppLogic
{
	namespace
	{
		/**
		 * The default reconstruction tree creator implementation until the client supplies one.
		 */
		class IdentityReconstructionTreeCreatorImpl :
				public ReconstructionTreeCreatorImpl
		{
		public:

			virtual
			ReconstructionTree::non_null_ptr_to_const_type
			get_reconstruction_tree(
					const double &reconstruction_time,
					GPlatesModel::integer_plate_id_type anchor_plate_id)
			{
				// Create a reconstruction tree that returns identity rotations.
				return create_reconstruction_tree(0, 0);
			}

			virtual
			ReconstructionTree::non_null_ptr_to_const_type
			get_reconstruction_tree_default_anchored_plate_id(
					const double &reconstruction_time)
			{
				// Create a reconstruction tree that returns identity rotations.
				return create_reconstruction_tree(0, 0);
			}
		};
	}
}


GPlatesAppLogic::ReconstructContext::ReconstructContext(
		const ReconstructMethodRegistry &reconstruct_method_registry) :
	d_reconstruct_method_registry(reconstruct_method_registry)
{
}


void
GPlatesAppLogic::ReconstructContext::set_features(
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
				reconstructable_feature_collections)
{
	std::vector<GPlatesModel::FeatureHandle::weak_ref> reconstructable_features;

	// Iterate over the feature collections and extract the list of features.
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

			reconstructable_features.push_back(feature_ref);
		}
	}

	set_features(reconstructable_features);
}


void
GPlatesAppLogic::ReconstructContext::set_features(
		const std::vector<GPlatesModel::FeatureHandle::weak_ref> &reconstructable_features)
{
	// First remove all reconstruct method features and present day geometries.
	d_reconstruct_method_feature_seq.clear();
	d_cached_present_day_geometries = boost::none;

	// Iterate over the features and assign default reconstruct methods to them.
	BOOST_FOREACH(const GPlatesModel::FeatureHandle::weak_ref &feature_ref, reconstructable_features)
	{
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
		boost::optional<ReconstructMethod::Type> reconstruct_method_type =
				d_reconstruct_method_registry.get_reconstruct_method_type(feature_ref);
		if (!reconstruct_method_type)
		{
			continue;
		}

		// Add the new reconstruct method to our list.
		d_reconstruct_method_feature_seq.push_back(
				ReconstructMethodFeature(feature_ref, reconstruct_method_type.get()));
	}

	// Re-initialise the context states since the features have changed.
	initialise_context_states();
}


GPlatesAppLogic::ReconstructContext::context_state_reference_type
GPlatesAppLogic::ReconstructContext::create_context_state(
		const ReconstructMethodInterface::Context &reconstruct_method_context)
{
	// Create a new context state.
	context_state_reference_type context_state_ref(new ContextState(reconstruct_method_context));

	// Populate the context state with reconstruct methods.
	// Note that these reconstruct methods could end up containing internal state that is specific
	// to the reconstruct context state passed into them. It's for this reason that we have
	// different reconstruct method instances for different context states.
	context_state_ref->d_reconstruct_methods.reserve(d_reconstruct_method_feature_seq.size());
	BOOST_FOREACH(
			const ReconstructMethodFeature &reconstruct_method_feature,
			d_reconstruct_method_feature_seq)
	{
		// Create a new reconstruct method for the current feature and its reconstruct method type.
		ReconstructMethodInterface::non_null_ptr_type context_state_reconstruct_method =
				d_reconstruct_method_registry.create_reconstruct_method(
						reconstruct_method_feature.reconstruction_method_type,
						reconstruct_method_feature.feature_ref,
						context_state_ref->d_reconstruct_method_context);

		context_state_ref->d_reconstruct_methods.push_back(context_state_reconstruct_method);
	}

	// Iterate over our sequence of context states and re-use the first expired slot if any.
	context_state_weak_ref_seq_type::iterator context_state_iter = d_context_states.begin();
	context_state_weak_ref_seq_type::iterator context_state_end = d_context_states.end();
	for ( ; context_state_iter != context_state_end; ++context_state_iter)
	{
		context_state_weak_reference_type &context_state_weak_ref = *context_state_iter;
		if (context_state_weak_ref.expired())
		{
			// We found a slot so store the context state reference in it.
			context_state_weak_ref = context_state_ref;
			return context_state_ref;
		}
	}

	// No slots found so add to the end of the sequence.
	d_context_states.push_back(context_state_ref);

	return context_state_ref;
}


const std::vector<GPlatesAppLogic::ReconstructContext::geometry_type> &
GPlatesAppLogic::ReconstructContext::get_present_day_feature_geometries()
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
GPlatesAppLogic::ReconstructContext::reconstruct_feature_geometries(
		std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &reconstructed_feature_geometries,
		const context_state_reference_type &context_state_ref,
		const double &reconstruction_time)
{
	//PROFILE_BLOCK("ReconstructContext::reconstruct_feature_geometries: ReconstructedFeatureGeometry's");

	// Get the next global reconstruct handle - it'll be stored in each RFG.
	const ReconstructHandle::type reconstruct_handle = ReconstructHandle::get_next_reconstruct_handle();

	// Iterate over the reconstruct methods in the context state.
	BOOST_FOREACH(
			const ReconstructMethodInterface::non_null_ptr_type &context_state_reconstruct_method,
			context_state_ref->d_reconstruct_methods)
	{
		// Reconstruct the current feature (reconstruct method).
		context_state_reconstruct_method->reconstruct_feature_geometries(
				reconstructed_feature_geometries,
				reconstruct_handle,
				context_state_ref->d_reconstruct_method_context,
				reconstruction_time);
	}

	return reconstruct_handle;
}


GPlatesAppLogic::ReconstructHandle::type
GPlatesAppLogic::ReconstructContext::reconstruct_feature_geometries(
		std::vector<Reconstruction> &reconstructions,
		const context_state_reference_type &context_state_ref,
		const double &reconstruction_time)
{
	//PROFILE_BLOCK("ReconstructContext::reconstruct_feature_geometries: Reconstruction's");

	// Since we're mapping RFGs to geometry property handles we need to ensure
	// that the handles have been assigned.
	if (!have_assigned_geometry_property_handles())
	{
		assign_geometry_property_handles();
	}

	// Get the next global reconstruct handle - it'll be stored in each RFG.
	const ReconstructHandle::type reconstruct_handle = ReconstructHandle::get_next_reconstruct_handle();

	// The context state should have the same number of features (reconstruct methods).
	const unsigned int num_features = d_reconstruct_method_feature_seq.size();
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			context_state_ref->d_reconstruct_methods.size() == num_features,
			GPLATES_ASSERTION_SOURCE);

	// Iterate over the reconstruct methods of the current context state and reconstruct.
	for (unsigned int feature_index = 0; feature_index < num_features; ++feature_index)
	{
		const ReconstructMethodFeature &reconstruct_method_feature =
				d_reconstruct_method_feature_seq[feature_index];
		const ReconstructMethodInterface::non_null_ptr_type context_state_reconstruct_method =
				context_state_ref->d_reconstruct_methods[feature_index];

		// Reconstruct the current feature.
		std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> reconstructed_feature_geometries;
		context_state_reconstruct_method->reconstruct_feature_geometries(
				reconstructed_feature_geometries,
				reconstruct_handle,
				context_state_ref->d_reconstruct_method_context,
				reconstruction_time);

		// Convert the reconstructed feature geometries to reconstructions for the current feature.
		get_feature_reconstructions(
				reconstructions,
				reconstruct_method_feature.geometry_property_to_handle_seq,
				reconstructed_feature_geometries);
	}

	return reconstruct_handle;
}


GPlatesAppLogic::ReconstructHandle::type
GPlatesAppLogic::ReconstructContext::reconstruct_feature_geometries(
		std::vector<ReconstructedFeature> &reconstructed_features,
		const context_state_reference_type &context_state_ref,
		const double &reconstruction_time)
{
	//PROFILE_BLOCK("ReconstructContext::reconstruct_feature_geometries: ReconstructedFeature's");

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
	const unsigned int num_features = d_reconstruct_method_feature_seq.size();
	// Avoid reallocations (note that 'ReconstructedFeature' contains a std::vector data member
	// itself which might also need to be deallocated/reallocated).
	reconstructed_features.reserve(reconstructed_features.size() + num_features);

	// The context state should have the same number of features (reconstruct methods).
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			context_state_ref->d_reconstruct_methods.size() == num_features,
			GPLATES_ASSERTION_SOURCE);

	// Iterate over the reconstruct methods of the current context state and reconstruct.
	for (unsigned int feature_index = 0; feature_index < num_features; ++feature_index)
	{
		const ReconstructMethodFeature &reconstruct_method_feature =
				d_reconstruct_method_feature_seq[feature_index];
		const ReconstructMethodInterface::non_null_ptr_type context_state_reconstruct_method =
				context_state_ref->d_reconstruct_methods[feature_index];

		// Reconstruct the current feature.
		std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> reconstructed_feature_geometries;
		context_state_reconstruct_method->reconstruct_feature_geometries(
				reconstructed_feature_geometries,
				reconstruct_handle,
				context_state_ref->d_reconstruct_method_context,
				reconstruction_time);

		// Add a reconstructed feature objects to the caller's sequence.
		reconstructed_features.push_back(
				ReconstructedFeature(context_state_reconstruct_method->get_feature_ref()));
		ReconstructedFeature &reconstructed_feature = reconstructed_features.back();

		// Convert the reconstructed feature geometries to reconstructions for the current feature.
		get_feature_reconstructions(
				// Note: Add to the reconstructed feature instead of a global sequence of
				// reconstructions like other 'reconstruct' methods...
				reconstructed_feature.d_reconstructions,
				reconstruct_method_feature.geometry_property_to_handle_seq,
				reconstructed_feature_geometries);
	}

	return reconstruct_handle;
}


GPlatesAppLogic::ReconstructHandle::type
GPlatesAppLogic::ReconstructContext::reconstruct_feature_velocities(
		std::vector<MultiPointVectorField::non_null_ptr_type> &reconstructed_feature_velocities,
		const context_state_reference_type &context_state_ref,
		const double &reconstruction_time)
{
	//PROFILE_FUNC();

	// Get the next global reconstruct handle - it'll be stored in each velocity field.
	const ReconstructHandle::type reconstruct_handle = ReconstructHandle::get_next_reconstruct_handle();

	// Iterate over the reconstruct methods in the context state.
	BOOST_FOREACH(
			const ReconstructMethodInterface::non_null_ptr_type &context_state_reconstruct_method,
			context_state_ref->d_reconstruct_methods)
	{
		// Reconstruct the current feature (reconstruct method).
		context_state_reconstruct_method->reconstruct_feature_velocities(
				reconstructed_feature_velocities,
				reconstruct_handle,
				context_state_ref->d_reconstruct_method_context,
				reconstruction_time);
	}

	return reconstruct_handle;
}


void
GPlatesAppLogic::ReconstructContext::get_feature_reconstructions(
		std::vector<Reconstruction> &reconstructions,
		const ReconstructMethodFeature::geometry_property_to_handle_seq_type &feature_geometry_property_handles,
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
				geometry_property_handle_iter = feature_geometry_property_handles.begin();
		const ReconstructMethodFeature::geometry_property_to_handle_seq_type::const_iterator
				geometry_property_handle_end = feature_geometry_property_handles.end();
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

	// Look for an existing context state so we can use it to get the present day geometries.
	// It doesn't matter what the context state is since it does not affect present day geometries.
	context_state_weak_ref_seq_type::const_iterator context_state_iter = std::find_if(
			d_context_states.begin(),
			d_context_states.end(),
					!boost::bind(&context_state_weak_reference_type::expired, _1));

	context_state_reference_type context_state;
	if (context_state_iter == d_context_states.end())
	{
		// If we couldn't find one then generate one temporarily.
		// Note that the context state will get released at the end of this function.
		//
		// Can use default reconstruct params and tree generator since does not affect present day geometries.
		context_state = create_context_state(
				ReconstructMethodInterface::Context(
						ReconstructParams(),
						ReconstructionTreeCreator(new IdentityReconstructionTreeCreatorImpl())));
	}
	else
	{
		context_state = context_state_iter->lock();
	}

	// Iterate over the reconstruct methods of the current context state.
	const unsigned int num_features = d_reconstruct_method_feature_seq.size();
	// Context state should have the expected number of features.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			context_state->d_reconstruct_methods.size() == num_features,
			GPLATES_ASSERTION_SOURCE);
	for (unsigned int feature_index = 0; feature_index < num_features; ++feature_index)
	{
		ReconstructMethodFeature &reconstruct_method_feature =
				d_reconstruct_method_feature_seq[feature_index];
		const ReconstructMethodInterface::non_null_ptr_type context_state_reconstruct_method =
				context_state->d_reconstruct_methods[feature_index];

		// Get the present day geometries for the current feature.
		// Note: There should be one for each geometry property that can be reconstructed.
		std::vector<ReconstructMethodInterface::Geometry> present_day_geometries;
		context_state_reconstruct_method->get_present_day_feature_geometries(present_day_geometries);

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


void
GPlatesAppLogic::ReconstructContext::initialise_context_states()
{
	// We take the opportunity to remove any expired context states (that the client is no
	// longer using) in order to compress the size of the array.
	d_context_states.erase(
			std::remove_if(d_context_states.begin(), d_context_states.end(),
					boost::bind(&context_state_weak_reference_type::expired, _1)),
			d_context_states.end());

	// Iterate over the existing context states and re-create their reconstruct methods also.
	// We do this because the features have changed and hence any internal state in the
	// reconstruct methods is no longer applicable so they need to start from scratch and the
	// easiest way to do this is to re-create them.
	context_state_weak_ref_seq_type::iterator context_state_iter = d_context_states.begin();
	context_state_weak_ref_seq_type::iterator context_state_end = d_context_states.end();
	for ( ; context_state_iter != context_state_end; ++context_state_iter)
	{
		context_state_weak_reference_type &context_state_weak_ref = *context_state_iter;

		// Shouldn't be expired because we removed expired references above.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				!context_state_weak_ref.expired(),
				GPLATES_ASSERTION_SOURCE);

		context_state_reference_type context_state_ref(context_state_weak_ref);

		const unsigned int num_features = d_reconstruct_method_feature_seq.size();

		// Remove the current reconstruct methods.
		context_state_ref->d_reconstruct_methods.clear();
		context_state_ref->d_reconstruct_methods.reserve(num_features);

		// Iterate over the reconstruct methods of the current context state and re-create.
		for (unsigned int feature_index = 0; feature_index < num_features; ++feature_index)
		{
			const ReconstructMethodFeature &reconstruct_method_feature =
					d_reconstruct_method_feature_seq[feature_index];

			// Create a new reconstruct method for the current feature and its reconstruct method type.
			ReconstructMethodInterface::non_null_ptr_type context_state_reconstruct_method =
					d_reconstruct_method_registry.create_reconstruct_method(
							reconstruct_method_feature.reconstruction_method_type,
							reconstruct_method_feature.feature_ref,
							context_state_ref->d_reconstruct_method_context);

			context_state_ref->d_reconstruct_methods.push_back(context_state_reconstruct_method);
		}
	}
}
