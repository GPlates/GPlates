/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010, 2011 The University of Sydney, Australia
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

#include "PyGPlatesModule.h"


#if !defined(GPLATES_NO_PYTHON)

// Exceptions
void export_exceptions();

// utils namespace
void export_strings();

// maths namespace
void export_date_line_wrapper();
void export_finite_rotation();
void export_float();
void export_geometries_on_sphere();
void export_great_circle_arc();
void export_integer();
void export_lat_lon_point();
void export_local_cartesian();
void export_real();
void export_vector_3d();

// file-io namespace
void export_feature_collection_file_format_registry();

// model namespace
void export_feature();
void export_feature_collection();
void export_geo_time_instant();
void export_ids();
void export_information_model();
void export_old_feature(); // TODO: Remove this once transitioned to 'export_feature()'.
void export_old_feature_collection();
void export_property_values();
void export_property_value_visitor();
void export_qualified_xml_names();
void export_revisioned_vector();
void export_top_level_property();

// app-logic namespace
void export_reconstruct();
void export_reconstruction_geometries();
void export_reconstruction_tree();
void export_rotation_model();

// api directory.
void export_version();
void export_console_reader();
void export_console_writer();

// presentation directory.
void export_instance();

void export_style();

// qt-widgets directory.
void export_main_window();

void export_co_registration();

void export_colour();
void export_topology_tools();

void export_coregistration_layer_proxy();


/**
 * Export the part of the python API consisting of C++ python bindings (ie, not pure python).
 */
void
export_cpp_python_api()
{
	// Register python exceptions.
	//
	// By default our GPlates C++ exceptions are translated to python's 'RuntimeError' exception with
	// a string message from 'exc.what()' - if they inherit (indirectly) from 'std::exception'
	// (which they all should do) - otherwise they get translated to 'RuntimeError' with a message of
	// "unidentifiable C++ exception".
	// So we only need to explicitly register exceptions that we don't want translated to 'RuntimeError'.
	// This is usually an exception we want the python user to be able to catch as a specific error,
	// as opposed to 'RuntimeError' which could be caused by anything really. For example:
	//
	//   try:
	//       feature_collection_file_format_registry.read(filename)
	//   except pygplates.FileFormatNotSupportedError:
	//       # Handle unrecognised file format.
	//       ...
	//
	export_exceptions();


#ifdef GPLATES_PYTHON_EMBEDDING
	// api directory.
	export_console_reader();
	export_console_writer();
	
	// presentation directory.
	export_instance();

	// qt-widgets directory.
	export_main_window();

	export_style();
	
	//export_topology_tools();

	export_coregistration_layer_proxy();
#endif	
	// utils namespace
	export_strings();

	// api directory
	export_version(); // Must be called after 'export_strings()'.

	// maths namespace
	export_float(); // Must be called before 'export_geometries_on_sphere()'.
	export_real(); // Must be called before 'export_geometries_on_sphere()'.
	export_finite_rotation();
	export_great_circle_arc();
	export_geometries_on_sphere();
	export_integer();
	export_lat_lon_point();
	export_date_line_wrapper();
	export_vector_3d();
	export_local_cartesian();

	// file-io namespace
	export_feature_collection_file_format_registry();

	// model namespace
	export_geo_time_instant(); // Must be called before 'export_feature()'.
	export_ids(); // Must be called before 'export_feature()'.
	export_information_model(); // Must be called before 'export_feature()'.
	export_qualified_xml_names(); // Must be called before 'export_feature()'.
	export_feature();
	export_feature_collection();
	export_old_feature(); // TODO: Remove this once transitioned to 'export_feature()'.
	export_old_feature_collection();
	export_property_values();
	export_property_value_visitor();
	export_revisioned_vector();
	export_top_level_property();

	// app-logic namespace
	export_reconstruct();
	export_reconstruction_geometries();
	export_reconstruction_tree();
	export_rotation_model();

	//export_co_registration();
	export_colour();
}



