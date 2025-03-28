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

#include "XsString.h"

#include "model/BubbleUpRevisionHandler.h"
#include "model/TranscribeStringContentTypeGenerator.h"

#include "scribe/Scribe.h"


const GPlatesPropertyValues::StructuralType
GPlatesPropertyValues::XsString::STRUCTURAL_TYPE = GPlatesPropertyValues::StructuralType::create_xsi("string");


void
GPlatesPropertyValues::XsString::set_value(
		const TextContent &tc)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().value = tc;
	revision_handler.commit();
}


std::ostream &
GPlatesPropertyValues::XsString::print_to(
		std::ostream &os) const
{
	return os << get_current_revision<Revision>().value.get();
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::XsString::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<XsString> &xs_string)
{
	if (scribe.is_saving())
	{
		scribe.save(TRANSCRIBE_SOURCE, xs_string->get_value(), "value");
	}
	else // loading
	{
		GPlatesScribe::LoadRef<GPlatesPropertyValues::TextContent> value =
				scribe.load<GPlatesPropertyValues::TextContent>(TRANSCRIBE_SOURCE, "value");
		if (!value.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		// Create the property value.
		xs_string.construct_object(value);
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::XsString::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!transcribed_construct_data)
	{
		if (scribe.is_saving())
		{
			scribe.save(TRANSCRIBE_SOURCE, get_value(), "value");
		}
		else // loading
		{
			GPlatesScribe::LoadRef<GPlatesPropertyValues::TextContent> value =
					scribe.load<GPlatesPropertyValues::TextContent>(TRANSCRIBE_SOURCE, "value");
			if (!value.is_valid())
			{
				return scribe.get_transcribe_result();
			}

			// Set the property value.
			set_value(value);
		}
	}

	// Record base/derived inheritance relationship.
	if (!scribe.transcribe_base<GPlatesModel::PropertyValue, XsString>(TRANSCRIBE_SOURCE))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}
