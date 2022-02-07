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

#include <QApplication>
#include <QDebug>
#include <QMetaType>
#include <QString>
#include <QThread>

#include <boost/any.hpp>

#include "api/PythonInterpreterLocker.h"
#include "api/PythonInterpreterUnlocker.h"

#include "gui/PythonManager.h"

#if !defined(GPLATES_NO_PYTHON)

#define DISPATCH_GUI_FUN \
	if(!GPlatesApi::PythonUtils::is_main_thread())  \
		return GPlatesApi::PythonUtils::run_in_main_thread

namespace GPlatesAppLogic
{
	// Forward declarations.
	class UserPreferences;
}

namespace GPlatesApi
{
	// Forward declarations.
	class PythonExecutionThread;

	namespace PythonUtils
	{
		class ThreadSwitchGuard
		{
		public:
			ThreadSwitchGuard()
			{
				d_gil_state = PyGILState_Ensure();
				d_thread_state =  PyEval_SaveThread();
			}

			~ThreadSwitchGuard()
			{
				PyEval_RestoreThread(d_thread_state);
				PyGILState_Release(d_gil_state);
			}
		private:
			PyGILState_STATE d_gil_state;
			PyThreadState *d_thread_state;
		};
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
		to_QString(const boost::python::object &obj);

		/**
		 * Runs all startup scripts in pre-defined search directories on the given
		 * @a python_execution_thread.
		 */
		void
		run_startup_scripts(
				PythonExecutionThread *python_execution_thread,
				GPlatesAppLogic::UserPreferences &user_prefs);

		QString
		get_error_message();

		inline
		bool
		is_main_thread()
		{
			return QThread::currentThread() == qApp->thread();
		}

		inline
		GPlatesGui::PythonManager&
		python_manager()
		{
			return *GPlatesGui::PythonManager::instance();
		}
		
		template<class Type>
		void
		helper_fun(
				const boost::function< Type () > &f,
				boost::any* ret)
		{
			*ret = f();
		}

		template<class ReturnType>
		ReturnType
		run_in_main_thread(
				const boost::function< ReturnType () > &f)
		{
			if(is_main_thread())
				return f();

			boost::any retVal;
			boost::function<void ()> fun = 
				boost::bind(
						&helper_fun<ReturnType>,
						f,
						&retVal);
			qRegisterMetaType< boost::function< void () > >("boost::function< void () >");
			ThreadSwitchGuard g;
			QMetaObject::invokeMethod(
				&python_manager(), 
				"exec_function_slot", 
				Qt::BlockingQueuedConnection,
				Q_ARG(boost::function<void () > , fun)
				);
			return  boost::any_cast<ReturnType>(retVal);
		}
		
		template<>
		inline
		void
		run_in_main_thread(
				const boost::function< void () > &f)
		{
			qRegisterMetaType< boost::function< void () > >("boost::function< void () >");
			ThreadSwitchGuard g;
			QMetaObject::invokeMethod(
					&python_manager(), 
					"exec_function_slot", 
					Qt::BlockingQueuedConnection,
					Q_ARG(boost::function< void () > , f));
			return ;
		}


		inline
		bool
		is_gui_object(const boost::python::object &obj)
		{
			bool ret = false;
			try
			{
				PythonInterpreterLocker interpreter_locker;
				ret =  boost::python::extract<bool>(obj.attr("gui_obj"));
			}
			catch (const  boost::python::error_already_set &)
			{
				qWarning() << GPlatesApi::PythonUtils::get_error_message();
			}
			return ret;
		}

		inline
		boost::python::str
		qstring_to_python_string(
				const QString& str_input)
		{
			QByteArray buf = str_input.toUtf8();
			return boost::python::str(buf.constData());
		}
	}
}
#endif   //GPLATES_NO_PYTHON)
#endif  // GPLATES_API_PYTHONUTILS_H
