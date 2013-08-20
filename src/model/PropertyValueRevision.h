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
	class PropertyValueRevisionContext;
	template <class PropertyValueType> class PropertyValueRevisionedReference;


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
		 * A shallow clone that deep copies everything except nested property value revision references.
		 *
		 * @a context is the optional (parent) context within which this revision is nested.
		 * A property value (revision) that is not attached to a parent has no context.
		 *
		 * Since property values nested within this property value are already revisioned
		 * we don't need to deep copy them. In other words two parent property value
		 * revisions can share the same nested property value revision.
		 */
		virtual
		non_null_ptr_type
		clone_revision(
				boost::optional<PropertyValueRevisionContext &> context = boost::none) const = 0;


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
		 * Constructor specified optional (parent) context in which this property value (revision) is nested.
		 */
		explicit
		PropertyValueRevision(
				boost::optional<PropertyValueRevisionContext &> context = boost::none) :
			d_context(context),
			d_revision_reference_ref_count(0)
		{  }

		/**
		 * NOTE: Copy-constructor is intentionally not defined anywhere (not strictly necessary to do
		 * this since base class ReferenceCount is already non-copyable, but makes it more obvious).
		 *
		 * Use the constructor (accepting revision context) when cloning a revision.
		 */
		PropertyValueRevision(
				const PropertyValueRevision &other);

	private:

		/**
		 * The bubble up callback to the parent property value (or top-level property), if any,
		 * that is called just prior to making a modification to 'this' property value.
		 */
		boost::optional<PropertyValueRevisionContext &> d_context;


		/**
		 * The reference-count of this instance used by @a PropertyValueRevisionedReference.
		 *
		 * This is used to detach 'this' property value revision from its revision context when
		 * the last @a PropertyValueRevisionedReference referencing 'this' is destroyed.
		 */
		mutable int d_revision_reference_ref_count;

		template <class PropertyValueType>
		friend class PropertyValueRevisionedReference;
	};
}

#endif // GPLATES_MODEL_PROPERTYVALUEREVISION_H
