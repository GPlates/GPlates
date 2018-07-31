/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#ifndef GPLATES_API_PYTHONINTERPRETERLOCKER_H
#define GPLATES_API_PYTHONINTERPRETERLOCKER_H
#include "Python.h"
#include "global/python.h"

#if !defined(GPLATES_NO_PYTHON)
namespace GPlatesApi
{
	/**
	 * A wrapper around Python's @a PyGILState_Ensure (which ensures that the
	 * calling thread is ready to call Python C API functions by acquiring the
	 * Global Interpreter Lock (GIL) for the current thread) and
	 * @a PyGILState_Release (which releases the GIL for the current thread).
	 *
	 * In GPlates, the use of these functions is necessary because the threads that
	 * we use are not threads created by Python.
	 *
	 * For more information, see http://docs.python.org/c-api/init.html#PyGILState_Ensure
	 */
	class PythonInterpreterLocker
	{
	public:

		/**
		 * Constructs a @a PythonInterpreterLocker.
		 *
		 * If @a ensure_ is true, acquires the GIL by calling @a ensure.
		 */
		explicit
		PythonInterpreterLocker(
				bool ensure_ = true);

		/**
		 * Releases the GIL if we have acquired the GIL, i.e. @a ensure has been
		 * called but @a release has not been called.
		 */
		~PythonInterpreterLocker();

		/**
		 * Ensures that the calling thread is ready to call Python C API functions by
		 * acquiring the Global Interpreter Lock (GIL). This is a wrapper around the
		 * function @a PyGILState_Ensure; the return value is saved internally so that
		 * the GIL may be released later.
		 *
		 * Precondition: For each instance of @a PythonInterpreterLocker, if @a ensure
		 * has already been called, it must not be called again without a prior call
		 * to @a release. Note that this does not imply that a thread may not have two
		 * or more active @a PythonInterpreterLocker instances; a thread may make as
		 * many calls to @a PyGILState_Ensure as it likes, as long as each call is
		 * matched with a call to @a PyGILState_Release.
		 */
		void
		ensure();

		/**
		 * Releases the Global Interpreter Lock (GIL). This is a wrapper around the
		 * function @a PyGIL_Release.
		 *
		 * Precondition: @a ensure must have been called, and @a release has not been
		 * called since.
		 */
		void
		release();

	private:

		bool d_has_gil;

		PyGILState_STATE d_gil_state;

	};
}
#endif  //GPLATES_NO_PYTHON
#endif  // GPLATES_API_PYTHONINTERPRETERLOCKER_H
