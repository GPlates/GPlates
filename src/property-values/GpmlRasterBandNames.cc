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

#include "GpmlRasterBandNames.h"


void
GPlatesPropertyValues::GpmlRasterBandNames::set_band_names(
		const band_names_list_type &band_names_)
{
	MutableRevisionHandler revision_handler(this);
	// To keep our revision state immutable we clone the time instant so that the client
	// can no longer modify it indirectly...
	revision_handler.get_mutable_revision<Revision>()
			.set_cloned_band_names(band_names_.begin(), band_names_.end());
	revision_handler.handle_revision_modification();
}


std::ostream &
GPlatesPropertyValues::GpmlRasterBandNames::print_to(
		std::ostream &os) const
{
	return os << "GpmlRasterBandNames";
}


GPlatesPropertyValues::GpmlRasterBandNames::Revision::Revision(
		const Revision &other)
{
	// Clone the band names.
	BOOST_FOREACH(const XsString::non_null_ptr_to_const_type &other_band_name, other.band_names)
	{
		band_names.push_back(other_band_name->clone());
	}
}


bool
GPlatesPropertyValues::GpmlRasterBandNames::Revision::equality(
		const GPlatesModel::PropertyValue::Revision &other) const
{
	const Revision &other_revision = dynamic_cast<const Revision &>(other);

	if (band_names.size() != other_revision.band_names.size())
	{
		return false;
	}

	for (unsigned int n = 0; n < band_names.size(); ++n)
	{
		// Compare PropertyValues, not pointers to PropertyValues...
		if (*band_names[n] != *other_revision.band_names[n])
		{
			return false;
		}
	}

	return GPlatesModel::PropertyValue::Revision::equality(other);
}
