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

#include "scribe/Scribe.h"


const GPlatesPropertyValues::StructuralType
GPlatesPropertyValues::GpmlRasterBandNames::STRUCTURAL_TYPE = GPlatesPropertyValues::StructuralType::create_gpml("RasterBandNames");


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


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GpmlRasterBandNames::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<GpmlRasterBandNames> &gpml_raster_band_names)
{
	if (scribe.is_saving())
	{
		scribe.save(TRANSCRIBE_SOURCE, gpml_raster_band_names->get_band_names(), "band_names");
	}
	else // loading
	{
		band_names_list_type band_names_;
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, band_names_, "band_names"))
		{
			return scribe.get_transcribe_result();
		}

		// Create the property value.
		gpml_raster_band_names.construct_object(band_names_.begin(), band_names_.end());
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GpmlRasterBandNames::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!transcribed_construct_data)
	{
		if (scribe.is_saving())
		{
			scribe.save(TRANSCRIBE_SOURCE, get_band_names(), "band_names");
		}
		else // loading
		{
			band_names_list_type band_names_;
			if (!scribe.transcribe(TRANSCRIBE_SOURCE, band_names_, "band_names"))
			{
				return scribe.get_transcribe_result();
			}

			set_band_names(band_names_);
		}
	}

	// Record base/derived inheritance relationship.
	if (!scribe.transcribe_base<GPlatesModel::PropertyValue, GpmlRasterBandNames>(TRANSCRIBE_SOURCE))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GpmlRasterBandNames::BandName::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<BandName> &band_name)
{
	if (scribe.is_saving())
	{
		scribe.save(TRANSCRIBE_SOURCE, band_name->get_name(), "name");
	}
	else // loading
	{
		GPlatesScribe::LoadRef<XsString::non_null_ptr_type> name_ =
				scribe.load<XsString::non_null_ptr_type>(TRANSCRIBE_SOURCE, "name");
		if (!name_.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		band_name.construct_object(name_);
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}

GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GpmlRasterBandNames::BandName::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!transcribed_construct_data)
	{
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_name, "name"))
		{
			return scribe.get_transcribe_result();
		}
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
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

	return PropertyValue::Revision::equality(other);
}
