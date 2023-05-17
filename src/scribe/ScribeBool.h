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

#ifndef GPLATES_SCRIBE_SCRIBEBOOL_H
#define GPLATES_SCRIBE_SCRIBEBOOL_H

#include <boost/shared_ptr.hpp>

#include "utils/CallStackTracker.h"
#include "utils/SafeBool.h"


namespace GPlatesScribe
{
	/**
	 * Boolean result for transcribe methods.
	 *
	 * This class is used instead of a 'bool' to ensure the caller checks transcribe results.
	 * If a return result is not checked then Exceptions::ScribeTranscribeResultNotChecked is
	 * thrown to notify the programmer to insert the check.
	 *
	 * For example, to check the return result of Scribe::transcribe():
	 *
	 *	if (!scribe.transcribe(...))
	 *	{
	 *		return scribe.get_transcribe_result();
	 *	}
	 *
	 * NOTE: Only the *load* path needs to be checked.
	 * @a transcribe handles both the load and save paths but if you split it into separate
	 * save and load paths then only the load path needs to be checked.
	 * For example:
	 *
	 *	if (scribe.is_saving())
	 *	{
	 *		scribe.transcribe(...);
	 *	}
	 *	else // loading...
	 *	{
	 *		if (!scribe.transcribe(...))
	 *		{
	 *			return scribe.get_transcribe_result();
	 *		}
	 *	}
	 */
	class Bool :
			public GPlatesUtils::SafeBool<Bool>
	{
	public:

		/**
		 * Boolean test - don't use directly.
		 *
		 * Instead use (for example):
		 *
		 *		if (!scribe.transcribe(...))
		 *		{
		 *			return scribe.get_transcribe_result();
		 *		}
		 *
		 * ...where Scribe::transcribe() returns a 'Bool'.
		 */
		bool
		boolean_test() const;

	private:
		struct CheckDeleter; //!< Custom boost::shared_ptr deleter when result checking required.

		Bool(
				// The location of the caller site that should be checking this returned 'Bool'...
				const GPlatesUtils::CallStack::Trace &transcribe_source,
				// Actual boolean result...
				bool result,
				// Whether to throw exception if boolean result is not checked...
				bool require_check);

		boost::shared_ptr<bool> d_bool;

		friend class Scribe;
		friend class ScribeInternalAccess;
	};
}

#endif // GPLATES_SCRIBE_SCRIBEBOOL_H
