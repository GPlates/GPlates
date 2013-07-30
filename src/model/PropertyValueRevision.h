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

#include "utils/ReferenceCount.h"


namespace GPlatesModel
{
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
		 * This is useful when cloning a feature to get an entirely new feature (and not a
		 * revision of the feature).
		 */
		virtual
		non_null_ptr_type
		clone() const = 0;


		/**
		 * Same as @a clone but shares, instead of copies, any property values that this
		 * revision instance might contain.
		 *
		 * This is used when cloning revisions *within* a property value instance because
		 * any nested property values will have their own revisioning so we don't need to
		 * do a full deep clone/copy in this situation.
		 *
		 * This defaults to @a clone when not implemented in derived class.
		 */
		virtual
		non_null_ptr_type
		clone_with_shared_nested_property_values() const
		{
			return clone();
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
	};
}

#endif // GPLATES_MODEL_PROPERTYVALUEREVISION_H
