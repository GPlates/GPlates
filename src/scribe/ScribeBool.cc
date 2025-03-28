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

#include "ScribeBool.h"

#include "ScribeExceptions.h"

#include "global/GPlatesAssert.h"


namespace GPlatesScribe
{
	struct Bool::CheckDeleter
	{
		explicit
		CheckDeleter(
				const GPlatesUtils::CallStack::Trace &transcribe_source_,
				bool require_check_) :
			transcribe_source(transcribe_source_),
			require_check(require_check_),
			has_been_checked(false)
		{  }

		void
		operator()(
				bool *bool_ptr)
		{
			if (require_check)
			{
				// Track the file/line of the call site for exception messages.
				// This is the file/line at which a transcribe call was made which, in turn,
				// returned a 'Bool'.
				GPlatesUtils::CallStackTracker call_stack_tracker(transcribe_source);

				// We shouldn't be throwing any exceptions in deleter, but this exception is to
				// force programmer to correct the program to check validity.
				//
				// We can get double exceptions if an exception came from outside - then the program will
				// just terminate with no exception information if we throw a second exception below.
				// But this is unlikely because the programmer should be checking the return result
				// straight after transcribing an object - and that should provide no window of
				// opportunity for double exceptions to get thrown (outside exception and our
				// has-been-checked exception).


				// Throw exception if the boolean result 'Bool' returned by 'Scribe::transcribe()',
				// or 'transcribe_base()', has not been checked by the caller (in the *load* path).
				//
				// If this assertion is triggered then it means:
				//   * A Scribe client has called 'Scribe::transcribe()', or a similar call,
				//     but has not checked the boolean result 'Bool'.
				//
				// To fix this do something like:
				//
				//	if (!scribe.transcribe(...))
				//	{
				//		return scribe.get_transcribe_result();
				//	}
				//
				GPlatesGlobal::Assert<Exceptions::ScribeTranscribeResultNotChecked>(
						has_been_checked,
						GPLATES_ASSERTION_SOURCE);
			}

			boost::checked_delete(bool_ptr);
		}

		GPlatesUtils::CallStack::Trace transcribe_source;
		bool require_check;
		bool has_been_checked;
	};
}


GPlatesScribe::Bool::Bool(
		const GPlatesUtils::CallStack::Trace &transcribe_source,
		bool result,
		bool require_check) :
	d_bool(new bool(result), CheckDeleter(transcribe_source, require_check))
{
}


bool
GPlatesScribe::Bool::boolean_test() const
{
	// Mark the Bool as having been checked by the client.
	boost::get_deleter<CheckDeleter>(d_bool)->has_been_checked = true;

	return *d_bool;
}
