/* $Id$ */

/**
 * \file 
 * Contains the definition of the FeatureRevision class.
 *
 * Most recent change:
 *   $Date$
 *
 * Copyright (C) 2006, 2007, 2009, 2010 The University of Sydney, Australia
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

#include <boost/function.hpp>

#include "BasicRevision.h"
#include "RevisionId.h"
#include "TopLevelProperty.h"

#include "global/PointerTraits.h"
#include "utils/ReferenceCount.h"

namespace GPlatesModel
{
	class FeatureHandle;

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
			public BasicRevision<FeatureHandle>,
			public GPlatesUtils::ReferenceCount<FeatureRevision>
	{

	public:

		/**
		 * The type of this class.
		 */
		typedef FeatureRevision this_type;

		/**
		 * Typedef for a function that accepts a pointer to a property and returns a boolean.
		 */
		typedef	child_predicate_type property_predicate_type;

		/**
		 * Creates a new FeatureRevision instance with an optional unique revision ID.
		 */
		static
		const non_null_ptr_type
		create(
				const RevisionId &revision_id_ = RevisionId());

		/**
		 * Creates a copy of this FeatureRevision instance.
		 *
		 * A new revision ID is created. The properties container is shallow copied.
		 */
		const non_null_ptr_type
		clone() const;

		/**
		 * Creates a copy of this FeatureRevision instance, copying only those
		 * properties for which the predicate @a clone_properties_predicate returns
		 * true.
		 *
		 * A new revision ID is created. The properties container is shallow copied.
		 */
		const non_null_ptr_type
		clone(
				const property_predicate_type &clone_properties_predicate) const;

		/**
		 * The unique revision ID for this feature revision.
		 *
		 * Note that there is no "setter" method because the revision ID of a feature
		 * revision should not be changed.
		 */
		const RevisionId &
		revision_id() const;

		/**
		 * @see BasicRevision::add()
		 */
		virtual
		const container_size_type
		add(
				TopLevelProperty::non_null_ptr_type new_child);

		/**
		 * @see BasicRevision::remove()
		 */
		virtual
		bool
		remove(
				container_size_type index);

		/**
		 * @see BasicRevision::set()
		 */
		virtual
		void
		set(
				container_size_type index,
				TopLevelProperty::non_null_ptr_type new_child);

	private:

		/**
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		FeatureRevision(
				const RevisionId &revision_id_);

		/**
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 *
		 * This copy constructor generates a new revision ID and does a shallow
		 * copy of the children (properties) container.
		 */
		FeatureRevision(
				const this_type &other);

		/**
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 *
		 * This copy constructor generates a new revision ID and does a shallow
		 * copy of the children (properties) container, copying those properties
		 * for which the predicate @a clone_properties_predicate returns true.
		 */
		FeatureRevision(
				const this_type &other,
				const property_predicate_type &clone_properties_predicate);

		/**
		 * This should not be defined, because we don't want to be able to copy
		 * one of these objects.
		 */
		this_type &
		operator=(
				const this_type &);

		/**
		 * The unique revision ID for this feature revision.
		 * FIXME: This need not be mutable once we actually create a new
		 * FeatureRevision object for every revision.
		 */
		mutable RevisionId d_revision_id;

		/**
		 * True if d_revision_id needs to be regenerated.
		 */
		mutable bool d_revision_id_dirty;

	};

}


#endif  // GPLATES_MODEL_FEATUREREVISION_H
