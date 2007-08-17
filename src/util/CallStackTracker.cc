/* $Id$ */

/**
 * \file
 * Contains the definitions of the member functions of the class CallStackTracker.
 *
 * Most recent change:
 *   $Date$
 *
 * Copyright (C) 2007 The University of Sydney, Australia
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

#include <iostream>
#include "CallStackTracker.h"


GPlatesUtil::CallStackTracker::CallStackTracker(
		const char *filename,
		int line_num):
	d_filename(filename),
	d_line_num(line_num)
{
	std::cerr << "+ " << this << ": line " << d_line_num << " in file " << d_filename << std::endl;
}


GPlatesUtil::CallStackTracker::~CallStackTracker()
{
	// Don't let any exceptions escape from the destructor.
	try {
		std::cerr << "- " << this << ": line " << d_line_num << " in file " << d_filename << std::endl;
	} catch (...) {
	}
}
