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

#ifndef GPLATES_SCRIBE_SCRIBEARCHIVEWRITER_H
#define GPLATES_SCRIBE_SCRIBEARCHIVEWRITER_H

#include <string>

#include "ScribeArchiveCommon.h"
#include "Transcription.h"

#include "utils/ReferenceCount.h"


namespace GPlatesScribe
{
	/**
	 * Base class for all scribe archive writers.
	 */
	class ArchiveWriter :
			public GPlatesUtils::ReferenceCount<ArchiveWriter>
	{
	public:

		// Convenience typedefs for a shared pointer to a @a ArchiveWriter.
		typedef GPlatesUtils::non_null_intrusive_ptr<ArchiveWriter> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const ArchiveWriter> non_null_ptr_to_const_type;


		virtual
		~ArchiveWriter()
		{  }


		/**
		 * Writes a @a Transcription to the archive.
		 *
		 * Note that multiple transcriptions can be written consecutively to the archive.
		 */
		virtual
		void
		write_transcription(
				const Transcription &transcription) = 0;


		/**
		 * Close the archive.
		 *
		 * Any final writes to the archive are done here.
		 *
		 * If this is not called then the archive writer's destructor should call this.
		 */
		virtual
		void
		close() = 0;

	};
}

#endif // GPLATES_SCRIBE_SCRIBEARCHIVEWRITER_H
