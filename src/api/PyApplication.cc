/* $Id:  $ */

/**
 * \file 
 * $Revision: 7584 $
 * $Date: 2010-02-10 19:29:36 +1100 (Wed, 10 Feb 2010) $
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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
#include "global/python.h"
#if !defined(GPLATES_NO_PYTHON)

#include <boost/foreach.hpp>

#include "PyFeatureCollection.h"
#include "PythonRunner.h"
#include "DeferredApiCall.h"
#include "PythonExecutionMonitor.h"
#include "PythonInterpreterLocker.h"
#include "PythonUtils.h"
#include "Sleeper.h"

#include "app-logic/FeatureCollectionFileState.h"

#include "global/CompilerWarnings.h"
#include "global/python.h"

#include "gui/DrawStyleManager.h"
#include "gui/FeatureFocus.h"
#include "gui/PythonManager.h"
#include "gui/UtilitiesMenu.h"

#include "presentation/Application.h"

#include "qt-widgets/ViewportWindow.h"

#include "utils/StringUtils.h"

namespace
{
	void
	call_utility(
			const boost::python::object &utility)
	{
		if(GPlatesApi::PythonUtils::is_gui_object(utility))
		{
			DISPATCH_GUI_FUN<void>(boost::bind(&call_utility, utility));
		}

		try
		{
			GPlatesApi::PythonInterpreterLocker l;
			utility();
		}
		catch (const boost::python::error_already_set &)
		{
			qWarning() << GPlatesApi::PythonUtils::get_error_message();
		}
	}
}
namespace GPlatesApi
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
			DISPATCH_GUI_FUN<void>(boost::bind(&Application::exec_gui_string, this, str));

			QString code = QString() + "exec(\"" + QString::fromUtf8(str) + "\")";
			PythonInterpreterLocker l;
			PyRun_SimpleString(code.toStdString().c_str());
		}


		void
		eval_gui_string(
				const char* str)
		{
			DISPATCH_GUI_FUN<void>(boost::bind(&Application::eval_gui_string, this, str));

			QString code = QString() + "eval(\"" + QString::fromUtf8(str) + "\")";
			PythonInterpreterLocker l;
			PyRun_SimpleString(code.toStdString().c_str());
		}


		void
		exec_gui_file(
				const char* filepath)
		{
			DISPATCH_GUI_FUN<void>(boost::bind(&Application::exec_gui_file,this, filepath));

			QString code = QString() + "execfile(\"" + QString::fromUtf8(filepath) + "\")";
			PythonInterpreterLocker l;
			PyRun_SimpleString(code.toStdString().c_str());
		}


		void
		register_utility(
				const object &utility)
		{
			DISPATCH_GUI_FUN<void>(boost::bind(&Application::register_utility, this, utility));
			
			try
			{
				object category = utility.attr("category");
				object name = utility.attr("name");
				utility.attr("__call__");//make sure it has __call__ function.

				GPlatesGui::UtilitiesMenu &utilities_menu = d_app.get_viewport_window().utilities_menu();
				utilities_menu.add_utility(
						PythonUtils::to_QString(category),
						PythonUtils::to_QString(name),
						boost::bind(&call_utility,utility));
			}
			catch (const error_already_set &)
			{
				qWarning() << PythonUtils::get_error_message();
			}
		}

		void
		register_draw_style(object &style)
		{
			using namespace GPlatesGui;
			try
			{
				PythonInterpreterLocker lock;
				//use python class name as catagory name.
				boost::python::object py_class = style.attr("__class__");
				QString cata_name	=  QString::fromUtf8(boost::python::extract<const char*>(py_class.attr("__name__")));

				DrawStyleManager* mgr = DrawStyleManager::instance();
				const StyleCatagory* sc = mgr->get_catagory(cata_name);
			
				if(!sc)
					sc = mgr->register_style_catagory(cata_name);

				//register the original python object as template
				PythonStyleAdapter* template_adapter = new PythonStyleAdapter(style, *sc);
				mgr->register_template_style(sc, template_adapter); 

				//register all built-in variants
				BOOST_FOREACH(StyleAdapter* adapter, mgr->get_built_in_styles(*sc))
				{
					mgr->register_style(adapter, true);
				}

				//register all saved variants
				BOOST_FOREACH(StyleAdapter* adapter, mgr->get_saved_styles(*sc))
				{
					mgr->register_style(adapter);
				}

				if(mgr->get_styles(*sc).size() == 0)
				{
					StyleAdapter* new_adapter = mgr->get_template_style(*sc)->deep_clone();
					new_adapter->set_name("Default");
					mgr->register_style(new_adapter);
				}
			}
			catch(const error_already_set &)
			{
				qWarning() << PythonUtils::get_error_message();
			}
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
				QByteArray ba = file_path.toUtf8();
				const char* s = ba.data();
				result.append(boost::python::str(s));
			}

			return result;
		}

		object
		get_feature_collection_from_loaded_file(
				const object &filename)
		{
			using namespace GPlatesAppLogic;

			QString qstring_filename = PythonUtils::to_QString(filename);

			FeatureCollectionFileState &file_state = d_app.get_application_state().get_feature_collection_file_state();
			BOOST_FOREACH(const FeatureCollectionFileState::file_reference &file, file_state.get_loaded_files())
			{
				GPlatesFileIO::File::Reference &file_ref = file.get_file();
				QString file_path = file_ref.get_file_info().get_qfileinfo().absoluteFilePath();
				if (file_path == qstring_filename)
				{
					return object(FeatureCollection::create(file_ref.get_feature_collection()));
				}
			}
			return object();
		}

		list
		feature_collections()
		{
			using namespace GPlatesAppLogic;
			list result;
			FeatureCollectionFileState &file_state = d_app.get_application_state().get_feature_collection_file_state();

			BOOST_FOREACH(const FeatureCollectionFileState::file_reference &file, file_state.get_loaded_files())
			{
				result.append(FeatureCollection::create(file.get_file().get_feature_collection()));
			}
			return result;
		}

		double
		current_time()
		{
			return d_app.get_application_state().get_current_reconstruction_time();
		}

		private:
			GPlatesPresentation::Application& d_app;
	};
	
}

void
export_instance()
{
	using namespace boost::python;
	using namespace GPlatesApi;

	class_<Application, boost::noncopyable>("Application")
		.def("get_main_window",
				&Application::get_viewport_window,
				return_value_policy<reference_existing_object>()) // ViewportWindow is noncopyable 
		//.def("exec_gui_file", &Application::exec_gui_file)
		//.def("exec_gui_string", &Application::exec_gui_string)
		//.def("eval_gui_string", &Application::eval_gui_string)
		.def("register_utility", &Application::register_utility)
		.def("get_loaded_files", &Application::get_loaded_files)
		//.def("get_feature_collection_from_loaded_file", &Application::get_feature_collection_from_loaded_file)
		.def("register_draw_style", &Application::register_draw_style)
		.def("feature_collections", &Application::feature_collections)
		.def("current_time", &Application::current_time)
		;
}

#endif








