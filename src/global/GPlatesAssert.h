/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006 The University of Sydney, Australia
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

#ifndef _GPLATES_GLOBAL_ASSERT_H_
#define _GPLATES_GLOBAL_ASSERT_H_

#include "utils/CallStackTracker.h"


#define GPLATES_ASSERTION_SOURCE \
	GPlatesUtils::CallStack::Trace(__FILE__, __LINE__)

namespace GPlatesGlobal
{
	namespace AssertInternals
	{
		/**
		 * Whether a failed assertion aborts instead of throwing.
		 *
		 * Defined in 'GPlatesAssert.cc', which sets the default for each product.
		 * Read it through 'assertion_failures_abort()' rather than directly.
		 */
		extern bool s_assertion_failures_abort;
	}


	/**
	 * Returns true if a failed assertion aborts, false if it throws.
	 *
	 * By default this is true only for GPlates in a GPLATES_DEBUG build (Debug or RelWithDebInfo).
	 * pyGPlates always throws, since an abort would take the host Python interpreter down with it.
	 *
	 * This is only consulted once an assertion has already failed, so the cost falls entirely on
	 * the failure path. Why it is a run-time flag rather than a compile-time one (and what was
	 * rejected instead) is recorded in "docs/design/testing/README.md".
	 */
	inline
	bool
	assertion_failures_abort()
	{
		return AssertInternals::s_assertion_failures_abort;
	}


	/**
	 * Chooses whether a failed assertion aborts or throws, overriding the build-type default.
	 *
	 * Aborting is what lets a developer catch a failure in a debugger at the point it happens,
	 * which is why it is the default in a debug build of GPlates. But a test that exercises an
	 * error path needs the exception, and an abort takes the whole test run down with it - so the
	 * GPlates unit-test executable turns it off before running the tests.
	 *
	 * Call this before starting any thread that can trip an assertion.
	 */
	void
	set_assertion_failures_abort(
			bool assertion_failures_abort);


	/**
	 * Outputs the call stack contained in @a CallStack and then calls @a std::abort
	 * (if @a assertion_failures_abort is true) or throws an instance of @a AbortException.
	 *
	 * @param abort_location the caller's call stack location.
	 */
	[[ noreturn ]]
	void
	Abort(
			const GPlatesUtils::CallStack::Trace &abort_location);


	/**
	 * This is our new favourite Assert() statement.
	 * You use it thusly:
	 *
	 *   Assert<ExceptionType>(assertion, assert_location, additional_exception_args...);
	 *
	 * If @a assertion is true then nothing happens.
	 * If @a assertion is false then either @a Abort is called (if @a assertion_failures_abort is
	 * true) or an instance of @a ExceptionType is instantiated and thrown.
	 * In the latter case the exception constructor's first argument is @a assert_location
	 * (which doubles as the exception location) and any additional arguments are provided
	 * by the various overloaded versions of @a Assert.
	 *
	 * @param assertion is the expression to test as the assertion condition.
	 * This can be any type that can be tested via an 'if' statement. Examples include bool,
	 * boost::optional, boost::shared_ptr, boost::intrusive_ptr, boost::scoped_ptr, etc.
	 *
	 * Note: There are several overloaded versions of @a Assert
	 * each taking a different number of arguments for the exception constructor.
	 * They take the constructor's arguments, rather than a ready-made exception object, so that
	 * the exception is only constructed once the assertion has failed - constructing one at every
	 * call site, on the successful path too, was found to be too costly.
	 *
	 * Note: The first argument to every exception constructor must be
	 * @a GPlatesUtils::CallStack::Trace. This means each class derived from
	 * @a GPlatesGlobal::Exception must order its constructor arguments this way.
	 *
	 * Note that previously only type 'bool' was accepted for @a assertion but this implied that
	 * other types (such as boost::optional and boost::shared_ptr) could be *implicitly* cast to
	 * bool when Assert is called (so that caller doesn't not need to explicitly cast to bool).
	 * However when the boost library made the 'bool' conversation operator (of these classes)
	 * 'explicit' (when compiling with c++11 enabled) this prevented implicit conversions.
	 * For example, this happened with boost::optional in version 1.56.
	 * So now we pass the boolean testable type directly to our internal 'if' statement which,
	 * according to c++11, is treated as a special case that allows implicit conversion to bool.
	 *
	 * This overload version accepts zero additional exception constructor arguments.
	 */
	template<class ExceptionType, typename AssertionConditionType>
	inline
	void
	Assert(
			const AssertionConditionType &assertion,
			const GPlatesUtils::CallStack::Trace &assert_location)
	{
		if (!assertion)
		{
			if (assertion_failures_abort())
			{
				Abort(assert_location);
			}
			throw ExceptionType(assert_location);
		}
	}


