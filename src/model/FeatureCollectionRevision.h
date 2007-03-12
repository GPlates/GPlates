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
		std::vector<boost::intrusive_ptr<FeatureHandle> > d_features;

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
