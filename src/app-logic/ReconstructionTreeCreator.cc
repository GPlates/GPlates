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
#include "ReconstructionGraph.h"
#include "ReconstructionTreePopulator.h"

#include "global/PreconditionViolationError.h"
#include "global/GPlatesAssert.h"

#include "utils/Profile.h"


namespace GPlatesAppLogic
{
	namespace
	{
		/**
		 * Clone an existing feature collection.
		 *
		 * NOTE: We don't use FeatureHandle::clone() because it currently does a shallow copy
		 * instead of a deep copy (and we're in the middle of updating the model).
		 * FIXME: Once FeatureHandle has been updated to use the same revisioning system as
		 * TopLevelProperty and PropertyValue then just use FeatureCollectionHandle::clone() instead.
		 */
		GPlatesModel::FeatureCollectionHandle::non_null_ptr_type
		clone_feature_collection(
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection_ref)
		{
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					feature_collection_ref.is_valid(),
					GPLATES_ASSERTION_SOURCE);

			GPlatesModel::FeatureCollectionHandle::non_null_ptr_type cloned_feature_collection =
					GPlatesModel::FeatureCollectionHandle::create();

			// Iterate over the feature of the feature collection and clone them.
			GPlatesModel::FeatureCollectionHandle::iterator features_iter = feature_collection_ref->begin();
			GPlatesModel::FeatureCollectionHandle::iterator features_end = feature_collection_ref->end();
			for ( ; features_iter != features_end; ++features_iter)
			{
				if (!features_iter.is_still_valid())
				{
					continue;
				}

				GPlatesModel::FeatureHandle::non_null_ptr_type feature = *features_iter;

				GPlatesModel::FeatureHandle::non_null_ptr_type cloned_feature =
						GPlatesModel::FeatureHandle::create(feature->feature_type());

				// Iterate over the properties of the feature and clone them.
				GPlatesModel::FeatureHandle::iterator properties_iter = feature->begin();
				GPlatesModel::FeatureHandle::iterator properties_end = feature->end();
				for ( ; properties_iter != properties_end; ++properties_iter)
				{
					GPlatesModel::TopLevelProperty::non_null_ptr_type feature_property = *properties_iter;

					cloned_feature->add(feature_property->clone());
				}

				cloned_feature_collection->add(cloned_feature);
			}

			// Copy the tags also.
			cloned_feature_collection->tags() = feature_collection_ref->tags();

			return cloned_feature_collection;
		}


