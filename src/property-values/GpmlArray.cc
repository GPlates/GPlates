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


void
GPlatesPropertyValues::GpmlArray::set_members(
		const member_array_type &members)
{
	MutableRevisionHandler revision_handler(this);
	revision_handler.get_mutable_revision<Revision>().members = members;
	revision_handler.handle_revision_modification();
}


std::ostream &
GPlatesPropertyValues::GpmlArray::print_to(
                std::ostream &os) const
{
	const member_array_type &members = get_members();

	os << "[ ";

	for (member_array_type::const_iterator members_iter = members.begin();
		members_iter != members.end();
		++members_iter)
	{
		os << *members_iter->get_const();
	}

	return os << " ]";
}


bool
GPlatesPropertyValues::GpmlArray::Revision::equality(
		const GPlatesModel::PropertyValue::Revision &other) const
{
	const Revision &other_revision = dynamic_cast<const Revision &>(other);

	if (members.size() != other_revision.members.size())
	{
		return false;
	}

	for (unsigned int n = 0; n < members.size(); ++n)
	{
		// Compare PropertyValues, not pointers to PropertyValues...
		if (*members[n].get_const() != *other_revision.members[n].get_const())
		{
			return false;
		}
	}

	return GPlatesModel::PropertyValue::Revision::equality(other);
}
