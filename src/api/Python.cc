/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010, 2011 The University of Sydney, Australia
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

#include "global/python.h"

//
// Note: this .cc file has no corresponding .h file.
//

#if !defined(GPLATES_NO_PYTHON)

// api directory.
void export_console_reader();
void export_console_writer();

// app-logic directory.
void export_application_state();

BOOST_PYTHON_MODULE(pygplates)
{
	// api directory.
	export_console_reader();
	export_console_writer();

	// app-logic directory.
	export_application_state();
}

#else

// Dummy variable so that this compilation unit isn't empty.
void *dummy_gplates_can_haz_python = 0;

#endif

