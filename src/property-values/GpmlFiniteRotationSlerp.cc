/**
 * Copyright (C) 2023 The University of Sydney, Australia
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

#include "GpmlFiniteRotationSlerp.h"

#include "model/BubbleUpRevisionHandler.h"
#include "model/TranscribeQualifiedXmlName.h"

#include "scribe/Scribe.h"


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GpmlFiniteRotationSlerp::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<GpmlFiniteRotationSlerp> &gpml_finite_rotation_slerp)
{
	if (scribe.is_saving())
	{
		scribe.save(TRANSCRIBE_SOURCE, gpml_finite_rotation_slerp->get_value_type(), "value_type");
	}
	else // loading
	{
		GPlatesScribe::LoadRef<GPlatesPropertyValues::StructuralType> value_type_ =
				scribe.load<GPlatesPropertyValues::StructuralType>(TRANSCRIBE_SOURCE, "value_type");
		if (!value_type_.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		// Create the property value.
		gpml_finite_rotation_slerp.construct_object(value_type_);
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GpmlFiniteRotationSlerp::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!transcribed_construct_data)
	{
		if (scribe.is_saving())
		{
			scribe.save(TRANSCRIBE_SOURCE, get_value_type(), "value_type");
		}
		else // loading
		{
			GPlatesScribe::LoadRef<GPlatesPropertyValues::StructuralType> value_type_ =
					scribe.load<GPlatesPropertyValues::StructuralType>(TRANSCRIBE_SOURCE, "value_type");
			if (!value_type_.is_valid())
			{
				return scribe.get_transcribe_result();
			}

			// Modify 'this' GpmlFiniteRotationSlerp object.
			//
			// There's no set method for assigning revisioned value type.
			// So we do the equivalent inline here.
			GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
			Revision &revision = revision_handler.get_revision<Revision>();

			// Set the value type.
			revision.value_type = value_type_;

			revision_handler.commit();
		}
	}

	// Transcribe base class.
	if (!scribe.transcribe_base<GpmlInterpolationFunction>(TRANSCRIBE_SOURCE, *this, "GpmlInterpolationFunction"))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}
