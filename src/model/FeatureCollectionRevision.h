/* $Id$ */

/**
 * \file 
 * File specific comments.
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

#ifndef GPLATES_MODEL_FEATURECOLLECTIONREVISION_H
#define GPLATES_MODEL_FEATURECOLLECTIONREVISION_H

#include <vector>
#include <boost/intrusive_ptr.hpp>
#include "FeatureHandle.h"


namespace GPlatesModel
{
	class DummyTransactionHandle;

	/**
	 * A feature collection revision is the part of a GML feature collection which can change
	 * with revisions.
	 *
	 * This class represents the part of a GML feature collection which can change with
	 * revisions.  That is, it contains the collection of features which is able to be modified
	 * by user-operations.
	 *
	 * A FeatureCollectionRevision instance can be referenced either by a
	 * FeatureCollectionHandle instance (when the FeatureCollectionRevision represents the
	 * current state of the feature collection) or by a TransactionItem (when the
	 * FeatureCollectionRevision represents the past or undone state of a feature collection).
	 */
	class FeatureCollectionRevision
	{
	public:
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
		const boost::intrusive_ptr<FeatureCollectionRevision>
		create() {
			boost::intrusive_ptr<FeatureCollectionRevision> ptr(
					new FeatureCollectionRevision());
			return ptr;
		}

		/**
		 * Create a duplicate of this FeatureCollectionRevision instance.
		 */
		const boost::intrusive_ptr<FeatureCollectionRevision>
		clone() const {
			boost::intrusive_ptr<FeatureCollectionRevision> dup(
					new FeatureCollectionRevision(*this));
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
			boost::intrusive_ptr<const FeatureHandle> ptr = NULL;
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
			boost::intrusive_ptr<FeatureHandle> ptr = NULL;
			if (index < size()) {
				ptr = d_features[index];
			}
			return ptr;
		}

		/**
		 * Append @a new_feature to the feature collection.
		 */
		void
		append_feature(
				boost::intrusive_ptr<FeatureHandle> new_feature,
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
		 * This function is used by boost::intrusive_ptr.
		 */
		void
		increment_ref_count() const {
			++d_ref_count;
		}

		/**
		 * Decrement the reference-count of this instance, and return the new
		 * reference-count.
		 *
		 * This function is used by boost::intrusive_ptr.
		 */
		ref_count_type
		decrement_ref_count() const {
			return --d_ref_count;
		}

	private:

		mutable ref_count_type d_ref_count;

		/**
		 * The collection of features contained within this feature collection.
		 *
		 * None of these pointers should be NULL.
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
		 * create a duplicate instance and return a new intrusive_ptr reference to the new
		 * duplicate.  Since initially the only reference to the new duplicate will be the
		 * one returned by the 'clone' function, *before* the new intrusive_ptr is created,
		 * the ref-count of the new FeatureCollectionRevision instance should be zero.
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
