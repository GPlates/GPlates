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

#include "ReconstructionLayerProxy.h"

#include "ReconstructUtils.h"

#include "global/GPlatesAssert.h"


namespace GPlatesAppLogic
{
	namespace
	{
		/**
		 * A reconstruction tree creator that delegates to @a ReconstructionLayerProxy.
		 *
		 * This allows clients of @a ReconstructionLayerProxy to keep a @a ReconstructionTreeCreator
		 * object even if the internal @a ReconstructionTreeCreator (used inside @a ReconstructionLayerProxy)
		 * is rebuilt (destroyed and recreated) over time.
		 */
		class DelegateReconstructionTreeCreator :
				public ReconstructionTreeCreatorImpl
		{
		public:
			explicit
			DelegateReconstructionTreeCreator(
						const ReconstructionLayerProxy::non_null_ptr_type &reconstruction_layer_proxy) :
				d_reconstruction_layer_proxy(reconstruction_layer_proxy)
			{  }


			//! Returns the reconstruction tree for the specified time and anchored plate id.
			virtual
			ReconstructionTree::non_null_ptr_to_const_type
			get_reconstruction_tree(
					const double &reconstruction_time,
					GPlatesModel::integer_plate_id_type anchor_plate_id)
			{
				return d_reconstruction_layer_proxy->get_reconstruction_tree(reconstruction_time, anchor_plate_id);
			}


			//! Returns the reconstruction tree for the specified time and the *default* anchored plate id.
			virtual
			ReconstructionTree::non_null_ptr_to_const_type
			get_reconstruction_tree_default_anchored_plate_id(
					const double &reconstruction_time)
			{
				return d_reconstruction_layer_proxy->get_reconstruction_tree(reconstruction_time);
			}

		private:
			ReconstructionLayerProxy::non_null_ptr_type d_reconstruction_layer_proxy;
		};


		/**
		 * Returns a @a ReconstructionTreeCreator that delegates to @a reconstruction_layer_proxy.
		 */
		ReconstructionTreeCreator
		get_delegate_reconstruction_tree_creator(
				const ReconstructionLayerProxy::non_null_ptr_type &reconstruction_layer_proxy)
		{
			ReconstructionTreeCreatorImpl::non_null_ptr_type impl(
					new DelegateReconstructionTreeCreator(
							reconstruction_layer_proxy));

			return ReconstructionTreeCreator(impl);
		}
	}
}


GPlatesAppLogic::ReconstructionLayerProxy::ReconstructionLayerProxy(
		unsigned int default_max_num_reconstruction_trees_in_cache,
		GPlatesModel::integer_plate_id_type initial_anchored_plate_id) :
	d_current_reconstruction_time(0),
	d_current_anchor_plate_id(initial_anchored_plate_id),
	d_default_max_num_reconstruction_trees_in_cache(default_max_num_reconstruction_trees_in_cache),
	d_current_max_num_reconstruction_trees_in_cache(default_max_num_reconstruction_trees_in_cache)
{
}


GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type
GPlatesAppLogic::ReconstructionLayerProxy::get_reconstruction_tree(
		const double &reconstruction_time,
		GPlatesModel::integer_plate_id_type anchor_plate_id)
{
	if (!d_cached_reconstruction_trees)
	{
		d_cached_reconstruction_trees = create_cached_reconstruction_tree_creator_impl(
				d_current_reconstruction_feature_collections,
				d_current_reconstruction_params.get_extend_total_reconstruction_poles_to_distant_past(),
				d_current_anchor_plate_id/*default_anchor_plate_id*/,
				d_current_max_num_reconstruction_trees_in_cache);
	}

	// See if there's a reconstruction tree cached for the specified reconstruction time.
	// If not then a new one will get created using the specified reconstruction time and anchor plate id.
	return d_cached_reconstruction_trees.get()->get_reconstruction_tree(
			reconstruction_time,
			anchor_plate_id);
}


