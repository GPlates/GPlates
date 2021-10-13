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

#include "ReconstructionTreeCreator.h"

#include "AppLogicUtils.h"
#include "ReconstructionGraph.h"
#include "ReconstructionTreePopulator.h"

#include "global/PreconditionViolationError.h"
#include "global/GPlatesAssert.h"


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
				d_max_num_reconstruction_trees_in_cache(reconstruction_tree_cache_size),
				d_num_reconstruction_trees_in_cache(0)
			{  }


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


			void
			clear()
			{
				d_reconstruction_tree_map.clear();
				d_reconstruction_tree_order.clear();
				d_num_reconstruction_trees_in_cache = 0;
			}

		private:
			//! Typedef to map a (reconstruction time, anchor plate id) pair to a reconstruction tree.
			typedef std::map<
					std::pair<GPlatesMaths::real_t, GPlatesModel::integer_plate_id_type>,
					ReconstructionTree::non_null_ptr_to_const_type>
							reconstruction_tree_map_type;

			//! Typedef to track least-recently to most-recently requested reconstruction times.
			typedef std::list<reconstruction_tree_map_type::iterator> reconstruction_tree_order_seq_type;


			std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> d_reconstruction_features_collection;
			GPlatesMaths::real_t d_default_reconstruction_time;
			GPlatesModel::integer_plate_id_type d_default_anchor_plate_id;

			const unsigned int d_max_num_reconstruction_trees_in_cache;
			reconstruction_tree_order_seq_type d_reconstruction_tree_order;
			reconstruction_tree_map_type d_reconstruction_tree_map;
			unsigned int d_num_reconstruction_trees_in_cache;


			/**
			 * Returns the reconstruction tree for the specified time/anchor if it exists.
			 */
			boost::optional<ReconstructionTree::non_null_ptr_to_const_type>
			have_reconstruction_tree(
					const double &reconstruction_time,
					GPlatesModel::integer_plate_id_type anchor_plate_id);

			/**
			 * Adds a reconstruction tree to the cache for the specified time/anchor.
			 */
			void
			add_reconstruction_tree(
					const double &reconstruction_time,
					GPlatesModel::integer_plate_id_type anchor_plate_id,
					const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree);
		};


		ReconstructionTree::non_null_ptr_to_const_type
		ReconstructionTreeCache::get_reconstruction_tree(
				const double &reconstruction_time,
				GPlatesModel::integer_plate_id_type anchor_plate_id)
		{
			// See if there's a reconstruction tree cached for the specified time/anchor.
			boost::optional<ReconstructionTree::non_null_ptr_to_const_type> cached_reconstruction_tree =
					have_reconstruction_tree(reconstruction_time, anchor_plate_id);

			// Create a new reconstruction tree if one isn't cached for the specified time/anchor.
			if (!cached_reconstruction_tree)
			{
				// Create a reconstruction tree for the specified time/anchor.
				cached_reconstruction_tree = create_reconstruction_tree(
						reconstruction_time,
						anchor_plate_id,
						d_reconstruction_features_collection);

				// Add to the cache.
				add_reconstruction_tree(
						reconstruction_time,
						anchor_plate_id,
						cached_reconstruction_tree.get());
			}

			return cached_reconstruction_tree.get();
		}


		boost::optional<ReconstructionTree::non_null_ptr_to_const_type>
		ReconstructionTreeCache::have_reconstruction_tree(
				const double &reconstruction_time,
				GPlatesModel::integer_plate_id_type anchor_plate_id)
		{
			reconstruction_tree_map_type::iterator recon_tree_iter =
					d_reconstruction_tree_map.find(
							std::make_pair(reconstruction_time, anchor_plate_id)/*key*/);
			if (recon_tree_iter == d_reconstruction_tree_map.end())
			{
				return boost::none;
			}

			return recon_tree_iter->second;
		}


		void
		ReconstructionTreeCache::add_reconstruction_tree(
				const double &reconstruction_time,
				GPlatesModel::integer_plate_id_type anchor_plate_id,
				const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree)
		{
			// If we already have the maximum number of reconstruction trees cached then
			// release the least-recently requested one to free a slot.
			if (d_num_reconstruction_trees_in_cache == d_max_num_reconstruction_trees_in_cache)
			{
				// Pop off the front of the list where the least-recent requests are.
				reconstruction_tree_map_type::iterator reconstruction_tree_map_entry =
						d_reconstruction_tree_order.front();
				d_reconstruction_tree_map.erase(reconstruction_tree_map_entry);
				d_reconstruction_tree_order.pop_front();
				--d_num_reconstruction_trees_in_cache;
			}

			std::pair<reconstruction_tree_map_type::iterator, bool> insert_result =
					d_reconstruction_tree_map.insert(
							reconstruction_tree_map_type::value_type(
									std::make_pair(reconstruction_time, anchor_plate_id)/*key*/,
									reconstruction_tree/*value*/));

			// The reconstruction time shouldn't already exist in the map.
			// If it does for some reason then we'll just leave the corresponding reconstruction tree in there.
			if (insert_result.second)
			{
				// Add to the back of the list where most-recent requests go.
				d_reconstruction_tree_order.push_back(insert_result.first);
				++d_num_reconstruction_trees_in_cache;
			}
		}
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
