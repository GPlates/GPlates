/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009, 2011 The University of Sydney, Australia
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

#include <string>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <QDebug>

#include "Application.h"

#include "api/FeatureCollection.h"
#include "api/PythonRunner.h"
#include "api/DeferredApiCall.h"
#include "api/PythonExecutionMonitor.h"
#include "api/PythonInterpreterLocker.h"
#include "api/PythonUtils.h"
#include "api/Sleeper.h"
#include "app-logic/FeatureCollectionFileState.h"

#include "global/python.h"

#include "gui/UtilitiesMenu.h"

#include "utils/StringUtils.h"
#include "utils/PythonManager.h"


GPlatesPresentation::Application::Application() :
	d_view_state(d_application_state),
	d_viewport_window(d_application_state, d_view_state)
{ }

#if !defined(GPLATES_NO_PYTHON)
namespace
{
	using namespace boost::python;
	void
	call_utility(
			const object &utility)
	{
		utility();
	}
}
namespace GPlatesAPI
{
	using namespace boost::python;

	class Application
	{
	public:
		Application() : application(*GPlatesPresentation::Application::instance()) { }

		GPlatesQtWidgets::ViewportWindow&
		get_viewport_window()
		{
			return GPlatesPresentation::Application::instance()->get_viewport_window();
		}

		void
		exec_gui_string(
				const char* str)
		{
			QString code = QString() + "exec(\"" + str + "\")";
			GPlatesApi::PythonInterpreterLocker interpreter_locker;
			PyRun_SimpleString(code.toStdString().c_str());
		}

		void _exec_gui_string(
				const char* str)
		{

			boost::function<void ()> f = 
				boost::bind(
						&GPlatesAPI::Application::exec_gui_string,
						this, 
						str);
			GPlatesApi::PythonUtils::run_in_main_thread(f);
		}

		void
		eval_gui_string(
				const char* str)
		{
			QString code = QString() + "eval(\"" + str + "\")";
			GPlatesApi::PythonInterpreterLocker interpreter_locker;
			PyRun_SimpleString(code.toStdString().c_str());
		}

		void
		_eval_gui_string(
				const char* str)
		{
			boost::function<void ()> f = 
				boost::bind(
						&GPlatesAPI::Application::eval_gui_string,
						this, 
						str);
			GPlatesApi::PythonUtils::run_in_main_thread(f);
		}

		void
		exec_gui_file(
				const char* filepath)
		{
			QString code = QString() + "execfile(\"" + filepath + "\")";
			GPlatesApi::PythonInterpreterLocker interpreter_locker;
			PyRun_SimpleString(code.toStdString().c_str());
		}

		void
		_exec_gui_file(
				const char* str)
		{
			boost::function<void ()> f = 
				boost::bind(
						&GPlatesAPI::Application::exec_gui_file,
						this, 
						str);
			GPlatesApi::PythonUtils::run_in_main_thread(f);
		}

		void
		register_utility(
				const object &utility)
		{
			if(GPlatesApi::PythonUtils::is_main_thread())
			{
				try
				{
					object category = utility.attr("category");
					object name = utility.attr("name");
					utility.attr("__call__");//make sure it has __call__ function.

					GPlatesGui::UtilitiesMenu &utilities_menu = application.get_viewport_window().utilities_menu();
					utilities_menu.add_utility(
							GPlatesApi::PythonUtils::stringify_object(category),
							GPlatesApi::PythonUtils::stringify_object(name),
							boost::bind(&call_utility,utility));
				}
				catch (const error_already_set &)
				{
					qWarning() << GPlatesApi::PythonUtils::get_error_message();
				}
			}
			else
			{
				boost::function<void ()> f = 
					boost::bind(
							&GPlatesAPI::Application::register_utility,
							this, 
							utility);
				GPlatesApi::PythonUtils::run_in_main_thread(f);
			}
		}

		list
		get_loaded_files()
		{
			using namespace GPlatesAppLogic;

			list result;

			FeatureCollectionFileState &file_state = application.get_application_state().get_feature_collection_file_state();
			BOOST_FOREACH(const FeatureCollectionFileState::file_reference &file, file_state.get_loaded_files())
			{
				QString file_path = file.get_file().get_file_info().get_qfileinfo().absoluteFilePath();
				std::wstring wstring = GPlatesUtils::make_wstring_from_qstring(file_path);
				result.append(wstring);
			}

			return result;
		}

		object
		get_feature_collection_from_loaded_file(
				const object &filename)
		{
			using namespace GPlatesAppLogic;

			QString qstring_filename = GPlatesApi::PythonUtils::stringify_object(filename);

			FeatureCollectionFileState &file_state = application.get_application_state().get_feature_collection_file_state();
			BOOST_FOREACH(const FeatureCollectionFileState::file_reference &file, file_state.get_loaded_files())
			{
				const GPlatesFileIO::File::Reference &file_ref = file.get_file();
				QString file_path = file_ref.get_file_info().get_qfileinfo().absoluteFilePath();
				if (file_path == qstring_filename)
				{
					return object(GPlatesApi::FeatureCollection::create(file_ref.get_feature_collection()));
				}
			}

			return object();
		}

		private:
			GPlatesPresentation::Application& application;
	};
}

void
export_instance()
{
	using namespace boost::python;
	using namespace GPlatesApi::DeferredApiCall;
	using namespace GPlatesAPI;

	class_<Application, boost::noncopyable>("Application")
		.def("get_main_window",
				&Application::get_viewport_window,
				return_value_policy<reference_existing_object>() /* ViewportWindow is noncopyable */)
		.def("exec_gui_file", &Application::_exec_gui_file)
		.def("exec_gui_string", &Application::_exec_gui_string)
		.def("eval_gui_string", &Application::_eval_gui_string)
		.def("register_utility", &Application::register_utility)
		.def("get_loaded_files", &Application::get_loaded_files)
		.def("get_feature_collection_from_loaded_file", &Application::get_feature_collection_from_loaded_file);
}
#endif

