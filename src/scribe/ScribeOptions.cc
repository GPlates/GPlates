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

#include "ScribeOptions.h"

#include "ScribeExceptions.h"

#include "global/GPlatesAssert.h"


GPlatesScribe::Options::Options(
		OptionFlag flag) :
	d_options(flag.get()) // Flags occupy low 16 bits.
{
	// Make sure flag doesn't exceed 16-bits...
	GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
			(flag.get() & 0xffff0000) == 0,
			GPLATES_ASSERTION_SOURCE,
			"Option flag exceeds 16-bits.");
}


GPlatesScribe::Options::Options(
		Version version) :
	d_options(version.get() << 16) // Version occupies high 16 bits.
{
	// Make sure version doesn't exceed 16-bits...
	GPlatesGlobal::Assert<Exceptions::ScribeUserError>(
			(version.get() & 0xffff0000) == 0,
			GPLATES_ASSERTION_SOURCE,
			"Version number exceeds 16-bits.");
}


GPlatesScribe::Options
GPlatesScribe::operator,(
		const Options &lhs,
		const Options &rhs)
{
	Options combined_options;

	// Make sure two non-zero versions are not specified...
	GPlatesGlobal::Assert<Exceptions::ScribeUserError>(
			(lhs.get_version() == 0) || (rhs.get_version() == 0),
			GPLATES_ASSERTION_SOURCE,
			"Attempted to combine two options with non-zero versions - only one can be non-zero.");

	// This will combine the options flags and select the sole non-zero version (if any).
	combined_options.d_options = lhs.d_options | rhs.d_options;

	return combined_options;
}
