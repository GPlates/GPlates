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

#ifndef GPLATES_APP_LOGIC_RECONSTRUCTIONTREECREATOR_H
#define GPLATES_APP_LOGIC_RECONSTRUCTIONTREECREATOR_H

#include <utility>
#include <vector>
#include <boost/bind.hpp>
#include <boost/function.hpp>

#include "ReconstructionTree.h"

#include "maths/types.h"

#include "model/FeatureCollectionHandle.h"
#include "model/types.h"

#include "utils/KeyValueCache.h"
#include "utils/ReferenceCount.h"


namespace GPlatesAppLogic
{
	/**
	 * Create and return a reconstruction tree for the reconstruction time @a time,
	 * with anchor plate id @a anchor_plate_id.
	 *
	 * The feature collections in @a reconstruction_features_collection are expected to
	 * contain reconstruction features (ie, total reconstruction sequences and absolute
	 * reference frames).
	 *
	 * If @a reconstruction_features_collection is empty then the returned @a ReconstructionTree
	 * will always give an identity rotation when queried for a composed absolute rotation.
	 */
	const ReconstructionTree::non_null_ptr_type
	create_reconstruction_tree(
			const double &reconstruction_time,
			GPlatesModel::integer_plate_id_type anchor_plate_id,
			const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
					reconstruction_features_collection =
							std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>());


	// Forward declaration.
	class ReconstructionTreeCreatorImpl;

	/**
	 * A wrapper around an implementation for creating reconstruction trees.
	 *
	 * For example some implementations may cache reconstruction trees, others may delegate to a
	 * reconstruction layer proxy, but the interface for creating reconstruction trees remains the same.
	 */
	class ReconstructionTreeCreator
	{
	public:
		explicit
		ReconstructionTreeCreator(
				const GPlatesUtils::non_null_intrusive_ptr<ReconstructionTreeCreatorImpl> &impl);

		~ReconstructionTreeCreator();

		/**
		 * Returns the reconstruction tree for the specified time and anchored plate id.
		 */
		ReconstructionTree::non_null_ptr_to_const_type
		get_reconstruction_tree(
				const double &reconstruction_time,
				GPlatesModel::integer_plate_id_type anchor_plate_id) const;


		/**
		 * Returns the reconstruction tree for the specified time and the *default* anchored plate id
		 * that 'this' ReconstructionTreeCreator was created with.
		 */
		ReconstructionTree::non_null_ptr_to_const_type
		get_reconstruction_tree(
				const double &reconstruction_time) const;

	private:
		GPlatesUtils::non_null_intrusive_ptr<ReconstructionTreeCreatorImpl> d_impl;
	};


	/**
	 * Creates a @a ReconstructionTreeCreator that is implemented as a least-recently-used cache
	 * of reconstruction trees.
	 *
	 * This is useful to reuse reconstruction trees spanning different reconstruction times and
	 * anchor plate ids. It's also useful if you aren't reusing trees in which case using the
	 * default value (one cached tree) means it won't get created each time you call it with
	 * the same parameters (reconstruction time and anchor plate id).
	 *
	 * NOTE: The reconstruction feature collection weak refs are copied internally.
	 *
	 * @throws @a PreconditionViolationError if @a reconstruction_tree_cache_size is zero.
	 */
	ReconstructionTreeCreator
	create_cached_reconstruction_tree_creator(
			const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstruction_features_collection,
			GPlatesModel::integer_plate_id_type default_anchor_plate_id = 0,
			unsigned int reconstruction_tree_cache_size = 1);

	/**
	 * Similar to @a create_cached_reconstruction_tree_creator but instead of directly creating
	 * reconstruction trees it gets then from an existing @a ReconstructionTreeCreator.
	 *
	 * This is useful for re-using an existing reconstruction tree creator but extending the
	 * cache size or specifying a desired cache size.
	 *
	 * @throws @a PreconditionViolationError if @a reconstruction_tree_cache_size is zero.
	 */
	ReconstructionTreeCreator
	create_cached_reconstruction_tree_adaptor(
			const ReconstructionTreeCreator &reconstruction_tree_creator,
			GPlatesModel::integer_plate_id_type default_anchor_plate_id = 0,
			unsigned int reconstruction_tree_cache_size = 1);


