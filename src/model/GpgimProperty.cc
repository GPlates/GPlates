/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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
#include <boost/foreach.hpp>

#include "GpgimProperty.h"

boost::optional<GPlatesModel::GpgimStructuralType::non_null_ptr_to_const_type>
GPlatesModel::GpgimProperty::get_structural_type(
		const GPlatesPropertyValues::StructuralType &structural_type_name) const
{
	// Iterate over the structural types.
	BOOST_FOREACH(
			const GPlatesModel::GpgimStructuralType::non_null_ptr_to_const_type &structural_type,
			d_structural_types)
	{
		if (structural_type->get_structural_type() == structural_type_name)
		{
			return structural_type;
		}
	}

	// Not found.
	return boost::none;
}


void
GPlatesModel::GpgimProperty::set_default_structural_type(
		unsigned int default_structural_type_index)
{
	// Should have at least one structural type.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			!d_structural_types.empty() &&
				default_structural_type_index < d_structural_types.size(),
			GPLATES_ASSERTION_SOURCE);

	// Move the default structural type to the beginning of the structural types sequence.
	if (default_structural_type_index != 0)
	{
		const GpgimStructuralType::non_null_ptr_to_const_type default_structural_type =
				d_structural_types[default_structural_type_index];
		d_structural_types.erase(d_structural_types.begin() + default_structural_type_index);
		d_structural_types.insert(d_structural_types.begin(), default_structural_type);
	}
}


void
GPlatesModel::GpgimProperty::set_has_geometry_structural_type()
{
	// Iterate over the structural types.
	BOOST_FOREACH(
			const GPlatesModel::GpgimStructuralType::non_null_ptr_to_const_type &structural_type,
			d_structural_types)
	{
		if (structural_type->is_geometry_structural_type())
		{
			d_has_geometry_structural_type = true;
			return;
		}
	}

	d_has_geometry_structural_type = false;
}
