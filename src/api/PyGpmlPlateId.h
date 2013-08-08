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

#ifndef GPLATES_API_PYGPMLPLATEID_H
#define GPLATES_API_PYGPMLPLATEID_H

#include "PyPropertyValue.h"

#include "global/python.h"

#include "property-values/GpmlPlateId.h"


#if !defined(GPLATES_NO_PYTHON)

namespace GPlatesApi
{
	/**
	 * Python wrapper class for GPlatesPropertyValues::GpmlPlateId.
	 */
	class GpmlPlateId :
			public PropertyValue
	{
	public:

		/**
		 * Creation method is only way to construct instance.
		 */
		static
		GpmlPlateId
		create(
				const GPlatesModel::integer_plate_id_type &value)
		{
			return GpmlPlateId(value);
		}

		/**
		 * Get the plate id.
		 */
		const GPlatesModel::integer_plate_id_type
		get_value() const
		{
			return get_property_value<GPlatesPropertyValues::GpmlPlateId>().get_value();
		}

		/**
		 * Set the plate id.
		 */
		void
		set_value(
				const GPlatesModel::integer_plate_id_type &p)
		{
			get_property_value<GPlatesPropertyValues::GpmlPlateId>().set_value(p);
		}

	private:

		explicit
		GpmlPlateId(
				const GPlatesModel::integer_plate_id_type &value) :
			PropertyValue(GPlatesPropertyValues::GpmlPlateId::create(value))
		{  }
	};
}

#endif // GPLATES_NO_PYTHON

#endif // GPLATES_API_PYGPMLPLATEID_H
