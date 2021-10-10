/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007, 2008 The University of Sydney, Australia
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

#ifndef GPLATES_FEATUREVISITORS_QUERYFEATUREPROPERTIESWIDGETPOPULATOR_H
#define GPLATES_FEATUREVISITORS_QUERYFEATUREPROPERTIESWIDGETPOPULATOR_H

#include <vector>
#include <QTreeWidget>

#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "model/ConstFeatureVisitor.h"
#include "model/PropertyValue.h"

namespace GPlatesPropertyValues
{
	class GpmlKeyValueDictionaryElement;
}


namespace GPlatesFeatureVisitors
{

	class QueryFeaturePropertiesWidgetPopulator:
			public GPlatesModel::ConstFeatureVisitor
	{
	public:

		explicit
		QueryFeaturePropertiesWidgetPopulator(
				QTreeWidget &tree_widget):
			d_tree_widget_ptr(&tree_widget) {  }

		virtual
		~QueryFeaturePropertiesWidgetPopulator() {  }

		virtual
		void
		visit_feature_handle(
				const GPlatesModel::FeatureHandle &feature_handle);

		virtual
		void
		visit_inline_property_container(
				const GPlatesModel::InlinePropertyContainer &inline_property_container);

		virtual
		void
		visit_enumeration(
				const GPlatesPropertyValues::Enumeration &enumeration);

		virtual
		void
		visit_gml_line_string(
				const GPlatesPropertyValues::GmlLineString &gml_line_string);

		virtual
		void
		visit_gml_multi_point(
				const GPlatesPropertyValues::GmlMultiPoint &gml_multi_point);

		virtual
		void
		visit_gml_orientable_curve(
				const GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve);

		virtual
		void
		visit_gml_point(
				const GPlatesPropertyValues::GmlPoint &gml_point);

		virtual
		void
		visit_gml_polygon(
				const GPlatesPropertyValues::GmlPolygon	&gml_polygon);

		virtual
		void
		visit_gml_time_instant(
				const GPlatesPropertyValues::GmlTimeInstant &gml_time_instant);

		virtual
		void
		visit_gml_time_period(
				const GPlatesPropertyValues::GmlTimePeriod &gml_time_period);

		virtual
		void
		visit_gpml_constant_value(
				const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value);

		virtual
		void
		visit_gpml_finite_rotation(
				const GPlatesPropertyValues::GpmlFiniteRotation &gpml_finite_rotation);

		virtual
		void
		visit_gpml_finite_rotation_slerp(
				const GPlatesPropertyValues::GpmlFiniteRotationSlerp &gpml_finite_rotation_slerp);

		virtual
		void
		visit_gpml_irregular_sampling(
				const GPlatesPropertyValues::GpmlIrregularSampling &gpml_irregular_sampling);

		virtual
		void
		visit_gpml_key_value_dictionary(
				const GPlatesPropertyValues::GpmlKeyValueDictionary &gpml_key_value_dictionary);

		virtual
		void
		visit_gpml_plate_id(
				const GPlatesPropertyValues::GpmlPlateId &gpml_plate_id);



		virtual
		void
		visit_gpml_old_plates_header(
				const GPlatesPropertyValues::GpmlOldPlatesHeader &gpml_old_plates_header);

		virtual
		void
		visit_xs_boolean(
				const GPlatesPropertyValues::XsBoolean &xs_boolean);

		virtual
		void
		visit_xs_double(
				const GPlatesPropertyValues::XsDouble &xs_double);

		virtual
		void
		visit_xs_integer(
				const GPlatesPropertyValues::XsInteger& xs_integer);

		virtual
		void
		visit_xs_string(
				const GPlatesPropertyValues::XsString &xs_string);



	private:

		QTreeWidget *d_tree_widget_ptr;
		std::vector<QTreeWidgetItem *> d_tree_widget_item_stack;


		QTreeWidgetItem *
		add_child(
				const QString &name,
				const QString &value);


		QTreeWidgetItem *
		add_child_then_visit_value(
				const QString &name,
				const QString &value,
				const GPlatesModel::PropertyValue &property_value_to_visit);

		void
		add_gpml_key_value_dictionary_element(
				const GPlatesPropertyValues::GpmlKeyValueDictionaryElement &element);


		void
		write_polygon_ring(
			GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_ptr);

		void
		write_multipoint_member(
			const GPlatesMaths::PointOnSphere &point);
	};

}

#endif  // GPLATES_FEATUREVISITORS_QUERYFEATUREPROPERTIESWIDGETPOPULATOR_H
