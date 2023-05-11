/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2010 The University of Sydney, Australia
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

#include "GmlMultiPoint.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "maths/MultiPointOnSphere.h"

#include "model/BubbleUpRevisionHandler.h"

#include "scribe/Scribe.h"


const GPlatesPropertyValues::StructuralType
GPlatesPropertyValues::GmlMultiPoint::STRUCTURAL_TYPE = GPlatesPropertyValues::StructuralType::create_gml("MultiPoint");


const GPlatesPropertyValues::GmlMultiPoint::non_null_ptr_type
GPlatesPropertyValues::GmlMultiPoint::create(
		const multipoint_type &multipoint_,
		const std::vector<GmlPoint::GmlProperty> &gml_properties_)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			multipoint_->number_of_points() == gml_properties_.size(),
			GPLATES_ASSERTION_SOURCE);

	// Because MultiPointOnSphere can only ever be handled via a non_null_ptr_to_const_type,
	// there is no way a MultiPointOnSphere instance can be changed.  Hence, it is safe to store
	// a pointer to the instance which was passed into this 'create' function.
	return GmlMultiPoint::non_null_ptr_type(new GmlMultiPoint(multipoint_, gml_properties_));
}


void
GPlatesPropertyValues::GmlMultiPoint::set_multipoint(
		const multipoint_type &p)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);

	Revision &revision = revision_handler.get_revision<Revision>();

	revision.multipoint = p;
	revision.fill_gml_properties();

	revision_handler.commit();
}


void
GPlatesPropertyValues::GmlMultiPoint::set_gml_properties(
		const std::vector<GmlPoint::GmlProperty> &gml_properties_)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);

	Revision &revision = revision_handler.get_revision<Revision>();

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			revision.multipoint->number_of_points() == gml_properties_.size(),
			GPLATES_ASSERTION_SOURCE);

	revision.gml_properties = gml_properties_;

	revision_handler.commit();
}


std::ostream &
GPlatesPropertyValues::GmlMultiPoint::print_to(
		std::ostream &os) const
{
	// FIXME: Implement properly when actually needed for debugging.
	return os << "{ GmlMultiPoint }";
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GmlMultiPoint::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<GmlMultiPoint> &gml_multi_point)
{
	if (scribe.is_saving())
	{
		scribe.save(TRANSCRIBE_SOURCE, gml_multi_point->get_multipoint(), "multipoint");
		scribe.save(TRANSCRIBE_SOURCE, gml_multi_point->get_gml_properties(), "gml_properties");
	}
	else // loading
	{
		GPlatesScribe::LoadRef<GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type> multi_point_ =
				scribe.load<GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type>(TRANSCRIBE_SOURCE, "multipoint");
		if (!multi_point_.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		std::vector<GmlPoint::GmlProperty> gml_properties_;
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, gml_properties_, "gml_properties"))
		{
			// Failed to load GmlProperty's (eg, a future GPlates might have removed them).
			// Just leave as the default (by using constructor with no GmlProperty's passed in).
			gml_multi_point.construct_object(multi_point_);

			return GPlatesScribe::TRANSCRIBE_SUCCESS;
		}

		// Create the property value.
		gml_multi_point.construct_object(multi_point_, gml_properties_);
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GmlMultiPoint::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!transcribed_construct_data)
	{
		if (scribe.is_saving())
		{
			scribe.save(TRANSCRIBE_SOURCE, get_multipoint(), "multipoint");
			scribe.save(TRANSCRIBE_SOURCE, get_gml_properties(), "gml_properties");
		}
		else // loading
		{
			GPlatesScribe::LoadRef<GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type> multi_point_ =
					scribe.load<GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type>(TRANSCRIBE_SOURCE, "multipoint");
			if (!multi_point_.is_valid())
			{
				return scribe.get_transcribe_result();
			}

			// Set the multi-point.
			//
			// Note: This also sets all points to use POS as their GmlProperty property.
			set_multipoint(multi_point_);

			std::vector<GmlPoint::GmlProperty> gml_properties_;
			if (!scribe.transcribe(TRANSCRIBE_SOURCE, gml_properties_, "gml_properties"))
			{
				// Failed to load GmlProperty's (eg, a future GPlates might have removed them).
				// Just leave as the default (set by 'set_multipoint()' above).
			}
			else
			{
				// GmlProperty's exist in transcription.
				set_gml_properties(gml_properties_);
			}
		}
	}

	// Record base/derived inheritance relationship.
	if (!scribe.transcribe_base<GPlatesModel::PropertyValue, GmlMultiPoint>(TRANSCRIBE_SOURCE))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


void
GPlatesPropertyValues::GmlMultiPoint::Revision::fill_gml_properties()
{
	gml_properties.clear();

	std::size_t number_of_points = multipoint->number_of_points();
	gml_properties.reserve(number_of_points);

	for (std::size_t i = 0; i != number_of_points; ++i)
	{
		gml_properties.push_back(GmlPoint::POS);
	}
}

