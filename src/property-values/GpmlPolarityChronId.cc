/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date $
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

#include "GpmlPolarityChronId.h"


void
GPlatesPropertyValues::GpmlPolarityChronId::set_era(
		const QString &era)
{
	MutableRevisionHandler revision_handler(this);
	revision_handler.get_mutable_revision<Revision>().era = era;
	revision_handler.handle_revision_modification();
}


void
GPlatesPropertyValues::GpmlPolarityChronId::set_major_region(
		unsigned int major_region)
{
	MutableRevisionHandler revision_handler(this);
	revision_handler.get_mutable_revision<Revision>().major_region = major_region;
	revision_handler.handle_revision_modification();
}


void
GPlatesPropertyValues::GpmlPolarityChronId::set_minor_region(
		const QString &minor_region)
{
	MutableRevisionHandler revision_handler(this);
	revision_handler.get_mutable_revision<Revision>().minor_region = minor_region;
	revision_handler.handle_revision_modification();
}


std::ostream &
GPlatesPropertyValues::GpmlPolarityChronId::print_to(
		std::ostream &os) const
{
	const Revision &revision = get_current_revision<Revision>();

	if (revision.era)
	{
		os << revision.era->toStdString() << " ";
	}
	if (revision.major_region)
	{
		os << *revision.major_region << " ";
	}
	if (revision.minor_region)
	{
		os << revision.minor_region->toStdString();
	}

	return os;
}

