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

#include <QDebug>

#include "ScribeBool.h"

#include "ScribeExceptions.h"
#include "ScribeInternalUtils.h"

#include "global/GPlatesAssert.h"


namespace GPlatesScribe
{
	/**
	 * Custom boost::shared_ptr deleter that carries the shared "has this result been checked?" state.
	 *
	 * NOTE: This deleter must not throw. It is called from
	 * 'boost::detail::shared_count::~shared_count()' which, being a destructor declared without an
	 * exception-specification, is implicitly 'noexcept(true)' (see [class.dtor]/3) - so any exception
	 * thrown here would call 'std::terminate' without unwinding the stack (no handler could catch it
	 * and nothing would be logged). The unchecked-result check is therefore done in '~Bool()' instead.
	 */
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


GPlatesScribe::Bool::~Bool() noexcept(false)
{
	CheckDeleter *const check_deleter = boost::get_deleter<CheckDeleter>(d_bool);

	if (check_deleter == NULL ||
		!check_deleter->require_check ||
		check_deleter->has_been_checked)
	{
		return;
	}

	// Only the *last* 'Bool' referencing this result gets to complain. The result is allowed to be
	// checked via any copy, so the verdict is only final once no other copy could still check it.
	if (d_bool.use_count() > 1)
	{
		return;
	}

	// Track the file/line of the call site for exception messages.
	// This is the file/line at which a transcribe call was made which, in turn, returned a 'Bool'.
	GPlatesUtils::CallStackTracker call_stack_tracker(check_deleter->transcribe_source);

	// If an exception is already propagating then throwing here would call 'std::terminate' and
	// destroy the information carried by that (more interesting) exception. So just log instead.
	//
	// This should be rare, since the programmer should be checking the return result straight after
	// transcribing an object - which leaves no real window for an outside exception to arrive first.
	if (InternalUtils::is_unwinding())
	{
		qWarning() << "Incorrect Scribe usage: the return result of a transcribe call was not "
				"checked (not throwing, since an exception is already propagating).";
		return;
	}

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
			false,
			GPLATES_ASSERTION_SOURCE);
}


bool
GPlatesScribe::Bool::boolean_test() const
{
	// Mark the Bool as having been checked by the client.
	boost::get_deleter<CheckDeleter>(d_bool)->has_been_checked = true;

	return *d_bool;
}
