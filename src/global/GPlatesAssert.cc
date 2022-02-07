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
#include <sstream>
#include <QtCore/qglobal.h>

#include "GPlatesAssert.h"
#include "AbortException.h"


void
GPlatesGlobal::Abort(
		const GPlatesUtils::CallStack::Trace &abort_location)
{
#ifdef GPLATES_DEBUG
	// Push the location of the caller onto the call stack writing out the
	// call stack trace.
	GPlatesUtils::CallStackTracker call_stack_tracker(abort_location);

	// Get the call stack trace as a string.
	std::ostringstream output_string_stream;
	GPlatesUtils::CallStack::instance().write_call_stack_trace(output_string_stream);

	// This is where the core dump or debugger trigger happens on debug builds.
	// Print the call stack trace.
	qFatal("Aborting: %s",
			QString::fromStdString(output_string_stream.str()).toLatin1().data());
#else
	throw AbortException(abort_location);
#endif
}
