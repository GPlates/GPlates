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

#include "GpmlRevisionId.h"

#include "scribe/Scribe.h"


const GPlatesPropertyValues::StructuralType
GPlatesPropertyValues::GpmlRevisionId::STRUCTURAL_TYPE = GPlatesPropertyValues::StructuralType::create_gpml("revisionId");


std::ostream &
GPlatesPropertyValues::GpmlRevisionId::print_to(
		std::ostream &os) const
{
	return os << d_value.get();
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GpmlRevisionId::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<GpmlRevisionId> &gpml_plate_id)
{
	if (scribe.is_saving())
	{
		scribe.save(TRANSCRIBE_SOURCE, gpml_plate_id->d_value, "value");
	}
	else // loading
	{
		GPlatesScribe::LoadRef<GPlatesModel::RevisionId> value_ =
				scribe.load<GPlatesModel::RevisionId>(TRANSCRIBE_SOURCE, "value");
		if (!value_.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		// Create the property value.
		gpml_plate_id.construct_object(value_);
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GpmlRevisionId::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!transcribed_construct_data)
	{
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_value, "value"))
		{
			return scribe.get_transcribe_result();
		}
	}

	// Record base/derived inheritance relationship.
	if (!scribe.transcribe_base<GPlatesModel::PropertyValue, GpmlRevisionId>(TRANSCRIBE_SOURCE))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}
