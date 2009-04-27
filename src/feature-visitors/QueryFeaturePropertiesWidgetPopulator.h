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

#include <boost/optional.hpp>
#include <QTreeWidget>

#include "gui/TreeWidgetBuilder.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "model/ConstFeatureVisitor.h"
#include "model/PropertyValue.h"
#include "model/ReconstructedFeatureGeometry.h"

namespace GPlatesPropertyValues
{
	class GpmlKeyValueDictionaryElement;
}


namespace GPlatesFeatureVisitors
{

	class QueryFeaturePropertiesWidgetPopulator:
			private GPlatesModel::FeatureVisitor
	{
	public:
		explicit
		QueryFeaturePropertiesWidgetPopulator(
				QTreeWidget &tree_widget):
			d_tree_widget_ptr(&tree_widget),
			d_tree_widget_builder(&tree_widget)
		{  }

		/**
		 * Populates the tree widget passed into constructor with the properties of
		 * @a feature_handle.
		 * @a focused_rfg is the clicked geometry, if any, and is the only geometry
		 * property that is expanded in the widget.
		 */
		void
		populate(
				GPlatesModel::FeatureHandle &feature_handle,
				GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type focused_rfg);

	private:
		virtual
		void
		visit_feature_handle(
				GPlatesModel::FeatureHandle &feature_handle);

		virtual
		void
		visit_feature_properties(
				GPlatesModel::FeatureHandle &feature_handle);

		virtual
		void
		visit_top_level_property_inline(
				GPlatesModel::TopLevelPropertyInline &top_level_property_inline);

		virtual
		void
		visit_enumeration(
				GPlatesPropertyValues::Enumeration &enumeration);

		virtual
		void
		visit_gml_line_string(
				GPlatesPropertyValues::GmlLineString &gml_line_string);

		virtual
		void
		visit_gml_multi_point(
				GPlatesPropertyValues::GmlMultiPoint &gml_multi_point);

		virtual
		void
		visit_gml_orientable_curve(
				GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve);

		virtual
		void
		visit_gml_point(
				GPlatesPropertyValues::GmlPoint &gml_point);

		virtual
		void
		visit_gml_polygon(
				GPlatesPropertyValues::GmlPolygon	&gml_polygon);

		virtual
		void
		visit_gml_time_instant(
				GPlatesPropertyValues::GmlTimeInstant &gml_time_instant);

		virtual
		void
		visit_gml_time_period(
				GPlatesPropertyValues::GmlTimePeriod &gml_time_period);

		virtual
		void
		visit_gpml_constant_value(
				GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value);

		virtual
		void
		visit_gpml_finite_rotation(
				GPlatesPropertyValues::GpmlFiniteRotation &gpml_finite_rotation);

		virtual
		void
		visit_gpml_finite_rotation_slerp(
				GPlatesPropertyValues::GpmlFiniteRotationSlerp &gpml_finite_rotation_slerp);

		virtual
		void
		visit_gpml_irregular_sampling(
				GPlatesPropertyValues::GpmlIrregularSampling &gpml_irregular_sampling);

		virtual
		void
		visit_gpml_key_value_dictionary(
				GPlatesPropertyValues::GpmlKeyValueDictionary &gpml_key_value_dictionary);

		virtual
		void
		visit_gpml_plate_id(
				GPlatesPropertyValues::GpmlPlateId &gpml_plate_id);



		virtual
		void
		visit_gpml_old_plates_header(
				GPlatesPropertyValues::GpmlOldPlatesHeader &gpml_old_plates_header);

		virtual
		void
		visit_xs_boolean(
				GPlatesPropertyValues::XsBoolean &xs_boolean);

		virtual
		void
		visit_xs_double(
				GPlatesPropertyValues::XsDouble &xs_double);

		virtual
		void
		visit_xs_integer(
				GPlatesPropertyValues::XsInteger& xs_integer);

		virtual
		void
		visit_xs_string(
				GPlatesPropertyValues::XsString &xs_string);



	private:
		QTreeWidget *d_tree_widget_ptr;

		//! Used to build the QTreeWidget from QTreeWidgetItems.
		GPlatesGui::TreeWidgetBuilder d_tree_widget_builder;

		//! The focused geometry if any.
		boost::optional<GPlatesModel::FeatureHandle::properties_iterator> d_focused_geometry;

		//! The last property visited when iterating through the property values.
		boost::optional<GPlatesModel::FeatureHandle::properties_iterator> d_last_property_visited;

		void
		add_child_then_visit_value(
				const QString &name,
				const QString &value,
				GPlatesModel::PropertyValue &property_value_to_visit);

		void
		add_gpml_key_value_dictionary_element(
				GPlatesPropertyValues::GpmlKeyValueDictionaryElement &element);


		void
		write_polygon_ring(
			GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_ptr);

		void
		write_multipoint_member(
			const GPlatesMaths::PointOnSphere &point);
	};

}

#endif  // GPLATES_FEATUREVISITORS_QUERYFEATUREPROPERTIESWIDGETPOPULATOR_H
