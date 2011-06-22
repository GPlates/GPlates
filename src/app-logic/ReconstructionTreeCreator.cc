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

#include <list>
#include <map>
#include <utility>
#include <boost/bind.hpp>

#include "ReconstructionTreeCreator.h"

#include "AppLogicUtils.h"
#include "ReconstructionGraph.h"
#include "ReconstructionTreePopulator.h"

#include "global/PreconditionViolationError.h"
#include "global/GPlatesAssert.h"

#include "utils/KeyValueCache.h"


namespace GPlatesAppLogic
{
	namespace
	{
		/**
		 * A cache of the most-recently requested reconstruction trees.
		 */
		class ReconstructionTreeCache :
				public ReconstructionTreeCreatorImpl
		{
		public:
			/**
			 * Creates a cache that will generate reconstruction trees.
			 *
			 * The maximum number of cached reconstruction trees is @a max_num_reconstruction_trees_in_cache.
			 */
			ReconstructionTreeCache(
					const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstruction_features_collection,
					const double &default_reconstruction_time,
					GPlatesModel::integer_plate_id_type default_anchor_plate_id,
					unsigned int reconstruction_tree_cache_size) :
				d_reconstruction_features_collection(reconstruction_features_collection),
				d_default_reconstruction_time(default_reconstruction_time),
				d_default_anchor_plate_id(default_anchor_plate_id),
				d_cache(
						boost::bind(&ReconstructionTreeCache::create_reconstruction_tree_from_cache_key, this, _1),
						reconstruction_tree_cache_size)
			{  }


			//! Returns the reconstruction tree for the specified time and anchored plate id.
			virtual
			ReconstructionTree::non_null_ptr_to_const_type
			get_reconstruction_tree(
					const double &reconstruction_time,
					GPlatesModel::integer_plate_id_type anchor_plate_id)
			{
				return d_cache.get_value(cache_key_type(reconstruction_time, anchor_plate_id));
			}


			//! Returns the reconstruction tree for the specified time and the *default* anchored plate id.
			virtual
			ReconstructionTree::non_null_ptr_to_const_type
			get_reconstruction_tree_default_anchored_plate_id(
					const double &reconstruction_time)
			{
				return get_reconstruction_tree(reconstruction_time, d_default_anchor_plate_id);
			}


			//! Returns the reconstruction tree for the specified anchored plate id and the *default* time.
			virtual
			ReconstructionTree::non_null_ptr_to_const_type
			get_reconstruction_tree_default_reconstruction_time(
					GPlatesModel::integer_plate_id_type anchor_plate_id)
			{
				return get_reconstruction_tree(d_default_reconstruction_time.dval(), anchor_plate_id);
			}


			//! Returns the reconstruction tree for the *default* time and the *default* anchored plate id.
			virtual
			ReconstructionTree::non_null_ptr_to_const_type
			get_reconstruction_tree_default_reconstruction_time_and_anchored_plate_id()
			{
				return get_reconstruction_tree(d_default_reconstruction_time.dval(), d_default_anchor_plate_id);
			}


			//! Changes the default reconstruction time.
			virtual
			void
			set_default_reconstruction_time(
					const double &reconstruction_time)
			{
				d_default_reconstruction_time = reconstruction_time;
			}


			//! Changes the default anchor plate id.
			virtual
			void
			set_default_anchor_plate_id(
					GPlatesModel::integer_plate_id_type anchor_plate_id)
			{
				d_default_anchor_plate_id = anchor_plate_id;
			}


			void
			clear()
			{
				d_cache.clear();
			}

		private:
			//! Typedef for the key in the reconstruction tree cache.
			typedef std::pair<GPlatesMaths::real_t, GPlatesModel::integer_plate_id_type> cache_key_type;

			//! Typedef for the value in the reconstruction tree cache.
			typedef ReconstructionTree::non_null_ptr_to_const_type cache_value_type;

			//! Typedef for the reconstruction tree cache.
			typedef GPlatesUtils::KeyValueCache<cache_key_type, cache_value_type> cache_type;


			std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> d_reconstruction_features_collection;
			GPlatesMaths::real_t d_default_reconstruction_time;
			GPlatesModel::integer_plate_id_type d_default_anchor_plate_id;

			cache_type d_cache;


			/**
			 * Creates a reconstruction tree given the cache key (reconstruction time and anchor plate id).
			 */
			ReconstructionTree::non_null_ptr_type
			create_reconstruction_tree_from_cache_key(
					const cache_key_type &key)
			{
				// Create a reconstruction tree for the specified time/anchor.
				return create_reconstruction_tree(
						key.first.dval()/*reconstruction_time*/,
						key.second/*anchor_plate_id*/,
						d_reconstruction_features_collection);
			}
		};
	}
}


