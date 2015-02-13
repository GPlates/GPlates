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

#ifndef GPLATES_SCRIBE_SCRIBEARCHIVEREADER_H
#define GPLATES_SCRIBE_SCRIBEARCHIVEREADER_H

#include <string>

#include "ScribeArchiveCommon.h"
#include "Transcription.h"

#include "utils/ReferenceCount.h"


namespace GPlatesScribe
{
	/**
	 * Base class for all scribe archive readers.
	 */
	class ArchiveReader :
			public GPlatesUtils::ReferenceCount<ArchiveReader>
	{
	public:

		// Convenience typedefs for a shared pointer to a @a ArchiveReader.
		typedef GPlatesUtils::non_null_intrusive_ptr<ArchiveReader> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const ArchiveReader> non_null_ptr_to_const_type;


		virtual
		~ArchiveReader()
		{  }


		/**
		 * Reads a @a Transcription from the archive.
		 *
		 * Note that multiple transcriptions can be read consecutively from the archive
		 * (depending on how many were written to the archive).
		 */
		virtual
		Transcription::non_null_ptr_type
		read_transcription() = 0;


		/**
		 * Close the archive.
		 *
		 * Any final reads, after all transcriptions have been read from the archive, are done here.
		 *
		 * Call this method when you have read all transcriptions and want the archive reader
		 * to check that it has reached the end of the archive.
		 *
		 * NOTE: If you are not reading all transcription in the archive then do not call this method
		 * otherwise an exception might get thrown depending on the archive type.
		 * Also note that this method is never called in the archive reader's destructor.
		 */
		virtual
		void
		close() = 0;

	};
}

#endif // GPLATES_SCRIBE_SCRIBEARCHIVEREADER_H