		/**
		 * An uncached reconstruction tree creator implementation that simply creates a new
		 * reconstruction tree whenever a reconstruction tree is requested.
		 */
		class UncachedReconstructionTreeCreatorImpl :
				public ReconstructionTreeCreatorImpl
		{
		public:

			UncachedReconstructionTreeCreatorImpl(
					const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstruction_features_collection,
					GPlatesModel::integer_plate_id_type default_anchor_plate_id,
					bool clone_reconstruction_features) :
				d_default_anchor_plate_id(default_anchor_plate_id)
			{
				BOOST_FOREACH(
						const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection_ref,
						reconstruction_features_collection)
				{
					// If requested then clone the feature collections to ensure that we will always create
					// a reconstruction tree using the state of the feature collections right now
					// (because they could subsequently get modified by some other client).
					if (feature_collection_ref.is_valid())
					{
						const GPlatesModel::FeatureCollectionHandle::non_null_ptr_type feature_collection =
								clone_reconstruction_features
								? clone_feature_collection(feature_collection_ref)
								: GPlatesModel::FeatureCollectionHandle::non_null_ptr_type(feature_collection_ref.handle_ptr());

						d_cloned_reconstruction_features_collection.push_back(feature_collection);
						d_cloned_reconstruction_features_collection_refs.push_back(feature_collection->reference());
					}
				}
			}

			//! Returns the reconstruction tree for the specified time and anchored plate id.
			virtual
			ReconstructionTree::non_null_ptr_to_const_type
			get_reconstruction_tree(
					const double &reconstruction_time,
					GPlatesModel::integer_plate_id_type anchor_plate_id)
			{
				return GPlatesAppLogic::create_reconstruction_tree(
						reconstruction_time,
						anchor_plate_id,
						d_cloned_reconstruction_features_collection_refs);
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

			std::vector<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type> d_cloned_reconstruction_features_collection;
			std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> d_cloned_reconstruction_features_collection_refs;
			GPlatesModel::integer_plate_id_type d_default_anchor_plate_id;
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
	ReconstructionGraph graph;
	ReconstructionTreePopulator rtp(time, graph);

	AppLogicUtils::visit_feature_collections(
			reconstruction_features_collection.begin(),
			reconstruction_features_collection.end(),
			rtp);

	// Build the reconstruction tree, using 'root' as the root of the tree.
	ReconstructionTree::non_null_ptr_type tree =
			graph.build_tree(anchor_plate_id, time, reconstruction_features_collection);

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


GPlatesAppLogic::ReconstructionTreeCreator
GPlatesAppLogic::create_cached_reconstruction_tree_creator(
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstruction_features_collection,
		GPlatesModel::integer_plate_id_type default_anchor_plate_id,
		unsigned int reconstruction_tree_cache_size,
		bool clone_reconstruction_features)
{
	const ReconstructionTreeCreatorImpl::non_null_ptr_type impl =
			create_cached_reconstruction_tree_creator_impl(
					reconstruction_features_collection,
					default_anchor_plate_id,
					reconstruction_tree_cache_size,
					clone_reconstruction_features);

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
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstruction_features_collection,
		GPlatesModel::integer_plate_id_type default_anchor_plate_id,
		unsigned int reconstruction_tree_cache_size,
		bool clone_reconstruction_features)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			reconstruction_tree_cache_size > 0,
			GPLATES_ASSERTION_SOURCE);

	return CachedReconstructionTreeCreatorImpl::create(
			reconstruction_features_collection,
			default_anchor_plate_id,
			reconstruction_tree_cache_size,
			clone_reconstruction_features);
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
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstruction_features_collection,
		GPlatesModel::integer_plate_id_type default_anchor_plate_id,
		bool clone_reconstruction_features)
{
	ReconstructionTreeCreatorImpl::non_null_ptr_type impl(
			new UncachedReconstructionTreeCreatorImpl(
					reconstruction_features_collection,
					default_anchor_plate_id,
					clone_reconstruction_features));

	return ReconstructionTreeCreator(impl);
}


GPlatesAppLogic::CachedReconstructionTreeCreatorImpl::CachedReconstructionTreeCreatorImpl(
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstruction_features_collection,
		GPlatesModel::integer_plate_id_type default_anchor_plate_id,
		unsigned int reconstruction_tree_cache_size,
		bool clone_reconstruction_features) :
	d_default_anchor_plate_id(default_anchor_plate_id),
	d_create_reconstruction_tree_function(
			boost::bind(
					&CachedReconstructionTreeCreatorImpl::create_reconstruction_tree_from_reconstruction_feature_collections,
					this,
					_1,
					// Using 'boost::cref()' to ensure vector not copied by boost-bind...
					// We'll initialise vector shortly.
					boost::cref(d_cloned_reconstruction_features_collection_refs))),
	d_cache(d_create_reconstruction_tree_function, reconstruction_tree_cache_size)
{
	BOOST_FOREACH(
			const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection_ref,
			reconstruction_features_collection)
	{
		// If requested then clone the feature collections to ensure that we will always create
		// a reconstruction tree using the state of the feature collections right now
		// (because they could subsequently get modified by some other client).
		if (feature_collection_ref.is_valid())
		{
			const GPlatesModel::FeatureCollectionHandle::non_null_ptr_type feature_collection =
					clone_reconstruction_features
					? clone_feature_collection(feature_collection_ref)
					: GPlatesModel::FeatureCollectionHandle::non_null_ptr_type(feature_collection_ref.handle_ptr());

			d_cloned_reconstruction_features_collection.push_back(feature_collection);
			d_cloned_reconstruction_features_collection_refs.push_back(feature_collection->reference());
		}
	}
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
GPlatesAppLogic::CachedReconstructionTreeCreatorImpl::create_reconstruction_tree_from_reconstruction_feature_collections(
		const cache_key_type &key,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
				reconstruction_features_collection)
{
	//PROFILE_FUNC();

	// Create a reconstruction tree for the specified time/anchor.
	return create_reconstruction_tree(
			key.first.dval()/*reconstruction_time*/,
			key.second/*anchor_plate_id*/,
			reconstruction_features_collection);
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