GPlatesAppLogic::ReconstructionTreeCreator
GPlatesAppLogic::ReconstructionLayerProxy::get_reconstruction_tree_creator(
		boost::optional<unsigned int> max_num_reconstruction_trees_in_cache_hint)
{
	// Use the cache size hint (if provided) to update the current maximum cache size,
	// otherwise leave it at whatever it currently is.
	if (max_num_reconstruction_trees_in_cache_hint)
	{
		// We can increase the cache size above the default cache size but we won't reduce it
		// below the default since that would reduce efficiency for other clients (an example is
		// flowlines which expect a reasonable cache size in order to operate efficiently).
		if (max_num_reconstruction_trees_in_cache_hint.get() > d_default_max_num_reconstruction_trees_in_cache)
		{
			d_current_max_num_reconstruction_trees_in_cache = max_num_reconstruction_trees_in_cache_hint.get();
		}
		else
		{
			d_current_max_num_reconstruction_trees_in_cache = d_default_max_num_reconstruction_trees_in_cache;
		}

		// If we currently have cached reconstruction trees then set the max cache size now,
		// otherwise set it later when the reconstruction tree creator is created.
		if (d_cached_reconstruction_trees)
		{
			d_cached_reconstruction_trees.get()->set_maximum_cache_size(
					d_current_max_num_reconstruction_trees_in_cache);
		}
	}

	// We always return a delegate that defers to 'this' layer proxy interface instead of
	// deferring directly to our internal cached reconstruction tree creator.
	// This way any changes to the current reconstruction time or anchor plate id will be visible to the
	// client (via the returned delegate) even if those changes are made *after* the delegate is returned.
	// And any changes to the cache size will also be visible by all clients (not just the client
	// that called us) regardless of whether they request a reconstruction tree creator object or not.
	return get_delegate_reconstruction_tree_creator(GPlatesUtils::get_non_null_pointer(this));
}


void
GPlatesAppLogic::ReconstructionLayerProxy::set_current_reconstruction_time(
		const double &reconstruction_time)
{
	if (d_current_reconstruction_time == GPlatesMaths::real_t(reconstruction_time))
	{
		// The current reconstruction time hasn't changed so avoid updating any observers unnecessarily.
		return;
	}
	d_current_reconstruction_time = reconstruction_time;

	// Note that we don't invalidate our cache because if a reconstruction tree is
	// not cached for a requested reconstruction time then a new tree is created.

	// UPDATE: Don't need to notify observers of change in reconstruction time because all layers
	// can easily find this out. We want to avoid observer updates here in case any of them
	// cache calculations based on the reconstruction time - if we told them we had changed they
	// would have no way of knowing that only the reconstruction time changed and hence they
	// would be forced to flush their caches losing any benefit of caching over reconstruction times.
#if 0
	// But observers need to be aware that the default reconstruction time has changed.
	d_subject_token.invalidate();
#endif
}


void
GPlatesAppLogic::ReconstructionLayerProxy::set_current_anchor_plate_id(
		GPlatesModel::integer_plate_id_type anchor_plate_id)
{
	if (d_current_anchor_plate_id == anchor_plate_id)
	{
		// The current anchor plate id hasn't changed so avoid updating any observers unnecessarily.
		return;
	}
	d_current_anchor_plate_id = anchor_plate_id;

	// The default anchor plate id (stored in the cached reconstruction tree creator) has changed
	// so we need to invalidate the reconstruction tree cache.
	invalidate();
}


void
GPlatesAppLogic::ReconstructionLayerProxy::set_current_reconstruction_params(
		const ReconstructionParams &reconstruction_params)
{
	if (d_current_reconstruction_params == reconstruction_params)
	{
		// The current reconstruction params haven't changed so avoid updating any observers unnecessarily.
		return;
	}
	d_current_reconstruction_params = reconstruction_params;

	// The reconstruction trees are now invalid.
	invalidate();
}


void
GPlatesAppLogic::ReconstructionLayerProxy::add_reconstruction_feature_collection(
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	d_current_reconstruction_feature_collections.push_back(feature_collection);

	// The reconstruction trees are now invalid.
	invalidate();
}


void
GPlatesAppLogic::ReconstructionLayerProxy::remove_reconstruction_feature_collection(
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	// Erase the feature collection from our list.
	d_current_reconstruction_feature_collections.erase(
			std::find(
					d_current_reconstruction_feature_collections.begin(),
					d_current_reconstruction_feature_collections.end(),
					feature_collection));

	// The reconstruction trees are now invalid.
	invalidate();
}


void
GPlatesAppLogic::ReconstructionLayerProxy::modified_reconstruction_feature_collection(
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	// The reconstruction trees are now invalid.
	invalidate();
}


void
GPlatesAppLogic::ReconstructionLayerProxy::invalidate()
{
	// Clear any cached reconstruction trees.
	d_cached_reconstruction_trees = boost::none;

	// Set the maximum reconstruction tree cache size back to the default.
	// We don't want client requests for very large caches to continue indefinitely.
	// In any case, due to this invalidation, the client will need to update itself by requesting
	// another reconstruction tree creator and it will again specify its desired cache size.
	d_current_max_num_reconstruction_trees_in_cache = d_default_max_num_reconstruction_trees_in_cache;

	// Polling observers need to update themselves.
	d_subject_token.invalidate();
}
