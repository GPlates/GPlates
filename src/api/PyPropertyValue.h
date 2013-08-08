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

#ifndef GPLATES_API_PYPROPERTYVALUE_H
#define GPLATES_API_PYPROPERTYVALUE_H

#include "global/python.h"

#include "property-values/GpmlPlateId.h"


#if !defined(GPLATES_NO_PYTHON)

namespace GPlatesApi
{
	/**
	 * Base property value wrapper class.
	 *
	 * Provides the property value reference and also enables 'isinstance(obj, PropertyValue)' in python.
	 */
	class PropertyValue
	{
	public:

		// No public interface.

	protected:

		/**
		 * Derived classes should create the derived property value instance and pass it down.
		 */
		explicit
		PropertyValue(
				GPlatesModel::PropertyValue::non_null_ptr_type impl) :
			d_impl(impl)
		{  }

		/**
		 * Returns the specified derived property value type.
		 */
		template <class PropertyValueType>
		PropertyValueType &
		get_property_value() const
		{
			return dynamic_cast<PropertyValueType &>(*d_impl);
		}

	private:

		GPlatesModel::PropertyValue::non_null_ptr_type d_impl;
	};
}

#endif // GPLATES_NO_PYTHON

#endif // GPLATES_API_PYPROPERTYVALUE_H
