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

#ifndef GPLATES_SCRIBE_TRANSCRIBESMARTPOINTERPROTOCOL_H
#define GPLATES_SCRIBE_TRANSCRIBESMARTPOINTERPROTOCOL_H

#include "Scribe.h"
#include "ScribeInternalAccess.h"
#include "TranscribeResult.h"

#include "global/GPlatesAssert.h"

#include "utils/CallStackTracker.h"


namespace GPlatesScribe
{
	/**
	 * Used to ensure different smart pointer types are transcribed such that they can be switched
	 * without breaking backward/forward compatibility.
	 *
	 * This also makes smart pointer classes interchangeable with raw pointers.
	 *
	 * Some smart pointer types include boost::shared_ptr, boost::scoped_ptr, boost::intrusive_ptr
	 * and GPlatesUtils::non_null_intrusive_ptr.
	 */
	template <typename ObjectType>
	TranscribeResult
	transcribe_smart_pointer_protocol(
			const GPlatesUtils::CallStack::Trace &transcribe_source, // Use 'TRANSCRIBE_SOURCE' here
			Scribe &scribe,
			ObjectType *&object_ptr,
			bool shared_owner)
	{
		// Track the file/line of the call site for exception messages.
		GPlatesUtils::CallStackTracker call_stack_tracker(transcribe_source);

		if (!ScribeInternalAccess::transcribe_smart_pointer(scribe, object_ptr, shared_owner))
		{
			return scribe.get_transcribe_result();
		}

		return TRANSCRIBE_SUCCESS;
	}
}

#endif // GPLATES_SCRIBE_TRANSCRIBESMARTPOINTERPROTOCOL_H
