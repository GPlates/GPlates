/* $Id$ */

/**
 * \file .
 * $Revision$
 * $Date$
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

#include "RegionOfInterestFilter.h"

#include "scribe/Scribe.h"


GPlatesScribe::TranscribeResult
GPlatesDataMining::RegionOfInterestFilter::Config::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<Config> &config)
{
	if (scribe.is_saving())
	{
		scribe.save(TRANSCRIBE_SOURCE, config->d_range, "range");
	}
	else // loading...
	{
		GPlatesScribe::LoadRef<double> range_ref =
				scribe.load<double>(TRANSCRIBE_SOURCE, "range");
		if (!range_ref.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		config.construct_object(range_ref);
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesDataMining::RegionOfInterestFilter::Config::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!transcribed_construct_data)
	{
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_range, "range"))
		{
			return scribe.get_transcribe_result();
		}
	}

	// Transcribe abstract base class.
	if (!scribe.transcribe_base<CoRegFilter::Config, Config>(TRANSCRIBE_SOURCE))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}
