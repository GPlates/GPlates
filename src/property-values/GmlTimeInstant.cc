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

#include "GmlTimeInstant.h"

#include "model/BubbleUpRevisionHandler.h"
#include "model/TranscribeQualifiedXmlName.h"
#include "model/TranscribeStringContentTypeGenerator.h"

#include "scribe/Scribe.h"


const GPlatesPropertyValues::StructuralType
GPlatesPropertyValues::GmlTimeInstant::STRUCTURAL_TYPE = GPlatesPropertyValues::StructuralType::create_gml("TimeInstant");


void
GPlatesPropertyValues::GmlTimeInstant::set_time_position(
		const GeoTimeInstant &tp)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().time_position = tp;
	revision_handler.commit();
}


void
GPlatesPropertyValues::GmlTimeInstant::set_time_position_xml_attributes(
		const xml_attribute_map_type &tpxa)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().time_position_xml_attributes = tpxa;
	revision_handler.commit();
}


std::ostream &
GPlatesPropertyValues::GmlTimeInstant::print_to(
		std::ostream &os) const
{
	return os << get_current_revision<Revision>().time_position;
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GmlTimeInstant::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<GmlTimeInstant> &gml_time_instant)
{
	if (scribe.is_saving())
	{
		scribe.save(TRANSCRIBE_SOURCE, gml_time_instant->get_time_position(), "time");
		scribe.save(TRANSCRIBE_SOURCE, gml_time_instant->get_time_position_xml_attributes(), "xml_attributes");
	}
	else // loading
	{
		GPlatesScribe::LoadRef<GPlatesPropertyValues::GeoTimeInstant> time =
				scribe.load<GPlatesPropertyValues::GeoTimeInstant>(TRANSCRIBE_SOURCE, "time");
		if (!time.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		GPlatesPropertyValues::GmlTimeInstant::xml_attribute_map_type xml_attributes;
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, xml_attributes, "xml_attributes"))
		{
			return scribe.get_transcribe_result();
		}

		// Create the property value.
		gml_time_instant.construct_object(
				time,
				xml_attributes);
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GmlTimeInstant::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!transcribed_construct_data)
	{
		if (scribe.is_saving())
		{
			scribe.save(TRANSCRIBE_SOURCE, get_time_position(), "time");
			scribe.save(TRANSCRIBE_SOURCE, get_time_position_xml_attributes(), "xml_attributes");
		}
		else // loading
		{
			GPlatesScribe::LoadRef<GPlatesPropertyValues::GeoTimeInstant> time =
					scribe.load<GPlatesPropertyValues::GeoTimeInstant>(TRANSCRIBE_SOURCE, "time");
			if (!time.is_valid())
			{
				return scribe.get_transcribe_result();
			}

			GPlatesPropertyValues::GmlTimeInstant::xml_attribute_map_type xml_attributes;
			if (!scribe.transcribe(TRANSCRIBE_SOURCE, xml_attributes, "xml_attributes"))
			{
				return scribe.get_transcribe_result();
			}

			// Set the property value.
			set_time_position(time);
			set_time_position_xml_attributes(xml_attributes);
		}
	}

	// Record base/derived inheritance relationship.
	if (!scribe.transcribe_base<GPlatesModel::PropertyValue, GmlTimeInstant>(TRANSCRIBE_SOURCE))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}