	// Forward declaration.
	class CachedReconstructionTreeCreatorImpl;

	/**
	 * Similar to @a create_cached_reconstruction_tree_creator but returns the implementation object
	 * (which can subsequently be wrapped in a @a ReconstructionTreeCreator).
	 *
	 * The main use of this function is for the client to obtain direct access to the implementation
	 * so they can change the default reconstruction time and anchor plate id and change the cache size.
	 */
	GPlatesUtils::non_null_intrusive_ptr<CachedReconstructionTreeCreatorImpl>
	create_cached_reconstruction_tree_creator_impl(
			const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstruction_features_collection,
			GPlatesModel::integer_plate_id_type default_anchor_plate_id = 0,
			unsigned int reconstruction_tree_cache_size = 1);

	/**
	 * Similar to @a create_cached_reconstruction_tree_adaptor but returns the implementation object
	 * (which can subsequently be wrapped in a @a ReconstructionTreeCreator).
	 *
	 * The main use of this function is for the client to obtain direct access to the implementation
	 * so they can change the default reconstruction time and anchor plate id and change the cache size.
	 */
	GPlatesUtils::non_null_intrusive_ptr<CachedReconstructionTreeCreatorImpl>
	create_cached_reconstruction_tree_adaptor_impl(
			const ReconstructionTreeCreator &reconstruction_tree_creator,
			GPlatesModel::integer_plate_id_type default_anchor_plate_id = 0,
			unsigned int reconstruction_tree_cache_size = 1);


	/**
	 * Creates a @a ReconstructionTreeCreator that creates a new reconstruction tree each time
	 * a reconstruction tree is requested.
	 *
	 * NOTE: Because there is no caching, this creator can be quite inefficient if a reconstruction
	 * tree with the same parameters is requested multiple times.
	 * In general you should consider creating a *cached* creator instead.
	 * This is currently used in @a ReconstructedFeatureGeometry to directly create reconstruction
	 * trees when it didn't have a shared cached reconstruction tree creator passed into it.
	 *
	 * The reconstruction feature collection weak refs are copied internally.
	 */
	ReconstructionTreeCreator
	create_uncached_reconstruction_tree_creator(
			const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstruction_features_collection,
			GPlatesModel::integer_plate_id_type default_anchor_plate_id = 0);


	/**
	 * Base implementation class for @a ReconstructionTreeCreator.
	 *
	 * Useful if you need to implement a new implementation (eg, different than that provided
	 * by @a create_cached_reconstruction_tree_creator).
	 */
	class ReconstructionTreeCreatorImpl :
			public GPlatesUtils::ReferenceCount<ReconstructionTreeCreatorImpl>
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<ReconstructionTreeCreatorImpl> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const ReconstructionTreeCreatorImpl> non_null_ptr_to_const_type;


		virtual
		~ReconstructionTreeCreatorImpl()
		{  }


		//! Returns the reconstruction tree for the specified time and anchored plate id.
		virtual
		ReconstructionTree::non_null_ptr_to_const_type
		get_reconstruction_tree(
				const double &reconstruction_time,
				GPlatesModel::integer_plate_id_type anchor_plate_id) = 0;


