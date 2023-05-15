/* $Id$ */

/**
 * \file 
 * Contains the implementation of the class Enumeration.
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

#include "Enumeration.h"

#include "model/BubbleUpRevisionHandler.h"
#include "model/TranscribeQualifiedXmlName.h"
#include "model/TranscribeStringContentTypeGenerator.h"

#include "scribe/Scribe.h"


void
GPlatesPropertyValues::Enumeration::set_value(
		const EnumerationContent &new_value)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().value = new_value;
	revision_handler.commit();
}


std::ostream &
GPlatesPropertyValues::Enumeration::print_to(
		std::ostream &os) const
{
	return os << get_current_revision<Revision>().value.get();
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::Enumeration::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<Enumeration> &enumeration)
{
	if (scribe.is_saving())
	{
		scribe.save(TRANSCRIBE_SOURCE, enumeration->get_type(), "type");
		scribe.save(TRANSCRIBE_SOURCE, enumeration->get_value(), "value");
	}
	else // loading
	{
		GPlatesScribe::LoadRef<EnumerationType> type_ = scribe.load<EnumerationType>(TRANSCRIBE_SOURCE, "type");
		GPlatesScribe::LoadRef<EnumerationContent> value_ = scribe.load<EnumerationContent>(TRANSCRIBE_SOURCE, "value");
		if (!type_.is_valid() ||
			!value_.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		// Create the property value.
		enumeration.construct_object(type_, value_);
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::Enumeration::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!transcribed_construct_data)
	{
		if (scribe.is_saving())
		{
			scribe.save(TRANSCRIBE_SOURCE, get_type(), "type");
			scribe.save(TRANSCRIBE_SOURCE, get_value(), "value");
		}
		else // loading
		{
			GPlatesScribe::LoadRef<EnumerationType> type_ = scribe.load<EnumerationType>(TRANSCRIBE_SOURCE, "type");
			GPlatesScribe::LoadRef<EnumerationContent> value_ = scribe.load<EnumerationContent>(TRANSCRIBE_SOURCE, "value");
			if (!type_.is_valid() ||
				!value_.is_valid())
			{
				return scribe.get_transcribe_result();
			}

			// Set the property value.
			d_type = type_;
			set_value(value_);
		}
	}

	// Record base/derived inheritance relationship.
	if (!scribe.transcribe_base<GPlatesModel::PropertyValue, Enumeration>(TRANSCRIBE_SOURCE))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}
