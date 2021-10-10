/* $Id$ */

/**
 * \file 
 * Contains the definition of the class FeatureCollectionRevision.
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

#ifndef GPLATES_MODEL_FEATURECOLLECTIONREVISION_H
#define GPLATES_MODEL_FEATURECOLLECTIONREVISION_H

#include <vector>
#include <boost/intrusive_ptr.hpp>
#include "FeatureHandle.h"
#include "utils/non_null_intrusive_ptr.h"
#include "utils/NullIntrusivePointerHandler.h"


namespace GPlatesModel
{
	class DummyTransactionHandle;

	/**
	 * A feature collection revision contains the revisioned content of a conceptual feature
	 * collection.
	 *
	 * The feature collection is the middle layer/component of the three-tiered conceptual
	 * hierarchy of revisioned objects contained in, and managed by, the feature store:  The
	 * feature collection aggregates a set of features into a collection which may be loaded,
	 * saved or unloaded in a single operation.  The feature store contains a single feature
	 * store root, which in turn contains all the currently-loaded feature collections.  Every
	 * currently-loaded feature is contained within a currently-loaded feature collection.
	 *
	 * The conceptual feature collection is implemented in two pieces: FeatureCollectionHandle
	 * and FeatureCollectionRevision.  A FeatureCollectionRevision instance contains the
	 * revisioned content of the conceptual feature collection, and is in turn referenced by
	 * either a FeatureCollectionHandle instance or a TransactionItem instance.
	 *
	 * A new instance of FeatureCollectionRevision will be created whenever the conceptual
	 * feature collection is modified by the addition or removal of feature elements -- a new
	 * instance of FeatureCollectionRevision is created, because the existing ("current")
	 * FeatureCollectionRevision instance will not be modified.  The newly-created
	 * FeatureCollectionRevision instance will then be "scheduled" in a TransactionItem.  When
	 * the TransactionItem is "committed", the pointer (in the TransactionItem) to the new
	 * FeatureCollectionRevision instance will be swapped with the pointer (in the
	 * FeatureCollectionHandle instance) to the "current" instance, so that the "new" instance
	 * will now become the "current" instance (referenced by the pointer in the
	 * FeatureCollectionHandle) and the "current" instance will become the "old" instance
	 * (referenced by the pointer in the now-committed TransactionItem).
	 *
	 * Client code should not reference FeatureCollectionRevision instances directly; rather,
	 * it should always access the "current" instance (whichever FeatureCollectionRevision
	 * instance it may be) through the feature collection handle.
	 */
	class FeatureCollectionRevision
	{
	public:
		/**
		 * A convenience typedef for 
		 * PlatesUtils::non_null_intrusive_ptr<FeatureCollectionRevision,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<FeatureCollectionRevision,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<const FeatureCollectionRevision,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const FeatureCollectionRevision,
				GPlatesUtils::NullIntrusivePointerHandler>
				non_null_ptr_to_const_type;

		/**
		 * The type used to store the reference-count of an instance of this class.
		 */
		typedef long ref_count_type;

		/**
		 * The type used for the collection of features.
		 */
		typedef std::vector<boost::intrusive_ptr<FeatureHandle> > feature_collection_type;

		~FeatureCollectionRevision()
		{  }

		/**
		 * Create a new FeatureCollectionRevision instance.
		 *
		 * This collection contains no features.
		 */
		static
		const non_null_ptr_type
		create() {
			non_null_ptr_type ptr(new FeatureCollectionRevision(),
					GPlatesUtils::NullIntrusivePointerHandler());
			return ptr;
		}

		/**
		 * Create a duplicate of this FeatureCollectionRevision instance.
		 */
		const non_null_ptr_type
		clone() const {
			non_null_ptr_type dup(new FeatureCollectionRevision(*this),
					GPlatesUtils::NullIntrusivePointerHandler());
			return dup;
		}

		/**
		 * Return the number of feature-slots currently contained within this feature
		 * collection.
		 *
		 * Note that feature-slots may be empty (ie, the pointer at that position may be
		 * NULL).  Thus, the number of features actually contained within this feature
		 * collection may be less than the number of feature-slots.
		 * 
		 * This value is intended to be used as an upper (open range) limit on the values
		 * of the index used to access the features within this collection.  Attempting to
		 * access a feature at an index which is greater-than or equal-to the number of
		 * feature-slots will always result in a NULL pointer.
		 */
		feature_collection_type::size_type
		size() const
		{
			return d_features.size();
		}

		/**
		 * Access the feature at @a index in the feature collection.
		 *
		 * The value of @a index is expected to be non-negative.  If the value of @a index
		 * is greater-than or equal-to the return value of the @a size function, a NULL
		 * pointer will be returned.  If the value of @a index is less-than the return
		 * value of the @a size function, a NULL pointer @em may be returned (depending
		 * upon whether that feature-slot is still being used or not).
		 *
		 * This is the overloading of this function for const FeatureCollectionRevision
		 * instances; it returns a pointer to a const FeatureHandle instance.
		 */
		const boost::intrusive_ptr<const FeatureHandle>
		operator[](
				feature_collection_type::size_type index) const
		{
			return access_feature(index);
		}

		/**
		 * Access the feature at @a index in the feature collection.
		 *
		 * The value of @a index is expected to be non-negative.  If the value of @a index
		 * is greater-than or equal-to the return value of the @a size function, a NULL
		 * pointer will be returned.  If the value of @a index is less-than the return
		 * value of the @a size function, a NULL pointer @em may be returned (depending
		 * upon whether that feature-slot is still being used or not).
		 *
		 * This is the overloading of this function for non-const FeatureCollectionRevision
		 * instances; it returns a pointer to a non-const FeatureHandle instance.
		 */
		const boost::intrusive_ptr<FeatureHandle>
		operator[](
				feature_collection_type::size_type index)
		{
			return access_feature(index);
		}

		/**
		 * Access the feature at @a index in the feature collection.
		 *
		 * The value of @a index is expected to be non-negative.  If the value of @a index
		 * is greater-than or equal-to the return value of the @a size function, a NULL
		 * pointer will be returned.  If the value of @a index is less-than the return
		 * value of the @a size function, a NULL pointer @em may be returned (depending
		 * upon whether that feature-slot is still being used or not).
		 *
		 * This is the overloading of this function for const FeatureCollectionRevision
		 * instances; it returns a pointer to a const FeatureHandle instance.
		 */
		const boost::intrusive_ptr<const FeatureHandle>
		access_feature(
				feature_collection_type::size_type index) const
		{
			boost::intrusive_ptr<const FeatureHandle> ptr;
			if (index < size()) {
				ptr = d_features[index];
			}
			return ptr;
		}

		/**
		 * Access the feature at @a index in the feature collection.
		 *
		 * The value of @a index is expected to be non-negative.  If the value of @a index
		 * is greater-than or equal-to the return value of the @a size function, a NULL
		 * pointer will be returned.  If the value of @a index is less-than the return
		 * value of the @a size function, a NULL pointer @em may be returned (depending
		 * upon whether that feature-slot is still being used or not).
		 *
		 * This is the overloading of this function for non-const FeatureCollectionRevision
		 * instances; it returns a pointer to a non-const FeatureHandle instance.
		 */
		const boost::intrusive_ptr<FeatureHandle>
		access_feature(
				feature_collection_type::size_type index)
		{
			boost::intrusive_ptr<FeatureHandle> ptr;
			if (index < size()) {
				ptr = d_features[index];
			}
			return ptr;
		}

		/**
		 * Append @a new_feature to the feature collection.
		 */
		feature_collection_type::size_type
		append_feature(
				FeatureHandle::non_null_ptr_type new_feature,
				DummyTransactionHandle &transaction);

		/**
		 * Remove the feature at @a index in the feature collection.
		 *
		 * The value of @a index is expected to be non-negative.  If the value of @a index
		 * is greater-than or equal-to the return value of the @a size function, this
		 * function will be a no-op.
		 */
		void
		remove_feature(
				feature_collection_type::size_type index,
				DummyTransactionHandle &transaction);

		/**
		 * Increment the reference-count of this instance.
		 *
		 * This function is used by boost::intrusive_ptr and
		 * GPlatesUtils::non_null_intrusive_ptr.
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
		 * GPlatesUtils::non_null_intrusive_ptr.
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
		 * The collection of features contained within this feature collection.
		 *
		 * Any of the pointers in this container might be NULL.
		 */
		feature_collection_type d_features;

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		FeatureCollectionRevision() :
			d_ref_count(0)
		{  }

		/*
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 *
		 * This ctor should only be invoked by the 'clone' member function, which will
		 * create a duplicate instance and return a new non_null_intrusive_ptr reference to
		 * the new duplicate.  Since initially the only reference to the new duplicate will
		 * be the one returned by the 'clone' function, *before* the new intrusive-pointer
		 * is created, the ref-count of the new FeatureCollectionRevision instance should
		 * be zero.
		 *
		 * Note that this ctor should act exactly the same as the default (auto-generated)
		 * copy-ctor, except that it should initialise the ref-count to zero.
		 */
		FeatureCollectionRevision(
				const FeatureCollectionRevision &other) :
			d_ref_count(0),
			d_features(other.d_features)
		{  }

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		FeatureCollectionRevision &
		operator=(
				const FeatureCollectionRevision &);

	};


	inline
	void
	intrusive_ptr_add_ref(
			const FeatureCollectionRevision *p) {
		p->increment_ref_count();
	}


	inline
	void
	intrusive_ptr_release(
			const FeatureCollectionRevision *p) {
		if (p->decrement_ref_count() == 0) {
			delete p;
		}
	}

}

#endif  // GPLATES_MODEL_FEATURECOLLECTIONREVISION_H
