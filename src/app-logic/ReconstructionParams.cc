/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2019 The University of Sydney, Australia
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

#include "ReconstructionParams.h"

#include "scribe/Scribe.h"


GPlatesAppLogic::ReconstructionParams::ReconstructionParams() :
	d_extend_total_reconstruction_poles_to_distant_past(false)
{
}


bool
GPlatesAppLogic::ReconstructionParams::operator==(
		const ReconstructionParams &rhs) const
{
	return d_extend_total_reconstruction_poles_to_distant_past == rhs.d_extend_total_reconstruction_poles_to_distant_past;
}


bool
GPlatesAppLogic::ReconstructionParams::operator<(
		const ReconstructionParams &rhs) const
{
	if (d_extend_total_reconstruction_poles_to_distant_past < rhs.d_extend_total_reconstruction_poles_to_distant_past)
	{
		return true;
	}
	if (d_extend_total_reconstruction_poles_to_distant_past > rhs.d_extend_total_reconstruction_poles_to_distant_past)
	{
		return false;
	}

	return false;
}


GPlatesScribe::TranscribeResult
GPlatesAppLogic::ReconstructionParams::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	// Provide default values for failed parameters instead of returning failure.
	// This way a future version of GPlates can add or remove parameters and still be backward/forward compatible.
	static const ReconstructionParams DEFAULT_PARAMS;

	if (!scribe.transcribe(TRANSCRIBE_SOURCE,
			d_extend_total_reconstruction_poles_to_distant_past,
			"extend_total_reconstruction_poles_to_distant_past"))
	{
		d_extend_total_reconstruction_poles_to_distant_past = DEFAULT_PARAMS.d_extend_total_reconstruction_poles_to_distant_past;
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}
