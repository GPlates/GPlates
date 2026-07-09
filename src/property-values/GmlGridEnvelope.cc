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

#include "GmlGridEnvelope.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "model/BubbleUpRevisionHandler.h"

#include "scribe/Scribe.h"


const GPlatesPropertyValues::StructuralType
GPlatesPropertyValues::GmlGridEnvelope::STRUCTURAL_TYPE = GPlatesPropertyValues::StructuralType::create_gml("GridEnvelope");


const GPlatesPropertyValues::GmlGridEnvelope::non_null_ptr_type
GPlatesPropertyValues::GmlGridEnvelope::create(
		const integer_list_type &low_,
		const integer_list_type &high_)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			low_.size() == high_.size(), GPLATES_ASSERTION_SOURCE);

	return non_null_ptr_type(new GmlGridEnvelope(low_, high_));
}


void
GPlatesPropertyValues::GmlGridEnvelope::set_low_and_high(
		const integer_list_type &low_,
		const integer_list_type &high_)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			low_.size() == high_.size(), GPLATES_ASSERTION_SOURCE);

	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);

	Revision &revision = revision_handler.get_revision<Revision>();

	revision.low = low_;
	revision.high = high_;

	revision_handler.commit();
}


std::ostream &
GPlatesPropertyValues::GmlGridEnvelope::print_to(
		std::ostream &os) const
{
	const Revision &revision = get_current_revision<Revision>();

	os << "{ ";

	BOOST_FOREACH(int d, revision.low)
	{
		os << d << " ";
	}

	os << "} { ";

	BOOST_FOREACH(int d, revision.high)
	{
		os << d << " ";
	}

	os << "}";

	return os;
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GmlGridEnvelope::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<GmlGridEnvelope> &gml_grid_envelope)
{
	if (scribe.is_saving())
	{
		scribe.save(TRANSCRIBE_SOURCE, gml_grid_envelope->get_low(), "low");
		scribe.save(TRANSCRIBE_SOURCE, gml_grid_envelope->get_high(), "high");
	}
	else // loading
	{
		integer_list_type low_;
		integer_list_type high_;
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, low_, "low") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, high_, "high"))
		{
			return scribe.get_transcribe_result();
		}

		// Create the property value.
		gml_grid_envelope.construct_object(low_, high_);
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GmlGridEnvelope::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!transcribed_construct_data)
	{
		if (scribe.is_saving())
		{
			scribe.save(TRANSCRIBE_SOURCE, get_low(), "low");
			scribe.save(TRANSCRIBE_SOURCE, get_high(), "high");
		}
		else // loading
		{
			integer_list_type low_;
			integer_list_type high_;
			if (!scribe.transcribe(TRANSCRIBE_SOURCE, low_, "low") ||
				!scribe.transcribe(TRANSCRIBE_SOURCE, high_, "high"))
			{
				return scribe.get_transcribe_result();
			}

			set_low_and_high(low_, high_);
		}
	}

	// Record base/derived inheritance relationship.
	if (!scribe.transcribe_base<GPlatesModel::PropertyValue, GmlGridEnvelope>(TRANSCRIBE_SOURCE))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}
