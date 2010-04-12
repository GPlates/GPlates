/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

#include <typeinfo>

#include "GpmlTopologicalPoint.h"


const GPlatesPropertyValues::GpmlTopologicalPoint::non_null_ptr_type
GPlatesPropertyValues::GpmlTopologicalPoint::deep_clone() const
{
	GpmlTopologicalPoint::non_null_ptr_type dup = clone();

	GpmlPropertyDelegate::non_null_ptr_type cloned_source_geometry =
			d_source_geometry->deep_clone();
	dup->d_source_geometry = cloned_source_geometry;

	return dup;
}


bool
GPlatesPropertyValues::GpmlTopologicalPoint::directly_modifiable_fields_equal(
		const GPlatesModel::PropertyValue &other) const
{
	try
	{
		const GpmlTopologicalPoint &other_casted =
			dynamic_cast<const GpmlTopologicalPoint &>(other);
		return *d_source_geometry == *other_casted.d_source_geometry;
	}
	catch (const std::bad_cast &)
	{
		// Should never get here, but doesn't hurt to check.
		return false;
	}
}

