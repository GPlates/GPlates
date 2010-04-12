/* $Id$ */

/**
 * \file 
 * Contains the implementation of the class PropertyValue.
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

#include "PropertyValue.h"


GPlatesModel::PropertyValue::instance_id_type
GPlatesModel::PropertyValue::s_next_instance_id = instance_id_type();


bool
GPlatesModel::PropertyValue::operator==(
		const PropertyValue &other) const
{
	// Note: this does not behave as a true operator==.
	// It will just check if one PropertyValue is an unmodified clone of the other
	// PropertyValue.
	// This suffices for what we're using operator== for: seeing if we should
	// check in a clone of PropertyValue back into the model.
	return d_instance_id == other.d_instance_id &&
		directly_modifiable_fields_equal(other);
}


std::ostream &
GPlatesModel::operator<<(
		std::ostream &os,
		const PropertyValue &property_value)
{
	return property_value.print_to(os);
}

