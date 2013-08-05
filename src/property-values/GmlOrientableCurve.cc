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

#include "GmlOrientableCurve.h"


const GPlatesPropertyValues::GmlOrientableCurve::non_null_ptr_type
GPlatesPropertyValues::GmlOrientableCurve::deep_clone() const
{
	GmlOrientableCurve::non_null_ptr_type dup = clone();
	GPlatesModel::PropertyValue::non_null_ptr_type cloned_base_curve =
			d_base_curve->deep_clone_as_prop_val();
	dup->d_base_curve = cloned_base_curve;

	return dup;
}


std::ostream &
GPlatesPropertyValues::GmlOrientableCurve::print_to(
		std::ostream &os) const
{
	return d_base_curve->print_to(os);
}


bool
GPlatesPropertyValues::GmlOrientableCurve::directly_modifiable_fields_equal(
		const GPlatesModel::PropertyValue &other) const
{
	try
	{
		const GmlOrientableCurve &other_casted =
			dynamic_cast<const GmlOrientableCurve &>(other);
		return *d_base_curve == *other_casted.d_base_curve &&
			d_xml_attributes == other_casted.d_xml_attributes;
	}
	catch (const std::bad_cast &)
	{
		// Should never get here, but doesn't hurt to check.
		return false;
	}
}