	//! Overloaded @a Assert taking one additional argument to the ExceptionType constructor.
	template<class ExceptionType, typename A1, typename AssertionConditionType>
	inline
	void
	Assert(
			const AssertionConditionType &assertion,
			const GPlatesUtils::CallStack::Trace &assert_location,
			const A1 &arg1)
	{
		if (!assertion)
		{
			if (assertion_failures_abort())
			{
				Abort(assert_location);
			}
			throw ExceptionType(assert_location, arg1);
		}
	}


	//! Overloaded @a Assert taking two additional arguments to the ExceptionType constructor.
	template<class ExceptionType, typename A1, typename A2, typename AssertionConditionType>
	inline
	void
	Assert(
			const AssertionConditionType &assertion,
			const GPlatesUtils::CallStack::Trace &assert_location,
			const A1 &arg1, const A2 &arg2)
	{
		if (!assertion)
		{
			if (assertion_failures_abort())
			{
				Abort(assert_location);
			}
			throw ExceptionType(assert_location, arg1, arg2);
		}
	}


	//! Overloaded @a Assert taking three additional arguments to the ExceptionType constructor.
	template<class ExceptionType, typename A1, typename A2, typename A3, typename AssertionConditionType>
	inline
	void
	Assert(
			const AssertionConditionType &assertion,
			const GPlatesUtils::CallStack::Trace &assert_location,
			const A1 &arg1, const A2 &arg2, const A3 &arg3)
	{
		if (!assertion)
		{
			if (assertion_failures_abort())
			{
				Abort(assert_location);
			}
			throw ExceptionType(assert_location, arg1, arg2, arg3);
		}
	}


	//! Overloaded @a Assert taking four additional arguments to the ExceptionType constructor.
	template<class ExceptionType, typename A1, typename A2, typename A3, typename A4, typename AssertionConditionType>
	inline
	void
	Assert(
			const AssertionConditionType &assertion,
			const GPlatesUtils::CallStack::Trace &assert_location,
			const A1 &arg1, const A2 &arg2, const A3 &arg3, const A4 &arg4)
	{
		if (!assertion)
		{
			if (assertion_failures_abort())
			{
				Abort(assert_location);
			}
			throw ExceptionType(assert_location, arg1, arg2, arg3, arg4);
		}
	}


	//! Overloaded @a Assert taking five additional arguments to the ExceptionType constructor.
	template<class ExceptionType, typename A1, typename A2, typename A3, typename A4, typename A5, typename AssertionConditionType>
	inline
	void
	Assert(
			const AssertionConditionType &assertion,
			const GPlatesUtils::CallStack::Trace &assert_location,
			const A1 &arg1, const A2 &arg2, const A3 &arg3, const A4 &arg4, const A5 &arg5)
	{
		if (!assertion)
		{
			if (assertion_failures_abort())
			{
				Abort(assert_location);
			}
			throw ExceptionType(assert_location, arg1, arg2, arg3, arg4, arg5);
		}
	}
}

#endif  // _GPLATES_GLOBAL_ASSERT_H_
