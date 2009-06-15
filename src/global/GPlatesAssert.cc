/* $Id$ */

/**
* \file 
* File specific comments.
*
* Most recent change:
*   $Date$
* 
* Copyright (C) 2009 The University of Sydney, Australia
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

#include <cstdlib>
#include <iostream>

#include "GPlatesAssert.h"


void
GPlatesGlobal::Abort(
		const GPlatesUtils::CallStack::Trace &abort_location)
{
	// Push the location of the caller onto the call stack writing out the
	// call stack trace.
	GPlatesUtils::CallStackTracker call_stack_tracker(abort_location);

	GPlatesUtils::CallStack::instance().write_call_stack_trace(std::cerr);

	std::abort();
}
