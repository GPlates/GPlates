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

#include "global/config.h" // GPLATES_NO_PYTHON

// The undef's are compile fixes for Windows.

#if !defined(GPLATES_NO_PYTHON)
#	if defined(HAVE_DIRECT_H)
#		undef HAVE_DIRECT_H
#	endif
#	if defined(HAVE_UNISTD_H)
#		undef HAVE_UNISTD_H
#	endif
#	if defined(ssize_t)
#		undef ssize_t
#	endif

// Partial workaround for compile error in <pyport.h> for Python versions less than 2.7.13 and 3.5.3.
// See https://bugs.python.org/issue10910
// The rest of the workaround involves including "global/python.h" at the top of some source files
// to ensure <Python.h> is included before <ctype.h>.
//
// Note: This should be included after the above HAVE_DIRECT_H definition to avoid compile error on Windows.
#	include <Python.h>

#	include <boost/python.hpp>
#endif //GPLATES_NO_PYTHON

#endif //Q_MOC_RUN

#endif  // GPLATES_GLOBAL_PYTHON_H
