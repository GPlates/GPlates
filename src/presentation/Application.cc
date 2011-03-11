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

#include "app-logic/FeatureCollectionFileState.h"

#include "global/python.h"

#include "gui/UtilitiesMenu.h"

#include "utils/StringUtils.h"


GPlatesPresentation::Application::Application() :
	d_view_state(d_application_state),
	d_viewport_window(d_application_state, d_view_state)
{
}


#if !defined(GPLATES_NO_PYTHON)
namespace
{
	using namespace boost::python;

	void
	exec_string_on_main_thread(
			GPlatesPresentation::Application &application,
			const object &string)
	{
		GPlatesApi::PythonRunner *python_runner =
			application.get_application_state().get_python_runner();
		GPlatesApi::PythonExecutionMonitor monitor;

		GPlatesApi::PythonInterpreterLocker interpreter_locker;
		try
		{
			QString qstring = GPlatesApi::PythonUtils::stringify_object(string);
			python_runner->exec_string(qstring, &monitor);
		}
		catch (const error_already_set &)
		{
			PyErr_Print();
		}
	}

	void
	exec_file_on_main_thread_with_coding(
			GPlatesPresentation::Application &application,
			const object &filename,
			const str &coding)
	{
		GPlatesApi::PythonRunner *python_runner =
			application.get_application_state().get_python_runner();
		GPlatesApi::PythonExecutionMonitor monitor;

		GPlatesApi::PythonInterpreterLocker interpreter_locker;
		try
		{
			QString qstring = GPlatesApi::PythonUtils::stringify_object(filename);
			std::string string_coding = extract<std::string>(coding);
			python_runner->exec_file(qstring, &monitor, string_coding.c_str());
		}
		catch (const error_already_set &)
		{
			PyErr_Print();
		}
	}

	void
	exec_file_on_main_thread(
			GPlatesPresentation::Application &application,
			const object &filename)
	{
		exec_file_on_main_thread_with_coding(application, filename, "utf-8"); // FIXME: hard coded coding
	}

	object
	eval_string_on_main_thread(
			GPlatesPresentation::Application &application,
			const object &string)
	{
		GPlatesApi::PythonRunner *python_runner =
			application.get_application_state().get_python_runner();
		GPlatesApi::PythonExecutionMonitor monitor;

		GPlatesApi::PythonInterpreterLocker interpreter_locker;
		try
		{
			QString qstring = GPlatesApi::PythonUtils::stringify_object(string);
			python_runner->eval_string(qstring, &monitor);
		}
		catch (const error_already_set &)
		{
			PyErr_Print();
		}
		
		return monitor.get_evaluation_result();
	}

	void
	call_utility(
			const object &utility)
	{
		utility();
	}

	void
	register_utility(
			GPlatesPresentation::Application &application,
			const object &utility)
	{
		try
		{
			object category = utility.attr("category");
			object name = utility.attr("name");

			GPlatesGui::UtilitiesMenu &utilities_menu = application.get_viewport_window().utilities_menu();
			utilities_menu.add_utility(
					GPlatesApi::PythonUtils::stringify_object(category),
					GPlatesApi::PythonUtils::stringify_object(name),
					boost::bind(
						&call_utility,
						utility));
		}
		catch (const error_already_set &)
		{
			PyErr_Print();
		}
	}

	list
	get_loaded_files(
			GPlatesPresentation::Application &application)
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
			GPlatesPresentation::Application &application,
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
}


void
export_instance()
{
	using namespace boost::python;
	using namespace GPlatesApi::DeferredApiCall;
#if _MSC_VER_ > 1400 
//Visual C++ 2005
//This code does not compile on Visual C++ 2005
	class_<GPlatesPresentation::Application, boost::noncopyable>("Instance", no_init /* not currently safe to do so */)
		.def("get_main_window",
				&GPlatesPresentation::Application::get_viewport_window,
				return_value_policy<reference_existing_object>() /* ViewportWindow is noncopyable */)
		.def("exec_string_on_main_thread",
				GPLATES_DEFERRED_API_CALL(&::exec_string_on_main_thread, ArgReferenceWrappings<ref>()))
		.def("exec_file_on_main_thread",
				GPLATES_DEFERRED_API_CALL(&::exec_file_on_main_thread, ArgReferenceWrappings<ref>()))
		.def("exec_file_on_main_thread",
				GPLATES_DEFERRED_API_CALL(&::exec_file_on_main_thread_with_coding, ArgReferenceWrappings<ref>()))
		.def("eval_string_on_main_thread",
				GPLATES_DEFERRED_API_CALL(&::eval_string_on_main_thread, ArgReferenceWrappings<ref>()))
		.def("register_utility",
				GPLATES_DEFERRED_API_CALL(&::register_utility, ArgReferenceWrappings<ref>()))
		.def("get_loaded_files", &::get_loaded_files)
		.def("get_feature_collection_from_loaded_file", &::get_feature_collection_from_loaded_file);
#endif
}
#endif

