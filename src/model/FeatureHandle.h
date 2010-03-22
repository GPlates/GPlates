/* $Id$ */

/**
 * \file 
 * Contains the definition of the class FeatureHandle.
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

#include "FeatureRevision.h"
#include "FeatureHandleData.h"
#include "FeatureId.h"
#include "FeatureType.h"
#include "RevisionAwareIterator.h"
#include "WeakObserverPublisher.h"
#include "WeakReference.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"

namespace GPlatesModel
{
	class DummyTransactionHandle;
	class FeatureCollectionHandle;

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
			public WeakObserverPublisher<FeatureHandle>,
			public GPlatesUtils::ReferenceCount<FeatureHandle>
	{

	public:

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<FeatureHandle>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<FeatureHandle> non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<const FeatureHandle>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const FeatureHandle> non_null_ptr_to_const_type;

		/**
		 * The type of this class.
		 *
		 * This definition is used for template magic.
		 */
		typedef FeatureHandle this_type;

		/**
		 * The type which contains the revisioning component of a feature.
		 *
		 * This typedef is used by the RevisionAwareIterator.
		 */
		typedef FeatureRevision revision_component_type;

		/**
		 * The type used for const-iterating over the collection of top-level properties.
		 */
		typedef RevisionAwareIterator<const FeatureHandle> children_const_iterator;

		/**
		 * The type used for (non-const) iterating over the collection of top-level
		 * properties.
		 */
		typedef RevisionAwareIterator<FeatureHandle> children_iterator;

		/**
		 * The type used for a weak-ref to a const feature handle.
		 */
		typedef WeakReference<const FeatureHandle> const_weak_ref;

		/**
		 * The type used for a weak-ref to a (non-const) feature handle.
		 */
		typedef WeakReference<FeatureHandle> weak_ref;

		/**
		 * Translate the non-const iterator @a iter to the equivalent const-iterator.
		 */
		static
		const children_const_iterator
		get_const_iterator(
				const children_iterator &iter);

		/**
		 * Translate the non-const weak-ref @a ref to the equivalent const-weak-ref.
		 */
		static
		const const_weak_ref
		get_const_weak_ref(
				const weak_ref &ref);

		/**
		 * Destructor.
		 */
		~FeatureHandle();

		/**
		 * Create a new FeatureHandle instance with feature type @a feature_type_ and
		 * feature ID @a feature_id_.
		 */
		static
		const non_null_ptr_type
		create(
				const FeatureType &feature_type_,
				const FeatureId &feature_id_);

		/**
		 * Create a new FeatureHandle instance with feature type @a feature_type_, feature
		 * ID @a feature_id_ and revision ID @a revision_id_.
		 */
		static
		const non_null_ptr_type
		create(
				const FeatureType &feature_type_,
				const FeatureId &feature_id_,
				const RevisionId &revision_id_);

		/**
		 * Create a duplicate of this FeatureHandle instance.
		 *
		 * Note that this will perform a "shallow copy".
		 */
		const non_null_ptr_type
		clone() const;

		/**
		 * Return a const-weak-ref to this FeatureHandle instance.
		 */
		const const_weak_ref
		reference() const;

		/**
		 * Return a (non-const) weak-ref to this FeatureHandle instance.
		 */
		const weak_ref
		reference();

		/**
		 * Returns a reference to additional properties of FeatureHandle.
		 */
		HandleData<FeatureHandle> &
		handle_data()
		{
			return d_handle_data;
		}

		/**
		 * Returns a const reference to additional properties of FeatureHandle.
		 */
		const HandleData<FeatureHandle> &
		handle_data() const
		{
			return d_handle_data;
		}

		/**
		 * Return the revision ID of the current revision.
		 *
		 * Note that no "setter" is provided:  The revision ID of a revision should never
		 * be changed.
		 */
		const RevisionId &
		revision_id() const;

		/**
		 * Return the "begin" const-iterator to iterate over the collection of top-level
		 * properties.
		 */
		const children_const_iterator
		children_begin() const;

		/**
		 * Return the "begin" iterator to iterate over the collection of top-level
		 * properties.
		 */
		const children_iterator
		children_begin();

		/**
		 * Return the "end" const-iterator used during iteration over the collection of
		 * top-level properties.
		 */
		const children_const_iterator
		children_end() const;

		/**
		 * Return the "end" iterator used during iteration over the collection of top-level
		 * properties.
		 */
		const children_iterator
		children_end();

		/**
		 * Append @a new_top_level_property to the collection.
		 *
		 * An iterator is returned which points to the new element in the collection.
		 *
		 * After the TopLevelProperty has been appended, the "end" iterator will have
		 * advanced -- the length of the sequence will have increased by 1, so what was the
		 * iterator to the last element of the sequence (the "back" of the container), will
		 * now be the iterator to the second-last element of the sequence; what was the
		 * "end" iterator will now be the iterator to the last element of the sequence.
		 */
		const children_iterator
		append_child(
				TopLevelProperty::non_null_ptr_type new_top_level_property,
				DummyTransactionHandle &transaction);

		/**
		 * Remove the top-level property indicated by @a iter in the collection.
		 *
		 * The results of this operation are only defined if @a iter is before @a end.
		 *
		 * The "end" iterator will not be changed by this operation -- the length of the
		 * sequence will not change, only a top-level-property-slot will become NULL.
		 */
		void
		remove_child(
				children_const_iterator iter,
				DummyTransactionHandle &transaction);

		/**
		 * Remove the top-level property indicated by @a iter in the collection.
		 *
		 * The results of this operation are only defined if @a iter is before @a end.
		 *
		 * The "end" iterator will not be changed by this operation -- the length of the
		 * sequence will not change, only a top-level-property-slot will become NULL.
		 */
		void
		remove_child(
				children_iterator iter,
				DummyTransactionHandle &transaction);

		/**
		 * Access the current revision of this feature.
		 *
		 * Client code should not need to access the revision directly!
		 *
		 * This is the overloading of this function for const FeatureHandle instances; it
		 * returns a pointer to a const FeatureRevision instance.
		 */
		const FeatureRevision::non_null_ptr_to_const_type
		current_revision() const;

		/**
		 * Access the current revision of this feature.
		 *
		 * Client code should not need to access the revision directly!
		 *
		 * This is the overloading of this function for non-const FeatureHandle instances;
		 * it returns a C++ reference to a pointer to a non-const FeatureRevision instance.
		 *
		 * Note that, because the copy-assignment operator of FeatureRevision is private,
		 * the FeatureRevision referenced by the return-value of this function cannot be
		 * assigned-to, which means that this function does not provide a means to directly
		 * switch the FeatureRevision within this FeatureHandle instance.  (This
		 * restriction is intentional.)
		 *
		 * To switch the FeatureRevision within this FeatureHandle instance, use the
		 * function @a set_current_revision below.
		 */
		const FeatureRevision::non_null_ptr_type
		current_revision();

		/**
		 * Set the current revision of this feature to @a rev.
		 *
		 * Client code should not use this function!
		 */
		void
		set_current_revision(
				FeatureRevision::non_null_ptr_type rev);

		/**
		 * Set the pointer to the FeatureCollectionHandle which contains this feature.
		 *
		 * Client code should not use this function!
		 *
		 * This function should only be invoked by a FeatureCollectionRevision instance
		 * when it has appended or removed a feature.  This is part of the mechanism which
		 * tracks whether a feature collection contains unsaved changes, and (later) part
		 * of the Bubble-Up mechanism.
		 */
		void
		set_parent_ptr(
				FeatureCollectionHandle *new_ptr);

	private:

		/**
		 * The current revision of this feature.
		 */
		FeatureRevision::non_null_ptr_type d_current_revision;

		/**
		 * Stores additional properties of FeatureHandle.
		 */
		HandleData<FeatureHandle> d_handle_data;

		/**
		 * The feature collection which contains this feature.
		 *
		 * Note that this should be held via a (regular, raw) pointer rather than a
		 * ref-counting pointer (or any other type of smart pointer) because:
		 *  -# The feature collection instance conceptually manages the instance of this
		 * class, not the other way around.
		 *  -# A feature collection instance will outlive the features it contains; thus,
		 * it doesn't make sense for a feature collection to have its memory managed by its
		 * contained features.
		 *  -# Class FeatureCollectionHandle contains a ref-counting pointer to class
		 * FeatureCollectionRevision, which contains a vector which contains ref-counting
		 * pointers to class FeatureHandle, and we don't want to set up a ref-counting loop
		 * (which would lead to memory leaks).
		 */
		FeatureCollectionHandle *d_feature_collection_handle_ptr;

		/**
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		FeatureHandle(
				const FeatureType &feature_type_,
				const FeatureId &feature_id_);

		/**
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		FeatureHandle(
				const FeatureType &feature_type_,
				const FeatureId &feature_id_,
				const RevisionId &revision_id_);

		/**
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 *
		 * This ctor should only be invoked by the 'clone' member function, which will
		 * create a duplicate instance and return a new non_null_intrusive_ptr reference to
		 * the new duplicate.  Since initially the only reference to the new duplicate will
		 * be the one returned by the 'clone' function, *before* the new intrusive-pointer
		 * is created, the ref-count of the new FeatureHandle instance should be zero.
		 *
		 * Note that this ctor should act exactly the same as the default (auto-generated)
		 * copy-ctor, except that it should initialise the ref-count to zero, and it will
		 * generate a new unique feature ID.
		 */
		FeatureHandle(
				const FeatureHandle &other);

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive-pointer to another.
		FeatureHandle &
		operator=(
				const FeatureHandle &);

	};

}

#endif  // GPLATES_MODEL_FEATUREHANDLE_H
