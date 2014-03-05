/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2014 The University of Sydney, Australia
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

#include <boost/optional.hpp>

#include "PythonConverterUtils.h"

#include "app-logic/ReconstructedFeatureGeometry.h"

#include "global/python.h"

#include "model/FeatureHandle.h"
#include "model/TopLevelProperty.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


namespace GPlatesApi
{
	// We return an optional feature in case the feature reference is invalid (this results
	// in python getting a Py_None). The reference should be valid though so we don't document
	// that Py_None can be returned to the caller.
	boost::optional<GPlatesModel::FeatureHandle::non_null_ptr_type>
	reconstructed_feature_geometry_get_feature(
			const GPlatesAppLogic::ReconstructedFeatureGeometry &reconstructed_feature_geometry)
	{
		GPlatesModel::FeatureHandle::weak_ref feature_ref = reconstructed_feature_geometry.get_feature_ref();
		if (!feature_ref.is_valid())
		{
			return boost::none;
		}

		const GPlatesModel::FeatureHandle::non_null_ptr_type feature(feature_ref.handle_ptr());

		return feature;
	}


	// We return an optional property in case the property iterator is invalid (this results
	// in python getting a Py_None). The iterator should be valid though so we don't document
	// that Py_None can be returned to the caller.
	boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_type>
	reconstructed_feature_geometry_get_property(
			const GPlatesAppLogic::ReconstructedFeatureGeometry &reconstructed_feature_geometry)
	{
		GPlatesModel::FeatureHandle::iterator property_iter = reconstructed_feature_geometry.property();
		if (!property_iter.is_still_valid())
		{
			return boost::none;
		}

		const GPlatesModel::TopLevelProperty::non_null_ptr_type property = *property_iter;

		return property;
	}
}


void
export_reconstructed_feature_geometry()
{
	//
	// ReconstructedFeatureGeometry - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesAppLogic::ReconstructedFeatureGeometry,
			GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type,
			boost::noncopyable>(
					"ReconstructedFeatureGeometry",
					"The geometry of a feature reconstructed to a geological time.\n"
					"\n"
					"Note that a single feature can have multiple geometry properties, and hence multiple "
					"reconstructed feature geometries, associated with it. "
					"Therefore each :class:`ReconstructedFeatureGeometry` references a different property of "
					"the feature via :meth:`get_property`.\n",
					// Don't allow creation from python side...
					// (Also there is no publicly-accessible default constructor).
					bp::no_init)
		.def("get_feature",
				&GPlatesApi::reconstructed_feature_geometry_get_feature,
				"get_feature() -> Feature\n"
				"  Returns the feature associated with this :class:`ReconstructedFeatureGeometry`.\n"
				"\n"
				"  :rtype: :class:`Feature`\n"
				"\n"
				"  Note that multiple :class:`reconstructed feature geometries<ReconstructedFeatureGeometry>` "
				"can be associated with the same feature if that feature has multiple geometry properties.\n")
		.def("get_property",
				&GPlatesApi::reconstructed_feature_geometry_get_property,
				"get_property() -> Property\n"
				"  Returns the feature property containing the present day (unreconstructed) geometry "
				"associated with this :class:`ReconstructedFeatureGeometry`.\n"
				"\n"
				"  :rtype: :class:`Property`\n"
				"\n"
				"  See :func:`get_geometry_from_property_value` for extracting the present day "
				"geometry from the returned property.\n")
		.def("get_reconstructed_geometry",
				&GPlatesAppLogic::ReconstructedFeatureGeometry::reconstructed_geometry,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_reconstructed_geometry() -> GeometryOnSphere\n"
				"  Returns the reconstructed geometry.\n"
				"\n"
				"  :rtype: :class:`GeometryOnSphere`\n")
	;

	// Enable boost::optional<ReconstructedFeatureGeometry::non_null_ptr_type> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<
			GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type>();

	// Registers 'non-const' to 'const' conversions.
	boost::python::implicitly_convertible<
			GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type,
			GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_to_const_type>();
	boost::python::implicitly_convertible<
			boost::optional<GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type>,
			boost::optional<GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_to_const_type> >();
}


void
export_reconstruction_geometries()
{
	export_reconstructed_feature_geometry();
}

#endif // GPLATES_NO_PYTHON
