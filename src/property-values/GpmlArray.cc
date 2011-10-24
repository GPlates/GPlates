/**
 * @file
 *
 * Most recent change:
 *   $Date: 2010-11-12 03:12:10 +1100 (Fri, 12 Nov 2010) $
 *
 * Copyright (C) 2010 Geological Survey of Norway
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

#include <QDebug>
#include <boost/lambda/lambda.hpp>

#include "GpmlArray.h"

#include "model/PropertyValue.h"

const GPlatesPropertyValues::GpmlArray::non_null_ptr_type
GPlatesPropertyValues::GpmlArray::deep_clone() const
{
	GpmlArray::non_null_ptr_type dup = clone();

	// Now we need to clear the property value vector in the duplicate, before we push-back the
	// cloned property values.
	dup->d_members.clear();

	std::vector<GPlatesModel::PropertyValue::non_null_ptr_type>::const_iterator iter, end = d_members.end();
	for (iter = d_members.begin(); iter != end; ++iter) {
		dup->d_members.push_back((*iter)->deep_clone_as_prop_val());
	}

    return dup;
}

std::ostream &
GPlatesPropertyValues::GpmlArray::print_to(
                std::ostream &os) const
{
        return os;
}

bool
GPlatesPropertyValues::GpmlArray::directly_modifiable_fields_equal(
	const PropertyValue &other) const
{

	try
	{
		const GpmlArray &other_casted =
			dynamic_cast<const GpmlArray &>(other);

		std::vector<GPlatesModel::PropertyValue::non_null_ptr_type>::const_iterator
			iter = d_members.begin(),
			end = d_members.end(),
			o_iter = other_casted.d_members.begin();
			//o_end = other_casted.d_members.end();

		if (d_members.size() != other_casted.d_members.size())
		{
		    return false;
		}
		for (; iter != end ; ++iter, ++o_iter)
		{
		    if (! (**iter == **o_iter))
		    {
			return false;
		    }
		}
		return true;

	}
	catch (const std::bad_cast &)
	{
		// Should never get here, but doesn't hurt to check.
		return false;
	}

}

