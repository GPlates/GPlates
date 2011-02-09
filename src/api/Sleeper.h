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

#ifndef GPLATES_API_SLEEPER_H
#define GPLATES_API_SLEEPER_H

#include "global/python.h"


namespace GPlatesApi
{
	/**
	 * On construction, replaces Python's time.sleep with our own functor and on
	 * destruction restores the original time.sleep.
	 *
	 * The reason for this replacement is that the function that we are using to
	 * interrupt threads (PyThreadState_SetAsyncExc) fails to interrupt the built-in
	 * time.sleep. This replacement calls the built-in time.sleep in small
	 * increments until the requested sleep time has passed.
	 */
	class Sleeper
	{
	public:

		explicit
		Sleeper();

		~Sleeper();

	private:

#if !defined(GPLATES_NO_PYTHON)
		boost::python::object d_old_object;
#endif
	};
}

#endif  // GPLATES_API_SLEEPER_H
