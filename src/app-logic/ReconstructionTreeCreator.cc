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
#include <boost/foreach.hpp>

#include "ReconstructionTreeCreator.h"

#include "AppLogicUtils.h"
#include "ReconstructionGraphBuilder.h"
#include "ReconstructionGraphPopulator.h"

#include "global/PreconditionViolationError.h"
#include "global/GPlatesAssert.h"

#include "utils/Profile.h"


namespace GPlatesAppLogic
{
	namespace
	{
		/**
		 * An uncached reconstruction tree creator implementation that simply creates a new
		 * reconstruction tree whenever a reconstruction tree is requested.
		 */
		class UncachedReconstructionTreeCreatorImpl :
				public ReconstructionTreeCreatorImpl
		{
		public:

			UncachedReconstructionTreeCreatorImpl(
					const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstruction_feature_collections,
					bool extend_total_reconstruction_poles_to_distant_past,
					GPlatesModel::integer_plate_id_type default_anchor_plate_id) :
				d_reconstruction_graph(
						create_reconstruction_graph(
								reconstruction_feature_collections,
								extend_total_reconstruction_poles_to_distant_past)),
				d_default_anchor_plate_id(default_anchor_plate_id)
			{  }

			//! Returns the reconstruction tree for the specified time and anchored plate id.
			virtual
			ReconstructionTree::non_null_ptr_to_const_type
			get_reconstruction_tree(
					const double &reconstruction_time,
					GPlatesModel::integer_plate_id_type anchor_plate_id)
			{
				return ReconstructionTree::create(d_reconstruction_graph, reconstruction_time, anchor_plate_id);
			}

			//! Returns the reconstruction tree for the specified time and the *default* anchored plate id.
			virtual
			ReconstructionTree::non_null_ptr_to_const_type
			get_reconstruction_tree_default_anchored_plate_id(
					const double &reconstruction_time)
			{
				return get_reconstruction_tree(reconstruction_time, d_default_anchor_plate_id);
			}

		private:

			ReconstructionGraph::non_null_ptr_to_const_type d_reconstruction_graph;
			GPlatesModel::integer_plate_id_type d_default_anchor_plate_id;
		};
	}
}


const GPlatesAppLogic::ReconstructionGraph::non_null_ptr_to_const_type
GPlatesAppLogic::create_reconstruction_graph(
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstruction_feature_collections,
		bool extend_total_reconstruction_poles_to_distant_past)
{
	PROFILE_FUNC();

	ReconstructionGraphBuilder graph_builder(extend_total_reconstruction_poles_to_distant_past);
	ReconstructionGraphPopulator rgp(graph_builder);

	AppLogicUtils::visit_feature_collections(
			reconstruction_feature_collections.begin(),
			reconstruction_feature_collections.end(),
			rgp);

	return graph_builder.build_graph();
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


GPlatesAppLogic::ReconstructionTreeCreator
GPlatesAppLogic::create_cached_reconstruction_tree_creator(
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstruction_feature_collections,
		bool extend_total_reconstruction_poles_to_distant_past,
		GPlatesModel::integer_plate_id_type default_anchor_plate_id,
		unsigned int reconstruction_tree_cache_size)
{
	const ReconstructionTreeCreatorImpl::non_null_ptr_type impl =
			create_cached_reconstruction_tree_creator_impl(
					reconstruction_feature_collections,
					extend_total_reconstruction_poles_to_distant_past,
					default_anchor_plate_id,
					reconstruction_tree_cache_size);

	return ReconstructionTreeCreator(impl);
}


GPlatesAppLogic::ReconstructionTreeCreator
GPlatesAppLogic::create_cached_reconstruction_tree_adaptor(
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		GPlatesModel::integer_plate_id_type default_anchor_plate_id,
		unsigned int reconstruction_tree_cache_size)
{
	const ReconstructionTreeCreatorImpl::non_null_ptr_type impl =
			create_cached_reconstruction_tree_adaptor_impl(
					reconstruction_tree_creator,
					default_anchor_plate_id,
					reconstruction_tree_cache_size);

	return ReconstructionTreeCreator(impl);
}


GPlatesUtils::non_null_intrusive_ptr<GPlatesAppLogic::CachedReconstructionTreeCreatorImpl>
GPlatesAppLogic::create_cached_reconstruction_tree_creator_impl(
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstruction_feature_collections,
		bool extend_total_reconstruction_poles_to_distant_past,
		GPlatesModel::integer_plate_id_type default_anchor_plate_id,
		unsigned int reconstruction_tree_cache_size)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			reconstruction_tree_cache_size > 0,
			GPLATES_ASSERTION_SOURCE);

	return CachedReconstructionTreeCreatorImpl::create(
			reconstruction_feature_collections,
			extend_total_reconstruction_poles_to_distant_past,
			default_anchor_plate_id,
			reconstruction_tree_cache_size);
}


