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
#include <boost/utility/compare_pointees.hpp>

#include "GmlPolygon.h"

#include "model/BubbleUpRevisionHandler.h"


void
GPlatesPropertyValues::GmlPolygon::set_exterior(
		const ring_type &exterior)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().exterior = exterior;
	revision_handler.commit();
}


void
GPlatesPropertyValues::GmlPolygon::set_interiors(
		const ring_sequence_type &interiors)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().interiors = interiors;
	revision_handler.commit();
}


std::ostream &
GPlatesPropertyValues::GmlPolygon::print_to(
		std::ostream &os) const
{
	// FIXME: Implement properly when actually needed for debugging.
	return os << "{ GmlPolygon }";
}


bool
GPlatesPropertyValues::GmlPolygon::Revision::equality(
		const GPlatesModel::Revision &other) const
{
	const Revision &other_revision = dynamic_cast<const Revision &>(other);

	if (*exterior != *other_revision.exterior)
	{
		return false;
	}

	if (interiors.size() != other_revision.interiors.size())
	{
		return false;
	}

	for (unsigned int n = 0; n < interiors.size(); ++n)
	{
		// Compare geometries not pointers.
		if (*interiors[n] != *other_revision.interiors[n])
		{
			return false;
		}
	}

	return GPlatesModel::Revision::equality(other);
}
