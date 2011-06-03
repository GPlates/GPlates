/* $Id: ApplicationState.cc 11614 2011-05-20 15:10:51Z jcannon $ */

/**
 * \file 
 * $Revision: 11614 $
 * $Date: 2011-05-21 01:10:51 +1000 (Sat, 21 May 2011) $
 * 
 * Copyright (C) 2009, 2010, 2011 The University of Sydney, Australia
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
#include "PythonManager.h"

#include "api/PythonExecutionThread.h"
#include "api/PythonInterpreterLocker.h"
#include "api/PythonRunner.h"
#include "api/Sleeper.h"
#include "app-logic/ApplicationState.h"

void
GPlatesUtils::PythonManager::initialize(GPlatesAppLogic::ApplicationState& state) 
{
#if !defined(GPLATES_NO_PYTHON)
	using namespace boost::python;

	// Hold references to the main module and its namespace for easy access from
	// all parts of GPlates.
	GPlatesApi::PythonInterpreterLocker interpreter_locker;
	try
	{
		d_python_main_module = import("__main__");
		d_python_main_namespace = d_python_main_module.attr("__dict__");
	}
	catch (const error_already_set &)
	{
		PyErr_Print();
	}
#endif

	// These two must be set up after d_python_main_module and
	// d_python_main_namespace have been set.
	d_python_runner = new GPlatesApi::PythonRunner(state, d_python_main_namespace,this);
	d_python_execution_thread = new GPlatesApi::PythonExecutionThread(state,d_python_main_namespace, this);
	d_python_execution_thread->start(QThread::IdlePriority);
	d_inited = true;
}

GPlatesUtils::PythonManager::~PythonManager()
{
	// Stop the Python execution thread.
	static const int WAIT_TIME = 1000 /* milliseconds */;
	d_python_execution_thread->quit_event_loop();
	d_python_execution_thread->wait(WAIT_TIME);
	d_python_execution_thread->terminate();
}

