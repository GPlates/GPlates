/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2021 The University of Sydney, Australia
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

#include "PyResolveTopologyParameters.h"

#include "PythonConverterUtils.h"

#include "global/python.h"


namespace bp = boost::python;


namespace GPlatesApi
{
	namespace
	{
		/**
		 * This is called directly from Python via 'ResolveTopologyParameters.__init__()'.
		 */
		ResolveTopologyParameters::non_null_ptr_type
		resolve_topology_parameters_create()
		{
			return ResolveTopologyParameters::create();
		}
	}
}


void
export_resolve_topology_parameters()
{
	//
	// ResolveTopologyParameters - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesApi::ResolveTopologyParameters,
			GPlatesApi::ResolveTopologyParameters::non_null_ptr_type,
			boost::noncopyable>(
					"ResolveTopologyParameters",
					"Specify parameters used to resolve topologies.\n"
					"\n"
					"  .. versionadded:: 31\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::resolve_topology_parameters_create,
						bp::default_call_policies()),
			// Specific overload signature...
			"__init__()\n"
			"  Create the parameters used to resolve topologies.\n")
	;

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesApi::ResolveTopologyParameters>();
}
