/**
 * Copyright (C) 2025 The University of Sydney, Australia
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

#include "RevisionId.h"

#include "scribe/Scribe.h"
#include "scribe/TranscribeDelegateProtocol.h"


GPlatesScribe::TranscribeResult
GPlatesModel::RevisionId::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<RevisionId> &revision_id)
{
	//
	// Using transcribe delegate protocol so that RevisionId and UnicodeString (and hence also QString)
	// can be used interchangeably (ie, are transcription compatible).
	//
	if (scribe.is_saving())
	{
		save_delegate_protocol(TRANSCRIBE_SOURCE, scribe, revision_id->d_id);
	}
	else // loading...
	{
		GPlatesUtils::UnicodeString id_;
		if (!transcribe_delegate_protocol(TRANSCRIBE_SOURCE, scribe, id_))
		{
			return scribe.get_transcribe_result();
		}

		revision_id.construct_object(id_);
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesModel::RevisionId::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	// If not already transcribed in 'transcribe_construct_data()'.
	if (!transcribed_construct_data)
	{
		//
		// Using transcribe delegate protocol so that RevisionId and UnicodeString (and hence also QString)
		// can be used interchangeably (ie, are transcription compatible).
		//
		if (!transcribe_delegate_protocol(TRANSCRIBE_SOURCE, scribe, d_id))
		{
			return scribe.get_transcribe_result();
		}
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}
