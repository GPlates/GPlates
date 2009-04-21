/* $Id$ */

/**
 * \file 
 * Contains the definition of the class FeatureCollectionHandle.
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

#ifndef GPLATES_MODEL_FEATURECOLLECTIONHANDLE_H
#define GPLATES_MODEL_FEATURECOLLECTIONHANDLE_H

#include "FeatureCollectionRevision.h"
#include "RevisionAwareIterator.h"
#include "WeakReference.h"
#include "utils/non_null_intrusive_ptr.h"
#include "utils/NullIntrusivePointerHandler.h"
#include "utils/ReferenceCount.h"


namespace GPlatesModel
{
	class DummyTransactionHandle;

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
			public GPlatesUtils::ReferenceCount<FeatureCollectionHandle>
	{
	public:
		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<FeatureCollectionHandle,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<FeatureCollectionHandle,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<const FeatureCollectionHandle,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const FeatureCollectionHandle,
				GPlatesUtils::NullIntrusivePointerHandler>
				non_null_ptr_to_const_type;

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
		typedef RevisionAwareIterator<const FeatureCollectionHandle,
				const revision_component_type::feature_collection_type,
				boost::intrusive_ptr<const FeatureHandle> >
				features_const_iterator;

		/**
		 * The type used for (non-const) iterating over the collection of feature handles.
		 */
		typedef RevisionAwareIterator<FeatureCollectionHandle,
				revision_component_type::feature_collection_type,
				boost::intrusive_ptr<FeatureHandle> >
				features_iterator;

 		/**
		 * The base type of all const weak observers of instances of this class.
		 */
		typedef WeakObserver<const FeatureCollectionHandle> const_weak_observer_type;

 		/**
		 * The base type of all (non-const) weak observers of instances of this class.
		 */
		typedef WeakObserver<FeatureCollectionHandle> weak_observer_type;

		/**
		 * The type used for a weak-ref to a const collection of feature handles.
		 */
		typedef WeakReference<const FeatureCollectionHandle> const_weak_ref;

		/**
		 * The type used for a weak-ref to a (non-const) collection of feature handles.
		 */
		typedef WeakReference<FeatureCollectionHandle> weak_ref;

		/**
		 * Translate the non-const iterator @a iter to the equivalent const-iterator.
		 */
		static
		const features_const_iterator
		get_const_iterator(
				const features_iterator &iter)
		{
			if (iter.collection_handle_ptr() == NULL) {
				return features_const_iterator();
			}
			return features_const_iterator::create_index(
					*(iter.collection_handle_ptr()), iter.index());
		}

		/**
		 * Translate the non-const weak-ref @a ref to the equivalent const-weak-ref.
		 */
		static
		const const_weak_ref
		get_const_weak_ref(
				const weak_ref &ref)
		{
			if (ref.handle_ptr() == NULL) {
				return const_weak_ref();
			}
			return const_weak_ref(*(ref.handle_ptr()));
		}

		/**
		 * Create a new FeatureCollectionHandle instance.
		 */
		static
		const non_null_ptr_type
		create()
		{
			non_null_ptr_type ptr(new FeatureCollectionHandle(),
					GPlatesUtils::NullIntrusivePointerHandler());
			return ptr;
		}

		/**
		 * Destructor.
		 *
		 * Unsubscribes all weak observers.
		 */
		~FeatureCollectionHandle();

		/**
		 * Create a duplicate of this FeatureCollectionHandle instance.
		 *
		 * Note that this will perform a "shallow copy".
		 */
		const non_null_ptr_type
		clone() const
		{
			non_null_ptr_type dup(new FeatureCollectionHandle(*this),
					GPlatesUtils::NullIntrusivePointerHandler());
			return dup;
		}

		/**
		 * Return a const-weak-ref to this FeatureCollectionHandle instance.
		 */
		const const_weak_ref
		reference() const
		{
			const_weak_ref ref(*this);
			return ref;
		}

		/**
		 * Return a (non-const) weak-ref to this FeatureCollectionHandle instance.
		 */
		const weak_ref
		reference()
		{
			weak_ref ref(*this);
			return ref;
		}

		/**
		 * Return the "begin" const-iterator to iterate over the collection of features.
		 */
		const features_const_iterator
		features_begin() const
		{
			return features_const_iterator::create_begin(*this);
		}

		/**
		 * Return the "begin" iterator to iterate over the collection of features.
		 */
		const features_iterator
		features_begin()
		{
			return features_iterator::create_begin(*this);
		}

		/**
		 * Return the "end" const-iterator used during iteration over the collection of
		 * features.
		 */
		const features_const_iterator
		features_end() const
		{
			return features_const_iterator::create_end(*this);
		}

		/**
		 * Return the "end" iterator used during iteration over the collection of features.
		 */
		const features_iterator
		features_end()
		{
			return features_iterator::create_end(*this);
		}

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
		const features_iterator
		append_feature(
				FeatureHandle::non_null_ptr_type new_feature,
				DummyTransactionHandle &transaction)
		{
			FeatureCollectionRevision::feature_collection_type::size_type new_index =
					current_revision()->append_feature(new_feature, transaction);
			return features_iterator::create_index(*this, new_index);
		}

		/**
		 * Remove the feature indicated by @a iter in the feature collection.
		 *
		 * The results of this operation are only defined if @a iter is before @a end.
		 *
		 * The "end" iterator will not be changed by this operation -- the length of the
		 * sequence will not change, only a feature-slot will become NULL.
		 */
		void
		remove_feature(
				features_const_iterator iter,
				DummyTransactionHandle &transaction)
		{
			current_revision()->remove_feature(iter.index(), transaction);
		}

		/**
		 * Remove the feature indicated by @a iter in the feature collection.
		 *
		 * The results of this operation are only defined if @a iter is before @a end.
		 *
		 * The "end" iterator will not be changed by this operation -- the length of the
		 * sequence will not change, only a feature-slot will become NULL.
		 */
		void
		remove_feature(
				features_iterator iter,
				DummyTransactionHandle &transaction)
		{
			current_revision()->remove_feature(iter.index(), transaction);
		}

		/**
		 * Return whether this feature collection contains unsaved changes.
		 *
		 * Note that this member function should be replaced when the revision mechanism is
		 * complete.
		 */
		bool
		contains_unsaved_changes() const
		{
			return d_contains_unsaved_changes;
		}

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
				bool new_status) const
		{
			d_contains_unsaved_changes = new_status;
		}

		/**
		 * Access the current revision of this feature collection.
		 *
		 * Client code should not need to access the revision directly!
		 *
		 * This is the overloading of this function for const FeatureCollectionHandle
		 * instances; it returns a pointer to a const FeatureCollectionRevision instance.
		 */
		const FeatureCollectionRevision::non_null_ptr_to_const_type
		current_revision() const
		{
			return d_current_revision;
		}

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
		current_revision()
		{
			return d_current_revision;
		}

		/**
		 * Set the current revision of this feature collection to @a rev.
		 *
		 * Client code should not need to access the revision directly!
		 */
		void
		set_current_revision(
				FeatureCollectionRevision::non_null_ptr_type rev)
		{
			d_current_revision = rev;
		}

 		/**
		 * Access the first const weak observer of this instance.
		 *
		 * Client code should not use this function!
		 *
		 * This function is used by WeakObserver.
		 */
		const_weak_observer_type *&
		first_const_weak_observer() const
		{
			return d_first_const_weak_observer;
		}

 		/**
		 * Access the first weak observer of this instance.
		 *
		 * Client code should not use this function!
		 *
		 * This function is used by WeakObserver.
		 */
		weak_observer_type *&
		first_weak_observer()
		{
			return d_first_weak_observer;
		}

		/**
		 * Access the last const weak observer of this instance.
		 *
		 * Client code should not use this function!
		 *
		 * This function is used by WeakObserver.
		 */
		const_weak_observer_type *&
		last_const_weak_observer() const
		{
			return d_last_const_weak_observer;
		}

		/**
		 * Access the last weak observer of this instance.
		 *
		 * Client code should not use this function!
		 *
		 * This function is used by WeakObserver.
		 */
		weak_observer_type *&
		last_weak_observer()
		{
			return d_last_weak_observer;
		}

	private:
		/**
		 * The current revision of this feature collection.
		 */
		FeatureCollectionRevision::non_null_ptr_type d_current_revision;

 		/**
		 * The first const weak observer of this instance.
		 */
		mutable const_weak_observer_type *d_first_const_weak_observer;

 		/**
		 * The first weak observer of this instance.
		 */
		mutable weak_observer_type *d_first_weak_observer;

		/**
		 * The last const weak observer of this instance.
		 */
		mutable const_weak_observer_type *d_last_const_weak_observer;

		/**
		 * The last weak observer of this instance.
		 */
		mutable weak_observer_type *d_last_weak_observer;

		/**
		 * Whether this feature collection contains unsaved changes.
		 *
		 * This member should be replaced when the revision mechanism is complete.
		 */
		mutable bool d_contains_unsaved_changes;

		/**
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		FeatureCollectionHandle():
			d_current_revision(FeatureCollectionRevision::create()),
			d_first_const_weak_observer(NULL),
			d_first_weak_observer(NULL),
			d_last_const_weak_observer(NULL),
			d_last_weak_observer(NULL),
			d_contains_unsaved_changes(true)  // FIXME:  Is this appropriate?
		{  }

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
				const FeatureCollectionHandle &other) :
			GPlatesUtils::ReferenceCount<FeatureCollectionHandle>(),
			d_current_revision(other.d_current_revision),
			d_first_const_weak_observer(NULL),
			d_first_weak_observer(NULL),
			d_last_const_weak_observer(NULL),
			d_last_weak_observer(NULL),
			d_contains_unsaved_changes(true)  // FIXME:  Is this appropriate?
		{  }

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		FeatureCollectionHandle &
		operator=(
				const FeatureCollectionHandle &);
	};

	/**
	 * Get the first weak observer of the publisher pointed-to by @a publisher_ptr.
	 *
	 * It is assumed that @a publisher_ptr is a non-NULL pointer which is valid to dereference.
	 *
	 * This function is used by the WeakObserver template class when subscribing and
	 * unsubscribing weak observers from the publisher.  This function mimics the Boost
	 * intrusive_ptr functions @a intrusive_ptr_add_ref and @a intrusive_ptr_release.
	 *
	 * The second parameter is used to enable strictly-typed overloads for WeakObserver<T> vs
	 * WeakObserver<const T> (since those two template instantiations are considered completely
	 * different types in C++, which, for the first time ever, is actually what we want).  The
	 * actual argument to the second parameter doesn't matter -- It's not used at all -- as
	 * long as it's of the correct type:  The @a this pointer will suffice; the NULL pointer
	 * will not.
	 */
	inline
	WeakObserver<const FeatureCollectionHandle> *&
	weak_observer_get_first(
			const FeatureCollectionHandle *publisher_ptr,
			const WeakObserver<const FeatureCollectionHandle> *)
	{
		return publisher_ptr->first_const_weak_observer();
	}


	/**
	 * Get the last weak observer of the publisher pointed-to by @a publisher_ptr.
	 *
	 * It is assumed that @a publisher_ptr is a non-NULL pointer which is valid to dereference.
	 *
	 * This function is used by the WeakObserver template class when subscribing and
	 * unsubscribing weak observers from the publisher.  This style of function mimics the
	 * Boost intrusive_ptr functions @a intrusive_ptr_add_ref and @a intrusive_ptr_release.
	 *
	 * The second parameter is used to enable strictly-typed overloads for WeakObserver<T> vs
	 * WeakObserver<const T> (since those two template instantiations are considered completely
	 * different types in C++, which, for the first time ever, is actually what we want).  The
	 * actual argument to the second parameter doesn't matter -- It's not used at all -- as
	 * long as it's of the correct type:  The @a this pointer will suffice; the NULL pointer
	 * will not.
	 */
	inline
	WeakObserver<const FeatureCollectionHandle> *&
	weak_observer_get_last(
			const FeatureCollectionHandle *publisher_ptr,
			const WeakObserver<const FeatureCollectionHandle> *)
	{
		return publisher_ptr->last_const_weak_observer();
	}


	/**
	 * Get the first weak observer of the publisher pointed-to by @a publisher_ptr.
	 *
	 * It is assumed that @a publisher_ptr is a non-NULL pointer which is valid to dereference.
	 *
	 * This function is used by the WeakObserver template class when subscribing and
	 * unsubscribing weak observers from the publisher.  This function mimics the Boost
	 * intrusive_ptr functions @a intrusive_ptr_add_ref and @a intrusive_ptr_release.
	 *
	 * The second parameter is used to enable strictly-typed overloads for WeakObserver<T> vs
	 * WeakObserver<const T> (since those two template instantiations are considered completely
	 * different types in C++, which, for the first time ever, is actually what we want).  The
	 * actual argument to the second parameter doesn't matter -- It's not used at all -- as
	 * long as it's of the correct type:  The @a this pointer will suffice; the NULL pointer
	 * will not.
	 */
	inline
	WeakObserver<FeatureCollectionHandle> *&
	weak_observer_get_first(
			FeatureCollectionHandle *publisher_ptr,
			const WeakObserver<FeatureCollectionHandle> *)
	{
		return publisher_ptr->first_weak_observer();
	}


	/**
	 * Get the last weak observer of the publisher pointed-to by @a publisher_ptr.
	 *
	 * It is assumed that @a publisher_ptr is a non-NULL pointer which is valid to dereference.
	 *
	 * This function is used by the WeakObserver template class when subscribing and
	 * unsubscribing weak observers from the publisher.  This style of function mimics the
	 * Boost intrusive_ptr functions @a intrusive_ptr_add_ref and @a intrusive_ptr_release.
	 *
	 * The second parameter is used to enable strictly-typed overloads for WeakObserver<T> vs
	 * WeakObserver<const T> (since those two template instantiations are considered completely
	 * different types in C++, which, for the first time ever, is actually what we want).  The
	 * actual argument to the second parameter doesn't matter -- It's not used at all -- as
	 * long as it's of the correct type:  The @a this pointer will suffice; the NULL pointer
	 * will not.
	 */
	inline
	WeakObserver<FeatureCollectionHandle> *&
	weak_observer_get_last(
			FeatureCollectionHandle *publisher_ptr,
			const WeakObserver<FeatureCollectionHandle> *)
	{
		return publisher_ptr->last_weak_observer();
	}
}

#endif  // GPLATES_MODEL_FEATURECOLLECTIONHANDLE_H
