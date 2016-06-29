/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2016 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_TRANSCRIBESTRINGCONTENTTYPEGENERATOR_H
#define GPLATES_MODEL_TRANSCRIBESTRINGCONTENTTYPEGENERATOR_H

#include "StringContentTypeGenerator.h"

#include "scribe/Scribe.h"
#include "scribe/TranscribeDelegateProtocol.h"


namespace GPlatesModel
{
	//
	// The transcribe implementation of StringContentTypeGenerator is placed in a separate header
	// that only needs to be included when transcribing.
	// This avoids "StringContentTypeGenerator.h" having to include the heavyweight "Scribe.h"
	// for regular code (non-transcribe) paths that don't need it.
	//

	//
	// Using transcribe delegate protocol so that StringContentTypeGenerator and UnicodeString
	// can be used interchangeably (ie, are transcription compatible).
	//

	template <typename SingletonType>
	GPlatesScribe::TranscribeResult
	StringContentTypeGenerator<SingletonType>::transcribe_construct_data(
			GPlatesScribe::Scribe &scribe,
			GPlatesScribe::ConstructObject< StringContentTypeGenerator<SingletonType> > &string_content)
	{
		if (scribe.is_saving())
		{
			GPlatesScribe::save_delegate_protocol(TRANSCRIBE_SOURCE, scribe, *string_content->d_ss_iter);
		}
		else // loading...
		{
			GPlatesScribe::LoadRef<GPlatesUtils::UnicodeString> string_ref =
					GPlatesScribe::load_delegate_protocol<GPlatesUtils::UnicodeString>(TRANSCRIBE_SOURCE, scribe);
			if (!string_ref.is_valid())
			{
				return scribe.get_transcribe_result();
			}

			string_content.construct_object(string_ref);
		}

		return GPlatesScribe::TRANSCRIBE_SUCCESS;
	}


	template <typename SingletonType>
	GPlatesScribe::TranscribeResult
	StringContentTypeGenerator<SingletonType>::transcribe(
			GPlatesScribe::Scribe &scribe,
			bool transcribed_construct_data)
	{
		// If not already transcribed in 'transcribe_construct_data()'.
		if (!transcribed_construct_data)
		{
			if (scribe.is_saving())
			{
				GPlatesScribe::save_delegate_protocol(TRANSCRIBE_SOURCE, scribe, *d_ss_iter);
			}
			else // loading...
			{
				GPlatesScribe::LoadRef<GPlatesUtils::UnicodeString> string_ref =
						GPlatesScribe::load_delegate_protocol<GPlatesUtils::UnicodeString>(TRANSCRIBE_SOURCE, scribe);
				if (!string_ref.is_valid())
				{
					return scribe.get_transcribe_result();
				}

				d_ss_iter = SingletonType::instance().insert(string_ref);
			}
		}

		return GPlatesScribe::TRANSCRIBE_SUCCESS;
	}
}

#endif // GPLATES_MODEL_TRANSCRIBESTRINGCONTENTTYPEGENERATOR_H
