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
#include <boost/foreach.hpp>

#include "GpmlPiecewiseAggregation.h"


std::vector<GPlatesPropertyValues::GpmlTimeWindow>
GPlatesPropertyValues::GpmlPiecewiseAggregation::get_time_windows() const
{
	// To keep our revision state immutable we clone the time windows so that the client
	// is unable modify them indirectly. If we didn't clone the time windows then the client
	// could copy the returned vector of time windows and then get access to pointers to 'non-const'
	// property values (from a GpmlTimeWindow) and then modify our internal immutable revision state.
	// And returning 'const' GpmlTimeWindows doesn't help us here - so cloning is the only solution.
	std::vector<GpmlTimeWindow> time_windows;

	const Revision &revision = get_current_revision<Revision>();

	// The copy constructor of GpmlTimeWindow does *not* clone its members,
	// instead it has a 'clone()' member function for that...
	BOOST_FOREACH(const GpmlTimeWindow &time_window, revision.time_windows)
	{
		time_windows.push_back(time_window.clone());
	}

	return time_windows;
}


void
GPlatesPropertyValues::GpmlPiecewiseAggregation::set_time_windows(
		const std::vector<GpmlTimeWindow> &time_windows)
{
	MutableRevisionHandler revision_handler(this);
	// To keep our revision state immutable we clone the time windows so that the client
	// can no longer modify them indirectly...
	revision_handler.get_mutable_revision<Revision>().set_cloned_time_windows(time_windows);
	revision_handler.handle_revision_modification();
}


std::ostream &
GPlatesPropertyValues::GpmlPiecewiseAggregation::print_to(
		std::ostream &os) const
{
	const Revision &revision = get_current_revision<Revision>();

	os << "[ ";

	for (std::vector<GpmlTimeWindow>::const_iterator iter = revision.time_windows.begin();
		iter != revision.time_windows.end();
		++iter)
	{
		os << *iter;
	}

	return os << " ]";
}


GPlatesPropertyValues::GpmlPiecewiseAggregation::Revision::Revision(
		const Revision &other)
{
	// The copy constructor of GpmlTimeWindow does *not* clone its members,
	// instead it has a 'clone()' member function for that...
	BOOST_FOREACH(const GpmlTimeWindow &other_time_window, other.time_windows)
	{
		time_windows.push_back(other_time_window.clone());
	}
}


void
GPlatesPropertyValues::GpmlPiecewiseAggregation::Revision::set_cloned_time_windows(
		const std::vector<GpmlTimeWindow> &time_windows_)
{
	// To keep our revision state immutable we clone the time windows so that the client
	// can no longer modify them indirectly...
	time_windows.clear();
	std::vector<GpmlTimeWindow>::const_iterator time_windows_iter_ = time_windows_.begin();
	std::vector<GpmlTimeWindow>::const_iterator time_windows_end_ = time_windows_.end();
	for ( ; time_windows_iter_ != time_windows_end_; ++time_windows_iter_)
	{
		const GpmlTimeWindow &time_window_ = *time_windows_iter_;
		time_windows.push_back(time_window_.clone());
	}
}


bool
GPlatesPropertyValues::GpmlPiecewiseAggregation::Revision::equality(
		const GPlatesModel::PropertyValue::Revision &other) const
{
	const Revision &other_revision = dynamic_cast<const Revision &>(other);

	return time_windows == other_revision.time_windows &&
		GPlatesModel::PropertyValue::Revision::equality(other);
}
