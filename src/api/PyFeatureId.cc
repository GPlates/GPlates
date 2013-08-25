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

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include "PythonConverterUtils.h"

#include "global/python.h"

#include "model/FeatureId.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


void
export_feature_id()
{
	//
	// FeatureId
	//
	// NOTE: Later we might wrap 'FeatureId::find_back_ref_targets()' to allow user to find feature
	// with the feature id (but for now it's probably not a good idea to expose this).
	//
	bp::class_<GPlatesModel::FeatureId>("FeatureId")
		.def("get",
				&GPlatesModel::FeatureId::get,
				bp::return_value_policy<bp::copy_const_reference>())
		.def(bp::self == bp::self)
		.def(bp::self != bp::self)
		// For '__str__' return the string form...
		.def("__str__",
				&GPlatesModel::FeatureId::get,
				bp::return_value_policy<bp::copy_const_reference>())
	;

	// Enable boost::optional<FeatureId> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::python_optional<GPlatesModel::FeatureId>();
}

#endif // GPLATES_NO_PYTHON
