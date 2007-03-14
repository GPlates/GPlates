/* $Id$ */

/**
 * \file 
 * Contains the definition of the class FeatureHandle.
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

#ifndef GPLATES_MODEL_FEATUREHANDLE_H
#define GPLATES_MODEL_FEATUREHANDLE_H

#include <boost/intrusive_ptr.hpp>
#include "FeatureRevision.h"
#include "FeatureId.h"
#include "FeatureType.h"
#include "ConstFeatureVisitor.h"
#include "FeatureVisitor.h"

namespace GPlatesModel {

	/**
	 * A feature handle is the part of a GPML feature which does not change with revisions.
	 *
	 * This class represents the part of a GPML feature which does not change with revisions.
	 * That is, it contains the components of a feature which never change (the feature type
	 * and the feature ID), as well as acting as an everlasting handle to the FeatureRevision
	 * which contains the rest of the feature.
	 *
	 * A FeatureHandle instance is referenced by a FeatureCollection.
	 */
	class FeatureHandle {

	public:
		/**
		 * The type used to store the reference-count of an instance of this class.
		 */
		typedef long ref_count_type;

		~FeatureHandle()
		{  }

		/**
		 * Create a new FeatureHandle instance with feature type @a feature_type_ and
		 * feature ID @a feature_id_.
		 */
		static
		const boost::intrusive_ptr<FeatureHandle>
		create(
				const FeatureType &feature_type_,
				const FeatureId &feature_id_)
		{
			boost::intrusive_ptr<FeatureHandle> ptr(new FeatureHandle(feature_type_, feature_id_));
			return ptr;
		}

		/**
		 * Create a duplicate of this FeatureHandle instance.
		 *
		 * Note that this will perform a "shallow copy".
		 */
		const boost::intrusive_ptr<FeatureHandle>
		clone() const
		{
			boost::intrusive_ptr<FeatureHandle> dup(new FeatureHandle(*this));
			return dup;
		}

		/**
		 * Return the feature type of this feature.
		 *
		 * Note that no "setter" is provided:  The feature type of a feature should never
		 * be changed.
		 */
		const FeatureType &
		feature_type() const {
			return d_feature_type;
		}

		/**
		 * Return the feature ID of this feature.
		 *
		 * Note that no "setter" is provided:  The feature ID of a feature should never be
		 * changed.
		 */
		const FeatureId &
		feature_id() const {
			return d_feature_id;
		}

		/**
		 * Return the revision ID of the current revision.
		 *
		 * Note that no "setter" is provided:  The revision ID of a revision should never
		 * be changed.
		 */
		const RevisionId &
		revision_id() const
		{
			return current_revision()->revision_id();
		}

		/**
		 * Return the collection of properties of this feature.
		 *
		 * This is the overloading of this function for const FeatureRevision instances; it
		 * returns a reference to a const collection, which in turn will only allow const
		 * access to its elements.
		 *
		 * @b FIXME:  Should this function be replaced with per-index const-access to
		 * elements of the property container collection (which will return possibly-NULL
		 * pointers)?  (For consistency with the non-const overload...)
		 */
		const FeatureRevision::property_container_collection_type &
		properties() const
		{
			return current_revision()->properties();
		}

		/**
		 * Return the collection of properties of this feature revision.
		 *
		 * This is the overloading of this function for not-const FeatureRevision
		 * instances; it returns a reference to a non-const collection, which in turn will
		 * allow non-const access to its elements.
		 *
		 * @b FIXME:  Should this function be replaced with per-index access to elements of
		 * the property container collection (which will return possibly-NULL pointers), as
		 * well as per-index assignment (setter) and removal operations?  This would ensure
		 * that revisioning is correctly handled...
		 */
		FeatureRevision::property_container_collection_type &
		properties()
		{
			return current_revision()->properties();
		}

		/**
		 * Set the current revision of this feature to @a rev.
		 *
		 * FIXME:  This pointer should not be allowed to be NULL.
		 */
		void
		set_current_revision(
				boost::intrusive_ptr<FeatureRevision> rev) {
			d_current_revision = rev;
		}

		/**
		 * Accept a ConstFeatureVisitor instance.
		 *
		 * See the Visitor pattern (p.331) in Gamma95 for information on the purpose of
		 * this function.
		 */
		void
		accept_visitor(
				ConstFeatureVisitor &visitor) const {
			visitor.visit_feature_handle(*this);
		}

		/**
		 * Accept a FeatureVisitor instance.
		 *
		 * See the Visitor pattern (p.331) in Gamma95 for information on the purpose of
		 * this function.
		 */
		void
		accept_visitor(
				FeatureVisitor &visitor) {
			visitor.visit_feature_handle(*this);
		}

		/**
		 * Increment the reference-count of this instance.
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
		 * The current revision of this feature.
		 *
		 * FIXME:  This pointer should not be allowed to be NULL.
		 */
		boost::intrusive_ptr<FeatureRevision> d_current_revision;

		/**
		 * The type of this feature.
		 */
		FeatureType d_feature_type;

		/**
		 * The unique feature ID of this feature instance.
		 */
		FeatureId d_feature_id;

		/**
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		FeatureHandle(
				const FeatureType &feature_type_,
				const FeatureId &feature_id_) :
			d_ref_count(0),
			d_current_revision(FeatureRevision::create()),
			d_feature_type(feature_type_),
			d_feature_id(feature_id_) {  }

		/**
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 *
		 * This ctor should only be invoked by the 'clone' member function, which will
		 * create a duplicate instance and return a new intrusive_ptr reference to the new
		 * duplicate.  Since initially the only reference to the new duplicate will be the
		 * one returned by the 'clone' function, *before* the new intrusive_ptr is created,
		 * the ref-count of the new FeatureHandle instance should be zero.
		 *
		 * Note that this ctor should act exactly the same as the default (auto-generated)
		 * copy-ctor, except that it should initialise the ref-count to zero.
		 */
		FeatureHandle(
				const FeatureHandle &other) :
			d_ref_count(0),
			d_current_revision(other.d_current_revision),
			d_feature_type(other.d_feature_type),
			d_feature_id(other.d_feature_id) {  }

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		FeatureHandle &
		operator=(
				const FeatureHandle &);

		/**
		 * Access the current revision of this feature.
		 *
		 * This is the overloading of this function for const FeatureHandle instances; it
		 * returns a pointer to a const FeatureRevision instance.
		 */
		const boost::intrusive_ptr<const FeatureRevision>
		current_revision() const
		{
			return d_current_revision;
		}

		/**
		 * Access the current revision of this feature.
		 *
		 * This is the overloading of this function for non-const FeatureHandle instances;
		 * it returns a C++ reference to a pointer to a non-const FeatureRevision instance.
		 *
		 * Note that, because the copy-assignment operator of FeatureRevision is private,
		 * the FeatureRevision referenced by the return-value of this function cannot be
		 * assigned-to, which means that this function does not provide a means to directly
		 * switch the FeatureRevision within this FeatureHandle instance.  (This
		 * restriction is intentional.)
		 *
		 * To switch the FeatureRevision within this FeatureHandle instance, use the
		 * function @a set_current_revision.
		 */
		const boost::intrusive_ptr<FeatureRevision>
		current_revision()
		{
			return d_current_revision;
		}
	};


	inline
	void
	intrusive_ptr_add_ref(
			const FeatureHandle *p)
	{
		p->increment_ref_count();
	}


	inline
	void
	intrusive_ptr_release(
			const FeatureHandle *p)
	{
		if (p->decrement_ref_count() == 0) {
			delete p;
		}
	}

}

#endif  // GPLATES_MODEL_FEATUREHANDLE_H
