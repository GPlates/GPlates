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

#ifndef GPLATES_MODEL_TRANSCRIBEREVISIONEDVECTOR_H
#define GPLATES_MODEL_TRANSCRIBEREVISIONEDVECTOR_H

#include <vector>
#include <boost/ref.hpp>

#include "RevisionedVector.h"

#include "scribe/Scribe.h"
#include "scribe/TranscribeDelegateProtocol.h"


namespace GPlatesModel
{
	//
	// The transcribe implementation of RevisionedVector is placed in a separate header
	// that only needs to be included when transcribing.
	// This avoids "RevisionedVector.h" having to include the heavyweight "Scribe.h"
	// for regular code (non-transcribe) paths that don't need it.
	//

	//
	// Using transcribe delegate protocol so that RevisionedVector and vector (and compatible sequence types)
	// can be used interchangeably (ie, are transcription compatible).
	//

	template <class RevisionableType>
	GPlatesScribe::TranscribeResult
	RevisionedVector<RevisionableType>::transcribe_construct_data(
			GPlatesScribe::Scribe &scribe,
			GPlatesScribe::ConstructObject<RevisionedVector<RevisionableType>> &revisioned_vector)
	{
		if (scribe.is_saving())
		{
			// Get the current elements.
			const std::vector<element_type> elements(
					revisioned_vector->begin(),
					revisioned_vector->end());

			// Save the elements.
			GPlatesScribe::save_delegate_protocol(TRANSCRIBE_SOURCE, scribe, elements);
		}
		else // loading...
		{
			// Load the elements.
			std::vector<element_type> elements;
			const GPlatesScribe::TranscribeResult transcribe_elements_result =
					GPlatesScribe::transcribe_delegate_protocol(TRANSCRIBE_SOURCE, scribe, elements);
			if (transcribe_elements_result != GPlatesScribe::TRANSCRIBE_SUCCESS)
			{
				return transcribe_elements_result;
			}

			// Create the revisioned vector.
			GPlatesModel::ModelTransaction transaction;
			revisioned_vector.construct_object(
					boost::ref(transaction),  // non-const ref
					elements.begin(),
					elements.end());
			transaction.commit();
		}

		return GPlatesScribe::TRANSCRIBE_SUCCESS;
	}


	template <class RevisionableType>
	GPlatesScribe::TranscribeResult
	RevisionedVector<RevisionableType>::transcribe(
			GPlatesScribe::Scribe &scribe,
			bool transcribed_construct_data)
	{
		// If not already transcribed in 'transcribe_construct_data()'.
		if (!transcribed_construct_data)
		{
			if (scribe.is_saving())
			{
				// Get the current elements.
				const std::vector<element_type> elements(begin(), end());

				// Save the elements.
				GPlatesScribe::save_delegate_protocol(TRANSCRIBE_SOURCE, scribe, elements);
			}
			else // loading...
			{
				// Load the elements.
				std::vector<element_type> elements;
				const GPlatesScribe::TranscribeResult transcribe_elements_result =
						GPlatesScribe::transcribe_delegate_protocol(TRANSCRIBE_SOURCE, scribe, elements);
				if (transcribe_elements_result != GPlatesScribe::TRANSCRIBE_SUCCESS)
				{
					return transcribe_elements_result;
				}

				// Set the elements.
				assign(elements.begin(), elements.end());
			}
		}

		return GPlatesScribe::TRANSCRIBE_SUCCESS;
	}
}

#endif // GPLATES_MODEL_TRANSCRIBEREVISIONEDVECTOR_H
