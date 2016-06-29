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

#ifndef GPLATES_SCRIBE_SCRIBEOPTIONS_H
#define GPLATES_SCRIBE_SCRIBEOPTIONS_H


namespace GPlatesScribe
{
	//
	// You can combine option flags using the OR('|') operator as in the following example...
	//
	//    scribe.transcribe(
	//            TRANSCRIBE_SOURCE,
	//            my_object,
	//            "my_object",
	//            GPlatesScribe::EXCLUSIVE_OWNER | GPlatesScribe::TRACK);
	//

	// Objects are *not* tracked by default - use this option to request tracking on an object...
	const unsigned int TRACK = (1 << 0);

	// A pointer can optionally specify that it exclusively owns the pointed-to object
	// (only applies to pointers)...
	const unsigned int EXCLUSIVE_OWNER = (1 << 1);

	// A pointer can optionally specify that it shares ownership of the pointed-to object with other pointers
	// (only applies to pointers)...
	const unsigned int SHARED_OWNER = (1 << 2);
}

#endif // GPLATES_SCRIBE_SCRIBEOPTIONS_H
