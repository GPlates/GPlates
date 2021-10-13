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

#ifndef GPLATES_API_PYTHONINTERPRETERUNLOCKER_H
#define GPLATES_API_PYTHONINTERPRETERUNLOCKER_H

#include "global/python.h"

#if !defined(GPLATES_NO_PYTHON)
namespace GPlatesApi
{
	/**
	 * A wrapper around Python's @a PyEval_SaveThread (which releases the Global
	 * Interpreter Lock (GIL)) and @a PyEval_RestoreThread (which acquires the GIL).
	 *
	 * The use of these functions can improve performance by releasing the GIL upon
	 * entering C++ extension code that has been called from Python.
	 *
	 * For more information, see http://docs.python.org/c-api/init.html#PyEval_SaveThread
	 */
	class PythonInterpreterUnlocker
	{
	public:

		/**
		 * Constructs a @a PythonInterpreterUnlocker.
		 *
		 * If @a save_thread_ is true, releases the GIL by calling @a save_thread. Note that
		 * the current thread must hold the lock in order to release it.
		 */
		explicit
		PythonInterpreterUnlocker(
				bool save_thread_ = true);

		/**
		 * Reacquires the GIL if we have released it, i.e. @a save_thread has been
		 * called but @a restore_thread has not been called.
		 */
		~PythonInterpreterUnlocker();

		/**
		 * Releases the GIL. This is a wrapper around the function
		 * @a PyEval_SaveThread; the return is saved internally so that the thread
		 * state can be restored later to reacquire the GIL.
		 *
		 * Precondition: The current thread must have acquired the GIL.
		 */
		void
		save_thread();

		/**
		 * Reacquires the GIL. This is a wrapper around the function
		 * @a PyEval_RestoreThread.
		 *
		 * Precondition: @a save_thread must have been called, and @a restore_thread
		 * has not been called since. The current thread must not have acquired the
		 * GIL, otherwise deadlock ensues.
		 */
		void
		restore_thread();

	private:
		PyThreadState *d_thread_state;
	};
}
#endif  //GPLATES_NO_PYTHON
#endif  // GPLATES_API_PYTHONINTERPRETERLOCKER_H
