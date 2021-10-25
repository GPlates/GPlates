/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#ifndef GPLATES_SCRIBE_TRANSCRIBECONTEXT_H
#define GPLATES_SCRIBE_TRANSCRIBECONTEXT_H

namespace GPlatesScribe
{
	/**
	 * The default transcribe context for object type 'ObjectType'.
	 *
	 * This class should be specialised if a particular object type requires extra information
	 * (not transcribed to the archive) in order to transcribe objects of that type.
	 *
	 * This is usually the case when loading an object from an archive and that object's constructor
	 * references objects that are *not* transcribed to the archive.
	 * In this case a transcribe context object can be created at some point during loading that
	 * contains this extra information.
	 */
	template <typename ObjectType>
	class TranscribeContext
	{
	};
}

#endif // GPLATES_SCRIBE_TRANSCRIBECONTEXT_H
