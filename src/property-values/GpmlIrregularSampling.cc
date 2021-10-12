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
#include <typeinfo>

#include "GpmlIrregularSampling.h"


namespace
{
	bool
	maybe_null_ptr_eq(
			const GPlatesPropertyValues::GpmlInterpolationFunction::maybe_null_ptr_type &p1,
			const GPlatesPropertyValues::GpmlInterpolationFunction::maybe_null_ptr_type &p2)
	{
		if (p1)
		{
			if (!p2)
			{
				return false;
			}
			return *p1 == *p2;
		}
		else
		{
			return !p2;
		}
	}
}


const GPlatesPropertyValues::GpmlIrregularSampling::non_null_ptr_type
GPlatesPropertyValues::GpmlIrregularSampling::deep_clone() const
{
	GpmlIrregularSampling::non_null_ptr_type dup = clone();

	// Now we need to clear the time sample vector in the duplicate, before we push-back the
	// cloned time samples.
	dup->d_time_samples.clear();
	std::vector<GpmlTimeSample>::const_iterator iter, end = d_time_samples.end();
	for (iter = d_time_samples.begin(); iter != end; ++iter) {
		dup->d_time_samples.push_back((*iter).deep_clone());
	}

	if (d_interpolation_function) {
		GpmlInterpolationFunction::non_null_ptr_type cloned_interpolation_function =
				d_interpolation_function->deep_clone_as_interp_func();
		dup->d_interpolation_function =
				GpmlInterpolationFunction::maybe_null_ptr_type(
						cloned_interpolation_function.get());
	}

	return dup;
}


std::ostream &
GPlatesPropertyValues::GpmlIrregularSampling::print_to(
		std::ostream &os) const
{
	// FIXME: Implement properly when actually needed for debugging.
	return os << "{ GpmlIrregularSampling }";
}


bool
GPlatesPropertyValues::GpmlIrregularSampling::directly_modifiable_fields_equal(
		const GPlatesModel::PropertyValue &other) const
{
	try
	{
		const GpmlIrregularSampling &other_casted =
			dynamic_cast<const GpmlIrregularSampling &>(other);
		return d_time_samples == other_casted.d_time_samples &&
			maybe_null_ptr_eq(d_interpolation_function, other_casted.d_interpolation_function);
	}
	catch (const std::bad_cast &)
	{
		// Should never get here, but doesn't hurt to check.
		return false;
	}
}

