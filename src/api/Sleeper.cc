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

#include <iostream>

#include "Sleeper.h"

#include "PythonInterpreterLocker.h"


GPlatesApi::Sleeper::Sleeper()
{
#if !defined(GPLATES_NO_PYTHON)
	using namespace boost::python;

	PythonInterpreterLocker interpreter_locker;
	try
	{
		// Save the old time.sleep before we repalce it, so we can restore it later.
		object time_module = import("time");
		d_old_object = time_module.attr("sleep");

		// Replace time.sleep with our own functor.
		if (PyRun_SimpleString(
				"import time\n"
				"class GPlatesSleeper:\n"
				"	def __init__(self):\n"
				"		self.original_sleep = time.sleep\n"
				"	def __call__(self, duration):\n"
				"		times_per_second = 10.0\n"
				"		duration *= times_per_second\n"
				"		for i in range(int(duration)):\n"
				"			self.original_sleep(1 / times_per_second)\n"
				"			self.original_sleep(0)\n"
				"			self.original_sleep(0)\n"
				"		self.original_sleep((duration - int(duration)) / times_per_second)\n"
				"time.sleep = GPlatesSleeper()\n"
				"del time, GPlatesSleeper\n"))
		{
			throw error_already_set();
		}
	}
	catch (const error_already_set &)
	{
		std::cerr << "Could not replace time.sleep." << std::endl;
		PyErr_Print();
	}
#endif
}


GPlatesApi::Sleeper::~Sleeper()
{
#if !defined(GPLATES_NO_PYTHON)
	using namespace boost::python;

	PythonInterpreterLocker interpreter_locker;
	try
	{
		// Restore the old time.sleep.
		object time_module = import("time");
		time_module.attr("sleep") = d_old_object;
		d_old_object = object();
	}
	catch (const error_already_set &)
	{
		std::cerr << "Could not restore time.sleep." << std::endl;
		PyErr_Print();
	}
#endif
}

