/* $Id$ */

/**
 * \file 
 * Contains the definition of the FeatureHandle class.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_FEATUREHANDLE_H
#define GPLATES_MODEL_FEATUREHANDLE_H

#include <ctime>
#if _MSC_VER == 1600 // For Boost 1.44 and Visual Studio 2010.
#	undef UINT8_C
#endif
#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>
#if _MSC_VER == 1600 // For Boost 1.44 and Visual Studio 2010.
#	undef UINT8_C
#	include <cstdint>
#endif

#include "BasicHandle.h"
#include "FeatureId.h"
#include "FeatureRevision.h"
#include "FeatureType.h"
#include "RevisionId.h"

#include "global/PointerTraits.h"
#include "utils/ReferenceCount.h"

namespace GPlatesModel
{
	/**
	 * A feature handle acts as a persistent handle to the revisioned content of a conceptual
	 * feature.
	 *
	 * The feature is the bottom layer/component of the three-tiered conceptual hierarchy of
	 * revisioned objects contained in, and managed by, the feature store:  The feature is an
	 * abstract model of some geological or plate-tectonic object or concept of interest,
	 * consisting of a collection of properties and a feature type.  The feature store contains
	 * a single feature store root, which in turn contains all the currently-loaded feature
	 * collections.  Every currently-loaded feature is contained within a currently-loaded
	 * feature collection.
	 *
	 * The conceptual feature is implemented in two pieces: FeatureHandle and FeatureRevision. 
	 * A FeatureHandle instance contains and manages a FeatureRevision instance, which in turn
	 * contains the revisioned content of the conceptual feature (the mutable properties of the
	 * feature).  A FeatureHandle instance is contained within, and managed by, a
	 * FeatureCollectionRevision instance.
	 *
	 * A feature handle instance is "persistent" in the sense that it will endure, in the same
	 * memory location, for as long as the conceptual feature exists (which will be determined
	 * by the user's choice of when to "flush" deleted features and unloaded feature
	 * collections, after the feature has been deleted or its feature collection has been
	 * unloaded).  The revisioned content of the conceptual feature will be contained within a
	 * succession of feature revisions (with a new revision created as the result of every
	 * modification), but the handle will endure as a persistent means of accessing the current
	 * revision and the content within it.
	 *
	 * The feature handle also contains the properties of a feature which can never change: the
	 * feature type and the feature ID.
	 */
	class FeatureHandle :
			public BasicHandle<FeatureHandle>,
			public GPlatesUtils::ReferenceCount<FeatureHandle>
	{

	public:

		/**
		 * The type of this class.
		 */
		typedef FeatureHandle this_type;

		/**
		 * Typedef for a function that accepts a pointer to a property and returns a boolean.
		 */
		typedef boost::function<bool (const GPlatesGlobal::PointerTraits<const TopLevelProperty>::non_null_ptr_type &)>
			property_predicate_type;

		/**
		 * Creates a new FeatureHandle instance with @a feature_type_ and optional
		 * @a feature_id_ and @a revision_id_.
		 *
		 * This new FeatureHandle instance is not in the model. It is the
		 * responsibility of the caller to add it to a FeatureCollectionHandle if
		 * that is desired.
		 */
		static
		const non_null_ptr_type
		create(
				const FeatureType &feature_type_,
				const FeatureId &feature_id_ = FeatureId(),
				const RevisionId &revision_id_ = RevisionId());

		/**
		 * Creates a new FeatureHandle instance with @a feature_type_ and optional
		 * @a feature_id_ and @a revision_id_.
		 *
		 * This new FeatureHandle instance is added to @a feature_collection and a
		 * weak-ref to the new instance is returned.
		 */
		static
		const weak_ref
		create(
				const WeakReference<FeatureCollectionHandle> &feature_collection,
				const FeatureType &feature_type_,
				const FeatureId &feature_id_ = FeatureId(),
				const RevisionId &revision_id_ = RevisionId());

		/**
		 * Makes a clone of this feature.
		 *
		 * The returned feature has a new feature ID
		 * and revision. The clone shares property objects with this feature, but
		 * that is fine, because property objects in the model are immutable; if a
		 * property were to be changed in this feature, the clone would point to the
		 * old property object, while this feature would point to the new property
		 * object. Hence, there is no need for a "deep clone" method.
		 *
		 * The new feature is not in a feature collection. The caller of this function
		 * is responsible for placing the feature in a feature collection, if that is
		 * desired.
		 */
		const non_null_ptr_type
		clone() const;

		/**
		 * Makes a clone of this feature.
		 *
		 * The returned feature has a new feature ID and revision.
		 * The clone shares property objects with this feature, but
		 * that is fine, because property objects in the model are immutable; if a
		 * property were to be changed in this feature, the clone would point to the
		 * old property object, while this feature would point to the new property
		 * object. Hence, there is no need for a "deep clone" method.
		 *
		 * The new feature is added to @a feature_collection and a weak_ref to the
		 * new feature is returned.
		 */
		const weak_ref
		clone(
				const WeakReference<FeatureCollectionHandle> &feature_collection) const;

		/**
		 * Makes a clone of this feature (but only the property values for which
		 * the given predicate @a clone_properties_predicate returns true).
		 *
		 * The returned feature has a new feature ID and revision.
		 * The clone shares property objects with this feature, but
		 * that is fine, because property objects in the model are immutable; if a
		 * property were to be changed in this feature, the clone would point to the
		 * old property object, while this feature would point to the new property
		 * object. Hence, there is no need for a "deep clone" method.
		 *
		 * The new feature is not in a feature collection. The caller of this function
		 * is responsible for placing the feature in a feature collection, if that is
		 * desired.
		 */
		const non_null_ptr_type
		clone(
				const property_predicate_type &clone_properties_predicate) const;

		/**
		 * Makes a clone of this feature (but only the property values for which
		 * the given predicate @a clone_properties_predicate returns true).
		 *
		 * The returned feature has a new feature ID and revision.
		 * The clone shares property objects with this feature, but
		 * that is fine, because property objects in the model are immutable; if a
		 * property were to be changed in this feature, the clone would point to the
		 * old property object, while this feature would point to the new property
		 * object. Hence, there is no need for a "deep clone" method.
		 *
		 * The new feature is added to @a feature_collection and a weak_ref to the
		 * new feature is returned.
		 */
		const weak_ref
		clone(
				const WeakReference<FeatureCollectionHandle> &feature_collection,
				const property_predicate_type &clone_properties_predicate) const;

		/**
		 * @see BasicHandle<FeatureHandle>::add.
		 *
		 * A new revision ID is generated if this Handle is not already present in the
		 * current changeset.
		 */
		iterator
		add(
				GPlatesGlobal::PointerTraits<TopLevelProperty>::non_null_ptr_type new_child);

		/**
		 * @see BasicHandle<FeatureHandle>::remove.
		 *
		 * A new revision ID is generated if this Handle is not already present in the
		 * current changeset.
		 */
		void
		remove(
				const_iterator iter);

		/**
		 * Changes the child pointed to by iterator @a iter into @a new_child.
		 */
		void
		set(
				iterator iter,
				child_type::non_null_ptr_to_const_type new_child);

		/**
		 * Removes all children properties that have the given @a property_name.
		 */
		void
		remove_properties_by_name(
				const PropertyName &property_name);

		/**
		 * Returns the feature type of this feature.
		 */
		const FeatureType &
		feature_type() const;

		/**
		 * Changes the feature type of this feature to @a feature_type_.
		 */
		void
		set_feature_type(
				const FeatureType &feature_type_);

		/**
		 * Returns the feature ID of this feature.
		 *
		 * No "setter" method is provided because the feature ID of a feature should
		 * never be changed.
		 */
		const FeatureId &
		feature_id() const;

		/**
		 * Returns the revision ID of the current revision of this feature.
		 *
		 * No "setter" method is provided because the revision ID should never be
		 * manually changed.
		 */
		const RevisionId &
		revision_id() const;

		/**
		 * Returns the time of creation of this instance.
		 *
		 * The time returned is the time as reported by the C function @a time().
		 */
		time_t
		creation_time() const;

	private:

		/**
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		FeatureHandle(
				const FeatureType &feature_type_,
				const FeatureId &feature_id_, 
				GPlatesGlobal::PointerTraits<revision_type>::non_null_ptr_type revision_);

		/**
		 * This constructor should not be defined, because we don't want to be able
		 * to copy construct one of these objects.
		 */
		FeatureHandle(
				const this_type &other);

		/**
		 * This should not be defined, because we don't want to be able to copy
		 * one of these objects.
		 */
		this_type &
		operator=(
				const this_type &);

		/**
		 * The type of this feature.
		 */
		FeatureType d_feature_type;

		/**
		 * The unique feature ID of this feature.
		 */
		FeatureId d_feature_id;

		/**
		 * The time of creation of this instance.
		 */
		time_t d_creation_time;
	};

}

// This include is not necessary for this header to function, but it would be
// convenient if client code could include this header and be able to use
// iterator or const_iterator without having to separately include the
// following header. It isn't placed above with the other includes because of
// cyclic dependencies.
#include "RevisionAwareIterator.h"

#endif  // GPLATES_MODEL_FEATUREHANDLE_H
