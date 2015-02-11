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

#ifndef GPLATES_SCRIBE_TRANSCRIBENONNULLINTRUSIVEPTR_H
#define GPLATES_SCRIBE_TRANSCRIBENONNULLINTRUSIVEPTR_H

#include "utils/non_null_intrusive_ptr.h"

#include "Transcribe.h"
#include "TranscribeSmartPointerProtocol.h"


namespace GPlatesScribe
{
	class Scribe;

	//! Overload to transcribe GPlatesUtils::non_null_intrusive_ptr.
	template <class T, class H>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			GPlatesUtils::non_null_intrusive_ptr<T, H> &intrusive_ptr_object,
			bool transcribed_construct_data);

	//! Overload to save/load construct a GPlatesUtils::non_null_intrusive_ptr.
	template <class T, class H>
	TranscribeResult
	transcribe_construct_data(
			Scribe &scribe,
			ConstructObject< GPlatesUtils::non_null_intrusive_ptr<T, H> > &intrusive_ptr_object);
}


//
// Template implementation
//


// Placing here avoids cyclic header dependency.
#include "Scribe.h"


namespace GPlatesScribe
{
	template <class T, class H>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			GPlatesUtils::non_null_intrusive_ptr<T, H> &intrusive_ptr_object,
			bool transcribed_construct_data)
	{
		if (!transcribed_construct_data)
		{
			T *raw_ptr = NULL;

			if (scribe.is_saving())
			{
				raw_ptr = intrusive_ptr_object.get();
			}

			TranscribeResult transcribe_result =
					transcribe_smart_pointer_protocol(TRANSCRIBE_SOURCE, scribe, raw_ptr, true/*shared_owner*/);
			if (transcribe_result != TRANSCRIBE_SUCCESS)
			{
				return transcribe_result;
			}

			if (scribe.is_loading())
			{
				intrusive_ptr_object = GPlatesUtils::non_null_intrusive_ptr<T, H>(raw_ptr);
			}
		}

		return TRANSCRIBE_SUCCESS;
	}


	template <class T, class H>
	TranscribeResult
	transcribe_construct_data(
			Scribe &scribe,
			ConstructObject< GPlatesUtils::non_null_intrusive_ptr<T, H> > &intrusive_ptr_object)
	{
		T *raw_ptr = NULL;

		if (scribe.is_saving())
		{
			raw_ptr = intrusive_ptr_object->get();
		}

		TranscribeResult transcribe_result =
				transcribe_smart_pointer_protocol(TRANSCRIBE_SOURCE, scribe, raw_ptr, true/*shared_owner*/);
		if (transcribe_result != TRANSCRIBE_SUCCESS)
		{
			return transcribe_result;
		}

		if (scribe.is_loading())
		{
			// Construct the object from the constructor data.
			intrusive_ptr_object.construct_object(raw_ptr);
		}

		return TRANSCRIBE_SUCCESS;
	}
}

#endif // GPLATES_SCRIBE_TRANSCRIBENONNULLINTRUSIVEPTR_H
