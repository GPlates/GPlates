/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
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

#include "ReconstructScalarCoverageParams.h"

#include "scribe/Scribe.h"


bool
GPlatesAppLogic::ReconstructScalarCoverageParams::operator==(
		const ReconstructScalarCoverageParams &rhs) const
{
	return true;
}


bool
GPlatesAppLogic::ReconstructScalarCoverageParams::operator<(
		const ReconstructScalarCoverageParams &rhs) const
{
	return false;
}


GPlatesScribe::TranscribeResult
GPlatesAppLogic::ReconstructScalarCoverageParams::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	// Provide default values for failed parameters instead of returning failure.
	// This way a future version of GPlates can add or remove parameters and still be backward/forward compatible.
	//static const ReconstructScalarCoverageParams DEFAULT_PARAMS;

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}
