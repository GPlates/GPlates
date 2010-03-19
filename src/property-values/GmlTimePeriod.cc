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

#include "GmlTimePeriod.h"


const GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type
GPlatesPropertyValues::GmlTimePeriod::deep_clone() const
{
	GmlTimePeriod::non_null_ptr_type dup = clone();

	GmlTimeInstant::non_null_ptr_type cloned_begin = d_begin->deep_clone();
	dup->d_begin = cloned_begin;
	GmlTimeInstant::non_null_ptr_type cloned_end = d_end->deep_clone();
	dup->d_end = cloned_end;

	return dup;
}
