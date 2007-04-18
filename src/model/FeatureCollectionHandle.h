/* $Id$ */

/**
 * \file 
 * Contains the definition of the class FeatureCollectionHandle.
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

#ifndef GPLATES_MODEL_FEATURECOLLECTIONHANDLE_H
#define GPLATES_MODEL_FEATURECOLLECTIONHANDLE_H

#include "FeatureCollectionRevision.h"
#include "HandleContainerIterator.h"
#include "contrib/non_null_intrusive_ptr.h"

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
	class FeatureCollectionHandle
	{
	public:
		/**
		 * The type of this class.
		 *
		 * This definition is used for template magic.
		 */
		typedef FeatureCollectionHandle this_type;

		/**
		 * The type used to store the reference-count of an instance of this class.
		 */
		typedef long ref_count_type;

		/**
		 * The type which contains the revisioning component of a feature collection.
		 *
		 * This typedef is used by the HandleContainerIterator.
		 */
		typedef FeatureCollectionRevision revision_component_type;

		/**
		 * The type used for const-iterating over the collection of feature handles.
		 */
		typedef HandleContainerIterator<const FeatureCollectionHandle,
				const revision_component_type::feature_collection_type,
				boost::intrusive_ptr<const FeatureHandle> > const_iterator;

		/**
		 * The type used for (non-const) iterating over the collection of feature handles.
		 */
		typedef HandleContainerIterator<FeatureCollectionHandle,
				revision_component_type::feature_collection_type,
				boost::intrusive_ptr<FeatureHandle> > iterator;

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
		 * Create a new FeatureCollectionHandle instance.
		 */
		static
		const GPlatesContrib::non_null_intrusive_ptr<FeatureCollectionHandle>
		create()
		{
			GPlatesContrib::non_null_intrusive_ptr<FeatureCollectionHandle> ptr(
					*(new FeatureCollectionHandle()));
			return ptr;
		}

		~FeatureCollectionHandle()
		{  }

		/**
		 * Create a duplicate of this FeatureCollectionHandle instance.
		 *
		 * Note that this will perform a "shallow copy".
		 */
		const GPlatesContrib::non_null_intrusive_ptr<FeatureCollectionHandle>
		clone() const
		{
			GPlatesContrib::non_null_intrusive_ptr<FeatureCollectionHandle> dup(
					*(new FeatureCollectionHandle(*this)));
			return dup;
		}

		/**
		 * Return the "begin" const-iterator to iterate over the collection of features.
		 */
		const const_iterator
		begin() const
		{
			return const_iterator::create_begin(*this);
		}

		/**
		 * Return the "begin" iterator to iterate over the collection of features.
		 */
		const iterator
		begin()
		{
			return iterator::create_begin(*this);
		}

		/**
		 * Return the "end" const-iterator used during iteration over the collection of
		 * features.
		 */
		const const_iterator
		end() const
		{
			return const_iterator::create_end(*this);
		}

		/**
		 * Return the "end" iterator used during iteration over the collection of features.
		 */
		const iterator
		end()
		{
			return iterator::create_end(*this);
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
		const iterator
		append_feature(
				GPlatesContrib::non_null_intrusive_ptr<FeatureHandle> new_feature,
				DummyTransactionHandle &transaction)
		{
			FeatureCollectionRevision::feature_collection_type::size_type new_index =
					current_revision()->append_feature(new_feature, transaction);
			return iterator(*this, new_index);
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
				const_iterator iter,
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
				iterator iter,
				DummyTransactionHandle &transaction)
		{
			current_revision()->remove_feature(iter.index(), transaction);
		}

		/**
		 * Access the current revision of this feature collection.
		 *
		 * Client code should not need to access the revision directly!
		 *
		 * This is the overloading of this function for const FeatureCollectionHandle
		 * instances; it returns a pointer to a const FeatureCollectionRevision instance.
		 */
		const GPlatesContrib::non_null_intrusive_ptr<const FeatureCollectionRevision>
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
		const GPlatesContrib::non_null_intrusive_ptr<FeatureCollectionRevision>
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
				GPlatesContrib::non_null_intrusive_ptr<FeatureCollectionRevision> rev)
		{
			d_current_revision = rev;
		}

		/**
		 * Increment the reference-count of this instance.
		 *
		 * Client code should not use this function!
		 *
		 * This function is used by boost::intrusive_ptr and
		 * GPlatesContrib::non_null_intrusive_ptr.
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
		 * This function is used by boost::intrusive_ptr and
		 * GPlatesContrib::non_null_intrusive_ptr.
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
		 * The current revision of this feature collection.
		 *
		 * FIXME:  This pointer should not be allowed to be NULL.
		 */
		GPlatesContrib::non_null_intrusive_ptr<FeatureCollectionRevision> d_current_revision;

		/**
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		FeatureCollectionHandle():
			d_ref_count(0),
			d_current_revision(FeatureCollectionRevision::create())
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
			d_ref_count(0),
			d_current_revision(other.d_current_revision)
		{  }

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		FeatureCollectionHandle &
		operator=(
				const FeatureCollectionHandle &);
	};


	inline
	void
	intrusive_ptr_add_ref(
			const FeatureCollectionHandle *p)
	{
		p->increment_ref_count();
	}


	inline
	void
	intrusive_ptr_release(
			const FeatureCollectionHandle *p)
	{
		if (p->decrement_ref_count() == 0) {
			delete p;
		}
	}

}

#endif  // GPLATES_MODEL_FEATURECOLLECTIONHANDLE_H
