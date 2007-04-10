/* $Id$ */

/**
 * \file 
 * Contains the definition of the class FeatureStoreRootHandle.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006 The University of Sydney, Australia
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

#include <boost/intrusive_ptr.hpp>
#include "FeatureStoreRootRevision.h"
#include "HandleContainerIterator.h"

namespace GPlatesModel
{
	class DummyTransactionHandle;

	/**
	 * A feature store root handle acts as a persistent handle to the revisioned content of
	 * the feature store root.
	 *
	 * The feature store root is the top-level component of the three-tiered hierarchy of
	 * revisioned objects contained in, and managed by, the feature store:  It is the "root"
	 * vertex of the tree of revisioned objects.  The feature store contains a single feature
	 * store root, which in turn contains all the currently-loaded feature collections (each of
	 * which corresponds to a single data file).  Every currently-loaded feature is contained
	 * within a currently-loaded feature collection.
	 *
	 * A FeatureStoreRootHandle instance manages a FeatureStoreRootRevision instance, which in
	 * turn contains the revisioned content of the feature store root.  A
	 * FeatureStoreRootHandle instance is contained within, and managed by, a FeatureStore
	 * instance.
	 *
	 * A feature store root handle instance is "persistent" in the sense that it will endure,
	 * in the same memory location, for as long as the conceptual feature store root exists
	 * (which will be determined by the lifetime of the feature store).  The revisioned content
	 * of the conceptual feature store root will be contained within a succession of feature
	 * store root revisions (a new revision will be created as the result of every
	 * modification), but the handle will act as an enduring means of accessing the current
	 * revision.
	 */
	class FeatureStoreRootHandle
	{
	public:
		/**
		 * The type of this class.
		 *
		 * This definition is used for template magic.
		 */
		typedef FeatureStoreRootHandle this_type;

		/**
		 * The type used to store the reference-count of an instance of this class.
		 */
		typedef long ref_count_type;

		/**
		 * The type which contains the revisioning component of a feature store root.
		 *
		 * This typedef is used by the HandleContainerIterator.
		 */
		typedef FeatureStoreRootRevision revision_component_type;

		/**
		 * The type used for const-iteration over the feature collections contained within
		 * this feature store root.
		 */
		typedef HandleContainerIterator<const FeatureStoreRootHandle,
				const revision_component_type::feature_collection_container_type,
				boost::intrusive_ptr<const FeatureCollectionHandle> >
				const_iterator;

		/**
		 * The type used for (non-const) iteration over the feature collections contained
		 * within this feature store root.
		 */
		typedef HandleContainerIterator<FeatureStoreRootHandle,
				revision_component_type::feature_collection_container_type,
				boost::intrusive_ptr<FeatureCollectionHandle> >
				iterator;

		/**
		 * Translate the non-const iterator @a iter to the equivalent const-iterator.
		 */
		static
		const const_iterator
		get_const_iterator(
				iterator iter)
		{
			return const_iterator(*(iter.d_collection_handle_ptr), iter.d_index);
		}

		/**
		 * Create a new FeatureStoreRootHandle instance.
		 */
		static
		const boost::intrusive_ptr<FeatureStoreRootHandle>
		create()
		{
			boost::intrusive_ptr<FeatureStoreRootHandle> ptr(
					new FeatureStoreRootHandle());
			return ptr;
		}

		~FeatureStoreRootHandle()
		{  }

		/**
		 * Create a duplicate of this FeatureStoreRootHandle instance.
		 *
		 * Note that this will perform a "shallow copy".
		 */
		const boost::intrusive_ptr<FeatureStoreRootHandle>
		clone() const
		{
			boost::intrusive_ptr<FeatureStoreRootHandle> dup(
					new FeatureStoreRootHandle(*this));
			return dup;
		}

		/**
		 * Return the "begin" const-iterator to iterate over the feature collections
		 * contained within this feature store root.
		 */
		const const_iterator
		begin() const
		{
			return const_iterator::create_begin(*this);
		}

		/**
		 * Return the "begin" iterator to iterate over the feature collections contained
		 * within this feature store root.
		 */
		const iterator
		begin()
		{
			return iterator::create_begin(*this);
		}

		/**
		 * Return the "end" const-iterator used during iteration over the feature
		 * collections contained within this feature store root.
		 */
		const const_iterator
		end() const
		{
			return const_iterator::create_end(*this);
		}

		/**
		 * Return the "end" iterator used during iteration over the feature collections
		 * contained within this feature store root.
		 */
		const iterator
		end()
		{
			return iterator::create_end(*this);
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
		const iterator
		append_feature_collection(
				boost::intrusive_ptr<FeatureCollectionHandle> new_feature_collection,
				DummyTransactionHandle &transaction)
		{
			FeatureStoreRootRevision::feature_collection_container_type::size_type new_index =
					current_revision()->append_feature_collection(new_feature_collection,
							transaction);
			return iterator(*this, new_index);
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
				const_iterator iter,
				DummyTransactionHandle &transaction)
		{
			current_revision()->remove_feature_collection(iter.index(), transaction);
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
				iterator iter,
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
		const boost::intrusive_ptr<const FeatureStoreRootRevision>
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
		const boost::intrusive_ptr<FeatureStoreRootRevision>
		current_revision()
		{
			return d_current_revision;
		}

		/**
		 * Set the current revision of this feature collection to @a rev.
		 *
		 * Client code should not need to access the revision directly!
		 *
		 * FIXME:  This pointer should not be allowed to be NULL.
		 */
		void
		set_current_revision(
				boost::intrusive_ptr<FeatureStoreRootRevision> rev)
		{
			d_current_revision = rev;
		}

		/**
		 * Increment the reference-count of this instance.
		 *
		 * Client code should not use this function!
		 *
		 * This function is used by boost::intrusive_ptr.
		 */
		void
		increment_ref_count() const
		{
			++d_ref_count;
		}

		/**
		 * Decrement the reference-count of this instance, and return the new
		 * reference-count.
		 *
		 * Client code should not use this function!
		 *
		 * This function is used by boost::intrusive_ptr.
		 */
		ref_count_type
		decrement_ref_count() const
		{
			return --d_ref_count;
		}

	private:

		/**
		 * The reference-count of this instance by intrusive-pointers.
		 */
		mutable ref_count_type d_ref_count;

		/**
		 * The current revision of this feature store root.
		 *
		 * FIXME:  This pointer should not be allowed to be NULL.
		 */
		boost::intrusive_ptr<FeatureStoreRootRevision> d_current_revision;

		/**
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		FeatureStoreRootHandle():
			d_ref_count(0),
			d_current_revision(FeatureStoreRootRevision::create())
		{  }

		/**
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 *
		 * This ctor should only be invoked by the 'clone' member function, which will
		 * create a duplicate instance and return a new intrusive_ptr reference to the new
		 * duplicate.  Since initially the only reference to the new duplicate will be the
		 * one returned by the 'clone' function, *before* the new intrusive_ptr is created,
		 * the ref-count of the new FeatureStoreRootHandle instance should be zero.
		 *
		 * Note that this ctor should act exactly the same as the default (auto-generated)
		 * copy-ctor, except that it should initialise the ref-count to zero.
		 */
		FeatureStoreRootHandle(
				const FeatureStoreRootHandle &other) :
			d_ref_count(0),
			d_current_revision(other.d_current_revision)
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


	inline
	void
	intrusive_ptr_add_ref(
			const FeatureStoreRootHandle *p)
	{
		p->increment_ref_count();
	}


	inline
	void
	intrusive_ptr_release(
			const FeatureStoreRootHandle *p)
	{
		if (p->decrement_ref_count() == 0) {
			delete p;
		}
	}

}

#endif  // GPLATES_MODEL_FEATURESTOREROOTHANDLE_H
