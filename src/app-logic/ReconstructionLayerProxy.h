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

#ifndef GPLATES_APP_LOGIC_RECONSTRUCTIONLAYERPROXY_H
#define GPLATES_APP_LOGIC_RECONSTRUCTIONLAYERPROXY_H

#include <vector>
#include <boost/optional.hpp>

#include "LayerProxy.h"
#include "ReconstructionTree.h"
#include "ReconstructionTreeCreator.h"
#include "ReconstructUtils.h"

#include "model/FeatureCollectionHandle.h"
#include "model/types.h"

#include "utils/SubjectObserverToken.h"


namespace GPlatesAppLogic
{
	/**
	 * A layer proxy for creating reconstruction trees at desired reconstruction times.
	 */
	class ReconstructionLayerProxy :
			public LayerProxy
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a ReconstructionLayerProxy.
		typedef GPlatesUtils::non_null_intrusive_ptr<ReconstructionLayerProxy> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a ReconstructionLayerProxy.
		typedef GPlatesUtils::non_null_intrusive_ptr<const ReconstructionLayerProxy> non_null_ptr_to_const_type;


		/**
		 * The maximum number of reconstruction trees to cache for different reconstruction times.
		 */
		static const unsigned int DEFAULT_MAX_NUM_RECONSTRUCTION_TREES_IN_CACHE = 64;


		/**
		 * Creates a @a ReconstructionLayerProxy object.
		 *
		 * @a default_max_num_reconstruction_trees_in_cache specifies the default cache size to use
		 * unless a cache size hint is requested via @a get_reconstruction_tree_creator.
		 */
		static
		non_null_ptr_type
		create(
				unsigned int default_max_num_reconstruction_trees_in_cache = DEFAULT_MAX_NUM_RECONSTRUCTION_TREES_IN_CACHE,
				GPlatesModel::integer_plate_id_type initial_anchored_plate_id = 0)
		{
			return non_null_ptr_type(
					new ReconstructionLayerProxy(
							default_max_num_reconstruction_trees_in_cache,
							initial_anchored_plate_id));
		}


		/**
		 * Returns the reconstruction tree for the current reconstruction time and current anchor plate id.
		 */
		ReconstructionTree::non_null_ptr_to_const_type
		get_reconstruction_tree()
		{
			return get_reconstruction_tree(d_current_reconstruction_time.dval(), d_current_anchor_plate_id);
		}


		/**
		 * Returns the reconstruction tree for the specified time - can be any reconstruction time.
		 *
		 * The current anchor plate id is used.
		 *
		 * A cache is used to store reconstruction trees for the most-recently requested time/anchors.
		 */
		ReconstructionTree::non_null_ptr_to_const_type
		get_reconstruction_tree(
				const double &reconstruction_time)
		{
			return get_reconstruction_tree(reconstruction_time, d_current_anchor_plate_id);
		}


		/**
		 * Returns the reconstruction tree for the specified anchor plate id.
		 *
		 * The current reconstruction time is used.
		 *
		 * A cache is used to store reconstruction trees for the most-recently requested time/anchors.
		 */
		ReconstructionTree::non_null_ptr_to_const_type
		get_reconstruction_tree(
				GPlatesModel::integer_plate_id_type anchor_plate_id)
		{
			return get_reconstruction_tree(d_current_reconstruction_time.dval(), anchor_plate_id);
		}


		/**
		 * Returns the reconstruction tree for the specified reconstruction time and anchor plate id.
		 *
		 * A cache is used to store reconstruction trees for the most-recently requested time/anchors.
		 */
		ReconstructionTree::non_null_ptr_to_const_type
		get_reconstruction_tree(
				const double &reconstruction_time,
				GPlatesModel::integer_plate_id_type anchor_plate_id);


