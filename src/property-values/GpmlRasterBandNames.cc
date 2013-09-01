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

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/NotYetImplementedException.h"

#include "model/BubbleUpRevisionHandler.h"
#include "model/ModelTransaction.h"


void
GPlatesPropertyValues::GpmlRasterBandNames::set_band_names(
		const band_names_list_type &band_names_)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().band_names = band_names_;
	revision_handler.commit();
}


std::ostream &
GPlatesPropertyValues::GpmlRasterBandNames::print_to(
		std::ostream &os) const
{
	const band_names_list_type &band_names = get_band_names();

	os << "[ ";

	for (band_names_list_type::const_iterator band_names_iter = band_names.begin();
		band_names_iter != band_names.end();
		++band_names_iter)
	{
		os << band_names_iter->get_name().get();
	}

	return os << " ]";
}


GPlatesModel::Revision::non_null_ptr_type
GPlatesPropertyValues::GpmlRasterBandNames::bubble_up(
		GPlatesModel::ModelTransaction &transaction,
		const Revisionable::non_null_ptr_to_const_type &child_revisionable)
{
	// Currently this can't be reached because we don't attach to our children yet.
	throw GPlatesGlobal::NotYetImplementedException(GPLATES_EXCEPTION_SOURCE);
}


bool
GPlatesPropertyValues::GpmlRasterBandNames::Revision::equality(
		const GPlatesModel::Revision &other) const
{
	const Revision &other_revision = dynamic_cast<const Revision &>(other);

	if (band_names.size() != other_revision.band_names.size())
	{
		return false;
	}

	for (unsigned int n = 0; n < band_names.size(); ++n)
	{
		if (band_names[n] != other_revision.band_names[n])
		{
			return false;
		}
	}

	return GPlatesModel::Revision::equality(other);
}
