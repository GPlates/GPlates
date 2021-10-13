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


			//! Returns the reconstruction tree for the specified anchored plate id and the *default* time.
			virtual
			ReconstructionTree::non_null_ptr_to_const_type
			get_reconstruction_tree_default_reconstruction_time(
					GPlatesModel::integer_plate_id_type anchor_plate_id)
			{
				return d_reconstruction_layer_proxy->get_reconstruction_tree(anchor_plate_id);
			}


			//! Returns the reconstruction tree for the *default* time and the *default* anchored plate id.
			virtual
			ReconstructionTree::non_null_ptr_to_const_type
			get_reconstruction_tree_default_reconstruction_time_and_anchored_plate_id()
			{
				return d_reconstruction_layer_proxy->get_reconstruction_tree();
			}


			//! Changes the default reconstruction time.
			virtual
			void
			set_default_reconstruction_time(
					const double &reconstruction_time)
			{
				// This method shouldn't be called because we're delegating to a
				// ReconstructionLayerProxy and the default should be set directly on that instead.
				GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
			}


			//! Changes the default anchor plate id.
			virtual
			void
			set_default_anchor_plate_id(
					GPlatesModel::integer_plate_id_type anchor_plate_id)
			{
				// This method shouldn't be called because we're delegating to a
				// ReconstructionLayerProxy and the default should be set directly on that instead.
				GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
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


GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type
GPlatesAppLogic::ReconstructionLayerProxy::get_reconstruction_tree(
		const double &reconstruction_time,
		GPlatesModel::integer_plate_id_type anchor_plate_id)
{
	if (!d_cached_reconstruction_trees)
	{
		d_cached_reconstruction_trees = get_cached_reconstruction_tree_creator(
				d_current_reconstruction_feature_collections,
				d_current_reconstruction_time.dval()/*default_reconstruction_time*/,
				d_current_anchor_plate_id/*default_anchor_plate_id*/,
				d_max_num_reconstruction_trees_in_cache);
	}

	// See if there's a reconstruction tree cached for the specified reconstruction time.
	// If not then a new one will get created using the specified reconstruction time and anchor plate id.
	return d_cached_reconstruction_trees->get_reconstruction_tree(
			reconstruction_time,
			anchor_plate_id);
}


GPlatesAppLogic::ReconstructionTreeCreator
GPlatesAppLogic::ReconstructionLayerProxy::get_reconstruction_tree_creator()
{
	// Note that 'set_default_reconstruction_time()' and 'set_default_anchor_plate_id()'
	// should not be called on this 
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

	// If 'd_cached_reconstruction_trees' is currently valid then set the new
	// default reconstruction time. This is not strictly necessary since we always call it
	// with an explicit reconstruction time argument but it's better to be safe.
	if (d_cached_reconstruction_trees)
	{
		d_cached_reconstruction_trees->set_default_reconstruction_time(d_current_reconstruction_time.dval());
	}

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

	// If 'd_cached_reconstruction_trees' is currently valid then set the new
	// default anchor plate id. This is not strictly necessary since we always call it
	// with an explicit anchor plate id argument but it's better to be safe.
	if (d_cached_reconstruction_trees)
	{
		d_cached_reconstruction_trees->set_default_anchor_plate_id(d_current_anchor_plate_id);
	}

	// Note that we don't invalidate our cache because if a reconstruction tree is
	// not cached for a requested anchor plate id then a new tree is created.
	// But observers need to be aware that the default anchor plate id has changed.
	d_subject_token.invalidate();
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

	// Polling observers need to update themselves.
	d_subject_token.invalidate();
}
