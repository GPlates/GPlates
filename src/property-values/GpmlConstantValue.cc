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

#include <iostream>

#include "GpmlConstantValue.h"


void
GPlatesPropertyValues::GpmlConstantValue::set_value(
		GPlatesModel::PropertyValue::non_null_ptr_type v)
{
	MutableRevisionHandler revision_handler(this);
	revision_handler.get_mutable_revision<Revision>().value = v;
	revision_handler.handle_revision_modification();
}


void
GPlatesPropertyValues::GpmlConstantValue::set_description(
		const GPlatesUtils::UnicodeString &new_description)
{
	MutableRevisionHandler revision_handler(this);
	revision_handler.get_mutable_revision<Revision>().description = new_description;
	revision_handler.handle_revision_modification();
}


std::ostream &
GPlatesPropertyValues::GpmlConstantValue::print_to(
		std::ostream &os) const
{
	return os << get_current_revision<Revision>().value;
}
