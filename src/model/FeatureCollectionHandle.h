/* $Id$ */

/**
 * \file 
 * Contains the definition of the class FeatureCollectionHandle.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2009, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_FEATURECOLLECTIONHANDLE_H
#define GPLATES_MODEL_FEATURECOLLECTIONHANDLE_H

#include <map>
#include <boost/any.hpp>

#include "FeatureCollectionRevision.h"
#include "RevisionAwareIterator.h"
#include "WeakObserverPublisher.h"
#include "WeakReference.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"
#include "utils/StringSet.h"

namespace GPlatesModel
{
	class DummyTransactionHandle;
	class FeatureStoreRootHandle;

	/**
	 * A feature collection handle acts as a persistent handle to the revisioned content of a
	 * conceptual feature collection.
	 *
	 * The feature collection is the middle layer/component of the three-tiered conceptual
	 * hierarchy of revisioned objects contained in, and managed by, the feature store:  The
	 * feature collection aggregates a set of features into a collection which may be loaded,
	 * saved or unloaded in a single operation.  The feature store contains a single feature
	 * store root, which in turn contains all the currently-loaded feature collections.  Every
	 * currently-loaded feature is contained within a currently-loaded feature collection.
	 *
	 * The conceptual feature collection is implemented in two pieces: FeatureCollectionHandle
	 * and FeatureCollectionRevision.  A FeatureCollectionHandle instance contains and manages
	 * a FeatureCollectionRevision instance, which in turn contains the revisioned content of
	 * the conceptual feature collection.  A FeatureCollectionHandle instance is contained
	 * within, and managed by, a FeatureStoreRootRevision instance.
	 *
	 * A feature collection handle instance is "persistent" in the sense that it will endure,
	 * in the same memory location, for as long as the conceptual feature collection exists
	 * (which will be determined by the user's choice of when to "flush" deleted features and
	 * unloaded feature collections, after the feature collection has been unloaded).  The
	 * revisioned content of the conceptual feature collection will be contained within a
	 * succession of feature collection revisions (with a new revision created as the result of
	 * every modification), but the handle will endure as a persistent means of accessing the
	 * current revision and the content within it.
	 *
	 * The name "feature collection" derives from the GML term for a collection of GML features
	 * -- one GML feature collection corresponds roughly to one data file, although it may be
	 * the transient result of a database query, for instance, rather than necessarily a file
	 * saved on disk.
	 */
	class FeatureCollectionHandle :
			public WeakObserverPublisher<FeatureCollectionHandle>,
			public GPlatesUtils::ReferenceCount<FeatureCollectionHandle>
	{

	public:

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<FeatureCollectionHandle>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<FeatureCollectionHandle> non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<const FeatureCollectionHandle>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const FeatureCollectionHandle> non_null_ptr_to_const_type;

		/**
		 * The type of this class.
		 *
		 * This definition is used for template magic.
		 */
		typedef FeatureCollectionHandle this_type;

		/**
		 * The type which contains the revisioning component of a feature collection.
		 *
		 * This typedef is used by the RevisionAwareIterator.
		 */
		typedef FeatureCollectionRevision revision_component_type;

		/**
		 * The type used for const-iterating over the collection of feature handles.
		 */
		typedef RevisionAwareIterator<const FeatureCollectionHandle> children_const_iterator;

		/**
		 * The type used for (non-const) iterating over the collection of feature handles.
		 */
		typedef RevisionAwareIterator<FeatureCollectionHandle> children_iterator;

		/**
		 * The type used for a weak-ref to a const collection of feature handles.
		 */
		typedef WeakReference<const FeatureCollectionHandle> const_weak_ref;

		/**
		 * The type used for a weak-ref to a (non-const) collection of feature handles.
		 */
		typedef WeakReference<FeatureCollectionHandle> weak_ref;

		/**
		 * The type of the miscellaneous tags collection.
		 */
		typedef std::map<GPlatesUtils::StringSet::SharedIterator, boost::any> tags_collection_type;

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
		 * Create a new FeatureCollectionHandle instance.
		 */
		static
		const non_null_ptr_type
		create();

		/**
		 * Destructor.
		 */
		~FeatureCollectionHandle();

		/**
		 * Create a duplicate of this FeatureCollectionHandle instance.
		 *
		 * Note that this will perform a "shallow copy".
		 */
		const non_null_ptr_type
		clone() const;

		/**
		 * Return a const-weak-ref to this FeatureCollectionHandle instance.
		 */
		const const_weak_ref
		reference() const;

		/**
		 * Unload this feature collection.
		 *
		 * Neither this feature collection, nor the features it contains, will be
		 * accessible after this.  All weak-refs and iterators which reference either this
		 * feature collection, or the features it contains, will be invalid.
		 */
		void
		unload();

		/**
		 * Return a (non-const) weak-ref to this FeatureCollectionHandle instance.
		 */
		const weak_ref
		reference();

		/**
		 * Return the "begin" const-iterator to iterate over the collection of features.
		 */
		const children_const_iterator
		children_begin() const;

		/**
		 * Return the "begin" iterator to iterate over the collection of features.
		 */
		const children_iterator
		children_begin();

		/**
		 * Return the "end" const-iterator used during iteration over the collection of
		 * features.
		 */
		const children_const_iterator
		children_end() const;

		/**
		 * Return the "end" iterator used during iteration over the collection of features.
		 */
		const children_iterator
		children_end();

		/**
		 * Append @a new_feature to the feature collection.
		 *
		 * An iterator is returned which points to the new element in the collection.
		 *
		 * After the FeatureHandle has been appended, the "end" iterator will have advanced
		 * -- the length of the sequence will have increased by 1, so what was the iterator
		 * to the last element of the sequence (the "back" of the container), will now be
		 * the iterator to the second-last element of the sequence; what was the "end"
		 * iterator will now be the iterator to the last element of the sequence.
		 */
		const children_iterator
		append_child(
				FeatureHandle::non_null_ptr_type new_feature,
				DummyTransactionHandle &transaction);

		/**
		 * Remove the feature indicated by @a iter in the feature collection.
		 *
		 * The results of this operation are only defined if @a iter is before @a end.
		 *
		 * The "end" iterator will not be changed by this operation -- the length of the
		 * sequence will not change, only a feature-slot will become NULL.
		 */
		void
		remove_child(
				children_const_iterator iter,
				DummyTransactionHandle &transaction);

		/**
		 * Remove the feature indicated by @a iter in the feature collection.
		 *
		 * The results of this operation are only defined if @a iter is before @a end.
		 *
		 * The "end" iterator will not be changed by this operation -- the length of the
		 * sequence will not change, only a feature-slot will become NULL.
		 */
		void
		remove_child(
				children_iterator iter,
				DummyTransactionHandle &transaction);

		/**
		 * Return whether this feature collection contains unsaved changes.
		 *
		 * Note that this member function should be replaced when the revision mechanism is
		 * complete.
		 */
		bool
		contains_unsaved_changes() const;

		/**
		 * Set whether this feature collection contains unsaved changes.
		 *
		 * It needs to be a const member function because it will be necessary to set the
		 * status (to @a false) after writing the contents of the feature collection to
		 * disk, but since the operation is not logically changing the contents of the
		 * feature collection, the reference to the feature collection should be a
		 * reference to a const feature collection.  Since the member is mutable, this is
		 * able to be a const member function.
		 *
		 * Note that this member function should be replaced when the revision mechanism is
		 * complete.
		 */
		void
		set_contains_unsaved_changes(
				bool new_status) const;

		/**
		 * Access the current revision of this feature collection.
		 *
		 * Client code should not need to access the revision directly!
		 *
		 * This is the overloading of this function for const FeatureCollectionHandle
		 * instances; it returns a pointer to a const FeatureCollectionRevision instance.
		 */
		const FeatureCollectionRevision::non_null_ptr_to_const_type
		current_revision() const;

		/**
		 * Access the current revision of this feature collection.
		 *
		 * Client code should not need to access the revision directly!
		 *
		 * This is the overloading of this function for non-const FeatureCollectionHandle
		 * instances; it returns a C++ reference to a pointer to a non-const
		 * FeatureCollectionRevision instance.
		 *
		 * Note that, because the copy-assignment operator of FeatureCollectionRevision is
		 * private, the FeatureCollectionRevision referenced by the return-value of this
		 * function cannot be assigned-to, which means that this function does not provide
		 * a means to directly switch the FeatureCollectionRevision within this
		 * FeatureCollectionHandle instance.  (This restriction is intentional.)
		 *
		 * To switch the FeatureCollectionRevision within this FeatureCollectionHandle
		 * instance, use the function @a set_current_revision below.
		 */
		const FeatureCollectionRevision::non_null_ptr_type
		current_revision();

		/**
		 * Set the current revision of this feature collection to @a rev.
		 *
		 * Client code should not need to access the revision directly!
		 */
		void
		set_current_revision(
				FeatureCollectionRevision::non_null_ptr_type rev);

		/**
		 * Access the handle of the container which contains this FeatureCollectionHandle.
		 *
		 * Client code should not use this function!
		 *
		 * Note that the return value may be a NULL pointer, which signifies that this
		 * feature collection is not contained within a container.
		 */
		FeatureStoreRootHandle *
		parent_ptr() const;

		/**
		 * Access the index of this feature collection within its container.
		 *
		 * Client code should not use this function!
		 *
		 * Note that the return value may be the invalid index INVALID_INDEX, which
		 * signifies that this feature collection is not contained within a container.
		 */
		container_size_type
		index_in_container() const;

		/**
		 * Set the handle of the container which contains this FeatureCollectionHandle, and
		 * the index of this feature collection within its container.
		 *
		 * Client code should not use this function!
		 *
		 * Note that @a new_handle may be a NULL pointer, and @a new_index may have the
		 * value @a INVALID_INDEX ... but only if this FeatureCollectionHandle instance
		 * will not be contained within a FeatureStoreRootHandle.
		 */
		void
		set_parent_ptr(
				FeatureStoreRootHandle *new_handle,
				container_size_type new_index);

		/**
		 * Get the collection of miscellaneous data associated with this feature collection.
		 */
		tags_collection_type &
		tags();

		/**
		 * Get a const reference to the collection of miscellaneous data associated with this feature collection
		 */
		const tags_collection_type &
		tags() const;

	private:

		/**
		 * The current revision of this feature collection.
		 */
		FeatureCollectionRevision::non_null_ptr_type d_current_revision;

		/**
		 * The FeatureStoreRootHandle whose FeatureStoreRootRevision contains this
		 * FeatureCollectionHandle instance.
		 *
		 * Note that this should be held via a (regular, raw) pointer rather than a
		 * ref-counting pointer (or any other type of smart pointer) because:
		 *  -# The FeatureStoreRootHandle instance conceptually manages the instance of
		 * this class, not the other way around.
		 *  -# A FeatureStoreRootHandle instance will outlive the FeatureCollectionHandle
		 * instances it contains; thus, it doesn't make sense for a FeatureStoreRootHandle
		 * to have its memory managed by its contained FeatureCollectionHandle.
		 *  -# Each FeatureStoreRootHandle will contain a ref-counting pointer to class
		 * FeatureStoreRootRevision, which will contain a ref-counting pointer to class
		 * FeatureCollectionHandle, and we don't want to set up a ref-counting loop (which
		 * would lead to memory leaks).
		 *
		 * This pointer may be NULL.  It will be NULL when this FeatureCollectionHandle is
		 * not contained.
		 */
		FeatureStoreRootHandle *d_container_handle;

		/**
		 * The index of this feature collection within its container.
		 *
		 * When this feature collection is not contained within a container (indicated by
		 * @a d_container_handle being a NULL pointer), this index will be reset to the
		 * value INVALID_INDEX.
		 */
		container_size_type d_index_in_container;

		/**
		 * Whether this feature collection contains unsaved changes.
		 *
		 * This member should be replaced when the revision mechanism is complete.
		 */
		mutable bool d_contains_unsaved_changes;

		/**
		 * A collection of miscellaneous data related to this feature collection,
		 * accessed by Unicode string keys.
		 */
		tags_collection_type d_tags;

		/**
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		FeatureCollectionHandle();

		/**
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 *
		 * This ctor should only be invoked by the 'clone' member function, which will
		 * create a duplicate instance and return a new non_null_intrusive_ptr reference to
		 * the new duplicate.  Since initially the only reference to the new duplicate will
		 * be the one returned by the 'clone' function, *before* the new intrusive-pointer
		 * is created, the ref-count of the new FeatureCollectionHandle instance should be
		 * zero.
		 *
		 * Note that this ctor should act exactly the same as the default (auto-generated)
		 * copy-ctor, except that it should initialise the ref-count to zero.
		 */
		FeatureCollectionHandle(
				const FeatureCollectionHandle &other);

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		FeatureCollectionHandle &
		operator=(
				const FeatureCollectionHandle &);

	};

	/**
	 * Return true if the feature  collection contains the given feature
	 * Otherwise, return false.
	 */
	bool
	feature_collection_contains_feature(
			GPlatesModel::FeatureCollectionHandle::weak_ref collection_ref,
			GPlatesModel::FeatureHandle::weak_ref feature_ref);

}

#endif  // GPLATES_MODEL_FEATURECOLLECTIONHANDLE_H
