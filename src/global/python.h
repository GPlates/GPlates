/* $Id$ */

/**
 * \file 
 *
 * Most recent change:
 *   $Date$
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

#ifndef GPLATES_GLOBAL_PYTHON_H
#define GPLATES_GLOBAL_PYTHON_H

// Workaround for Qt moc failing to parse BOOST_JOIN macro in Boost library.
// Workaround mentioned at https://bugreports.qt.io/browse/QTBUG-22829
#ifndef Q_MOC_RUN

#	include "global/config.h"

// The undef's are compile fixes for Windows.

#	if defined(HAVE_DIRECT_H)
#		undef HAVE_DIRECT_H
#	endif
#	if defined(HAVE_UNISTD_H)
#		undef HAVE_UNISTD_H
#	endif
#	if defined(ssize_t)
#		undef ssize_t
#	endif

// Avoid linker error on Windows where cannot find the debug library "python27_d.lib" in Debug build.
// This happens because the Windows Python installer only has a release build library and, furthermore,
// "pyconfig.h" forces linking to "python27_d.lib" in Debug build (via #pragma comment())
// whether it exists or not.
//
// So we use the same workaround provided by Boost (in "boost/python/detail/wrap_python.hpp") below.
// Note that we can't just include "boost/python/detail/wrap_python.hpp" (via <boost/python.hpp>)
// instead of <Python.h> because we have another workaround below that requires including <Python.h>
// *before* <boost/python.hpp>.
#	ifdef _DEBUG  // Note that _DEBUG appears to be specific to the Microsoft compiler.
		// Boost allows the user to specify BOOST_DEBUG_PYTHON to use the Python debugging library.
#		ifndef BOOST_DEBUG_PYTHON
#			ifdef _MSC_VER
				//
				// According to Boost...
				//
				// "VC8.0 will complain if system headers are #included both with
				//  and without _DEBUG defined, so we have to #include all the
				//  system headers used by pyconfig.h right here."
				//
#				include <stddef.h>
#				include <stdarg.h>
#				include <stdio.h>
#				include <stdlib.h>
#				include <assert.h>
#				include <errno.h>
#				include <ctype.h>
#				include <wchar.h>
#				include <basetsd.h>
#				include <io.h>
#				include <limits.h>
#				include <float.h>
#				include <string.h>
#				include <math.h>
#				include <time.h>
#			endif
#			undef _DEBUG // Avoids 'pragma comment(lib,"python27_d.lib")' in 'pyconfig.h'.
#			define DEBUG_UNDEFINED_FROM_GLOBAL_PYTHON_H
#		endif
#	endif

// Partial workaround for compile error in <pyport.h> for Python versions less than 2.7.13 and 3.5.3.
// See https://bugs.python.org/issue10910
// The rest of the workaround involves including "global/python.h" at the top of some source files
// to ensure <Python.h> is included before <ctype.h>.
//
// Note: This should be included after the above HAVE_DIRECT_H definition to avoid compile error on Windows.
#	include <Python.h>

// Re-define _DEBUG if it was undefined above (for <Python.h>).
#	ifdef DEBUG_UNDEFINED_FROM_GLOBAL_PYTHON_H
#		undef DEBUG_UNDEFINED_FROM_GLOBAL_PYTHON_H
#		define _DEBUG
#	endif

#	include <boost/python.hpp>

#endif //Q_MOC_RUN

#endif  // GPLATES_GLOBAL_PYTHON_H
