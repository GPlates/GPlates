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
	/**
	 * Outputs the call stack contained in @a CallStack to std::cerr
	 * and then calls @a std::abort.
	 *
	 * @param abort_location the caller's call stack location.
	 */
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
	 * If @a assertion is false then either @a Abort is called (if the current GPlates build
	 * is considered a debug build) or an instance of @a ExceptionType is instantiated and thrown.
	 * In the latter case the exception constructor's first argument is @a assert_location
	 * (which doubles as the exception location) and any additional arguments are provided
	 * by the various overloaded versions of @a Assert.
	 *
	 * @param assertion is the expression to test as the assertion condition.
	 *
	 * Note: There are several overloaded versions of @a Assert
	 * each taking a different number of arguments for the exception constructor.
	 *
	 * Note: The first argument to every exception constructor must be
	 * @a GPlatesUtils::CallStack::Trace. This means each class derived from
	 * @a GPlatesGlobal::Exception must order its constructor arguments this way.
	 *
	 * This overload version accepts zero additional exception constructor arguments.
	 */
	template<class ExceptionType>
	inline
	void
	Assert(
			bool assertion,
			const GPlatesUtils::CallStack::Trace &assert_location)
	{
		if (!assertion)
		{
#ifdef GPLATES_DEBUG
			Abort(assert_location);
#else
			throw ExceptionType(assert_location);
#endif
		}
	}


	//! Overloaded @a Assert taking one additional argument to the ExceptionType constructor.
	template<class ExceptionType, typename A1>
	inline
	void
	Assert(
			bool assertion,
			const GPlatesUtils::CallStack::Trace &assert_location,
			const A1 &arg1)
	{
		if (!assertion)
		{
#ifdef GPLATES_DEBUG
			(void)arg1; // Unused variables.
			Abort(assert_location);
#else
			throw ExceptionType(assert_location, arg1);
#endif
		}
	}


	//! Overloaded @a Assert taking two additional arguments to the ExceptionType constructor.
	template<class ExceptionType, typename A1, typename A2>
	inline
	void
	Assert(
			bool assertion,
			const GPlatesUtils::CallStack::Trace &assert_location,
			const A1 &arg1, const A2 &arg2)
	{
		if (!assertion)
		{
#ifdef GPLATES_DEBUG
			(void)arg1; (void)arg2; // Unused variables.
			Abort(assert_location);
#else
			throw ExceptionType(assert_location, arg1, arg2);
#endif
		}
	}


	//! Overloaded @a Assert taking three additional arguments to the ExceptionType constructor.
	template<class ExceptionType, typename A1, typename A2, typename A3>
	inline
	void
	Assert(
			bool assertion,
			const GPlatesUtils::CallStack::Trace &assert_location,
			const A1 &arg1, const A2 &arg2, const A3 &arg3)
	{
		if (!assertion)
		{
#ifdef GPLATES_DEBUG
			(void)arg1; (void)arg2; (void)arg3; // Unused variables.
			Abort(assert_location);
#else
			throw ExceptionType(assert_location, arg1, arg2, arg3);
#endif
		}
	}


	//! Overloaded @a Assert taking four additional arguments to the ExceptionType constructor.
	template<class ExceptionType, typename A1, typename A2, typename A3, typename A4>
	inline
	void
	Assert(
			bool assertion,
			const GPlatesUtils::CallStack::Trace &assert_location,
			const A1 &arg1, const A2 &arg2, const A3 &arg3, const A4 &arg4)
	{
		if (!assertion)
		{
#ifdef GPLATES_DEBUG
			(void)arg1; (void)arg2; (void)arg3; (void)arg4; // Unused variables.
			Abort(assert_location);
#else
			throw ExceptionType(assert_location, arg1, arg2, arg3, arg4);
#endif
		}
	}


	//! Overloaded @a Assert taking five additional arguments to the ExceptionType constructor.
	template<class ExceptionType, typename A1, typename A2, typename A3, typename A4, typename A5>
	inline
	void
	Assert(
			bool assertion,
			const GPlatesUtils::CallStack::Trace &assert_location,
			const A1 &arg1, const A2 &arg2, const A3 &arg3, const A4 &arg4, const A5 &arg5)
	{
		if (!assertion)
		{
#ifdef GPLATES_DEBUG
			(void)arg1; (void)arg2; (void)arg3; (void)arg4; (void)arg5; // Unused variables.
			Abort(assert_location);
#else
			throw ExceptionType(assert_location, arg1, arg2, arg3, arg4, arg5);
#endif
		}
	}
}

#endif  // _GPLATES_GLOBAL_ASSERT_H_