// Export the part of the python API that is *pure* python code (ie, not C++ python bindings).
void export_pure_python_api();


namespace
{
	boost::python::object pygplates_module;
}


boost::python::object
GPlatesApi::get_pygplates_module()
{
	return pygplates_module;
}


BOOST_PYTHON_MODULE(pygplates)
{
	namespace bp = boost::python;

	// The 'pygplates' module is the current scope.
	pygplates_module = bp::scope();


	//
	// Specify the 'pygplate' module's docstring options.
	//
	// Note that we *disable* python and C++ signatures since we explicitly specify the
	// signatures in the first line of each function's (or class method's) docstring.
	// Sphinx is used to generate API documentation (see http://sphinx-doc.org) and it
	// uses the first docstring line as the function signature (if it looks like a signature).
	//
	// The following limitations apply to using ReStructuredText in Sphinx's autodoc extension
	// (autodoc imports modules and looks up their docstrings):
	//  - '::' to indicate end-of-paragraph must be on a separate line,
	//  - the docstrings on special methods such as '__init__', '__str__', '__lt__' are ignored
	//    by Sphinx (by default). However we use the :special-members: Sphinx directive which includes
	//    all special members. Normally this is too much, but we ask Sphinx not to document classes
	//    or methods that have no docstring - and our current policy is not to have docstrings for
	//    special members other than '__init__'.
	//    We could have used the "autoclass_content='both'" setting in the 'conf.py' file to only
	//    include the '__init__' special method, but it concatenates '__init__'s docstring into
	//    the class docstring and we'd rather keep it separate since ':param:', ':type:' and ':rtype:'
	//    directives (in docstrings) only work when applied within a *method* docstring (ie, no class docstring).
	//
	bp::docstring_options module_docstring_options(
			true/*show_user_defined*/,
			false/*show_py_signatures*/,
			false/*show_cpp_signatures*/);

	// Set the 'pygplates' module docstring.
	pygplates_module.attr("__doc__") =
			"**GPlates Python Application Programming Interface (API)**\n"
			"\n"
			"  A Python module consisting of classes and functions providing access to "
			"GPlates functionality.\n";

	// Set the array type (and module) to numpy array (so we can use bp::numeric::array in C++ code).
	bp::numeric::array::set_module_and_type("numpy", "ndarray");

	// Inject the __builtin__ module into the 'pygplates' module's __dict__.
	//
	// This enables us to pass the 'pygplates' __dict__ as the globals/locals parameter
	// of 'bp::exec' in order to add pure python source code to the python API (to complement our
	// our C++ bindings API). The reason for injecting __builtin__ is our boost-python 'pygplates'
	// module doesn't have it by default (like pure python modules do) and it is needed if
	// our pure python code is using the 'import' statement for example.
	// And by using 'pygplates's __dict__ instead of __main__'s __dict__ the classes/functions
	// in our pure python code will get automatically added to the 'pygplates' module, whereas this
	// would need to be done explicitly for each pure python function/class defined (if we used __main__).
	// It also means our pure python functions/classes have a '__module__' attribute of 'pygplates'.
	// It also means our pure python API code does not need to prefix 'pygplates.' when it calls the
	// 'pygplates' API (whether that is, in turn, pure python or C++ bindings doesn't matter).
	pygplates_module.attr("__dict__")["__builtins__"] = bp::import("__builtin__");

	// Export the part of the python API that consists of C++ python bindings (ie, not pure python).
	export_cpp_python_api();

	// Export any *pure* python code that contributes to the python API.
	//
	// We've already exported all the C++ python bindings - this is important because the pure python
	// code injects methods into the python classes already defined by the C++ python bindings.
	export_pure_python_api();
}

#else

// Dummy variable so that this compilation unit isn't empty.
void *dummy_gplates_can_haz_python = 0;

#endif

