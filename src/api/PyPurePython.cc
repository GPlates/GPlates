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

#include <QFile>
#include <QString>

#include "file-io/ErrorOpeningFileForReadingException.h"

#include "global/python.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


namespace
{
	/**
	 * Read the specified file (which will be a Qt resource) containing python source code and
	 * essentially import it into the current namespace/scope (which is currently the 'pygplates' module).
	 */
	void
	export_pure_python_code(
			QString python_code_filename)
	{
		QFile python_code_file(python_code_filename);
		// This should never fail since we are reading from files that are embedded Qt resources.
		if (!python_code_file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			throw GPlatesFileIO::ErrorOpeningFileForReadingException(
					GPLATES_EXCEPTION_SOURCE,
					python_code_filename);
		}

		// Read the entire file.
		const QByteArray python_code = python_code_file.readAll();

		// Essentially means the python code will get imported into the current module/scope (which is 'pygplates').
		bp::object pygplates_globals = bp::scope().attr("__dict__");
		// These two code segments essentially do the same thing.
		// The difference is the latter includes the filename in the exception traceback output
		// which is helpful when locating errors in the pure python API or errors in API usage
		// by python users (that only manifests inside the pure python API implementation).
#if 0
        bp::exec(python_code.constData(), pygplates_globals); 
#else
		bp::object compiled_object = bp::object(bp::handle<>(
				// Returns a new reference so no need for 'bp::borrowed'...
				Py_CompileString(
						python_code.constData(),
						python_code_filename.toLatin1().constData(),
						Py_file_input)));
		bp::object eval_object = bp::object(bp::handle<>(
				// Returns a new reference so no need for 'bp::borrowed'...
				PyEval_EvalCode(
#if PY_MAJOR_VERSION < 3
						reinterpret_cast<PyCodeObject *>(compiled_object.ptr()),
#else
						compiled_object.ptr(),
#endif
						// Note that the 'globals' and 'locals' arguments to 'PyEval_EvalCode()' should
						// be the *same* object because we are executing the code as if it were embedded
						// in the 'pygplates' *module* and according the docs for 'exec'...
						//
						//   "Remember that at module level, globals and locals are the same dictionary.
						//    If two separate objects are given as globals and locals, the code will be
						//    executed as if it were embedded in a class definition."
						//
						// If 'globals' and 'locals' are were different objects
						// (eg, 'globals' was bp::import("__main__").attr("__dict__") and
						//  'locals' was bp::scope().attr("__dict__"))
						// then the following example code...
						//
						//     x = 1
						//     def foo():
						//         print x
						//     foo()
						//
						// ...would be executed as something like...
						//
						//     class Scope(object):
						//         x = 1
						//         def foo():
						//             print x
						//         foo()
						//
						// ...and the 'print x' statement would raise an error since 'x' is not defined
						// (because it looks for a global, not class, 'x' and doesn't find one).
						// So, in our 'pygplates' module case, any reference to a 'pygplates' module variable
						// would have needed to be prefixed by 'pygplates.' since the globals dict
						// would be the "__main__" module (ie, one level up from 'pygplates').
						pygplates_globals.ptr(),
						pygplates_globals.ptr())));
#endif
	}
}


void
export_pure_python_api()
{
	//
	// Any pure python source code files (Qt resources) should be exported here.
	//
	// NOTE: These should be Qt resources that get embedded in the GPlates executable or
	// 'pygplates' shared library/DLL.
	//
	// To add a source code file as a Qt resource:
	// (1) place the python source code file in the 'src/qt-resources/python/api/' directory,
	// (2) add the file to the 'src/qt-resources/python.qrc' file,
	// (3) add a call to 'export_pure_python_code()' below and use the ':/' prefix in the filename
	//     (signals a Qt resource) plus the file's path relative to the 'src/qt-resources/' directory,
	// (4) add/commit the changes to Subversion.
	//
	// Note that since the pure python code is part of the 'pygplates' module, its source code
	// does not need to prefix 'pygplates'. if it calls the GPlates python API.
	//

	export_pure_python_code(":/python/api/Crossovers.py");
	export_pure_python_code(":/python/api/Feature.py");
	export_pure_python_code(":/python/api/GeometriesOnSphere.py");
	export_pure_python_code(":/python/api/PlatePartitioning.py");
	export_pure_python_code(":/python/api/Property.py");
	export_pure_python_code(":/python/api/PropertyValues.py");
}

#endif // GPLATES_NO_PYTHON