const GPlatesAppLogic::ReconstructionTree::non_null_ptr_type
GPlatesAppLogic::create_reconstruction_tree(
		const double &time,
		GPlatesModel::integer_plate_id_type anchor_plate_id,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
				reconstruction_features_collection)
{
	ReconstructionGraph graph(time, reconstruction_features_collection);
	ReconstructionTreePopulator rtp(time, graph);

	AppLogicUtils::visit_feature_collections(
			reconstruction_features_collection.begin(),
			reconstruction_features_collection.end(),
			rtp);

	// Build the reconstruction tree, using 'root' as the root of the tree.
	ReconstructionTree::non_null_ptr_type tree = graph.build_tree(anchor_plate_id);

	return tree;
}


GPlatesAppLogic::ReconstructionTreeCreator::ReconstructionTreeCreator(
		const GPlatesUtils::non_null_intrusive_ptr<ReconstructionTreeCreatorImpl> &impl) :
	d_impl(impl)
{
}


GPlatesAppLogic::ReconstructionTreeCreator::~ReconstructionTreeCreator()
{
	// Defined in '.cc' file so non_null_intrusive_ptr destructor has access to complete type.
}


GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type
GPlatesAppLogic::ReconstructionTreeCreator::get_reconstruction_tree(
		const double &reconstruction_time,
		GPlatesModel::integer_plate_id_type anchor_plate_id) const
{
	return d_impl->get_reconstruction_tree(reconstruction_time, anchor_plate_id);
}


GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type
GPlatesAppLogic::ReconstructionTreeCreator::get_reconstruction_tree(
		const double &reconstruction_time) const
{
	return d_impl->get_reconstruction_tree_default_anchored_plate_id(reconstruction_time);
}


GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type
GPlatesAppLogic::ReconstructionTreeCreator::get_reconstruction_tree(
		GPlatesModel::integer_plate_id_type anchor_plate_id) const
{
	return d_impl->get_reconstruction_tree_default_reconstruction_time(anchor_plate_id);
}


GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type
GPlatesAppLogic::ReconstructionTreeCreator::get_reconstruction_tree() const
{
	return d_impl->get_reconstruction_tree_default_reconstruction_time_and_anchored_plate_id();
}


void
GPlatesAppLogic::ReconstructionTreeCreator::set_default_reconstruction_time(
		const double &reconstruction_time)
{
	d_impl->set_default_reconstruction_time(reconstruction_time);
}


void
GPlatesAppLogic::ReconstructionTreeCreator::set_default_anchor_plate_id(
		GPlatesModel::integer_plate_id_type anchor_plate_id)
{
	d_impl->set_default_anchor_plate_id(anchor_plate_id);
}


GPlatesAppLogic::ReconstructionTreeCreator
GPlatesAppLogic::get_cached_reconstruction_tree_creator(
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstruction_features_collection,
		const double &default_reconstruction_time,
		GPlatesModel::integer_plate_id_type default_anchor_plate_id,
		unsigned int reconstruction_tree_cache_size)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			reconstruction_tree_cache_size > 0,
			GPLATES_ASSERTION_SOURCE);

	ReconstructionTreeCreatorImpl::non_null_ptr_type impl(
			new ReconstructionTreeCache(
					reconstruction_features_collection,
					default_reconstruction_time,
					default_anchor_plate_id,
					reconstruction_tree_cache_size));

	return ReconstructionTreeCreator(impl);
}
