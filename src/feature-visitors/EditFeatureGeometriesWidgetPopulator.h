/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#ifndef GPLATES_FEATUREVISITORS_EDITFEATUREGEOMETRIESWIDGETPOPULATOR_H
#define GPLATES_FEATUREVISITORS_EDITFEATUREGEOMETRIESWIDGETPOPULATOR_H

#include <list>
#include <boost/optional.hpp>
#include <QTreeWidget>
#include "model/FeatureVisitor.h"
#include "model/PropertyValue.h"
#include "model/PropertyName.h"
#include "model/Reconstruction.h"


namespace GPlatesFeatureVisitors
{
	class EditFeatureGeometriesWidgetPopulator:
			public GPlatesModel::FeatureVisitor
	{
	public:
		
		/**
		 * Records details about the top-level items (properties) that we are building.
		 * This allows us to add all top-level items in a single pass, after we have figured
		 * out whether the property contains geometry or not.
		 */
		struct PropertyInfo
		{
			bool is_geometric_property;
			QTreeWidgetItem *item;
		};

		typedef std::vector<PropertyInfo> property_info_vector_type;
		typedef property_info_vector_type::const_iterator property_info_vector_const_iterator;

		/**
		 * Stores reconstructed geometry and the property it belongs to.
		 * This allows us to add the reconstructed coordinates at the same time as the
		 * present-day coordinates.
		 */
		struct ReconstructedPointInfo
		{
			const GPlatesModel::FeatureHandle::properties_iterator d_property;
			const GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type d_point;
			
			ReconstructedPointInfo(
					const GPlatesModel::FeatureHandle::properties_iterator property,
					const GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point):
				d_property(property),
				d_point(point)
			{  }
		};
		
		/**
		 * Stores reconstructed geometry and the property it belongs to.
		 * This allows us to add the reconstructed coordinates at the same time as the
		 * present-day coordinates.
		 */
		struct ReconstructedPolylineInfo
		{
			const GPlatesModel::FeatureHandle::properties_iterator d_property;
			const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type d_polyline;

			ReconstructedPolylineInfo(
					const GPlatesModel::FeatureHandle::properties_iterator property,
					const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline):
				d_property(property),
				d_polyline(polyline)
			{  }
		};
		
		typedef std::list<ReconstructedPointInfo> points_for_property_type;
		typedef points_for_property_type::const_iterator points_for_property_const_iterator;
		typedef std::list<ReconstructedPolylineInfo> polylines_for_property_type;
		typedef polylines_for_property_type::const_iterator polylines_for_property_const_iterator;


		explicit
		EditFeatureGeometriesWidgetPopulator(
				GPlatesModel::Reconstruction &reconstruction,
				QTreeWidget &tree_widget):
			d_reconstruction_ptr(&reconstruction),
			d_tree_widget_ptr(&tree_widget)
		{  }

		virtual
		~EditFeatureGeometriesWidgetPopulator() {  }

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
		visit_inline_property_container(
				GPlatesModel::InlinePropertyContainer &inline_property_container);


		virtual
		void
		visit_gml_line_string(
				GPlatesPropertyValues::GmlLineString &gml_line_string);

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
		visit_gpml_constant_value(
				GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value);


	private:
		
		/**
		 * The Reconstruction which we will scan for RFGs from.
		 */
		GPlatesModel::Reconstruction *d_reconstruction_ptr;
		
		/**
		 * The tree widget we are populating.
		 */
		QTreeWidget *d_tree_widget_ptr;
		
		/**
		 * A stack of tree widget items, used to keep track of where we must add new
		 * leaf nodes.
		 */
		std::vector<QTreeWidgetItem *> d_tree_widget_item_stack;
		
		/**
		 * When visiting a FeatureHandle, this member will record the properties_iterator
		 * of the last property visited.
		 */
		boost::optional<GPlatesModel::FeatureHandle::properties_iterator> d_last_property_visited;

		/**
		 * Records details about the top-level items (properties) that we are building.
		 * This allows us to add all top-level items in a single pass, after we have figured
		 * out whether the property contains geometry or not.
		 */
		property_info_vector_type d_property_info_vector;

		/**
		 * Stores reconstructed geometry and the property it belongs to.
		 * This allows us to add the reconstructed coordinates at the same time as the
		 * present-day coordinates.
		 */
		points_for_property_type d_rfg_points;
		polylines_for_property_type d_rfg_polylines;
		
		/**
		 * Iterates over d_reconstruction_ptr's RFGs, fills in the d_rfg_points table
		 * with geometry found from RFGs which belong to the given feature.
		 */
		void
		populate_rfg_points_for_feature(
				const GPlatesModel::FeatureHandle &feature_handle);

		/**
		 * Iterates over d_reconstruction_ptr's RFGs, fills in the d_rfg_polylines table
		 * with geometry found from RFGs which belong to the given feature.
		 */
		void
		populate_rfg_polylines_for_feature(
				const GPlatesModel::FeatureHandle &feature_handle);
		
		/**
		 * Searches the d_rfg_points table for geometry matching the given property.
		 */
		boost::optional<const GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type>
		get_reconstructed_point_for_property(
				const GPlatesModel::FeatureHandle::properties_iterator property);

		/**
		 * Searches the d_rfg_polylines table for geometry matching the given property.
		 */
		boost::optional<const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>
		get_reconstructed_polyline_for_property(
				const GPlatesModel::FeatureHandle::properties_iterator property);
		

		QTreeWidgetItem *
		add_child(
				const QString &name,
				const QString &value);


		QTreeWidgetItem *
		add_child_then_visit_value(
				const QString &name,
				const QString &value,
				GPlatesModel::PropertyValue &property_value_to_visit);
	};

}

#endif  // GPLATES_FEATUREVISITORS_EDITFEATUREGEOMETRIESWIDGETPOPULATOR_H
