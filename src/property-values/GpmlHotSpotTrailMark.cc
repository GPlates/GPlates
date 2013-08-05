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

#include <boost/utility/compare_pointees.hpp>
#include <iostream>

#include "GpmlHotSpotTrailMark.h"


namespace
{
	template<class T>
	bool
	opt_cow_eq(
			const boost::optional<GPlatesUtils::CopyOnWrite<T> > &opt1,
			const boost::optional<GPlatesUtils::CopyOnWrite<T> > &opt2)
	{
		if (opt1)
		{
			if (!opt2)
			{
				return false;
			}
			return *opt1.get().get_const() == *opt2.get().get_const();
		}
		else
		{
			return !opt2;
		}
	}
}


const GPlatesPropertyValues::GmlPoint::non_null_ptr_to_const_type
GPlatesPropertyValues::GpmlHotSpotTrailMark::get_position() const
{
	return get_current_revision<Revision>().position.get();
}


void
GPlatesPropertyValues::GpmlHotSpotTrailMark::set_position(
		GmlPoint::non_null_ptr_to_const_type pos)
{
	MutableRevisionHandler revision_handler(this);
	revision_handler.get_mutable_revision<Revision>().position = pos;
	revision_handler.handle_revision_modification();
}


const boost::optional<GPlatesPropertyValues::GpmlMeasure::non_null_ptr_to_const_type>
GPlatesPropertyValues::GpmlHotSpotTrailMark::get_trail_width() const
{
	const Revision &revision = get_current_revision<Revision>();

	if (!revision.trail_width)
	{
		return boost::none;
	}

	return revision.trail_width->get();
}


void
GPlatesPropertyValues::GpmlHotSpotTrailMark::set_trail_width(
		GpmlMeasure::non_null_ptr_to_const_type tw)
{
	MutableRevisionHandler revision_handler(this);
	revision_handler.get_mutable_revision<Revision>().trail_width = tw;
	revision_handler.handle_revision_modification();
}


const boost::optional<GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_to_const_type>
GPlatesPropertyValues::GpmlHotSpotTrailMark::get_measured_age() const
{
	const Revision &revision = get_current_revision<Revision>();

	if (!revision.measured_age)
	{
		return boost::none;
	}

	return revision.measured_age->get();
}


void
GPlatesPropertyValues::GpmlHotSpotTrailMark::set_measured_age(
		GmlTimeInstant::non_null_ptr_to_const_type ti)
{
	MutableRevisionHandler revision_handler(this);
	revision_handler.get_mutable_revision<Revision>().measured_age = ti;
	revision_handler.handle_revision_modification();
}


const boost::optional<GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_to_const_type>
GPlatesPropertyValues::GpmlHotSpotTrailMark::get_measured_age_range() const
{
	const Revision &revision = get_current_revision<Revision>();

	if (!revision.measured_age_range)
	{
		return boost::none;
	}

	return revision.measured_age_range->get();
}


void
GPlatesPropertyValues::GpmlHotSpotTrailMark::set_measured_age_range(
		GmlTimePeriod::non_null_ptr_to_const_type tp)
{
	MutableRevisionHandler revision_handler(this);
	revision_handler.get_mutable_revision<Revision>().measured_age_range = tp;
	revision_handler.handle_revision_modification();
}


std::ostream &
GPlatesPropertyValues::GpmlHotSpotTrailMark::print_to(
		std::ostream &os) const
{
	const Revision &revision = get_current_revision<Revision>();

	os << "[ " << *revision.position.get_const() << " , ";
	if (revision.trail_width)
	{
		os << *revision.trail_width->get_const();
	}
	os << " , ";
	if (revision.measured_age)
	{
		os << *revision.measured_age->get_const();
	}
	os << " , ";
	if (revision.measured_age_range)
	{
		os << *revision.measured_age_range->get_const();
	}
	return os << " ]";
}


bool
GPlatesPropertyValues::GpmlHotSpotTrailMark::Revision::equality(
		const GPlatesModel::PropertyValue::Revision &other) const
{
	const Revision &other_revision = dynamic_cast<const Revision &>(other);

	return *position.get_const() == *other_revision.position.get_const() &&
			opt_cow_eq(trail_width, other_revision.trail_width) &&
			opt_cow_eq(measured_age, other_revision.measured_age) &&
			opt_cow_eq(measured_age_range, other_revision.measured_age_range) &&
			GPlatesModel::PropertyValue::Revision::equality(other);
}
