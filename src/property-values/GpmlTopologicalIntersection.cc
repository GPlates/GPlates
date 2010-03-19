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

#include "GpmlTopologicalIntersection.h"


const GPlatesPropertyValues::GpmlTopologicalIntersection
GPlatesPropertyValues::GpmlTopologicalIntersection::deep_clone() const
{
	GpmlTopologicalIntersection dup(*this);

	GpmlPropertyDelegate::non_null_ptr_type cloned_intersection_geometry =
			d_intersection_geometry->deep_clone();
	dup.d_intersection_geometry = cloned_intersection_geometry;

	GmlPoint::non_null_ptr_type cloned_reference_point = d_reference_point->deep_clone();
	dup.d_reference_point = cloned_reference_point;

	GpmlPropertyDelegate::non_null_ptr_type cloned_reference_point_plate_id =
			d_reference_point_plate_id->deep_clone();
	dup.d_reference_point_plate_id = cloned_reference_point_plate_id;

	return dup;
}
