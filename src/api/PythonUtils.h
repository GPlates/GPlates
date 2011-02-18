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

#ifndef GPLATES_API_PYTHONUTILS_H
#define GPLATES_API_PYTHONUTILS_H

// This needs to go before the <QString> due to compile problems on the Mac.
#include "global/python.h"

#include <QString>


namespace GPlatesApi
{
	// Forward declarations.
	class PythonExecutionThread;

	namespace PythonUtils
	{
#if !defined(GPLATES_NO_PYTHON)
		/**
		 * Stringifies @a obj.
		 *
		 * If @a obj is a Python unicode object, it is converted to a QString without
		 * any encoding/decoding.
		 *
		 * If @a obj is a Python str object, it is decoded using @a coding into a
		 * Python unicode object first.
		 *
		 * If it is any other type of object, it is converted into a Python str object
		 * and then dealt with as above.
		 *
		 * @throws boost::python::error_already_set.
		 */
		QString
		stringify_object(
				const boost::python::object &obj,
				const char *coding = "ascii");

		/**
		 * Runs all startup scripts in pre-defined search directories on the given
		 * @a python_execution_thread.
		 */
		void
		run_startup_scripts(
				PythonExecutionThread *python_execution_thread);
#endif
	}
}

#endif  // GPLATES_API_PYTHONUTILS_H
