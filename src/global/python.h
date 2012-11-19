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

#ifndef Q_MOC_RUN //workaround. Qt moc doesn't like BOOST_JOIN. Make them not seeing each other.

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
#	include <boost/python.hpp>
#endif //GPLATES_NO_PYTHON

#endif //Q_MOC_RUN

#endif  // GPLATES_GLOBAL_PYTHON_H
