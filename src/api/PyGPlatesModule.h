/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#ifndef GPLATES_API_PYGPLATESMODULE_H
#define GPLATES_API_PYGPLATESMODULE_H

#include "global/python.h"


#if !defined(GPLATES_NO_PYTHON)

namespace GPlatesApi
{
	/**
	 * Returns the 'pygplates' module (or Py_None if the 'pygplates' module has not been initialised).
	 *
	 * This function is useful for calling the pygplates python API from C++ code.
	 * For example - to construct a temporary 'pygplates.FeatureCollectionFileFormatRegistry' and
	 * use it to read a feature collection from a file:
	 *
	 *    GPlatesModel::FeatureCollectionHandle::non_null_ptr_type feature_collection =
	 *        bp::extract<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type>(
	 *            get_pygplates_module().attr("FeatureCollectionFileFormatRegistry")()
	 *                .attr("read")(filename));
	 *
	 * ...although usually it's better (and in most cases easier) just to call C++ code from C++ code.
	 */
	boost::python::object
	get_pygplates_module();
}

#endif   // GPLATES_NO_PYTHON

#endif // GPLATES_API_PYGPLATESMODULE_H
