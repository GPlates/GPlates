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

#include "global/python.h"

#include "model/FeatureVisitor.h"

#include "property-values/GmlLineString.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlOrientableCurve.h"
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
	 * Property value visitor base class to be inherited by a python class.
	 *
	 * This class only visits a property value.
	 * This is to make it easy to understand for python users.
	 *
	 * This is similar to the feature visitor except it does not iterate over feature properties
	 * or over property value(s) within a top-level feature property.
	 *
	 * Python users can still simulate a feature visitor by iterating of the properties of a feature.
	 *
	 * So we wrap 'GPlatesModel::FeatureVisitor', instead of defining our own visitor class,
	 * even though we're only using it to visit property values. All the feature property iteration
	 * function will simply not get exposed (or used) by our python wrapping.
	 */
	struct FeatureVisitorWrap :
			public GPlatesModel::FeatureVisitor,
			public bp::wrapper<GPlatesModel::FeatureVisitor>
	{

		virtual
		void
		visit_gml_line_string(
				gml_line_string_type &gml_line_string)
		{
			if (bp::override visit = this->get_override("visit_gml_line_string"))
			{
				// Pass 'non_null_ptr_type' to python since that's the boost python held type of
				// property values and also we want the python object to have an 'owning' reference.
				visit(gml_line_string_type::non_null_ptr_type(&gml_line_string));
				return;
			}
			GPlatesModel::FeatureVisitor::visit_gml_line_string(gml_line_string);
		}

		void
		default_visit_gml_line_string(
				gml_line_string_type &gml_line_string)
		{
			this->GPlatesModel::FeatureVisitor::visit_gml_line_string(gml_line_string);
		}


		virtual
		void
		visit_gml_multi_point(
				gml_multi_point_type &gml_multi_point)
		{
			if (bp::override visit = this->get_override("visit_gml_multi_point"))
			{
				// Pass 'non_null_ptr_type' to python since that's the boost python held type of
				// property values and also we want the python object to have an 'owning' reference.
				visit(gml_multi_point_type::non_null_ptr_type(&gml_multi_point));
				return;
			}
			GPlatesModel::FeatureVisitor::visit_gml_multi_point(gml_multi_point);
		}

		void
		default_visit_gml_multi_point(
				gml_multi_point_type &gml_multi_point)
		{
			this->GPlatesModel::FeatureVisitor::visit_gml_multi_point(gml_multi_point);
		}


		virtual
		void
		visit_gml_orientable_curve(
				gml_orientable_curve_type &gml_orientable_curve)
		{
			if (bp::override visit = this->get_override("visit_gml_orientable_curve"))
			{
				// Pass 'non_null_ptr_type' to python since that's the boost python held type of
				// property values and also we want the python object to have an 'owning' reference.
				visit(gml_orientable_curve_type::non_null_ptr_type(&gml_orientable_curve));
				return;
			}
			GPlatesModel::FeatureVisitor::visit_gml_orientable_curve(gml_orientable_curve);
		}

		void
		default_visit_gml_orientable_curve(
				gml_orientable_curve_type &gml_orientable_curve)
		{
			this->GPlatesModel::FeatureVisitor::visit_gml_orientable_curve(gml_orientable_curve);
		}


		virtual
		void
		visit_gml_point(
				gml_point_type &gml_point)
		{
			if (bp::override visit = this->get_override("visit_gml_point"))
			{
				// Pass 'non_null_ptr_type' to python since that's the boost python held type of
				// property values and also we want the python object to have an 'owning' reference.
				visit(gml_point_type::non_null_ptr_type(&gml_point));
				return;
			}
			GPlatesModel::FeatureVisitor::visit_gml_point(gml_point);
		}

		void
		default_visit_gml_point(
				gml_point_type &gml_point)
		{
			this->GPlatesModel::FeatureVisitor::visit_gml_point(gml_point);
		}


		virtual
		void
		visit_gml_polygon(
				gml_polygon_type &gml_polygon)
		{
			if (bp::override visit = this->get_override("visit_gml_polygon"))
			{
				// Pass 'non_null_ptr_type' to python since that's the boost python held type of
				// property values and also we want the python object to have an 'owning' reference.
				visit(gml_polygon_type::non_null_ptr_type(&gml_polygon));
				return;
			}
			GPlatesModel::FeatureVisitor::visit_gml_polygon(gml_polygon);
		}

		void
		default_visit_gml_polygon(
				gml_polygon_type &gml_polygon)
		{
			this->GPlatesModel::FeatureVisitor::visit_gml_polygon(gml_polygon);
		}


		virtual
		void
		visit_gml_time_instant(
				gml_time_instant_type &gml_time_instant)
		{
			if (bp::override visit = this->get_override("visit_gml_time_instant"))
			{
				// Pass 'non_null_ptr_type' to python since that's the boost python held type of
				// property values and also we want the python object to have an 'owning' reference.
				visit(gml_time_instant_type::non_null_ptr_type(&gml_time_instant));
				return;
			}
			GPlatesModel::FeatureVisitor::visit_gml_time_instant(gml_time_instant);
		}

		void
		default_visit_gml_time_instant(
				gml_time_instant_type &gml_time_instant)
		{
			this->GPlatesModel::FeatureVisitor::visit_gml_time_instant(gml_time_instant);
		}


		virtual
		void
		visit_gml_time_period(
				gml_time_period_type &gml_time_period)
		{
			if (bp::override visit = this->get_override("visit_gml_time_period"))
			{
				// Pass 'non_null_ptr_type' to python since that's the boost python held type of
				// property values and also we want the python object to have an 'owning' reference.
				visit(gml_time_period_type::non_null_ptr_type(&gml_time_period));
				return;
			}
			GPlatesModel::FeatureVisitor::visit_gml_time_period(gml_time_period);
		}

		void
		default_visit_gml_time_period(
				gml_time_period_type &gml_time_period)
		{
			this->GPlatesModel::FeatureVisitor::visit_gml_time_period(gml_time_period);
		}


		virtual
		void
		visit_gpml_constant_value(
				gpml_constant_value_type &gpml_constant_value)
		{
			if (bp::override visit = this->get_override("visit_gpml_constant_value"))
			{
				// Pass 'non_null_ptr_type' to python since that's the boost python held type of
				// property values and also we want the python object to have an 'owning' reference.
				visit(gpml_constant_value_type::non_null_ptr_type(&gpml_constant_value));
				return;
			}
			GPlatesModel::FeatureVisitor::visit_gpml_constant_value(gpml_constant_value);
		}

		void
		default_visit_gpml_constant_value(
				gpml_constant_value_type &gpml_constant_value)
		{
			this->GPlatesModel::FeatureVisitor::visit_gpml_constant_value(gpml_constant_value);
		}


		virtual
		void
		visit_gpml_finite_rotation_slerp(
				gpml_finite_rotation_slerp_type &gpml_finite_rotation_slerp)
		{
			if (bp::override visit = this->get_override("visit_gpml_finite_rotation_slerp"))
			{
				// Pass 'non_null_ptr_type' to python since that's the boost python held type of
				// property values and also we want the python object to have an 'owning' reference.
				visit(gpml_finite_rotation_slerp_type::non_null_ptr_type(&gpml_finite_rotation_slerp));
				return;
			}
			GPlatesModel::FeatureVisitor::visit_gpml_finite_rotation_slerp(gpml_finite_rotation_slerp);
		}

		void
		default_visit_gpml_finite_rotation_slerp(
				gpml_finite_rotation_slerp_type &gpml_finite_rotation_slerp)
		{
			this->GPlatesModel::FeatureVisitor::visit_gpml_finite_rotation_slerp(gpml_finite_rotation_slerp);
		}


		virtual
		void
		visit_gpml_irregular_sampling(
				gpml_irregular_sampling_type &gpml_irregular_sampling)
		{
			if (bp::override visit = this->get_override("visit_gpml_irregular_sampling"))
			{
				// Pass 'non_null_ptr_type' to python since that's the boost python held type of
				// property values and also we want the python object to have an 'owning' reference.
				visit(gpml_irregular_sampling_type::non_null_ptr_type(&gpml_irregular_sampling));
				return;
			}
			GPlatesModel::FeatureVisitor::visit_gpml_irregular_sampling(gpml_irregular_sampling);
		}

		void
		default_visit_gpml_irregular_sampling(
				gpml_irregular_sampling_type &gpml_irregular_sampling)
		{
			this->GPlatesModel::FeatureVisitor::visit_gpml_irregular_sampling(gpml_irregular_sampling);
		}


		virtual
		void
		visit_gpml_piecewise_aggregation(
				gpml_piecewise_aggregation_type &gpml_piecewise_aggregation)
		{
			if (bp::override visit = this->get_override("visit_gpml_piecewise_aggregation"))
			{
				// Pass 'non_null_ptr_type' to python since that's the boost python held type of
				// property values and also we want the python object to have an 'owning' reference.
				visit(gpml_piecewise_aggregation_type::non_null_ptr_type(&gpml_piecewise_aggregation));
				return;
			}
			GPlatesModel::FeatureVisitor::visit_gpml_piecewise_aggregation(gpml_piecewise_aggregation);
		}

		void
		default_visit_gpml_piecewise_aggregation(
				gpml_piecewise_aggregation_type &gpml_piecewise_aggregation)
		{
			this->GPlatesModel::FeatureVisitor::visit_gpml_piecewise_aggregation(gpml_piecewise_aggregation);
		}


		virtual
		void
		visit_gpml_plate_id(
				gpml_plate_id_type &gpml_plate_id)
		{
			if (bp::override visit = this->get_override("visit_gpml_plate_id"))
			{
				// Pass 'non_null_ptr_type' to python since that's the boost python held type of
				// property values and also we want the python object to have an 'owning' reference.
				visit(gpml_plate_id_type::non_null_ptr_type(&gpml_plate_id));
				return;
			}
			GPlatesModel::FeatureVisitor::visit_gpml_plate_id(gpml_plate_id);
		}

		void
		default_visit_gpml_plate_id(
				gpml_plate_id_type &gpml_plate_id)
		{
			this->GPlatesModel::FeatureVisitor::visit_gpml_plate_id(gpml_plate_id);
		}


		virtual
		void
		visit_xs_boolean(
				xs_boolean_type &xs_boolean)
		{
			if (bp::override visit = this->get_override("visit_xs_boolean"))
			{
				// Pass 'non_null_ptr_type' to python since that's the boost python held type of
				// property values and also we want the python object to have an 'owning' reference.
				visit(xs_boolean_type::non_null_ptr_type(&xs_boolean));
				return;
			}
			GPlatesModel::FeatureVisitor::visit_xs_boolean(xs_boolean);
		}

		void
		default_visit_xs_boolean(
				xs_boolean_type &xs_boolean)
		{
			this->GPlatesModel::FeatureVisitor::visit_xs_boolean(xs_boolean);
		}


		virtual
		void
		visit_xs_double(
				xs_double_type &xs_double)
		{
			if (bp::override visit = this->get_override("visit_xs_double"))
			{
				// Pass 'non_null_ptr_type' to python since that's the boost python held type of
				// property values and also we want the python object to have an 'owning' reference.
				visit(xs_double_type::non_null_ptr_type(&xs_double));
				return;
			}
			GPlatesModel::FeatureVisitor::visit_xs_double(xs_double);
		}

		void
		default_visit_xs_double(
				xs_double_type &xs_double)
		{
			this->GPlatesModel::FeatureVisitor::visit_xs_double(xs_double);
		}


		virtual
		void
		visit_xs_integer(
				xs_integer_type &xs_integer)
		{
			if (bp::override visit = this->get_override("visit_xs_integer"))
			{
				// Pass 'non_null_ptr_type' to python since that's the boost python held type of
				// property values and also we want the python object to have an 'owning' reference.
				visit(xs_integer_type::non_null_ptr_type(&xs_integer));
				return;
			}
			GPlatesModel::FeatureVisitor::visit_xs_integer(xs_integer);
		}

		void
		default_visit_xs_integer(
				xs_integer_type &xs_integer)
		{
			this->GPlatesModel::FeatureVisitor::visit_xs_integer(xs_integer);
		}


		virtual
		void
		visit_xs_string(
				xs_string_type &xs_string)
		{
			if (bp::override visit = this->get_override("visit_xs_string"))
			{
				// Pass 'non_null_ptr_type' to python since that's the boost python held type of
				// property values and also we want the python object to have an 'owning' reference.
				visit(xs_string_type::non_null_ptr_type(&xs_string));
				return;
			}
			GPlatesModel::FeatureVisitor::visit_xs_string(xs_string);
		}

		void
		default_visit_xs_string(
				xs_string_type &xs_string)
		{
			this->GPlatesModel::FeatureVisitor::visit_xs_string(xs_string);
		}
	};
}


