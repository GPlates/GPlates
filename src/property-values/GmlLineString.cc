/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2008, 2010 The University of Sydney, Australia
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

#include "GmlLineString.h"

#include "maths/PolylineOnSphere.h"

#include "model/BubbleUpRevisionHandler.h"

#include "scribe/Scribe.h"


const GPlatesPropertyValues::StructuralType
GPlatesPropertyValues::GmlLineString::STRUCTURAL_TYPE = GPlatesPropertyValues::StructuralType::create_gml("LineString");


void
GPlatesPropertyValues::GmlLineString::set_polyline(
		const polyline_type &p)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().polyline = p;
	revision_handler.commit();
}


std::ostream &
GPlatesPropertyValues::GmlLineString::print_to(
		std::ostream &os) const
{
	// FIXME: Implement properly when actually needed for debugging.
	return os << "{ GmlLineString }";
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GmlLineString::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<GmlLineString> &gml_line_string)
{
	if (scribe.is_saving())
	{
		scribe.save(TRANSCRIBE_SOURCE, gml_line_string->get_polyline(), "polyline");
	}
	else // loading
	{
		GPlatesScribe::LoadRef<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> polyline_ =
				scribe.load<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>(TRANSCRIBE_SOURCE, "polyline");
		if (!polyline_.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		// Create the property value.
		gml_line_string.construct_object(polyline_);
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GmlLineString::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!transcribed_construct_data)
	{
		if (scribe.is_saving())
		{
			scribe.save(TRANSCRIBE_SOURCE, get_polyline(), "polyline");
		}
		else // loading
		{
			GPlatesScribe::LoadRef<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> polyline_ =
					scribe.load<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>(TRANSCRIBE_SOURCE, "polyline");
			if (!polyline_.is_valid())
			{
				return scribe.get_transcribe_result();
			}

			// Set the property value.
			set_polyline(polyline_);
		}
	}

	// Record base/derived inheritance relationship.
	if (!scribe.transcribe_base<GPlatesModel::PropertyValue, GmlLineString>(TRANSCRIBE_SOURCE))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}
