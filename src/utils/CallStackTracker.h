/* $Id$ */

/**
 * \file
 * Contains the definition of the class CallStackTracker.
 *
 * Most recent change:
 *   $Date$
 *
 * Copyright (C) 2007 The University of Sydney, Australia
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

#ifndef GPLATES_UTILS_CALLSTACKTRACKER_H
#define GPLATES_UTILS_CALLSTACKTRACKER_H

/**
 * Do not invoke this macro directly.
 *
 * This macro is used by the TRACK_CALL_STACK macro.
 */
#define CALL_STACK_MAGIC2(x) GPlatesUtils::CallStackTracker call_stack_tracker##x(__FILE__, __LINE__);

/**
 * Do not invoke this macro directly.
 *
 * This macro is used by the TRACK_CALL_STACK macro.
 */
#define CALL_STACK_MAGIC1(x) CALL_STACK_MAGIC2(x)

/**
 * Track the call stack.
 *
 * This is a convenience macro to simplify the use of class GPlatesUtils::CallStackTracker.
 *
 * Invoke this macro function at the start of any function block (or, indeed, any other blocks)
 * which you wish to track:
 *
 * @code
 *  TRACK_CALL_STACK()
 * @endcode
 *
 * When the program is executed, whenever a tracked block begins (ie, whenever the code in this
 * macro is executed), a message will be printed to the standard error stream.  When the tracked
 * block ends, another message will be printed.
 */
#define TRACK_CALL_STACK() CALL_STACK_MAGIC1(__LINE__)

namespace GPlatesUtils
{
	/**
	 * This class provides a means to track the call stack.
	 *
	 * Instances of this class should be instantiated on the stack (ie, as local variables) at
	 * the start of any function blocks (or, indeed, any other blocks) which you wish to track.
	 *
	 * The constructor should be passed the arguments @c __FILE__ and @c __LINE__, which will
	 * be automatically translated by the compiler into the current file name and line number,
	 * respectively.
	 *
	 * When the program is executed, whenever a tracked block begins (ie, whenever an instance
	 * of this class is instantiated on the stack), a message will be printed to the standard
	 * error stream.  When the tracked block ends, another message will be printed.
	 *
	 * The macro @a TRACK_CALL_STACK() can be used to simplify the use of this class.  Wherever
	 * you wish to track the call stack, simply invoke the macro function:
	 *
	 * @code
	 *  TRACK_CALL_STACK()
	 * @endcode
	 *
	 * This macro will supply the appropriate arguments to the CallStackTracker constructor.
	 */
	class CallStackTracker
	{
	public:
		CallStackTracker(
				const char *filename,
				int line_num);

		~CallStackTracker();
	private:
		const char *d_filename;
		int d_line_num;

		// Don't allow copy-construction.
		CallStackTracker(
				const CallStackTracker &);

		// Don't allow copy-assignment.
		CallStackTracker &
		operator=(
				const CallStackTracker &);
	};
}

#endif  // GPLATES_UTILS_CALLSTACKTRACKER_H
