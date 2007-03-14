/* $Id$ */

/**
 * \file 
 * Contains the definition of the class FeatureRevision.
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

#ifndef GPLATES_MODEL_FEATUREREVISION_H
#define GPLATES_MODEL_FEATUREREVISION_H

#include <vector>
#include <boost/intrusive_ptr.hpp>
#include "RevisionId.h"
#include "PropertyContainer.h"


namespace GPlatesModel {

	/**
	 * A feature revision is the part of a GPML feature which can change with revisions.
	 *
	 * This class represents the part of a GPML feature which can change (or, in the case of
	 * the revision ID, always changes) with revisions.  That is, it contains the revision ID,
	 * as well as all the properties of the feature, which can be modified by user-operations.
	 *
	 * A FeatureRevision instance can be referenced either by a FeatureHandle instance (when
	 * the FeatureRevision represents the current state of the feature) or by a TransactionItem
	 * (when the FeatureRevision represents the past or undone state of a feature).
	 */
	class FeatureRevision {

	public:
		/**
		 * The type used to store the reference-count of an instance of this class.
		 */
		typedef long ref_count_type;

		/**
		 * The type used to contain the properties of this feature revision.
		 */
		typedef std::vector<boost::intrusive_ptr<PropertyContainer> >
				property_container_collection_type;

		~FeatureRevision()
		{  }

		/**
		 * Create a new FeatureRevision instance with a default-constructed revision ID.
		 */
		static
		const boost::intrusive_ptr<FeatureRevision>
		create() {
			boost::intrusive_ptr<FeatureRevision> ptr(new FeatureRevision());
			return ptr;
		}

		/**
		 * Create a new FeatureRevision instance with a revision ID @a revision_id_.
		 */
		static
		const boost::intrusive_ptr<FeatureRevision>
		create(
				const RevisionId &revision_id_) {
			boost::intrusive_ptr<FeatureRevision> ptr(new FeatureRevision(revision_id_));
			return ptr;
		}

		/**
		 * Create a duplicate of this FeatureRevision instance.
		 */
		const boost::intrusive_ptr<FeatureRevision>
		clone() const {
			boost::intrusive_ptr<FeatureRevision> dup(new FeatureRevision(*this));
			return dup;
		}

		/**
		 * Return the revision ID of this revision.
		 *
		 * Note that no "setter" is provided:  The revision ID of a revision should never
		 * be changed.
		 */
		const RevisionId &
		revision_id() const {
			return d_revision_id;
		}

		/**
		 * Return the collection of properties of this feature revision.
		 *
		 * This is the overloading of this function for const FeatureRevision instances; it
		 * returns a reference to a const collection, which in turn will only allow const
		 * access to its elements.
		 *
		 * @b FIXME:  Should this function be replaced with per-index const-access to
		 * elements of the property container collection (which will return possibly-NULL
		 * pointers)?  (For consistency with the non-const overload...)
		 */
		const property_container_collection_type &
		properties() const {
			return d_properties;
		}

		/**
		 * Return the collection of properties of this feature revision.
		 *
		 * This is the overloading of this function for non-const FeatureRevision
		 * instances; it returns a reference to a non-const collection, which in turn will
		 * allow non-const access to its elements.
		 *
		 * @b FIXME:  Should this function be replaced with per-index access to elements of
		 * the property container collection (which will return possibly-NULL pointers), as
		 * well as per-index assignment (setter) and removal operations?  This would ensure
		 * that revisioning is correctly handled...
		 */
		property_container_collection_type &
		properties() {
			return d_properties;
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

		/**
		 * The reference-count of this instance by intrusive-pointers.
		 */
		mutable ref_count_type d_ref_count;

		/**
		 * The unique revision ID of this feature revision.
		 */
		RevisionId d_revision_id;

		/*
		 * The collection of properties possessed by this feature.
		 *
		 * Note that any of the pointers contained as elements in this vector can be NULL. 
		 *
		 * An element which is a NULL pointer indicates that the property, which was
		 * referenced by that element, has been deleted.  The element is set to a NULL
		 * pointer rather than removed, so that the indices, which are used to reference
		 * the other elements in the vector, remain valid.
		 */
		property_container_collection_type d_properties;

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		FeatureRevision() :
			d_ref_count(0),
			d_revision_id()
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		explicit
		FeatureRevision(
				const RevisionId &revision_id_) :
			d_ref_count(0),
			d_revision_id(revision_id_)
		{  }

		/*
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 *
		 * This ctor should only be invoked by the 'clone' member function, which will
		 * create a duplicate instance and return a new intrusive_ptr reference to the new
		 * duplicate.  Since initially the only reference to the new duplicate will be the
		 * one returned by the 'clone' function, *before* the new intrusive_ptr is created,
		 * the ref-count of the new FeatureRevision instance should be zero.
		 *
		 * Note that this ctor should act exactly the same as the default (auto-generated)
		 * copy-ctor, except that it should initialise the ref-count to zero.
		 */
		FeatureRevision(
				const FeatureRevision &other) :
			d_ref_count(0),
			d_revision_id(other.d_revision_id),
			d_properties(other.d_properties)
		{  }

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		FeatureRevision &
		operator=(
				const FeatureRevision &);

	};


	inline
	void
	intrusive_ptr_add_ref(
			const FeatureRevision *p) {
		p->increment_ref_count();
	}


	inline
	void
	intrusive_ptr_release(
			const FeatureRevision *p) {
		if (p->decrement_ref_count() == 0) {
			delete p;
		}
	}

}

#endif  // GPLATES_MODEL_FEATUREREVISION_H