GPlatesUtils::non_null_intrusive_ptr<GPlatesAppLogic::CachedReconstructionTreeCreatorImpl>
GPlatesAppLogic::create_cached_reconstruction_tree_adaptor_impl(
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		GPlatesModel::integer_plate_id_type default_anchor_plate_id,
		unsigned int reconstruction_tree_cache_size)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			reconstruction_tree_cache_size > 0,
			GPLATES_ASSERTION_SOURCE);

	return CachedReconstructionTreeCreatorImpl::create(
			reconstruction_tree_creator,
			default_anchor_plate_id,
			reconstruction_tree_cache_size);
}


GPlatesAppLogic::ReconstructionTreeCreator
GPlatesAppLogic::create_uncached_reconstruction_tree_creator(
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstruction_feature_collections,
		bool extend_total_reconstruction_poles_to_distant_past,
		GPlatesModel::integer_plate_id_type default_anchor_plate_id)
{
	ReconstructionTreeCreatorImpl::non_null_ptr_type impl(
			new UncachedReconstructionTreeCreatorImpl(
					reconstruction_feature_collections,
					extend_total_reconstruction_poles_to_distant_past,
					default_anchor_plate_id));

	return ReconstructionTreeCreator(impl);
}


GPlatesAppLogic::CachedReconstructionTreeCreatorImpl::CachedReconstructionTreeCreatorImpl(
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstruction_feature_collections,
		bool extend_total_reconstruction_poles_to_distant_past,
		GPlatesModel::integer_plate_id_type default_anchor_plate_id,
		unsigned int reconstruction_tree_cache_size) :
	d_default_anchor_plate_id(default_anchor_plate_id),
	d_create_reconstruction_tree_function(
			boost::bind(
					&CachedReconstructionTreeCreatorImpl::create_reconstruction_tree_from_reconstruction_graph,
					this,
					_1,
					// Non-null intrusive pointer to reconstruction graph gets copied by boost-bind...
					create_reconstruction_graph(
							reconstruction_feature_collections,
							extend_total_reconstruction_poles_to_distant_past))),
	d_cache(d_create_reconstruction_tree_function, reconstruction_tree_cache_size)
{
}


GPlatesAppLogic::CachedReconstructionTreeCreatorImpl::CachedReconstructionTreeCreatorImpl(
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		GPlatesModel::integer_plate_id_type default_anchor_plate_id,
		unsigned int reconstruction_tree_cache_size) :
	d_default_anchor_plate_id(default_anchor_plate_id),
	d_create_reconstruction_tree_function(
			boost::bind(
					&CachedReconstructionTreeCreatorImpl::create_reconstruction_tree_from_reconstruction_tree_creator,
					this,
					_1,
					reconstruction_tree_creator)),
	d_cache(d_create_reconstruction_tree_function, reconstruction_tree_cache_size)
{
}


GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type
GPlatesAppLogic::CachedReconstructionTreeCreatorImpl::get_reconstruction_tree(
		const double &reconstruction_time,
		GPlatesModel::integer_plate_id_type anchor_plate_id)
{
	return d_cache.get_value(cache_key_type(reconstruction_time, anchor_plate_id));
}


GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type
GPlatesAppLogic::CachedReconstructionTreeCreatorImpl::get_reconstruction_tree_default_anchored_plate_id(
		const double &reconstruction_time)
{
	return get_reconstruction_tree(reconstruction_time, d_default_anchor_plate_id);
}


void
GPlatesAppLogic::CachedReconstructionTreeCreatorImpl::set_maximum_cache_size(
		unsigned int maximum_num_cache_size)
{
	d_cache.set_maximum_num_values_in_cache(maximum_num_cache_size);
}


void
GPlatesAppLogic::CachedReconstructionTreeCreatorImpl::clear_cache()
{
	d_cache.clear();
}


GPlatesAppLogic::CachedReconstructionTreeCreatorImpl::cache_value_type
GPlatesAppLogic::CachedReconstructionTreeCreatorImpl::create_reconstruction_tree_from_reconstruction_graph(
		const cache_key_type &key,
		ReconstructionGraph::non_null_ptr_to_const_type reconstruction_graph)
{
	//PROFILE_FUNC();

	// Create a reconstruction tree for the specified time/anchor.
	return ReconstructionTree::create(
			reconstruction_graph,
			key.first.dval()/*reconstruction_time*/,
			key.second/*anchor_plate_id*/);
}


GPlatesAppLogic::CachedReconstructionTreeCreatorImpl::cache_value_type
GPlatesAppLogic::CachedReconstructionTreeCreatorImpl::create_reconstruction_tree_from_reconstruction_tree_creator(
		const cache_key_type &key,
		const ReconstructionTreeCreator &reconstruction_tree_creator)
{
	//PROFILE_FUNC();

	// Create a reconstruction tree for the specified time/anchor.
	return reconstruction_tree_creator.get_reconstruction_tree(
			key.first.dval()/*reconstruction_time*/,
			key.second/*anchor_plate_id*/);
}
