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
#include "PythonInterpreterUnlocker.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#if !defined(GPLATES_NO_PYTHON)
GPlatesApi::PythonInterpreterUnlocker::PythonInterpreterUnlocker(
		bool save_thread_)
	: d_thread_state(NULL)
{
	if (save_thread_)
	{
		save_thread();
	}
}


GPlatesApi::PythonInterpreterUnlocker::~PythonInterpreterUnlocker()
{
	if (d_thread_state != NULL)
	{
		restore_thread();
	}
}


void
GPlatesApi::PythonInterpreterUnlocker::save_thread()
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_thread_state == NULL,
			GPLATES_ASSERTION_SOURCE);

	d_thread_state = PyEval_SaveThread();
}


void
GPlatesApi::PythonInterpreterUnlocker::restore_thread()
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_thread_state != NULL,
			GPLATES_ASSERTION_SOURCE);

	PyEval_RestoreThread(d_thread_state);
	d_thread_state = NULL;
}
#endif //GPLATES_NO_PYTHON
