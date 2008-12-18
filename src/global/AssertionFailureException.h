/* $Id$ */

/**
 * \file
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#ifndef GPLATES_GLOBAL_ASSERTIONFAILUREEXCEPTION_H
#define GPLATES_GLOBAL_ASSERTIONFAILUREEXCEPTION_H

// FIXME: I suspect that the appropriate thing to do here is similar to
// IntrusivePointerZeroRefCountException: When the definition of 'write_message'
// moves to a .cc file, replace this with <iosfwd>
#include <ostream>
#include "GPlatesException.h"

namespace GPlatesGlobal
{
	/**
	 * Base GPlatesGlobal::Exception class which should be used for assertion
	 * failures; these exceptions indicate something is seriously wrong with
	 * the internal state of the program.
	 */
	class AssertionFailureException :
			public Exception
	{
	public:
		/**
		 * @param filename_ should be supplied using the @c __FILE__ macro,
		 * @param line_num_ should be supplied using the @c __LINE__ macro.
		 *
		 * FIXME: Ideally, we'd be tracking the call stack etc, and also supplying
		 * some sort of function object that might be used to do damage control
		 * for the program should such an exception be thrown. For example, the
		 * DigitisationStateUndoCommands have a few exceptional 'should never
		 * ever reach here' states. The 'recovery' function of those exceptions
		 * might be to clear the digitisation widget and wipe the undo stack clean,
		 * restoring the widget to a known sane state, and then alerting the user.
		 */
		explicit
		AssertionFailureException(
				const char *filename_,
				int line_num_):
			d_filename(filename_),
			d_line_num(line_num_)
		{  }

		virtual
		~AssertionFailureException()
		{  }

	protected:

		virtual
		const char *
		ExceptionName() const
		{
			return "AssertionFailureException";
		}

		virtual
		void
		write_message(
				std::ostream &os) const
		{
			os << "Assertion failure: " << ExceptionName() << " in " <<
					d_filename << ":" << d_line_num << std::endl;
		}

	private:
		const char *d_filename;
		int d_line_num;
	};
}

#endif  // GPLATES_GLOBAL_ASSERTIONFAILUREEXCEPTION_H