void
export_property_value_visitor()
{
	//
	// PropertyValueVisitor - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<GPlatesApi::FeatureVisitorWrap, boost::noncopyable>(
			"PropertyValueVisitor",
			"The base class inherited by all derived property value *visitor* classes. "
			"A property value visitor is used to visit a property value and discover "
			"its derived property value class type. Note that there is no common interface "
			"shared by all property value types, hence the visitor pattern provides one way "
			"to find out which type of property value is being visited. A property value instance "
			"is visited by passing an instance of a visitor to the property value's "
			":meth:`PropertyValue.accept_visitor` method. Typically you create your own class that "
			"inherits from :class:`PropertyValueVisitor` and implement some of the *visit* methods. "
			"All *visit* methods default to doing nothing.\n"
			"\n"
			"The following example demonstrates how to implement a simple visitor class by inheriting "
			":class:`PropertyValueVisitor` and implementing only those *visit* methods related to the "
			":class:`GpmlPlateId` property value type (note that plate ids can be time-dependent in "
			"some contexts so we also use :class:`GpmlConstantValue` in case the plate id property value "
			"is wrapped in a constant value wrapper). This example retrieves the integer plate id "
			"from a (possibly nested) :class:`GpmlPlateId` property value, or ``None`` if the visited "
			"property value is of a different *type*.\n"
			"  ::\n"
			"\n"
			"    class GetPlateIdVisitor(pygplates.PropertyValueVisitor):\n"
			"        def __init__(self):\n"
			"            # NOTE: You must call base class '__init__' otherwise you will\n"
			"            # get a 'Boost.Python.ArgumentError' exception.\n"
			"            super(GetPlateIdVisitor, self).__init__()\n"
			"            self.plate_id = None\n"
			"\n"
			"        # Returns the plate id from the visited GpmlPlateId property value,\n"
			"        # or None if property value was a different type.\n"
			"        def get_plate_id(self):\n"
			"            return self.plate_id\n"
			"\n"
			"        def visit_gpml_constant_value(self, gpml_constant_value):\n"
			"            # Visit the GpmlConstantValue's nested property value.\n"
			"            property_value = gpml_constant_value.get_value().accept_visitor(self)\n"
			"\n"
			"        def visit_gpml_plate_id(self, gpml_plate_id):\n"
			"            self.plate_id = gpml_plate_id.get_plate_id()\n"
			"\n"
			"    # Visitor can extract plate id from this...\n"
			"    property_value1 = pygplates.GpmlPlateId(701)\n"
			"    # Visitor cannot extract plate id from this...\n"
			"    property_value2 = pygplates.XsInteger(701)\n"
			"\n"
			"    plate_id_visitor = GetPlateIdVisitor()\n"
			"    property_value1.accept_visitor(plate_id_visitor)\n"
			"    plate_id = plate_id_visitor.get_plate_id()\n"
			"\n"
			"    # If we found a 'GpmlPlateId' then print its plate id.\n"
			"    if plate_id:\n"
			"        print 'plate id: %d' % plate_id\n"
			"\n"
			"NOTE: You must call the base class *__init__* otherwise you will "
			"get a *Boost.Python.ArgumentError* exception.\n"
			// NOTE: Must not define 'bp::no_init' because this base class is meant to be inherited
			// by a python class (see http://www.boostpro.com/writing/bpl.html#inheritance).
			)
		.def("visit_gml_line_string",
				&GPlatesModel::FeatureVisitor::visit_gml_line_string,
				&GPlatesApi::FeatureVisitorWrap::default_visit_gml_line_string,
				"visit_gml_line_string(gml_line_string)\n"
				"  Visits a :class:`GmlLineString` property value.\n")
		.def("visit_gml_multi_point",
				&GPlatesModel::FeatureVisitor::visit_gml_multi_point,
				&GPlatesApi::FeatureVisitorWrap::default_visit_gml_multi_point,
				"visit_gml_multi_point(gml_multi_point)\n"
				"  Visits a :class:`GmlMultiPoint` property value.\n")
		.def("visit_gml_orientable_curve",
				&GPlatesModel::FeatureVisitor::visit_gml_orientable_curve,
				&GPlatesApi::FeatureVisitorWrap::default_visit_gml_orientable_curve,
				"visit_gml_orientable_curve(gml_orientable_curve)\n"
				"  Visits a :class:`GmlOrientableCurve` property value.\n")
		.def("visit_gml_point",
				&GPlatesModel::FeatureVisitor::visit_gml_point,
				&GPlatesApi::FeatureVisitorWrap::default_visit_gml_point,
				"visit_gml_point(gml_point)\n"
				"  Visits a :class:`GmlPoint` property value.\n")
		.def("visit_gml_polygon",
				&GPlatesModel::FeatureVisitor::visit_gml_polygon,
				&GPlatesApi::FeatureVisitorWrap::default_visit_gml_polygon,
				"visit_gml_polygon(gml_polygon)\n"
				"  Visits a :class:`GmlPolygon` property value.\n")
		.def("visit_gml_time_instant",
				&GPlatesModel::FeatureVisitor::visit_gml_time_instant,
				&GPlatesApi::FeatureVisitorWrap::default_visit_gml_time_instant,
				"visit_gml_time_instant(gml_time_instant)\n"
				"  Visits a :class:`GmlTimeInstant` property value.\n")
		.def("visit_gml_time_period",
				&GPlatesModel::FeatureVisitor::visit_gml_time_period,
				&GPlatesApi::FeatureVisitorWrap::default_visit_gml_time_period,
				"visit_gml_time_period(gml_time_period)\n"
				"  Visits a :class:`GmlTimePeriod` property value.\n")
		.def("visit_gpml_constant_value",
				&GPlatesModel::FeatureVisitor::visit_gpml_constant_value,
				&GPlatesApi::FeatureVisitorWrap::default_visit_gpml_constant_value,
				"visit_gpml_constant_value(gpml_constant_value)\n"
				"  Visits a :class:`GpmlConstantValue` property value.\n")
		.def("visit_gpml_finite_rotation_slerp",
				&GPlatesModel::FeatureVisitor::visit_gpml_finite_rotation_slerp,
				&GPlatesApi::FeatureVisitorWrap::default_visit_gpml_finite_rotation_slerp,
				"visit_gpml_finite_rotation_slerp(gpml_finite_rotation_slerp)\n"
				"  Visits a :class:`GpmlFiniteRotationSlerp` property value.\n")
		.def("visit_gpml_irregular_sampling",
				&GPlatesModel::FeatureVisitor::visit_gpml_irregular_sampling,
				&GPlatesApi::FeatureVisitorWrap::default_visit_gpml_irregular_sampling,
				"visit_gpml_irregular_sampling(gpml_irregular_sampling)\n"
				"  Visits a :class:`GpmlIrregularSampling` property value.\n")
		.def("visit_gpml_piecewise_aggregation",
				&GPlatesModel::FeatureVisitor::visit_gpml_piecewise_aggregation,
				&GPlatesApi::FeatureVisitorWrap::default_visit_gpml_piecewise_aggregation,
				"visit_gpml_piecewise_aggregation(gpml_piecewise_aggregation)\n"
				"  Visits a :class:`GpmlPiecewiseAggregation` property value.\n")
		.def("visit_gpml_plate_id",
				&GPlatesModel::FeatureVisitor::visit_gpml_plate_id,
				&GPlatesApi::FeatureVisitorWrap::default_visit_gpml_plate_id,
				"visit_gpml_plate_id(gpml_plate_id)\n"
				"  Visits a :class:`GpmlPlateId` property value.\n")
		.def("visit_xs_boolean",
				&GPlatesModel::FeatureVisitor::visit_xs_boolean,
				&GPlatesApi::FeatureVisitorWrap::default_visit_xs_boolean,
				"visit_xs_boolean(xs_boolean)\n"
				"  Visits a :class:`XsBoolean` property value.\n")
		.def("visit_xs_double",
				&GPlatesModel::FeatureVisitor::visit_xs_double,
				&GPlatesApi::FeatureVisitorWrap::default_visit_xs_double,
				"visit_xs_double(xs_double)\n"
				"  Visits a :class:`XsDouble` property value.\n")
		.def("visit_xs_integer",
				&GPlatesModel::FeatureVisitor::visit_xs_integer,
				&GPlatesApi::FeatureVisitorWrap::default_visit_xs_integer,
				"visit_xs_integer(xs_integer)\n"
				"  Visits a :class:`XsInteger` property value.\n")
		.def("visit_xs_string",
				&GPlatesModel::FeatureVisitor::visit_xs_string,
				&GPlatesApi::FeatureVisitorWrap::default_visit_xs_string,
				"visit_xs_string(xs_string)\n"
				"  Visits a :class:`XsString` property value.\n")
	;
}

#endif // GPLATES_NO_PYTHON