		/**
		 * An alternative to two overloaded versions of @a get_reconstruction_tree - provides
		 * an easy to pass them to other code sections that shouldn't know about layers.
		 *
		 * Note that any updates to 'this' layer proxy will be available when querying the returned
		 * reconstruction tree creator object - this is because it defers queries to 'this' layer proxy.
		 * Modifications include things such as modified rotation feature collections (and hence
		 * modified reconstruction trees) and changes to the current reconstruction time and anchor plate.
		 *
		 * If a cache size hint is specified then the maximum number of internally cached
		 * reconstruction trees is set to this value (or the default passed into constructor,
		 * whichever is larger). If a cache size hint is not specified then the cache size is
		 * left at whatever it currently is.
		 */
		ReconstructionTreeCreator
		get_reconstruction_tree_creator(
				boost::optional<unsigned int> max_num_reconstruction_trees_in_cache_hint = boost::none);


		/**
		 * Gets the current anchor plate id.
		 */
		GPlatesModel::integer_plate_id_type
		get_current_anchor_plate_id() const
		{
			return d_current_anchor_plate_id;
		}


		/**
		 * Returns the subject token that clients can use to determine if a reconstruction tree
		 * has changed since they last retrieved one.
		 *
		 * This is mainly useful for other layers that have this layer connected as their input.
		 */
		const GPlatesUtils::SubjectToken &
		get_subject_token()
		{
			return d_subject_token;
		}


		/**
		 * Accept a ConstLayerProxyVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				ConstLayerProxyVisitor &visitor) const
		{
			visitor.visit(GPlatesUtils::get_non_null_pointer(this));
		}

		/**
		 * Accept a LayerProxyVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				LayerProxyVisitor &visitor)
		{
			visitor.visit(GPlatesUtils::get_non_null_pointer(this));
		}


		//
		// Used by LayerTask...
		//

		/**
		 * Sets the current reconstruction time as set by the layer system.
		 */
		void
		set_current_reconstruction_time(
				const double &reconstruction_time);

		/**
		 * Sets the current anchor plate id as set by the layer system.
		 */
		void
		set_current_anchor_plate_id(
				GPlatesModel::integer_plate_id_type anchor_plate_id);

		/**
		 * Add to the list of feature collections that are used to build reconstruction trees.
		 */
		void
		add_reconstruction_feature_collection(
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection);

		/**
		 * Remove from the list of feature collections that are used to build reconstruction trees.
		 */
		void
		remove_reconstruction_feature_collection(
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection);

		/**
		 * A reconstruction feature collection was modified.
		 */
		void
		modified_reconstruction_feature_collection(
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection);

	private:
		/**
		 * The input feature collections used to generate reconstruction trees
		 * at reconstruction times specified by clients.
		 */
		std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> d_current_reconstruction_feature_collections;

		/**
		 * The current reconstruction time as set by the layer system.
		 */
		GPlatesMaths::real_t d_current_reconstruction_time;

		/**
		 * The current anchored plate id as set by the layer system.
		 */
		GPlatesModel::integer_plate_id_type d_current_anchor_plate_id;

		/**
		 * Manages cached reconstruction trees for the most-recently requested reconstruction time/anchors.
		 */
		boost::optional<CachedReconstructionTreeCreatorImpl::non_null_ptr_type> d_cached_reconstruction_trees;

		/**
		 * Used to notify polling observers that we've been updated.
		 */
		mutable GPlatesUtils::SubjectToken d_subject_token;

		/**
		 * The default value for the maximum number of reconstruction trees in the cache.
		 */
		unsigned int d_default_max_num_reconstruction_trees_in_cache;

		/**
		 * The current maximum number of reconstruction trees in the cache before we start evicting.
		 */
		unsigned int d_current_max_num_reconstruction_trees_in_cache;


		explicit
		ReconstructionLayerProxy(
				unsigned int default_max_num_reconstruction_trees_in_cache,
				GPlatesModel::integer_plate_id_type initial_anchored_plate_id) :
			d_current_reconstruction_time(0),
			d_current_anchor_plate_id(initial_anchored_plate_id),
			d_default_max_num_reconstruction_trees_in_cache(default_max_num_reconstruction_trees_in_cache),
			d_current_max_num_reconstruction_trees_in_cache(default_max_num_reconstruction_trees_in_cache)
		{  }


		/**
		 * Called when we are updated.
		 */
		void
		invalidate();
	};
}

#endif // GPLATES_APP_LOGIC_RECONSTRUCTIONLAYERPROXY_H
