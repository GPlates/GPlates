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

#ifndef GPLATES_SCRIBE_SCRIBEINTERNALUTILSIMPL_H
#define GPLATES_SCRIBE_SCRIBEINTERNALUTILSIMPL_H

#include "ScribeInternalUtils.h"

#include "Scribe.h"
#include "ScribeInternalAccess.h"
#include "ScribeSaveLoadConstructObject.h"
#include "Transcribe.h"


namespace GPlatesScribe
{
	namespace InternalUtils
	{
		template <typename ObjectType>
		void
		TranscribeOwningPointerTemplate<ObjectType>::save_object(
				Scribe &scribe,
				void *object_memory,
				object_id_type object_id,
				Options options) const
		{
			// The 'void *' passed in is expected to point to an object of type 'ObjectType'.
			// In other words it points to the entire object (and doesn't need any multiple
			// inheritance pointer fix-ups).
			ObjectType &object = *static_cast<ObjectType *>(object_memory);

			// Mirror the load path.
			SaveConstructObject<ObjectType> construct_object(object);

			// On the save path this should always succeed.
			ScribeInternalAccess::transcribe_construct(scribe, construct_object, object_id, options);
		}


		template <typename ObjectType>
		bool
		TranscribeOwningPointerTemplate<ObjectType>::load_object(
				Scribe &scribe,
				object_id_type object_id,
				Options options) const
		{
			// Construct the object on the heap.
			LoadConstructObjectOnHeap<ObjectType> construct_object;

			// Transcribe the pointer-owned object.
			// This requires private access to class Scribe.
			if (!ScribeInternalAccess::transcribe_construct(scribe, construct_object, object_id, options))
			{
				return false;
			}

			// On success release ownership of the constructed object from 'LoadConstructObjectOnHeap'.
			// It will now be owned by the pointer we are transcribing for.
			construct_object.release();

			return true;
		}
	}
}

#endif // GPLATES_SCRIBE_SCRIBEINTERNALUTILSIMPL_H
