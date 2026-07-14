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

	// Transcribe the object, and its entire subtree of child objects, into a single raw stream
	// of bytes (the "raw lane") instead of the usual one-transcription-object-per-child encoding.
	//
	// This is much faster (no per-object ids, tags or tracking inside the subtree) but the
	// resulting stream is *positional* (order-defined): the same transcribe calls must be made,
	// in the same order, on the load path as on the save path. There is no per-field tag lookup,
	// so fields cannot be individually skipped, reordered or probed on load - any evolution of
	// the transcribed layout must be gated by a version transcribed at the front of the stream.
	//
	// Object references (Scribe::save_reference/load_reference) and *non-owning* pointers are
	// not supported inside a raw subtree (they require object tracking) - attempting to
	// transcribe them will throw Exceptions::InvalidRawTranscribeOperation.
	//
	// This option only has an effect at the *boundary* (outer-most) transcribe call - it is
	// ignored on transcribe calls inside a raw subtree (everything inside is already raw).
	// The boundary object itself is transcribed normally (object id, tag, optional tracking),
	// so on the *load* path the transcription kind of the boundary object determines whether
	// the subtree is loaded via the raw lane (RAW_STREAM) or the general path (eg, an archive
	// written by an older version without this option) - clients transcribing with RAW can
	// therefore still load archives that were saved without it.
	const unsigned int RAW = (1 << 3);
}

#endif // GPLATES_SCRIBE_SCRIBEOPTIONS_H
