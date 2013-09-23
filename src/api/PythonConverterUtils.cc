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

#include "PythonConverterUtils.h"

#include "global/python.h"

#include "maths/ConstGeometryOnSphereVisitor.h"

#include "model/FeatureVisitor.h"

#include "property-values/GmlLineString.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlPolygon.h"
#include "property-values/GmlTimeInstant.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlFiniteRotationSlerp.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlPiecewiseAggregation.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/XsBoolean.h"
#include "property-values/XsDouble.h"
#include "property-values/XsInteger.h"
#include "property-values/XsString.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


namespace GPlatesApi
{
	/**
	 * Visits a property value and converts from its derived type to a python object.
	 */
	class GetPropertyValueAsDerivedTypeVisitor : 
			public GPlatesModel::FeatureVisitor
	{
	public:

		/**
		 * The derived property value retrieved after visiting a property value.
		 */
		bp::object
		get_property_value_as_derived_type()
		{
			return d_property_value;
		}

		virtual
		void 
		visit_gml_line_string(
				gml_line_string_type &gml_line_string)
		{
			// Use to-python converter registered for derived property value's 'non_null_ptr_type'.
			d_property_value = bp::object(gml_line_string_type::non_null_ptr_type(&gml_line_string));
		}

		virtual
		void 
		visit_gml_multi_point(
				gml_multi_point_type &gml_multi_point)
		{
			// Use to-python converter registered for derived property value's 'non_null_ptr_type'.
			d_property_value = bp::object(gml_multi_point_type::non_null_ptr_type(&gml_multi_point));
		}

		virtual
		void 
		visit_gml_point(
				gml_point_type &gml_point)
		{
			// Use to-python converter registered for derived property value's 'non_null_ptr_type'.
			d_property_value = bp::object(gml_point_type::non_null_ptr_type(&gml_point));
		}

		virtual
		void 
		visit_gml_polygon(
				gml_polygon_type &gml_polygon)
		{
			// Use to-python converter registered for derived property value's 'non_null_ptr_type'.
			d_property_value = bp::object(gml_polygon_type::non_null_ptr_type(&gml_polygon));
		}

		virtual
		void 
		visit_gml_time_instant(
				gml_time_instant_type &gml_time_instant)
		{
			// Use to-python converter registered for derived property value's 'non_null_ptr_type'.
			d_property_value = bp::object(gml_time_instant_type::non_null_ptr_type(&gml_time_instant));
		}

		virtual
		void
		visit_gml_time_period(
				gml_time_period_type &gml_time_period)
		{
			// Use to-python converter registered for derived property value's 'non_null_ptr_type'.
			d_property_value = bp::object(gml_time_period_type::non_null_ptr_type(&gml_time_period));
		}

		virtual
		void
		visit_gpml_constant_value(
				gpml_constant_value_type &gpml_constant_value)
		{
			// Use to-python converter registered for derived property value's 'non_null_ptr_type'.
			d_property_value = bp::object(gpml_constant_value_type::non_null_ptr_type(&gpml_constant_value));
		}

		virtual
		void
		visit_gpml_finite_rotation_slerp(
				gpml_finite_rotation_slerp_type &gpml_finite_rotation_slerp)
		{
			// Use to-python converter registered for derived property value's 'non_null_ptr_type'.
			d_property_value = bp::object(gpml_finite_rotation_slerp_type::non_null_ptr_type(&gpml_finite_rotation_slerp));
		}

		virtual
		void
		visit_gpml_irregular_sampling(
				gpml_irregular_sampling_type &gpml_irregular_sampling)
		{
			// Use to-python converter registered for derived property value's 'non_null_ptr_type'.
			d_property_value = bp::object(gpml_irregular_sampling_type::non_null_ptr_type(&gpml_irregular_sampling));
		}

		virtual
		void
		visit_gpml_piecewise_aggregation(
				gpml_piecewise_aggregation_type &gpml_piecewise_aggregation)
		{
			// Use to-python converter registered for derived property value's 'non_null_ptr_type'.
			d_property_value = bp::object(gpml_piecewise_aggregation_type::non_null_ptr_type(&gpml_piecewise_aggregation));
		}

		virtual
		void
		visit_gpml_plate_id(
				gpml_plate_id_type &gpml_plate_id)
		{
			// Use to-python converter registered for derived property value's 'non_null_ptr_type'.
			d_property_value = bp::object(gpml_plate_id_type::non_null_ptr_type(&gpml_plate_id));
		}

		virtual
		void
		visit_xs_boolean(
				xs_boolean_type &xs_boolean)
		{
			// Use to-python converter registered for derived property value's 'non_null_ptr_type'.
			d_property_value = bp::object(xs_boolean_type::non_null_ptr_type(&xs_boolean));
		}

		virtual
		void
		visit_xs_double(
				xs_double_type &xs_double)
		{
			// Use to-python converter registered for derived property value's 'non_null_ptr_type'.
			d_property_value = bp::object(xs_double_type::non_null_ptr_type(&xs_double));
		}

		virtual
		void
		visit_xs_integer(
				xs_integer_type &xs_integer)
		{
			// Use to-python converter registered for derived property value's 'non_null_ptr_type'.
			d_property_value = bp::object(xs_integer_type::non_null_ptr_type(&xs_integer));
		}

		virtual
		void
		visit_xs_string(
				xs_string_type &xs_string)
		{
			// Use to-python converter registered for derived property value's 'non_null_ptr_type'.
			d_property_value = bp::object(xs_string_type::non_null_ptr_type(&xs_string));
		}

	private:
		bp::object d_property_value;
	};


