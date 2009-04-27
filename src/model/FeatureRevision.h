/* $Id$ */

/**
 * \file 
 * Contains the definition of the class FeatureRevision.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2009 The University of Sydney, Australia
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
#include "TopLevelProperty.h"
#include "utils/non_null_intrusive_ptr.h"
#include "utils/NullIntrusivePointerHandler.h"
#include "utils/ReferenceCount.h"


namespace GPlatesModel
{
	class DummyTransactionHandle;

	/**
	 * A feature revision contains the revisioned content of a conceptual feature.
	 *
	 * The feature is the bottom layer/component of the three-tiered conceptual hierarchy of
	 * revisioned objects contained in, and managed by, the feature store:  The feature is an
	 * abstract model of some geological or plate-tectonic object or concept of interest,
	 * consisting of a collection of properties and a feature type.  The feature store contains
	 * a single feature store root, which in turn contains all the currently-loaded feature
	 * collections.  Every currently-loaded feature is contained within a currently-loaded
	 * feature collection.
	 *
	 * The conceptual feature is implemented in two pieces: FeatureHandle and FeatureRevision. 
	 * A FeatureRevision instance contains the revisioned content of the conceptual feature
	 * (the mutable properties of the feature), and is in turn referenced by either a
	 * FeatureHandle instance or a TransactionItem instance.
	 *
	 * A new instance of FeatureRevision will be created whenever the conceptual feature is
	 * modified by the addition, deletion or modification of properties -- a new instance of
	 * FeatureRevision is created, because the existing ("current") FeatureRevision instance
	 * will not be modified.  The newly-created FeatureRevision instance will then be
	 * "scheduled" in a TransactionItem.  When the TransactionItem is "committed", the pointer
	 * (in the TransactionItem) to the new FeatureRevision instance will be swapped with the
	 * pointer (in the FeatureHandle instance) to the "current" instance, so that the "new"
	 * instance will now become the "current" instance (referenced by the pointer in the
	 * FeatureHandle) and the "current" instance will become the "old" instance (referenced by
	 * the pointer in the now-committed TransactionItem).
	 *
	 * Client code should not reference FeatureRevision instances directly; rather, it should
	 * always access the "current" instance (whichever FeatureRevision instance it may be)
	 * through the feature handle.
	 *
	 * The feature revision contains all the properties of a feature, except those which can
	 * never change: the feature type and the feature ID.
	 */
	class FeatureRevision :
			public GPlatesUtils::ReferenceCount<FeatureRevision>
	{
	public:
		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<FeatureRevision,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<FeatureRevision,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<const FeatureRevision,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const FeatureRevision,
				GPlatesUtils::NullIntrusivePointerHandler>
				non_null_ptr_to_const_type;

		/**
		 * The type used to represent the top-level properties of this feature revision.
		 */
		typedef std::vector<boost::intrusive_ptr<TopLevelProperty> >
				top_level_property_collection_type;

		~FeatureRevision()
		{  }

		/**
		 * Create a new FeatureRevision instance with a default-constructed revision ID.
		 */
		static
		const non_null_ptr_type
		create()
		{
			non_null_ptr_type ptr(new FeatureRevision(),
					GPlatesUtils::NullIntrusivePointerHandler());
			return ptr;
		}

		/**
		 * Create a new FeatureRevision instance with a revision ID @a revision_id_.
		 */
		static
		const non_null_ptr_type
		create(
				const RevisionId &revision_id_)
		{
			non_null_ptr_type ptr(new FeatureRevision(revision_id_),
					GPlatesUtils::NullIntrusivePointerHandler());
			return ptr;
		}

		/**
		 * Create a duplicate of this FeatureRevision instance.
		 */
		const non_null_ptr_type
		clone() const
		{
			non_null_ptr_type dup(new FeatureRevision(*this),
					GPlatesUtils::NullIntrusivePointerHandler());
			return dup;
		}

		/**
		 * Return the revision ID of this revision.
		 *
		 * Note that no "setter" is provided:  The revision ID of a revision should never
		 * be changed.
		 */
		const RevisionId &
		revision_id() const
		{
			return d_revision_id;
		}

		/**
		 * Return the number of top-level-property-slots currently contained within this
		 * feature.
		 *
		 * Note that top-level-property-slots may be empty (ie, the pointer at that
		 * position may be NULL).  Thus, the number of top-level properties actually
		 * contained within this feature may be less than the number of
		 * top-level-property-slots.
		 * 
		 * This value is intended to be used as an upper (open range) limit on the values
		 * of the index used to access the top-level properties within this feature. 
		 * Attempting to access a top-level property at an index which is greater-than or
		 * equal-to the number of top-level-property-slots will always result in a NULL
		 * pointer.
		 */
		top_level_property_collection_type::size_type
		size() const
		{
			return d_properties.size();
		}

		/**
		 * Access the top-level property at @a index in the collection.
		 *
		 * The value of @a index is expected to be non-negative.  If the value of @a index
		 * is greater-than or equal-to the return value of the @a size function, a NULL
		 * pointer will be returned.  If the value of @a index is less-than the return
		 * value of the @a size function, a NULL pointer @em may be returned (depending
		 * upon whether that top-level-property-slot is still being used or not).
		 *
		 * This is the overloading of this function for const FeatureRevision instances; it
		 * returns a pointer to a const TopLevelProperty instance.
		 */
		const boost::intrusive_ptr<const TopLevelProperty>
		operator[](
				top_level_property_collection_type::size_type index) const
		{
			return access_top_level_property(index);
		}

		/**
		 * Access the top-level property at @a index in the collection.
		 *
		 * The value of @a index is expected to be non-negative.  If the value of @a index
		 * is greater-than or equal-to the return value of the @a size function, a NULL
		 * pointer will be returned.  If the value of @a index is less-than the return
		 * value of the @a size function, a NULL pointer @em may be returned (depending
		 * upon whether that top-level-property-slot is still being used or not).
		 *
		 * This is the overloading of this function for non-const FeatureRevision
		 * instances; it returns a pointer to a non-const TopLevelProperty instance.
		 */
		const boost::intrusive_ptr<TopLevelProperty>
		operator[](
				top_level_property_collection_type::size_type index)
		{
			return access_top_level_property(index);
		}

		/**
		 * Access the top-level property at @a index in the collection.
		 *
		 * The value of @a index is expected to be non-negative.  If the value of @a index
		 * is greater-than or equal-to the return value of the @a size function, a NULL
		 * pointer will be returned.  If the value of @a index is less-than the return
		 * value of the @a size function, a NULL pointer @em may be returned (depending
		 * upon whether that top-level-property-slot is still being used or not).
		 *
		 * This is the overloading of this function for const FeatureRevision instances; it
		 * returns a pointer to a const TopLevelProperty instance.
		 */
		const boost::intrusive_ptr<const TopLevelProperty>
		access_top_level_property(
				top_level_property_collection_type::size_type index) const
		{
			boost::intrusive_ptr<const TopLevelProperty> ptr;
			if (index < size()) {
				ptr = d_properties[index];
			}
			return ptr;
		}

		/**
		 * Access the top-level property at @a index in the collection.
		 *
		 * The value of @a index is expected to be non-negative.  If the value of @a index
		 * is greater-than or equal-to the return value of the @a size function, a NULL
		 * pointer will be returned.  If the value of @a index is less-than the return
		 * value of the @a size function, a NULL pointer @em may be returned (depending
		 * upon whether that top-level-property-slot is still being used or not).
		 *
		 * This is the overloading of this function for non-const FeatureRevision
		 * instances; it returns a pointer to a non-const TopLevelProperty instance.
		 */
		const boost::intrusive_ptr<TopLevelProperty>
		access_top_level_property(
				top_level_property_collection_type::size_type index)
		{
			boost::intrusive_ptr<TopLevelProperty> ptr;
			if (index < size()) {
				ptr = d_properties[index];
			}
			return ptr;
		}

		/**
		 * Append @a new_top_level_property to the collection.
		 */
		top_level_property_collection_type::size_type
		append_top_level_property(
				TopLevelProperty::non_null_ptr_type new_top_level_property,
				DummyTransactionHandle &transaction);

		/**
		 * Remove the top_level_property at @a index in the collection.
		 *
		 * The value of @a index is expected to be non-negative.  If the value of @a index
		 * is greater-than or equal-to the return value of the @a size function, this
		 * function will be a no-op.
		 */
		void
		remove_top_level_property(
				top_level_property_collection_type::size_type index,
				DummyTransactionHandle &transaction);

	private:

		/**
		 * The unique revision ID of this feature revision.
		 */
		RevisionId d_revision_id;

		/*
		 * The collection of top-level properties possessed by this feature.
		 *
		 * Note that any of the pointers contained as elements in this vector can be NULL. 
		 *
		 * An element which is a NULL pointer indicates that the property, which was
		 * referenced by that element, has been deleted.  The element is set to a NULL
		 * pointer rather than removed, so that the indices, which are used to reference
		 * the other elements in the vector, remain valid.
		 */
		top_level_property_collection_type d_properties;

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		FeatureRevision() :
			d_revision_id()
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		explicit
		FeatureRevision(
				const RevisionId &revision_id_) :
			d_revision_id(revision_id_)
		{  }

		/*
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 *
		 * This ctor should only be invoked by the 'clone' member function, which will
		 * create a duplicate instance and return a new non_null_intrusive_ptr reference to
		 * the new duplicate.  Since initially the only reference to the new duplicate will
		 * be the one returned by the 'clone' function, *before* the new intrusive-pointer
		 * is created, the ref-count of the new FeatureRevision instance should be zero.
		 *
		 * Note that this ctor should act exactly the same as the default (auto-generated)
		 * copy-ctor, except that it should initialise the ref-count to zero.
		 */
		FeatureRevision(
				const FeatureRevision &other) :
			GPlatesUtils::ReferenceCount<FeatureRevision>(),
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
}

#endif  // GPLATES_MODEL_FEATUREREVISION_H
