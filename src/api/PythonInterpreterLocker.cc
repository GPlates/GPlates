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
#include "PythonInterpreterLocker.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#if !defined(GPLATES_NO_PYTHON)
GPlatesApi::PythonInterpreterLocker::PythonInterpreterLocker(
		bool ensure_) :
	d_has_gil(false)
{
	if (ensure_)
	{
		ensure();
	}
}


GPlatesApi::PythonInterpreterLocker::~PythonInterpreterLocker()
{
	if (d_has_gil)
	{
		release();
	}
}


void
GPlatesApi::PythonInterpreterLocker::ensure()
{
	// We must not have the GIL yet.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			!d_has_gil,
			GPLATES_ASSERTION_SOURCE);

	d_has_gil = true;
	d_gil_state = PyGILState_Ensure();
}


void
GPlatesApi::PythonInterpreterLocker::release()
{
	// We must have the GIL.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_has_gil,
			GPLATES_ASSERTION_SOURCE);

	d_has_gil = false;
	PyGILState_Release(d_gil_state);
}

#endif //GPLATES_NO_PYTHON