	/**
	 * Visits a @a GeometryOnSphere and converts from its derived type to a python object.
	 */
	class GetGeometryOnSphereAsDerivedTypeVisitor:
			public GPlatesMaths::ConstGeometryOnSphereVisitor
	{
	public:

		/**
		 * The derived geometry-on-sphere retrieved after visiting a @a GeometryOnSphere.
		 */
		bp::object
		get_geometry_on_sphere_as_derived_type()
		{
			return d_geometry_on_sphere;
		}

		virtual
		void
		visit_multi_point_on_sphere(
				GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere)
		{
			// Use to-python converter registered for derived geometry-on-sphere's 'non_null_ptr_to_const_type'.
			d_geometry_on_sphere = bp::object(multi_point_on_sphere);
		}

		virtual
		void
		visit_point_on_sphere(
				GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere)
		{
			// Use to-python converter registered for derived geometry-on-sphere's 'non_null_ptr_to_const_type'.
			d_geometry_on_sphere = bp::object(point_on_sphere);
		}

		virtual
		void
		visit_polygon_on_sphere(
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
		{
			// Use to-python converter registered for derived geometry-on-sphere's 'non_null_ptr_to_const_type'.
			d_geometry_on_sphere = bp::object(polygon_on_sphere);
		}

		virtual
		void
		visit_polyline_on_sphere(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
		{
			// Use to-python converter registered for derived geometry-on-sphere's 'non_null_ptr_to_const_type'.
			d_geometry_on_sphere = bp::object(polyline_on_sphere);
		}

	private:
		bp::object d_geometry_on_sphere;
	};
}


bp::object/*derived property value*/
GPlatesApi::PythonConverterUtils::get_property_value_as_derived_type(
		GPlatesModel::PropertyValue::non_null_ptr_type property_value)
{
	GetPropertyValueAsDerivedTypeVisitor visitor;
	property_value->accept_visitor(visitor);

	bp::object derived_property_value = visitor.get_property_value_as_derived_type();

	//////////////////////////////////////////////////////////////////////////
	// TEMPORARY
	//
	// The following handles derived property value types that have not yet been bound to python.
	// They will only have access to the base class PropertyValue functionality.
	//
	// TODO: Remove when all derived PropertyValue types have been bound to python. Some derived
	// PropertyValue types will need to have a better, more solid interface before this can happen.
	//
	if (derived_property_value == bp::object())
	{
		// If we didn't visit a derived property value then just return the base PropertyValue to python.
		//
		// Only the exposed methods in the base PropertyValue class will be available to the python user.
		return bp::object(property_value);
	}
	//
	//////////////////////////////////////////////////////////////////////////

	return derived_property_value;
}


boost::python::object/*derived geometry-on-sphere non_null_ptr_to_const_type*/
GPlatesApi::PythonConverterUtils::get_geometry_on_sphere_as_derived_type(
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_on_sphere)
{
	GetGeometryOnSphereAsDerivedTypeVisitor visitor;
	geometry_on_sphere->accept_visitor(visitor);

	return visitor.get_geometry_on_sphere_as_derived_type();
}


#endif // GPLATES_NO_PYTHON