		//! Returns the reconstruction tree for the specified time and the *default* anchored plate id.
		virtual
		ReconstructionTree::non_null_ptr_to_const_type
		get_reconstruction_tree_default_anchored_plate_id(
				const double &reconstruction_time) = 0;
	};


	/**
	 * A reconstruction tree creator implementation that caches the most-recently requested reconstruction trees.
	 */
	class CachedReconstructionTreeCreatorImpl :
			public ReconstructionTreeCreatorImpl
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<CachedReconstructionTreeCreatorImpl> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const CachedReconstructionTreeCreatorImpl> non_null_ptr_to_const_type;


		/**
		 * Creates a cache that will generate reconstruction trees.
		 *
		 * The maximum number of cached reconstruction trees is @a max_num_reconstruction_trees_in_cache.
		 */
		static
		non_null_ptr_type
		create(
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstruction_features_collection,
				GPlatesModel::integer_plate_id_type default_anchor_plate_id,
				unsigned int reconstruction_tree_cache_size)
		{
			return non_null_ptr_type(
					new CachedReconstructionTreeCreatorImpl(
							reconstruction_features_collection,
							default_anchor_plate_id,
							reconstruction_tree_cache_size));
		}


		/**
		 * Creates a cache that will generate reconstruction trees.
		 *
		 * Very similar to the other constructor but instead of directly creating reconstruction
		 * trees it gets then from an existing @a ReconstructionTreeCreator.
		 * This is useful for re-using an existing reconstruction tree creator but extending the
		 * cache size or specifying a desired cache size.
		 *
		 * The maximum number of cached reconstruction trees is @a max_num_reconstruction_trees_in_cache.
		 */
		static
		non_null_ptr_type
		create(
				const ReconstructionTreeCreator &reconstruction_tree_creator,
				GPlatesModel::integer_plate_id_type default_anchor_plate_id,
				unsigned int reconstruction_tree_cache_size)
		{
			return non_null_ptr_type(
					new CachedReconstructionTreeCreatorImpl(
							reconstruction_tree_creator,
							default_anchor_plate_id,
							reconstruction_tree_cache_size));
		}


		/**
		 * Sets the maximum number of cached reconstruction trees.
		 *
		 * If the current number of reconstruction trees exceeds the maximum then the
		 * least-recently used reconstruction trees are removed.
		 */
		void
		set_maximum_cache_size(
				unsigned int maximum_num_cache_size);


		/**
		 * Clears any cached reconstruction trees.
		 */
		void
		clear_cache();


		//! Returns the reconstruction tree for the specified time and anchored plate id.
		virtual
		ReconstructionTree::non_null_ptr_to_const_type
		get_reconstruction_tree(
				const double &reconstruction_time,
				GPlatesModel::integer_plate_id_type anchor_plate_id);

		//! Returns the reconstruction tree for the specified time and the *default* anchored plate id.
		virtual
		ReconstructionTree::non_null_ptr_to_const_type
		get_reconstruction_tree_default_anchored_plate_id(
				const double &reconstruction_time);

	private:
		//! Typedef for the key in the reconstruction tree cache.
		typedef std::pair<GPlatesMaths::real_t, GPlatesModel::integer_plate_id_type> cache_key_type;

		//! Typedef for the value in the reconstruction tree cache.
		typedef ReconstructionTree::non_null_ptr_to_const_type cache_value_type;

		//! Typedef for the reconstruction tree cache.
		typedef GPlatesUtils::KeyValueCache<cache_key_type, cache_value_type> cache_type;

		//! Typedef for a function accepting a cache key and returning a reconstruction tree.
		typedef boost::function< cache_value_type (const cache_key_type &) >
				create_reconstruction_tree_function_type;


		GPlatesModel::integer_plate_id_type d_default_anchor_plate_id;

		create_reconstruction_tree_function_type d_create_reconstruction_tree_function;
		cache_type d_cache;


		CachedReconstructionTreeCreatorImpl(
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstruction_features_collection,
				GPlatesModel::integer_plate_id_type default_anchor_plate_id,
				unsigned int reconstruction_tree_cache_size);

		CachedReconstructionTreeCreatorImpl(
				const ReconstructionTreeCreator &reconstruction_tree_creator,
				GPlatesModel::integer_plate_id_type default_anchor_plate_id,
				unsigned int reconstruction_tree_cache_size);


		/**
		 * Creates a reconstruction tree given the cache key (reconstruction time and anchor plate id).
		 */
		cache_value_type
		create_reconstruction_tree_from_reconstruction_feature_collections(
				const cache_key_type &key,
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
						reconstruction_features_collection);

		/**
		 * Creates a reconstruction tree given the cache key (reconstruction time and anchor plate id).
		 */
		cache_value_type
		create_reconstruction_tree_from_reconstruction_tree_creator(
				const cache_key_type &key,
				const ReconstructionTreeCreator &reconstruction_tree_creator);
	};
}

#endif // GPLATES_APP_LOGIC_RECONSTRUCTIONTREECREATOR_H
