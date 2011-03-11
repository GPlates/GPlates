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

#include <boost/foreach.hpp>
#include <QDir>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDebug>

#include "PythonUtils.h"

#include "PythonExecutionMonitor.h"
#include "PythonExecutionThread.h"
#include "PythonInterpreterLocker.h"

#include "global/CompilerWarnings.h"

#include "utils/StringUtils.h"


#if !defined(GPLATES_NO_PYTHON)
using namespace boost::python;

// For PyUnicode_Check and PyString_Check below.
DISABLE_GCC_WARNING("-Wold-style-cast")

QString
GPlatesApi::PythonUtils::stringify_object(
		const object &obj,
		const char *coding)
{
	PythonInterpreterLocker interpreter_locker;

	object decoded;
	if (PyUnicode_Check(obj.ptr()))
	{
		decoded = obj;
	}
	else if (PyString_Check(obj.ptr()))
	{
		decoded = obj.attr("decode")(coding, "replace");
	}
	else
	{
		decoded = str(obj).attr("decode")(coding, "replace");
	}
	
	return GPlatesUtils::make_qstring_from_wstring(extract<std::wstring>(decoded));
}

// See above.
ENABLE_GCC_WARNING("-Wold-style-cast")


namespace
{
	using namespace GPlatesApi;

	void
	run_startup_scripts(
			PythonExecutionThread *python_execution_thread,
			const QDir &dir)
	{
		QFileInfoList file_list = dir.entryInfoList(QDir::Files, QDir::Name);
		BOOST_FOREACH(QFileInfo file, file_list)
		{
			PythonExecutionMonitor monitor;
			python_execution_thread->exec_file(
					file.absoluteFilePath(),
					&monitor,
					"utf-8"); // FIXME: hard coded codec
			monitor.exec();
		}
	}
}


void
GPlatesApi::PythonUtils::run_startup_scripts(
		PythonExecutionThread *python_execution_thread)
{
	//
	// We search for scripts in the following places:
	//  - "scripts" subdirectory under the current working directory, and
	//  - "scripts" subdirectory under the GPlates executable directory.
	//
	// FIXME: Make this more flexible, and look in places suitable for Linux (when
	// GPlates is installed as a package).
	//

	QDir cwd;
	if (cwd.cd("scripts"))
	{
		::run_startup_scripts(python_execution_thread, cwd);
	}

	// On MacOS X systems the 'scripts/' directory is placed in the 'Resources/' directory
	// inside the application bundle. And the 'Resources/' directory is a sibling directory
	// of the directory containing the executable.
#if defined(Q_OS_MAC) || defined(Q_WS_MAC)
	QDir app_dir(QCoreApplication::applicationDirPath() + "/../Resources");
	if (app_dir.cd("scripts"))
	{
		::run_startup_scripts(python_execution_thread, app_dir);
	}
#else
	QDir app_dir(QCoreApplication::applicationDirPath());
	if (app_dir.cd("scripts"))
	{
		::run_startup_scripts(python_execution_thread, app_dir);
	}
#endif
}

#else

// Just so that this isn't an empty compilation unit.
void *gplates_python_utils_disabled = 0;

#endif

