/* $Id$ */

/**
 * \file
 * Used to manually keep track of the call stack.
 *
 * Most recent change:
 *   $Date$
 *
 * Copyright (C) 2007, 2009 The University of Sydney, Australia
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

#include <vector>
#include <iosfwd>
#include <boost/noncopyable.hpp>


/**
 * Do not invoke this macro directly.
 *
 * This macro is used by the TRACK_CALL_STACK macro.
 */
#define CALL_STACK_MAGIC2(x) \
		GPlatesUtils::CallStackTracker call_stack_tracker##x(__FILE__, __LINE__);

/**
 * Do not invoke this macro directly.
 *
 * This macro is used by the TRACK_CALL_STACK macro.
 */
#define CALL_STACK_MAGIC1(x) CALL_STACK_MAGIC2(x)

/**
 * Track the call stack.
 *
 * This is a convenience macro to simplify the use of class GPlatesUtils::CallStack.
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
	 * This class is a singleton that keeps track of the call stack.
	 */
	class CallStack :
			public boost::noncopyable
	{
	public:
		//! Returns singleton instance of this class.
		static
		CallStack &
		instance()
		{
			static CallStack s_call_stack_tracker;
			return s_call_stack_tracker;
		}


		/**
		 * Keeps track of the location of a specific trace in the call stack.
		 */
		class Trace
		{
		public:
			Trace(
					const char *filename,
					int line_num) :
				d_filename(filename),
				d_line_num(line_num)
			{  }

			//! Get filename.
			const char *
			get_filename() const
			{
				return d_filename;
			}

			//! Get line number.
			int
			get_line_num() const
			{
				return d_line_num;
			}

		private:
			const char *d_filename;
			int d_line_num;
		};

		//! Typedef for a stack of @a CallStackElement objects.
		typedef std::vector<Trace> trace_seq_type;

		//! Typedef for iterator over const @a Trace objects.
		typedef trace_seq_type::const_iterator trace_const_iterator;


		/**
		 * Start tracking a new stack trace .
		 */
		void
		push(
				const Trace &);

		/**
		 * Stop tracking matching stack trace from @a push.
		 * Calls to @a pop must match calls to @a push.
		 */
		void
		pop();

		/**
		 * Begin iterator of current call stack sequence.
		 * Dereferencing iterator gives a @a Trace object.
		 * Should not call @a push or @a pop between calls to
		 * @a call_stack_begin and @a call_stack_end.
		 */
		trace_const_iterator
		call_stack_begin()
		{
			return d_call_stack.begin();
		}

		/**
		 * End iterator of current call stack sequence.
		 * Should not call @a push or @a pop between calls to
		 * @a call_stack_begin and @a call_stack_end.
		 */
		trace_const_iterator
		call_stack_end()
		{
			return d_call_stack.end();
		}

		/**
		 * Writes the call stack trace to @a output.
		 */
		void
		write_call_stack_trace(
				std::ostream &output);

	private:
		//! Constructor is private for singleton.
		CallStack()
		{  }

		trace_seq_type d_call_stack;
	};


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
	 * The macro @a TRACK_CALL_STACK() can be used to simplify the use of this class.  Wherever
	 * you wish to track the call stack, simply invoke the macro function:
	 *
	 * @code
	 *  TRACK_CALL_STACK()
	 * @endcode
	 *
	 * This macro will supply the appropriate arguments to the CallStack constructor.
	 */
	class CallStackTracker
	{
	public:
		CallStackTracker(
				const CallStack::Trace &trace)
		{
			CallStack::instance().push(trace);
		}

		~CallStackTracker()
		{
			// Since this is a destructor we cannot let any exceptions escape.
			// If one is thrown we just have to lump it and continue on.
			try
			{
				CallStack::instance().pop();
			}
			catch (...)
			{
			}
		}
	};
}

#endif  // GPLATES_UTILS_CALLSTACKTRACKER_H
