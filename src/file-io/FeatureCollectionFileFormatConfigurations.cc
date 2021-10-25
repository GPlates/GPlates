/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#include "FeatureCollectionFileFormatConfigurations.h"

#include "scribe/Scribe.h"


GPlatesScribe::TranscribeResult
GPlatesFileIO::FeatureCollectionFileFormat::GMTConfiguration::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_header_format, "d_header_format") ||
		// Transcribe base class - it has no data members so we just register conversion casts
		// between this class and base class.
		!scribe.transcribe_base<Configuration, GMTConfiguration>(TRANSCRIBE_SOURCE))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesFileIO::FeatureCollectionFileFormat::OGRConfiguration::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_wrap_to_dateline, "d_wrap_to_dateline") ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, d_model_to_attribute_map, "d_model_to_attribute_map") ||
		// Transcribe base class - it has no data members so we just register conversion casts
		// between this class and base class.
		!scribe.transcribe_base<Configuration, OGRConfiguration>(TRANSCRIBE_SOURCE))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}
