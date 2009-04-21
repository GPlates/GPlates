/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2006, 2007, 2008, 2009 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_MODEL_H
#define GPLATES_MODEL_MODEL_H

#include <boost/intrusive_ptr.hpp>
#ifdef HAVE_PYTHON
# include <boost/python.hpp>
#endif

#include <vector>
#include "FeatureStore.h"
#include "Reconstruction.h"
#include "types.h"


namespace GPlatesModel
{
	/**
	 * The interface to the Model tier of GPlates.
	 *
	 * This contains a feature store and (later) Undo/Redo stacks.
	 *
	 * This class is hidden from the other GPlates tiers behind class ModelInterface, in an
	 * instance of the "p-impl" idiom.  This creates a "compiler firewall" (in order to speed
	 * up build times) as well as an architectural separation between the Model tier (embodied
	 * by the Model class) and the other GPlates tiers which use the Model.
	 *
	 * Classes outside the Model tier should pass around ModelInterface instances, never Model
	 * instances.  A ModelInterface instance will contain and hide a Model instance, managing
	 * its memory automatically.
	 *
	 * Header files outside of the Model tier should only #include "model/ModelInterface.h",
	 * never "model/Model.h".  "model/Model.h" should only be #included in ".cc" files when
	 * necessary (ie, whenever you want to access the members of the Model instance through the
	 * ModelInterface, and hence need the class definition of Model).
	 */
	class Model
	{
	public:
		/**
		 * Create a new instance of the Model, which contains an empty feature store.
		 */
		Model();


		/**
		 * Create a new, empty feature collection in the feature store.
		 *
		 * A valid weak reference to the new feature collection will be returned.
		 */
		const FeatureCollectionHandle::weak_ref
		create_feature_collection();


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
		const FeatureHandle::weak_ref
		create_feature(
				const FeatureType &feature_type,
				const FeatureCollectionHandle::weak_ref &target_collection);


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
		const FeatureHandle::weak_ref
		create_feature(
				const FeatureType &feature_type,
				const FeatureId &feature_id,
				const FeatureCollectionHandle::weak_ref &target_collection);


		/**
		 * Create a new feature of feature-type @a feature_type, with revision ID
		 * @a revision_id, within @a target_collection.
		 *
		 * A valid weak reference to the new feature will be returned.  As a result of this
		 * function, the feature collection referenced by @a target_collection will be
		 * modified.
		 *
		 * If the feature collection referenced by @a target_collection was already
		 * deactivated, or the reference @a target collection was already not valid, before
		 * @a target_collection was passed as a parameter, this function will throw an
		 * exception.
		 *
		 * FIXME:  Why is there no feature ID parameter?  Is this overload needed?
		 */
		const FeatureHandle::weak_ref
		create_feature(
				const FeatureType &feature_type,
				const RevisionId &revision_id,
				const FeatureCollectionHandle::weak_ref &target_collection);


		/**
		 * Create a new feature of feature-type @a feature_type, with feature ID
		 * @a feature_id and revision ID @ revision_id, within @a target_collection.
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
		const FeatureHandle::weak_ref
		create_feature(
				const FeatureType &feature_type,
				const FeatureId &feature_id,
				const RevisionId &revision_id,
				const FeatureCollectionHandle::weak_ref &target_collection);


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
		void
		remove_feature_collection(
				const FeatureCollectionHandle::weak_ref &collection);


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
		void
		remove_feature(
				const FeatureHandle::weak_ref &feature,
				const FeatureCollectionHandle::weak_ref &containing_collection);
#endif


		/**
		 * Create and return a reconstruction tree for the reconstruction time @a time,
		 * with root @a root.
		 *
		 * The feature collections in @a reconstruction_features_collection are expected to
		 * contain reconstruction features (ie, total reconstruction sequences and absolute
		 * reference frames).
		 *
		 * Question:  Do any of those other functions actually throw exceptions when
		 * they're passed invalid weak_refs?  They should.
		 */
		const ReconstructionTree::non_null_ptr_type
		create_reconstruction_tree(
				const std::vector<FeatureCollectionHandle::weak_ref> &reconstruction_features_collection,
				const double &time,
				integer_plate_id_type root);


		/**
		 * Create and return a reconstruction for the reconstruction time @a time, with
		 * root @a root.
		 *
		 * The feature collections in @a reconstruction_features_collection are expected to
		 * contain reconstruction features (ie, total reconstruction sequences and absolute
		 * reference frames).
		 *
		 * Question:  Do any of those other functions actually throw exceptions when
		 * they're passed invalid weak_refs?  They should.
		 */
		const Reconstruction::non_null_ptr_type
		create_reconstruction(
				const std::vector<FeatureCollectionHandle::weak_ref> &reconstructable_features_collection,
				const std::vector<FeatureCollectionHandle::weak_ref> &reconstruction_features_collection,
				const double &time,
				integer_plate_id_type root);


		/**
		 * Create and return an empty reconstruction for the reconstruction time @a time,
		 * with root @a root.
		 *
		 * The reconstruction tree contained within the reconstruction will also be empty.
		 *
		 * FIXME:  Remove this function once it is possible to create empty reconstructions
		 * by simply passing empty lists of feature-collections into the prev function.
		 */
		const Reconstruction::non_null_ptr_type
		create_empty_reconstruction(
				const double &time,
				integer_plate_id_type root);


#ifdef HAVE_PYTHON
		// A Python wrapper for create_reconstruction
		boost::python::tuple
		create_reconstruction_py(
				const double &time,
				unsigned long root);
#endif

	private:
		FeatureStore::non_null_ptr_type d_feature_store;
		FeatureIdRegistry d_feature_id_registry;
	};


	void export_Model();
}

#endif  // GPLATES_MODEL_MODEL_H
