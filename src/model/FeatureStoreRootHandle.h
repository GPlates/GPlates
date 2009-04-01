/* $Id$ */

/**
 * \file 
 * Contains the definition of the class FeatureStoreRootHandle.
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

#ifndef GPLATES_MODEL_FEATURESTOREROOTHANDLE_H
#define GPLATES_MODEL_FEATURESTOREROOTHANDLE_H

#include "FeatureStoreRootRevision.h"
#include "RevisionAwareIterator.h"
#include "utils/non_null_intrusive_ptr.h"
#include "utils/NullIntrusivePointerHandler.h"
#include "utils/ReferenceCount.h"

namespace GPlatesModel
{
	class DummyTransactionHandle;

	/**
	 * A feature store root handle acts as a persistent handle to the revisioned content of a
	 * conceptual feature store root.
	 *
	 * The feature store root is the top layer/component of the three-tiered conceptual
	 * hierarchy of revisioned objects contained in, and managed by, the feature store:  It is
	 * the "root" node of the tree of revisioned objects.  The feature store contains a single
	 * feature store root, which in turn contains all the currently-loaded feature collections
	 * (each of which corresponds to a single data file).  Every currently-loaded feature is
	 * contained within a currently-loaded feature collection.
	 *
	 * The conceptual feature store root is implemented in two pieces: FeatureStoreRootHandle
	 * and FeatureStoreRootRevision.  A FeatureStoreRootHandle instance contains and manages a
	 * FeatureStoreRootRevision instance, which in turn contains the revisioned content of the
	 * conceptual feature store root.  A FeatureStoreRootHandle instance is contained within,
	 * and managed by, a FeatureStore instance.
	 *
	 * A feature store root handle instance is "persistent" in the sense that it will endure,
	 * in the same memory location, for as long as the conceptual feature store root exists
	 * (which will be determined by the lifetime of the feature store).  The revisioned content
	 * of the conceptual feature store root will be contained within a succession of feature
	 * store root revisions (with a new revision created as the result of every modification),
	 * but the handle will endure as a persistent means of accessing the current revision and
	 * the content within it.
	 */
	class FeatureStoreRootHandle :
			public GPlatesUtils::ReferenceCount<FeatureStoreRootHandle>
	{
	public:
		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<FeatureStoreRootHandle,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<FeatureStoreRootHandle,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<const FeatureStoreRootHandle,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const FeatureStoreRootHandle,
				GPlatesUtils::NullIntrusivePointerHandler>
				non_null_ptr_to_const_type;

		/**
		 * The type of this class.
		 *
		 * This definition is used for template magic.
		 */
		typedef FeatureStoreRootHandle this_type;

		/**
		 * The type which contains the revisioning component of a feature store root.
		 *
		 * This typedef is used by the RevisionAwareIterator.
		 */
		typedef FeatureStoreRootRevision revision_component_type;

		/**
		 * The type used for (non-const) iteration over the feature collections contained
		 * within this feature store root.
		 */
		typedef RevisionAwareIterator<FeatureStoreRootHandle,
				revision_component_type::feature_collection_container_type,
				boost::intrusive_ptr<FeatureCollectionHandle> >
				collections_iterator;

 		/**
		 * The base type of all weak observers of instances of this class.
		 */
		typedef WeakObserver<FeatureStoreRootHandle> weak_observer_type;

		/**
		 * Create a new FeatureStoreRootHandle instance.
		 */
		static
		const non_null_ptr_type
		create()
		{
			non_null_ptr_type ptr(new FeatureStoreRootHandle(),
					GPlatesUtils::NullIntrusivePointerHandler());
			return ptr;
		}

		/**
		 * Destructor.
		 *
		 * Unsubscribes all weak observers.
		 */
		~FeatureStoreRootHandle();

		/**
		 * Create a duplicate of this FeatureStoreRootHandle instance.
		 *
		 * Note that this will perform a "shallow copy".
		 */
		const non_null_ptr_type
		clone() const
		{
			non_null_ptr_type dup(new FeatureStoreRootHandle(*this),
					GPlatesUtils::NullIntrusivePointerHandler());
			return dup;
		}

		/**
		 * Return the "begin" iterator to iterate over the feature collections contained
		 * within this feature store root.
		 */
		const collections_iterator
		collections_begin()
		{
			return collections_iterator::create_begin(*this);
		}

		/**
		 * Return the "end" iterator used during iteration over the feature collections
		 * contained within this feature store root.
		 */
		const collections_iterator
		collections_end()
		{
			return collections_iterator::create_end(*this);
		}

		/**
		 * Append @a new_feature_collection to the container of feature collections.
		 *
		 * An iterator is returned which points to the new element in the container.
		 *
		 * After the FeatureCollectionHandle has been appended, the "end" iterator will
		 * have advanced -- the length of the sequence will have increased by 1, so what
		 * was the iterator to the last element of the sequence (the "back" of the
		 * container), will now be the iterator to the second-last element of the sequence;
		 * what was the "end" iterator will now be the iterator to the last element of the
		 * sequence.
		 */
		const collections_iterator
		append_feature_collection(
				FeatureCollectionHandle::non_null_ptr_type new_feature_collection,
				DummyTransactionHandle &transaction)
		{
			FeatureStoreRootRevision::feature_collection_container_type::size_type new_index =
					current_revision()->append_feature_collection(new_feature_collection,
							transaction);
			return collections_iterator::create_index(*this, new_index);
		}

		/**
		 * Remove the feature collection indicated by @a iter in the feature-collection
		 * container.
		 *
		 * The results of this operation are only defined if @a iter is before @a end.
		 *
		 * The "end" iterator will not be changed by this operation -- the length of the
		 * sequence will not change, only a feature-collection-slot will become NULL.
		 */
		void
		remove_feature_collection(
				collections_iterator iter,
				DummyTransactionHandle &transaction)
		{
			current_revision()->remove_feature_collection(iter.index(), transaction);
		}

		/**
		 * Access the current revision of this feature store root.
		 *
		 * Client code should not need to access the revision directly!
		 *
		 * This is the overloading of this function for const FeatureStoreRootHandle
		 * instances; it returns a pointer to a const FeatureStoreRootRevision instance.
		 */
		const FeatureStoreRootRevision::non_null_ptr_to_const_type
		current_revision() const
		{
			return d_current_revision;
		}

		/**
		 * Access the current revision of this feature collection.
		 *
		 * Client code should not need to access the revision directly!
		 *
		 * This is the overloading of this function for non-const FeatureStoreRootHandle
		 * instances; it returns a C++ reference to a pointer to a non-const
		 * FeatureStoreRootRevision instance.
		 *
		 * Note that, because the copy-assignment operator of FeatureStoreRootRevision is
		 * private, the FeatureStoreRootRevision instance referenced by the return-value of
		 * this function cannot be assigned-to, which means that this function does not
		 * provide a means to directly switch the FeatureStoreRootRevision instance within
		 * this FeatureStoreRootHandle instance.  (This restriction is intentional.)
		 *
		 * To switch the FeatureStoreRootRevision within this FeatureStoreRootHandle
		 * instance, use the function @a set_current_revision below.
		 */
		const FeatureStoreRootRevision::non_null_ptr_type
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
				FeatureStoreRootRevision::non_null_ptr_type rev)
		{
			d_current_revision = rev;
		}

 		/**
		 * Access the first weak observer of this instance.
		 *
		 * Client code should not use this function!
		 *
		 * This function is used by WeakObserver.
		 */
		weak_observer_type *&
		first_weak_observer() const
		{
			return d_first_weak_observer;
		}

		/**
		 * Access the last weak observer of this instance.
		 *
		 * Client code should not use this function!
		 *
		 * This function is used by WeakObserver.
		 */
		weak_observer_type *&
		last_weak_observer() const
		{
			return d_last_weak_observer;
		}

	private:

		/**
		 * The current revision of this feature store root.
		 */
		FeatureStoreRootRevision::non_null_ptr_type d_current_revision;

 		/**
		 * The first weak observer of this instance.
		 */
		mutable weak_observer_type *d_first_weak_observer;

		/**
		 * The last weak observer of this instance.
		 */
		mutable weak_observer_type *d_last_weak_observer;

		/**
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		FeatureStoreRootHandle():
			d_current_revision(FeatureStoreRootRevision::create()),
			d_first_weak_observer(NULL),
			d_last_weak_observer(NULL)
		{  }

		/**
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 *
		 * This ctor should only be invoked by the 'clone' member function, which will
		 * create a duplicate instance and return a new non_null_intrusive_ptr reference to
		 * the new duplicate.  Since initially the only reference to the new duplicate will
		 * be the one returned by the 'clone' function, *before* the new intrusive-pointer
		 * is created, the ref-count of the new FeatureStoreRootHandle instance should be
		 * zero.
		 *
		 * Note that this ctor should act exactly the same as the default (auto-generated)
		 * copy-ctor, except that it should initialise the ref-count to zero.
		 */
		FeatureStoreRootHandle(
				const FeatureStoreRootHandle &other) :
			GPlatesUtils::ReferenceCount<FeatureStoreRootHandle>(),
			d_current_revision(other.d_current_revision),
			d_first_weak_observer(NULL),
			d_last_weak_observer(NULL)
		{  }

		/**
		 * This operator should never be defined, because we don't want to allow
		 * copy-assignment.
		 * 
		 * All copying should use the virtual copy-constructor @a clone (which will in turn
		 * use the copy-constructor); all "copy-assignment" should really only be the
		 * assignment of one intrusive_ptr to another.
		 */
		FeatureStoreRootHandle &
		operator=(
				const FeatureStoreRootHandle &);
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
	WeakObserver<FeatureStoreRootHandle> *&
	weak_observer_get_first(
			FeatureStoreRootHandle *publisher_ptr,
			const WeakObserver<FeatureStoreRootHandle> *)
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
	WeakObserver<FeatureStoreRootHandle> *&
	weak_observer_get_last(
			FeatureStoreRootHandle *publisher_ptr,
			const WeakObserver<FeatureStoreRootHandle> *)
	{
		return publisher_ptr->last_weak_observer();
	}
}

#endif  // GPLATES_MODEL_FEATURESTOREROOTHANDLE_H
