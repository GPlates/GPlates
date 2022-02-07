/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2014 The University of Sydney, Australia
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

#include "TranscribeEnumProtocol.h"

#include "Scribe.h"


//
// These implementation functions are purely to avoid having to include the heavyweight "Scribe.h"
// in the header file (and hence indirectly include it in many other headers).
//


bool
GPlatesScribe::EnumImplementation::is_scribe_saving(
		Scribe &scribe)
{
	return scribe.is_saving();
}


bool
GPlatesScribe::EnumImplementation::is_scribe_loading(
		Scribe &scribe)
{
	return scribe.is_loading();
}


GPlatesScribe::TranscribeResult
GPlatesScribe::EnumImplementation::transcribe_enum_name(
		Scribe &scribe,
		const std::string &enum_name)
{
	// Transcribe the enumeration value's string name.
	if (!scribe.transcribe(TRANSCRIBE_SOURCE, enum_name, "value"))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}
