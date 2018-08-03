/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#ifndef GPLATES_API_PYGPLATESMODULE_H
#define GPLATES_API_PYGPLATESMODULE_H

#include "global/python.h"


#if !defined(GPLATES_NO_PYTHON)

namespace GPlatesApi
{
	//
	// Some commonly used Python built-in attributes.
	//

	/**
	 * Returns the Python built-in 'hash()' function.
	 *
	 * This is a cached version of the 'pygplates' module attribute 'attr("__builtins__").attr("hash")'.
	 */
	boost::python::object
	get_builtin_hash();

	/**
	 * Returns the Python built-in 'iter()' function.
	 *
	 * This is a cached version of the 'pygplates' module attribute 'attr("__builtins__").attr("iter")'.
	 */
	boost::python::object
	get_builtin_iter();

	/**
	 * Returns the Python built-in 'next()' function.
	 *
	 * This is a cached version of the 'pygplates' module attribute 'attr("__builtins__").attr("next")'.
	 */
	boost::python::object
	get_builtin_next();
}

#endif   // GPLATES_NO_PYTHON

#endif // GPLATES_API_PYGPLATESMODULE_H
