/* $Id$ */

/**
 * \file 
 * Contains the definition of the class FeatureStoreRootRevision.
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

#ifndef GPLATES_MODEL_FEATURESTOREROOTREVISION_H
#define GPLATES_MODEL_FEATURESTOREROOTREVISION_H

#include <vector>
#include <boost/intrusive_ptr.hpp>
#include "FeatureCollectionHandle.h"
#include "contrib/non_null_intrusive_ptr.h"


namespace GPlatesModel
{
	class DummyTransactionHandle;

	/**
	 * A feature store root revision contains the revisioned content of a conceptual feature
	 * store root.
	 *
	 * The feature store root is the top layer/component of the three-tiered conceptual
	 * hierarchy of revisioned objects contained in, and managed by, the feature store:  It is
	 * the "root" node of the tree of revisioned objects.  The feature store contains a single
	 * feature store root, which in turn contains all the currently-loaded feature collections
	 * (each of which corresponds to a single data file).  Every currently-loaded feature is
	 * contained within a currently-loaded feature collection.
	 *
	 * The conceptual feature store root is implemented in two pieces: FeatureStoreRootHandle
	 * and FeatureStoreRootRevision.  A FeatureStoreRootRevision instance contains the
	 * revisioned content of the conceptual feature store root, and is in turn referenced by
	 * either a FeatureStoreRootHandle instance or a TransactionItem instance.
	 *
	 * A new instance of FeatureStoreRootRevision will be created whenever the conceptual
	 * feature store root is modified by the addition or removal of feature-collection elements
	 * -- a new instance of FeatureStoreRootRevision is created, because the existing
	 * ("current") FeatureStoreRootRevision instance will not be modified.  The newly-created
	 * FeatureStoreRootRevision instance will then be "scheduled" in a TransactionItem.  When
	 * the TransactionItem is "committed", the pointer (in the TransactionItem) to the new
	 * FeatureStoreRootRevision instance will be swapped with the pointer (in the
	 * FeatureStoreRootHandle instance) to the "current" instance, so that the "new" instance
	 * will now become the "current" instance (referenced by the pointer in the
	 * FeatureStoreRootHandle) and the "current" instance will become the "old" instance
	 * (referenced by the pointer in the now-committed TransactionItem).
	 *
	 * Client code should not reference FeatureStoreRootRevision instances directly; rather, it
	 * should always access the "current" instance (whichever FeatureStoreRootRevision instance
	 * it may be) through the feature store root handle.
	 */
	class FeatureStoreRootRevision
	{
	public:
		/**
		 * The type used to store the reference-count of an instance of this class.
		 */
		typedef long ref_count_type;

		/**
		 * The type used to contain the feature collections.
		 */
		typedef std::vector<boost::intrusive_ptr<FeatureCollectionHandle> >
				feature_collection_container_type;

		~FeatureStoreRootRevision()
		{  }

		/**
		 * Create a new FeatureStoreRootRevision instance.
		 *
		 * This collection contains no features.
		 */
		static
		const GPlatesContrib::non_null_intrusive_ptr<FeatureStoreRootRevision>
		create() {
			GPlatesContrib::non_null_intrusive_ptr<FeatureStoreRootRevision> ptr(
					*(new FeatureStoreRootRevision()));
			return ptr;
		}

		/**
		 * Create a duplicate of this FeatureStoreRootRevision instance.
		 */
		const GPlatesContrib::non_null_intrusive_ptr<FeatureStoreRootRevision>
		clone() const {
			GPlatesContrib::non_null_intrusive_ptr<FeatureStoreRootRevision> dup(
					*(new FeatureStoreRootRevision(*this)));
			return dup;
		}

		/**
		 * Return the number of feature-collection slots currently contained within the
		 * container.
		 *
		 * Note that feature-collection slots may be empty (ie, the pointer at that
		 * position may be NULL).  Thus, the number of feature collections actually
		 * contained within this feature store root may be less than the number of
		 * feature-collection slots.
		 * 
		 * This value is intended to be used as an upper (open range) limit on the values
		 * of the index used to access the feature collections within the container. 
		 * Attempting to access a feature collection at an index which is greater-than or
		 * equal-to the number of feature-collection slots will always result in a NULL
		 * pointer.
		 */
		feature_collection_container_type::size_type
		size() const
		{
			return d_feature_collections.size();
		}

		/**
		 * Access the feature collection at @a index in the feature-collection container.
		 *
		 * The value of @a index is expected to be non-negative.  If the value of @a index
		 * is greater-than or equal-to the return value of the @a size function, a NULL
		 * pointer will be returned.  If the value of @a index is less-than the return
		 * value of the @a size function, a NULL pointer @em may be returned (depending
		 * upon whether that feature-collection slot is still being used or not).
		 *
		 * This is the overloading of this function for const FeatureStoreRootRevision
		 * instances; it returns a pointer to a const FeatureCollectionHandle instance.
		 */
		const boost::intrusive_ptr<const FeatureCollectionHandle>
		operator[](
				feature_collection_container_type::size_type index) const
		{
			return access_feature(index);
		}

		/**
		 * Access the feature collection at @a index in the feature-collection container.
		 *
		 * The value of @a index is expected to be non-negative.  If the value of @a index
		 * is greater-than or equal-to the return value of the @a size function, a NULL
		 * pointer will be returned.  If the value of @a index is less-than the return
		 * value of the @a size function, a NULL pointer @em may be returned (depending
		 * upon whether that feature-collection slot is still being used or not).
		 *
		 * This is the overloading of this function for non-const FeatureStoreRootRevision
		 * instances; it returns a pointer to a non-const FeatureCollectionHandle instance.
		 */
		const boost::intrusive_ptr<FeatureCollectionHandle>
		operator[](
				feature_collection_container_type::size_type index)
		{
			return access_feature(index);
		}

		/**
		 * Access the feature collection at @a index in the feature-collection container.
		 *
		 * The value of @a index is expected to be non-negative.  If the value of @a index
		 * is greater-than or equal-to the return value of the @a size function, a NULL
		 * pointer will be returned.  If the value of @a index is less-than the return
		 * value of the @a size function, a NULL pointer @em may be returned (depending
		 * upon whether that feature-collection slot is still being used or not).
		 *
		 * This is the overloading of this function for const FeatureStoreRootRevision
		 * instances; it returns a pointer to a const FeatureCollectionHandle instance.
		 */
		const boost::intrusive_ptr<const FeatureCollectionHandle>
		access_feature(
				feature_collection_container_type::size_type index) const
		{
			boost::intrusive_ptr<const FeatureCollectionHandle> ptr = NULL;
			if (index < size()) {
				ptr = d_feature_collections[index];
			}
			return ptr;
		}

		/**
		 * Access the feature at @a index in the feature-collection container.
		 *
		 * The value of @a index is expected to be non-negative.  If the value of @a index
		 * is greater-than or equal-to the return value of the @a size function, a NULL
		 * pointer will be returned.  If the value of @a index is less-than the return
		 * value of the @a size function, a NULL pointer @em may be returned (depending
		 * upon whether that feature-collection slot is still being used or not).
		 *
		 * This is the overloading of this function for non-const FeatureStoreRootRevision
		 * instances; it returns a pointer to a non-const FeatureCollectionHandle instance.
		 */
		const boost::intrusive_ptr<FeatureCollectionHandle>
		access_feature(
				feature_collection_container_type::size_type index)
		{
			boost::intrusive_ptr<FeatureCollectionHandle> ptr = NULL;
			if (index < size()) {
				ptr = d_feature_collections[index];
			}
			return ptr;
		}

		/**
		 * Append @a new_feature_collection to the container of feature collections.
		 *
		 * The return-value is the index of the new element in the container.
		 */
		feature_collection_container_type::size_type
		append_feature_collection(
				GPlatesContrib::non_null_intrusive_ptr<FeatureCollectionHandle> new_feature_collection,
				DummyTransactionHandle &transaction);

		/**
		 * Remove the feature collection at @a index in the feature-collection container.
		 *
		 * The value of @a index is expected to be non-negative.  If the value of @a index
		 * is greater-than or equal-to the return value of the @a size function, this
		 * function will be a no-op.
		 */
		void
		remove_feature_collection(
				feature_collection_container_type::size_type index,
				DummyTransactionHandle &transaction);

		/**
		 * Increment the reference-count of this instance.
		 *
		 * This function is used by boost::intrusive_ptr and
		 * GPlatesContrib::non_null_intrusive_ptr.
		 */
		void
		increment_ref_count() const {
			++d_ref_count;
		}

		/**
		 * Decrement the reference-count of this instance, and return the new
		 * reference-count.
		 *
		 * This function is used by boost::intrusive_ptr and
		 * GPlatesContrib::non_null_intrusive_ptr.
		 */
		ref_count_type
		decrement_ref_count() const {
			return --d_ref_count;
		}

	private:

		/**
		 * The reference-count of this instance by intrusive-pointers.
		 */
		mutable ref_count_type d_ref_count;

		/**
		 * The container of feature collections contained within this feature store root.
		 *
		 * Any of the pointers in this container might be NULL.
		 */
		feature_collection_container_type d_feature_collections;

		/**
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		FeatureStoreRootRevision() :
			d_ref_count(0)
		{  }

		/**
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 *
		 * This ctor should only be invoked by the 'clone' member function, which will
		 * create a duplicate instance and return a new non_null_intrusive_ptr reference to
		 * the new duplicate.  Since initially the only reference to the new duplicate will
		 * be the one returned by the 'clone' function, *before* the new intrusive-pointer
		 * is created, the ref-count of the new FeatureStoreRootRevision instance should be
		 * zero.
		 *
		 * Note that this ctor should act exactly the same as the default (auto-generated)
		 * copy-ctor, except that it should initialise the ref-count to zero.
		 */
		FeatureStoreRootRevision(
				const FeatureStoreRootRevision &other) :
			d_ref_count(0),
			d_feature_collections(other.d_feature_collections)
		{  }

		/**
		 * This operator should never be defined, because we don't want to allow
		 * copy-assignment.
		 *
		 * All copying should use the virtual copy-constructor @a clone (which will in turn
		 * use the copy-constructor); all "copy-assignment" should really only be the
		 * assignment of one intrusive_ptr to another.
		 */
		FeatureStoreRootRevision &
		operator=(
				const FeatureStoreRootRevision &);

	};


	inline
	void
	intrusive_ptr_add_ref(
			const FeatureStoreRootRevision *p) {
		p->increment_ref_count();
	}


	inline
	void
	intrusive_ptr_release(
			const FeatureStoreRootRevision *p) {
		if (p->decrement_ref_count() == 0) {
			delete p;
		}
	}

}

#endif  // GPLATES_MODEL_FEATURESTOREROOTREVISION_H
