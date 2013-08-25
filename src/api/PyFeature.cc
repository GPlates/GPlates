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

#include "model/FeatureHandle.h"
#include "model/TopLevelProperty.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;

#define TEST_FEATURE_CREATE_MODIFY

namespace GPlatesApi
{
#ifdef TEST_FEATURE_CREATE_MODIFY
	const GPlatesModel::FeatureHandle::non_null_ptr_type
	feature_handle_create(
			const GPlatesModel::FeatureType &feature_type)
	{
		return GPlatesModel::FeatureHandle::create(feature_type);
	}

	void
	feature_handle_add(
			GPlatesModel::FeatureHandle::non_null_ptr_type feature_handle,
			GPlatesModel::TopLevelProperty::non_null_ptr_type property)
	{
		// Ignore the returned iterator.
		feature_handle->add(property);
	}
#endif // TEST_FEATURE_CREATE_MODIFY
}


void
export_feature()
{
	//
	// Feature
	//
	bp::class_<
			GPlatesModel::FeatureHandle,
			GPlatesModel::FeatureHandle::non_null_ptr_type,
			boost::noncopyable>(
					"Feature", bp::no_init)
#ifdef TEST_FEATURE_CREATE_MODIFY
		.def("create", &GPlatesApi::feature_handle_create)
 		.staticmethod("create")
		.def("add", &GPlatesApi::feature_handle_add)
#endif
		.def("get_feature_type",
				&GPlatesModel::FeatureHandle::feature_type,
				bp::return_value_policy<bp::copy_const_reference>())
		.def("__iter__", bp::iterator<GPlatesModel::FeatureHandle>())
	;

	// Enable boost::optional<FeatureHandle::non_null_ptr_type> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::python_optional<GPlatesModel::FeatureHandle::non_null_ptr_type>();

	// Registers 'non-const' to 'const' conversions.
	boost::python::implicitly_convertible<
			GPlatesModel::FeatureHandle::non_null_ptr_type,
			GPlatesModel::FeatureHandle::non_null_ptr_to_const_type>();
	boost::python::implicitly_convertible<
			boost::optional<GPlatesModel::FeatureHandle::non_null_ptr_type>,
			boost::optional<GPlatesModel::FeatureHandle::non_null_ptr_to_const_type> >();
}

#endif // GPLATES_NO_PYTHON
