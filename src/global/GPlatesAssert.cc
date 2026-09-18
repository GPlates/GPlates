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

#include <sstream>
#include <qglobal.h>

#include "GPlatesAssert.h"
#include "AbortException.h"


// The per-product default is decided here, at compile time, and not by a call from
// BOOST_PYTHON_MODULE(pygplates): that module init also runs when GPlates' embedded interpreter
// imports pygplates (see 'api/PyGPlatesModule.cc'), so a call there would silently flip a debug
// build of GPlates from aborting to throwing.
bool GPlatesGlobal::AssertInternals::s_assertion_failures_abort =
#if defined(GPLATES_DEBUG) && defined(GPLATES_PYTHON_EMBEDDING)
		// GPlates: abort in a debug build, so that a debugger stops where the assertion failed.
		true;
#else
		// pyGPlates: always throw. GPLATES_PYTHON_EMBEDDING is defined only when building GPlates,
		// so this is the pygplates module - where an abort would take the host Python interpreter
		// down with it, and where these exceptions are translated into Python exceptions anyway
		// (see 'api/PyExceptions.cc').
		false;
#endif


void
GPlatesGlobal::set_assertion_failures_abort(
		bool assertion_failures_abort)
{
	AssertInternals::s_assertion_failures_abort = assertion_failures_abort;
}


void
GPlatesGlobal::Abort(
		const GPlatesUtils::CallStack::Trace &abort_location)
{
	if (assertion_failures_abort())
	{
		// Push the location of the caller onto the call stack writing out the
		// call stack trace.
		GPlatesUtils::CallStackTracker call_stack_tracker(abort_location);

		// Get the call stack trace as a string.
		std::ostringstream output_string_stream;
		GPlatesUtils::CallStack::instance().write_call_stack_trace(output_string_stream);

		// This is where the core dump or debugger trigger happens.
		// Print the call stack trace.
		qFatal("Aborting: %s",
				QString::fromStdString(output_string_stream.str()).toLatin1().data());
	}

	throw AbortException(abort_location);
}
