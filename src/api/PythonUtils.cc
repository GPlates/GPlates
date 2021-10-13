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
#include <QStringList>
#include <QDebug>

#include "PythonUtils.h"

#include "PythonExecutionMonitor.h"
#include "PythonExecutionThread.h"
#include "PythonInterpreterLocker.h"

#include "app-logic/UserPreferences.h"

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
		PythonExecutionThread *python_execution_thread,
		GPlatesAppLogic::UserPreferences &user_prefs)
{
	//
	// We search for scripts in the following places:
	//
	//  - "scripts" subdirectory under the current working directory,
	//
	//  - "scripts" subdirectory in a system-specific area, for included sample scripts and
	//    possible custom site-specific scripts.
	//      - Linux: /usr/share/gplates/scripts/
	//      - Mac OS X: the 'scripts/' directory is placed in the 'Resources/' directory
	//        inside the application bundle.
	//      - Windows: under the GPlates executable directory.
	//
	//  - "scripts" subdirectory in a user-specific application data area, for scripts the
	//    user has created or downloaded separately.
	//      - Linux: ~/.local/share/data/GPlates/GPlates/scripts/ yes two GPlates that is not a typo
	//      - Mac OS X: somewhere in ~/Library/Application Data/gplates.org/GPlates I think; needs confirmation.
	//      - Windows: Varies wildly depending on version, but would be found with the rest of the user data.
	//
	// The system-specific and user-specific scripts locations default to the most appropriate
	// location for the platform, as defined in UserPreferences.cc, but can be customised by the
	// user assuming we ever get a GUI for that.

	// Only attempt to run *.py and *.pyc files through the interpreter, in case the scripts dir is cluttered.
	QStringList filters;
	filters << "*.py" << "*.pyc";

	QDir cwd;
	cwd.setNameFilters(filters);
	if (cwd.cd("scripts")) {
		::run_startup_scripts(python_execution_thread, cwd);
	}

	// Look in system-specific locations for supplied sample scripts, site-specific scripts, etc.
	// The default location will be platform-dependent and is currently set up in UserPreferences.cc.
	QDir system_scripts_dir(user_prefs.get_value("paths/python_system_script_dir").toString());
	system_scripts_dir.setNameFilters(filters);
	if (system_scripts_dir.exists() && system_scripts_dir.isReadable()) {
		::run_startup_scripts(python_execution_thread, system_scripts_dir);
	}

	// Also look in user-specific application data locations for scripts the user may have made.
	// The default location will be platform dependent and is constructed based on what Qt tells us about
	// the platform conventions.
	QDir user_scripts_dir(user_prefs.get_value("paths/python_user_script_dir").toString());
	user_scripts_dir.setNameFilters(filters);
	if (user_scripts_dir.exists() && user_scripts_dir.isReadable()) {
		::run_startup_scripts(python_execution_thread, user_scripts_dir);
	}

}

#else

// Just so that this isn't an empty compilation unit.
void *gplates_python_utils_disabled = 0;

#endif

