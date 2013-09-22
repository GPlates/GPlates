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

//
// Note: this .cc file has no corresponding .h file.
//

#if !defined(GPLATES_NO_PYTHON)

// Exceptions
void export_exceptions();

// utils namespace
void export_strings();

// maths namespace
void export_finite_rotation();
void export_great_circle_arc();
void export_geometries_on_sphere();
void export_lat_lon_point();
void export_real();
void export_unit_quaternion_3d();
void export_unit_vector_3d();

// model namespace
void export_feature();
void export_feature_collection();
void export_feature_collection_file_format_registry();
void export_ids();
void export_old_feature(); // TODO: Remove this once transitioned to 'export_feature()'.
void export_old_feature_collection();
void export_property_values();
void export_property_value_visitor();
void export_qualified_xml_names();
void export_revisioned_vector();
void export_reconstruction_tree();
void export_top_level_property();

// api directory.
void export_console_reader();
void export_console_writer();

// presentation directory.
void export_instance();

void export_style();

// qt-widgets directory.
void export_main_window();

void export_co_registration();

void export_functions();

void export_colour();
void export_topology_tools();

void export_coregistration_layer_proxy();

BOOST_PYTHON_MODULE(pygplates)
{
	//
	// Specify the 'pygplate' module's docstring options.
	//
	// Note that we *disable* python and C++ signatures since we explicitly specify the
	// signatures in the first line of each function's (or class method's) docstring.
	// Sphinx is used to generate API documentation (see http://sphinx-doc.org) and it
	// uses the first docstring line as the function signature (if it looks like a signature).
	//
	// The following limitations apply to using ReStructuredText in Sphinx's autodoc extension
	// (autodoc imports modules and looks up there docstrings):
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
	boost::python::docstring_options module_docstring_options(
			true/*show_user_defined*/,
			false/*show_py_signatures*/,
			false/*show_cpp_signatures*/);

	// Set the 'pygplates' module docstring.
	boost::python::scope().attr("__doc__") =
			"GPlates python Application Programming Interface (API)\n"
			"------------------------------------------------------\n"
			"\n"
			"  This document lists the python classes and functions that make up the 'pygplates' "
			"module that provides the API into GPlates functionality. Within each class is a list "
			" of class methods and a description of their usage and parameters.\n"
			"\n"
			"  Before GPlates functionality can be used, the 'pygplates' module "
			"should be imported. For example:"
			"  ::\n"
			"\n"
			"    import pygplates\n";


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

	// maths namespace
	export_finite_rotation();
	export_great_circle_arc();
	export_geometries_on_sphere();
	export_lat_lon_point();
	export_real();
	export_unit_quaternion_3d();
	export_unit_vector_3d();

	// model namespace
	export_ids(); // Must be called before 'export_feature()'.
	export_qualified_xml_names(); // Must be called before 'export_feature()'.
	export_feature();
	export_feature_collection();
	export_feature_collection_file_format_registry();
	export_old_feature(); // TODO: Remove this once transitioned to 'export_feature()'.
	export_old_feature_collection();
	export_property_values();
	export_property_value_visitor();
	export_reconstruction_tree();
	export_revisioned_vector();
	export_top_level_property();

	//export_co_registration();
	export_functions();
	export_colour();
}

#else

// Dummy variable so that this compilation unit isn't empty.
void *dummy_gplates_can_haz_python = 0;

#endif

