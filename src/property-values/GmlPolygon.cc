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

#include "scribe/Scribe.h"


const GPlatesPropertyValues::StructuralType
GPlatesPropertyValues::GmlPolygon::STRUCTURAL_TYPE = GPlatesPropertyValues::StructuralType::create_gml("Polygon");


void
GPlatesPropertyValues::GmlPolygon::set_polygon(
		const internal_polygon_type &polygon)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().polygon = polygon;
	revision_handler.commit();
}


std::ostream &
GPlatesPropertyValues::GmlPolygon::print_to(
		std::ostream &os) const
{
	// FIXME: Implement properly when actually needed for debugging.
	return os << "{ GmlPolygon }";
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GmlPolygon::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<GmlPolygon> &gml_polygon)
{
	if (scribe.is_saving())
	{
		scribe.save(TRANSCRIBE_SOURCE, gml_polygon->get_polygon(), "polygon");
	}
	else // loading
	{
		GPlatesScribe::LoadRef<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> polygon_ =
				scribe.load<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type>(TRANSCRIBE_SOURCE, "polygon");
		if (!polygon_.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		// Create the property value.
		gml_polygon.construct_object(polygon_);
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GmlPolygon::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!transcribed_construct_data)
	{
		if (scribe.is_saving())
		{
			scribe.save(TRANSCRIBE_SOURCE, get_polygon(), "polygon");
		}
		else // loading
		{
			GPlatesScribe::LoadRef<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> polygon_ =
					scribe.load<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type>(TRANSCRIBE_SOURCE, "polygon");
			if (!polygon_.is_valid())
			{
				return scribe.get_transcribe_result();
			}

			// Set the property value.
			set_polygon(polygon_);
		}
	}

	// Record base/derived inheritance relationship.
	if (!scribe.transcribe_base<GPlatesModel::PropertyValue, GmlPolygon>(TRANSCRIBE_SOURCE))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


bool
GPlatesPropertyValues::GmlPolygon::Revision::equality(
		const GPlatesModel::Revision &other) const
{
	const Revision &other_revision = dynamic_cast<const Revision &>(other);

	if (*polygon != *other_revision.polygon)
	{
		return false;
	}

	return PropertyValue::Revision::equality(other);
}
