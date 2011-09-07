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

#include "global/CompilerWarnings.h"
#include "global/python.h"

#include "gui/DrawStyleManager.h"
#include "gui/FeatureFocus.h"
#include "gui/UtilitiesMenu.h"

#include "utils/StringUtils.h"
#include "gui/PythonManager.h"


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
		Application() : d_app(*GPlatesPresentation::Application::instance()) { }

		GPlatesQtWidgets::ViewportWindow&
		get_viewport_window()
		{
			return GPlatesPresentation::Application::instance()->get_viewport_window();
		}

		void
		exec_gui_string(
				const char* str)
		{
			if(!GPlatesApi::PythonUtils::is_main_thread())
			{
				boost::function<void ()> f = boost::bind(
						&GPlatesAPI::Application::exec_gui_string,
						this, 
						str);
				return GPlatesApi::PythonUtils::run_in_main_thread(f);
			}

			QString code = QString() + "exec(\"" + str + "\")";
			GPlatesApi::PythonInterpreterLocker interpreter_locker;
			PyRun_SimpleString(code.toStdString().c_str());
		}


		void
		eval_gui_string(
				const char* str)
		{
			if(!GPlatesApi::PythonUtils::is_main_thread())
			{
				boost::function<void ()> f = boost::bind(
						&GPlatesAPI::Application::eval_gui_string,
						this, 
						str);
				return GPlatesApi::PythonUtils::run_in_main_thread(f);
			}

			QString code = QString() + "eval(\"" + str + "\")";
			GPlatesApi::PythonInterpreterLocker interpreter_locker;
			PyRun_SimpleString(code.toStdString().c_str());
		}


		void
		exec_gui_file(
				const char* filepath)
		{
			if(!GPlatesApi::PythonUtils::is_main_thread())
			{
				boost::function<void ()> f = boost::bind(
						&GPlatesAPI::Application::exec_gui_file,
						this, 
						filepath);
				return GPlatesApi::PythonUtils::run_in_main_thread(f);
			}

			QString code = QString() + "execfile(\"" + filepath + "\")";
			GPlatesApi::PythonInterpreterLocker interpreter_locker;
			PyRun_SimpleString(code.toStdString().c_str());
		}


		void
		register_utility(
				const object &utility)
		{
			if(!GPlatesApi::PythonUtils::is_main_thread())
			{
				boost::function<void ()> f = 
					boost::bind(
							&GPlatesAPI::Application::register_utility,
							this, 
							utility);
				return GPlatesApi::PythonUtils::run_in_main_thread(f);
			}
			
			try
			{
				object category = utility.attr("category");
				object name = utility.attr("name");
				utility.attr("__call__");//make sure it has __call__ function.

				GPlatesGui::UtilitiesMenu &utilities_menu = d_app.get_viewport_window().utilities_menu();
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

		void
		register_draw_style(const object &style)
		{
			if(!GPlatesApi::PythonUtils::is_main_thread())
			{
				boost::function<void ()> f = 
					boost::bind(
							&GPlatesAPI::Application::register_draw_style,
							this, 
							style);
				return GPlatesApi::PythonUtils::run_in_main_thread(f);
			}
			const GPlatesGui::StyleCatagory* sc = 
				GPlatesGui::DrawStyleManager::instance()->register_style_catagory("python style");
			GPlatesGui::DrawStyleManager::instance()->register_style(
					new GPlatesGui::PythonStyleAdapter(style,sc));

		}

		list
		get_loaded_files()
		{
			using namespace GPlatesAppLogic;

			list result;

			FeatureCollectionFileState &file_state = d_app.get_application_state().get_feature_collection_file_state();
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

			FeatureCollectionFileState &file_state = d_app.get_application_state().get_feature_collection_file_state();
			BOOST_FOREACH(const FeatureCollectionFileState::file_reference &file, file_state.get_loaded_files())
			{
				const GPlatesFileIO::File::Reference &file_ref = file.get_file();
				QString file_path = file_ref.get_file_info().get_qfileinfo().absoluteFilePath();
				if (file_path == qstring_filename)
				{
					return object();
					//return object(GPlatesApi::FeatureCollection::create(file_ref.get_feature_collection()));
				}
			}

			return object();
		}


		DISABLE_GCC_WARNING("-Wshadow")
		void
		set_focus(
				boost::python::object id)
		{
			if(!GPlatesApi::PythonUtils::is_main_thread())
			{
				boost::function<void ()> f = boost::bind(
						&GPlatesAPI::Application::set_focus,
						this, 
						id);
				return GPlatesApi::PythonUtils::run_in_main_thread(f);
			}

			std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> files =
				d_app.get_application_state().get_feature_collection_file_state().get_loaded_files();
			
			BOOST_FOREACH(GPlatesAppLogic::FeatureCollectionFileState::file_reference& file_ref, files)
			{
				GPlatesModel::FeatureCollectionHandle::weak_ref fc = file_ref.get_file().get_feature_collection();
				BOOST_FOREACH(GPlatesModel::FeatureHandle::non_null_ptr_type feature, *fc)
				{
					if(feature->feature_id().get().qstring() == QString(boost::python::extract<const char*>(id)))
					{
						d_app.get_view_state().get_feature_focus().set_focus(feature->reference());
						return;
					}
				}
			}
			qWarning() << QString("Cannot found the feature by id: %1").arg(QString(boost::python::extract<const char*>(id)));
		}

		private:
			GPlatesPresentation::Application& d_app;
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
		.def("exec_gui_file", &Application::exec_gui_file)
		.def("exec_gui_string", &Application::exec_gui_string)
		.def("eval_gui_string", &Application::eval_gui_string)
		.def("register_utility", &Application::register_utility)
		.def("get_loaded_files", &Application::get_loaded_files)
		.def("get_feature_collection_from_loaded_file", &Application::get_feature_collection_from_loaded_file)
		.def("set_focus", &Application::set_focus)
		.def("register_draw_style", &Application::register_draw_style)
		;
}

#include "gui/DrawStyleManager.h"

void
export_style()
{
	using namespace boost::python;

	class_<GPlatesGui::Feature>("Feature")
		;
	class_<GPlatesGui::DrawStyle>("DrawStyle")
		.def_readwrite("Dummy", &GPlatesGui::DrawStyle::dummy)
		;
}

#endif

