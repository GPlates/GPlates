/**
 * Copyright (C) 2023 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_TRANSCRIBEIDTYPEGENERATOR_H
#define GPLATES_MODEL_TRANSCRIBEIDTYPEGENERATOR_H

#include "IdTypeGenerator.h"

#include "scribe/Scribe.h"


namespace GPlatesModel
{
	//
	// The transcribe implementation of IdTypeGenerator is placed in a separate header
	// that only needs to be included when transcribing.
	// This avoids "IdTypeGenerator.h" having to include the heavyweight "Scribe.h"
	// for regular code (non-transcribe) paths that don't need it.
	//
	// NOTE: We're currently not transcribing the back-reference target.
	//       That's more complicated and typically not necessary for current use cases
	//       (such as saving/loading projects/sessions, and Python pickling).
	//
	//

	template <typename SingletonType, typename BackRefTargetType>
	GPlatesScribe::TranscribeResult
	IdTypeGenerator<SingletonType, BackRefTargetType>::transcribe_construct_data(
			GPlatesScribe::Scribe &scribe,
			GPlatesScribe::ConstructObject< IdTypeGenerator<SingletonType, BackRefTargetType> > &id_type_generator)
	{
		if (scribe.is_saving())
		{
			scribe.save(TRANSCRIBE_SOURCE, id_type_generator->get(), "id");
		}
		else // loading...
		{
			GPlatesUtils::UnicodeString id;
			if (!scribe.transcribe(TRANSCRIBE_SOURCE, id, "id"))
			{
				return scribe.get_transcribe_result();
			}

			id_type_generator.construct_object(id);
		}

		return GPlatesScribe::TRANSCRIBE_SUCCESS;
	}


	template <typename SingletonType, typename BackRefTargetType>
	GPlatesScribe::TranscribeResult
	IdTypeGenerator<SingletonType, BackRefTargetType>::transcribe(
			GPlatesScribe::Scribe &scribe,
			bool transcribed_construct_data)
	{
		// If not already transcribed in 'transcribe_construct_data()'.
		if (!transcribed_construct_data)
		{
			if (scribe.is_saving())
			{
				scribe.save(TRANSCRIBE_SOURCE, get(), "id");
			}
			else // loading...
			{
				GPlatesUtils::UnicodeString id;
				if (!scribe.transcribe(TRANSCRIBE_SOURCE, id, "id"))
				{
					return scribe.get_transcribe_result();
				}

				d_sh_iter = SingletonType::instance().insert(id);
			}
		}

		return GPlatesScribe::TRANSCRIBE_SUCCESS;
	}
}

#endif // GPLATES_MODEL_TRANSCRIBEIDTYPEGENERATOR_H
