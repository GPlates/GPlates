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

#include "property-values/GmlPoint.h"
#include "property-values/GmlTimeInstant.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlPlateId.h"



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
	};
}


void
export_property_value_visitor()
{
	//
	// PropertyValueVisitor
	//
	bp::class_<GPlatesApi::FeatureVisitorWrap, boost::noncopyable>("PropertyValueVisitor")
		.def("visit_gml_point",
				&GPlatesModel::FeatureVisitor::visit_gml_point,
				&GPlatesApi::FeatureVisitorWrap::default_visit_gml_point)
		.def("visit_gml_time_instant",
				&GPlatesModel::FeatureVisitor::visit_gml_time_instant,
				&GPlatesApi::FeatureVisitorWrap::default_visit_gml_time_instant)
		.def("visit_gml_time_period",
				&GPlatesModel::FeatureVisitor::visit_gml_time_period,
				&GPlatesApi::FeatureVisitorWrap::default_visit_gml_time_period)
		.def("visit_gpml_constant_value",
				&GPlatesModel::FeatureVisitor::visit_gpml_constant_value,
				&GPlatesApi::FeatureVisitorWrap::default_visit_gpml_constant_value)
		.def("visit_gpml_plate_id",
				&GPlatesModel::FeatureVisitor::visit_gpml_plate_id,
				&GPlatesApi::FeatureVisitorWrap::default_visit_gpml_plate_id)
	;
}

#endif // GPLATES_NO_PYTHON
