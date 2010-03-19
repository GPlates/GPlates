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

#include "GpmlPiecewiseAggregation.h"


const GPlatesPropertyValues::GpmlPiecewiseAggregation::non_null_ptr_type
GPlatesPropertyValues::GpmlPiecewiseAggregation::deep_clone() const
{
	GpmlPiecewiseAggregation::non_null_ptr_type dup = clone();

	// Now we need to clear the time sample vector in the duplicate, before we push-back the
	// cloned time samples.
	dup->d_time_windows.clear();
	std::vector<GpmlTimeWindow>::const_iterator iter, end = d_time_windows.end();
	for (iter = d_time_windows.begin(); iter != end; ++iter) {
		dup->d_time_windows.push_back((*iter).deep_clone());
	}

	return dup;
}
