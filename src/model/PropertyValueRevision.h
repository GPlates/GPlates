/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_PROPERTYVALUEREVISION_H
#define GPLATES_MODEL_PROPERTYVALUEREVISION_H

#include <boost/optional.hpp>

#include "utils/ReferenceCount.h"


namespace GPlatesModel
{
	// Forward declarations.
	class ModelTransaction;
	class PropertyValue;
	class PropertyValueRevision;
	class PropertyValueRevisionContext;


	/**
	 * Base class inherited by derived revision classes (in derived property values) where
	 * mutable/revisionable property value state is stored so it can be revisioned.
	 */
	class PropertyValueRevision :
			public GPlatesUtils::ReferenceCount<PropertyValueRevision>
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<PropertyValueRevision> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const PropertyValueRevision> non_null_ptr_to_const_type;


		virtual
		~PropertyValueRevision()
		{  }


		/**
		 * Create a duplicate of this Revision instance, including a recursive copy
		 * of any property values this instance might contain.
		 *
		 * This is used when cloning a property value instance because we want an entirely
		 * new property value instance that does not share anything with the original instance.
		 * This is useful when cloning a feature to get an entirely new feature (and not a new
		 * revision of the same feature) - in this situation if we didn't do a deep clone then,
		 * while both feature's would each have their own property value, they would still
		 * share a (more deeply) nested property value - in which case a modification to that
		 * nested property value would incorrectly affect both features.
		 */
		non_null_ptr_type
		clone(
				boost::optional<PropertyValueRevisionContext &> context = boost::none) const
		{
			// Clone the property value and set the (parent) context.
			non_null_ptr_type cloned_revision = clone_impl();
			cloned_revision->d_context = context;
			return cloned_revision;
		}


		/**
		 * A shallow version of @a clone that deep copies everything except nested property value
		 * revision references.
		 *
		 * Since property values nested within this property value are already revisioned
		 * we don't need to deep copy them. In other words two parent property value
		 * revisions can share the same nested property value revision.
		 */
		non_null_ptr_type
		clone_revision(
				boost::optional<PropertyValueRevisionContext &> context = boost::none) const
		{
			// Clone the revision and set the (parent) context.
			non_null_ptr_type cloned_revision = clone_revision_impl();
			cloned_revision->d_context = context;
			return cloned_revision;
		}


		/**
		 * Determine if two Revision instances ('this' and 'other') value compare equal.
		 *
		 * This should recursively test for equality as needed.
		 *
		 * A precondition of this method is that the type of 'this' is the same as the type of 'other'.
		 */
		virtual
		bool
		equality(
				const PropertyValueRevision &other) const
		{
			return true; // Terminates derived-to-base recursion.
		}


		/**
		 * Returns the (parent) context of this revision, if any.
		 *
		 * Note: There's no set method since it should not be possible to alter the context
		 * after a revision has been created.
		 */
		boost::optional<PropertyValueRevisionContext &>
		get_context() const
		{
			return d_context;
		}

	protected:

		/**
		 * Default constructor - is needed since no implicit constructor generated because copy constructor defined.
		 */
		PropertyValueRevision()
		{  }

		/**
		 * Copy constructor - calls default constructor of ReferenceCount (non-copyable) base class.
		 *
		 * Note: This does not copy the revision context.
		 */
		PropertyValueRevision(
				const PropertyValueRevision &other)
		{  }


		/**
		 * Create a duplicate of this Revision instance, including a recursive copy
		 * of any property values this instance might contain.
		 */
		virtual
		non_null_ptr_type
		clone_impl() const = 0;


		/**
		 * A shallow version of @a clone that deep copies everything except nested property value
		 * revision references.
		 */
		virtual
		non_null_ptr_type
		clone_revision_impl() const = 0;

	private:

		/**
		 * The bubble up callback to the parent property value (or top-level property), if any,
		 * that is called just prior to making a modification to 'this' property value.
		 */
		boost::optional<PropertyValueRevisionContext &> d_context;
	};
}

#endif // GPLATES_MODEL_PROPERTYVALUEREVISION_H
