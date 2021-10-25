/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2016 The University of Sydney, Australia
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

#include "CoRegFilter.h"

#include "scribe/Scribe.h"


GPlatesScribe::TranscribeResult
GPlatesDataMining::DummyFilter::Config::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	// Transcribe abstract base class.
	if (!scribe.transcribe_base<CoRegFilter::Config, Config>(TRANSCRIBE_SOURCE))
	{
		return scribe.get_transcribe_result();
	}

	// Nothing to transcribe.
	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}
