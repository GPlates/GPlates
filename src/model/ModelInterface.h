/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_MODELINTERFACE_H
#define GPLATES_MODEL_MODELINTERFACE_H

#include <vector>
#include "FeatureCollectionHandle.h"
#include "FeatureHandle.h"
#include "WeakReference.h"
#include "Reconstruction.h"
#include "types.h"


namespace GPlatesModel
{
	class ModelInterface
	{
	public:
		/**
		 * Create a new, empty feature collection in the feature store.
		 *
		 * A valid weak reference to the new feature collection will be returned.
		 */
		virtual
		const FeatureCollectionHandle::weak_ref
		create_feature_collection() = 0;

		/**
		 * Create a new feature of feature-type @a feature_type within the feature
		 * collection referenced by @a target_collection.
		 *
		 * The feature ID of this feature will be auto-generated.
		 *
		 * A valid weak reference to the new feature will be returned.  As a result of this
		 * function, the feature collection referenced by @a target_collection will be
		 * modified.
		 *
		 * If the feature collection referenced by @a target_collection was already
		 * deactivated, or the reference @a target collection was already not valid, before
		 * @a target_collection was passed as a parameter, this function will throw an
		 * exception.
		 */
		virtual
		const FeatureHandle::weak_ref
		create_feature(
				const FeatureType &feature_type,
				const FeatureCollectionHandle::weak_ref &target_collection) = 0;

		/**
		 * Create a new feature of feature-type @a feature_type, with feature ID
		 * @a feature_id, within @a target_collection.
		 *
		 * A valid weak reference to the new feature will be returned.  As a result of this
		 * function, the feature collection referenced by @a target_collection will be
		 * modified.
		 *
		 * If the feature collection referenced by @a target_collection was already
		 * deactivated, or the reference @a target collection was already not valid, before
		 * @a target_collection was passed as a parameter, this function will throw an
		 * exception.
		 */
		virtual
		const FeatureHandle::weak_ref
		create_feature(
				const FeatureType &feature_type,
				const FeatureId &feature_id,
				const FeatureCollectionHandle::weak_ref &target_collection) = 0;

#if 0
		/**
		 * Remove @a collection from the feature store.
		 *
		 * As a result of this operation, the feature collection referenced by
		 * @a collection will become deactivated.
		 *
		 * If the feature collection referenced by @a collection was already deactivated,
		 * or the reference @a collection was already not valid, before @a collection was
		 * passed as a parameter, this function will throw an exception.
		 */
		virtual
		void
		remove_feature_collection(
				const FeatureCollectionHandle::weak_ref &collection) = 0;

		/**
		 * Remove @a feature from @a containing_collection.
		 *
		 * If @a feature is not actually an element of @a containing_collection, this
		 * operation will be a no-op.
		 *
		 * If @a feature @em is an element of @a containing_collection, @a feature will
		 * become deactivated, and @a containing_collection will be modified.
		 *
		 * If the feature referenced by @a feature was already deactivated, or the
		 * reference @a feature was already not valid, before @a feature was passed as a
		 * parameter, this function will throw an exception.
		 *
		 * If the feature collection referenced by @a containing_collection was already
		 * deactivated, or the reference @a containing collection was already not valid,
		 * before @a containing_collection was passed as a parameter, this function will
		 * throw an exception.
		 */
		virtual
		void
		remove_feature(
				const FeatureHandle::weak_ref &feature,
				const FeatureCollectionHandle::weak_ref &containing_collection) = 0;
#endif

		/**
		 * FIXME:  Where is the comment for this function?
		 *
		 * And while we're at it, do any of those other functions actually throw exceptions
		 * when they're passed invalid weak_refs?  They should.
		 */
		virtual
		const Reconstruction::non_null_ptr_type
		create_reconstruction(
				const std::vector<FeatureCollectionHandle::weak_ref> &reconstructable_features_collection,
				const std::vector<FeatureCollectionHandle::weak_ref> &reconstruction_features_collection,
				const double &time,
				integer_plate_id_type root) = 0;

		/**
		 * FIXME:  Where is the comment for this function?
		 *
		 * And while we're at it, do any of those other functions actually throw exceptions
		 * when they're passed invalid weak_refs?  They should.
		 *
		 * FIXME:  Remove this function once it is possible to create empty reconstructions
		 * by simply passing empty lists of feature-collections into the prev function.
		 */
		virtual
		const Reconstruction::non_null_ptr_type
		create_empty_reconstruction(
				const double &time,
				integer_plate_id_type root) = 0;

		virtual
		~ModelInterface()
		{  }
	};
}

#endif  // GPLATES_MODEL_MODELINTERFACE_H
