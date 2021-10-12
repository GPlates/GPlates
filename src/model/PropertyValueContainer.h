/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_PROPERTYVALUECONTAINER_H
#define GPLATES_MODEL_PROPERTYVALUECONTAINER_H

#include "PropertyValue.h"


namespace GPlatesModel
{
	class DummyTransactionHandle;


	/**
	 * The abstract base class (ABC) interface for all classes which contain a PropertyValue
	 * instance.
	 *
	 * This class is part of the mechanism which tracks whether a feature collection contains
	 * unsaved changes, and (later) part of the Bubble-Up mechanism.
	 */
	class PropertyValueContainer
	{
	public:
		virtual
		~PropertyValueContainer()
		{  }

		virtual
		void
		bubble_up_change(
				const PropertyValue *old_value,
				PropertyValue::non_null_ptr_type new_value,
				DummyTransactionHandle &transaction) = 0;

	protected:
		// Just in case a compiler demands a constructor...
		PropertyValueContainer()
		{  }

		// Just in case a compiler demands a constructor...
		PropertyValueContainer(
				const PropertyValueContainer &other)
		{  }

	private:
		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		PropertyValueContainer &
		operator=(
				const PropertyValueContainer &);

	};

}

#endif  // GPLATES_MODEL_PROPERTYVALUECONTAINER_H
