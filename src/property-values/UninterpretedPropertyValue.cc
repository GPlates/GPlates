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

#include "UninterpretedPropertyValue.h"


const GPlatesPropertyValues::UninterpretedPropertyValue::non_null_ptr_type
GPlatesPropertyValues::UninterpretedPropertyValue::deep_clone() const
{
	UninterpretedPropertyValue::non_null_ptr_type dup = clone();

	// As a first pass, assign the XmlElementNode pointer directly, since the XmlElementNode is
	// never modified (AFAIK).
	// FIXME:  On the next pass, implement the same recursive "deep-clone" in the XmlNode class
	// hierarchy, and invoke it here, to do this "properly".  Either that, or change the
	// 'GPlatesModel::XmlElementNode::non_null_ptr_type' to
	// 'GPlatesModel::XmlElementNode::non_null_ptr_to_const_type'.

	dup->d_value = d_value;

	return dup;
}
