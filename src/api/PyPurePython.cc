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
	void
	export_pure_python_code(
			QString python_code_filename)
	{
		QFile python_code_file(python_code_filename);
		if (!python_code_file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			throw GPlatesFileIO::ErrorOpeningFileForReadingException(
					GPLATES_EXCEPTION_SOURCE,
					python_code_filename);
		}

		// Read the entire file.
		const QByteArray python_code = python_code_file.readAll();

		// Essentially imports the python code into the current module/scope which is 'pygplates'.
		bp::exec(
				python_code.constData(),
				bp::scope().attr("__dict__"));
	}
}


void
export_pure_python_api()
{
	export_pure_python_code(":/python/api/total_reconstruction_pole.py");
}

#endif // GPLATES_NO_PYTHON
