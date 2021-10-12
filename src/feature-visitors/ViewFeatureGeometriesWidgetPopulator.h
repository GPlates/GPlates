/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2009, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_FEATUREVISITORS_VIEWFEATUREGEOMETRIESWIDGETPOPULATOR_H
#define GPLATES_FEATUREVISITORS_VIEWFEATUREGEOMETRIESWIDGETPOPULATOR_H

#include <vector>
#include <boost/optional.hpp>
#include <QTreeWidget>

#include "app-logic/Reconstruction.h"
#include "gui/TreeWidgetBuilder.h"
#include "model/FeatureHandle.h"
#include "model/FeatureVisitor.h"
#include "model/PropertyValue.h"
#include "model/PropertyName.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"


namespace GPlatesFeatureVisitors
{
	class ViewFeatureGeometriesWidgetPopulator:
			private GPlatesModel::FeatureVisitor
	{
	public:

		explicit
		ViewFeatureGeometriesWidgetPopulator(
				const GPlatesAppLogic::Reconstruction &reconstruction,
				QTreeWidget &tree_widget):
			d_reconstruction_ptr(&reconstruction),
			d_tree_widget_ptr(&tree_widget),
			d_tree_widget_builder(&tree_widget)
		{  }

		/**
		 * Populates the tree widget passed into constructor with the properties of
		 * @a feature_handle.
		 * @a focused_rg is the clicked geometry, if any, and is the only geometry
		 * property that is expanded in the widget.
		 */
		void
		populate(
				GPlatesModel::FeatureHandle::weak_ref &feature,
				GPlatesAppLogic::ReconstructionGeometry::maybe_null_ptr_to_const_type focused_rg);

	private:
		virtual
		bool
		initialise_pre_feature_properties(
				GPlatesModel::FeatureHandle &feature_handle);

		virtual
		void
		finalise_post_feature_properties(
				GPlatesModel::FeatureHandle &feature_handle);

		virtual
		bool
		initialise_pre_property_values(
				GPlatesModel::TopLevelPropertyInline &top_level_property_inline);

		virtual
		void
		finalise_post_property_values(
				GPlatesModel::TopLevelPropertyInline &top_level_property_inline);


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
				GPlatesPropertyValues::GmlPolygon &gml_polygon);

		virtual
		void
		visit_gpml_constant_value(
				GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value);


	private:
		
		/**
		 * Records details about the top-level items (properties) that we are building.
		 * This allows us to add all top-level items in a single pass, after we have figured
		 * out whether the property contains geometry or not.
		 */
		struct PropertyInfo
		{
			bool is_geometric_property;
			GPlatesGui::TreeWidgetBuilder::item_handle_type item_handle;
		};

		typedef std::vector<PropertyInfo> property_info_vector_type;
		typedef property_info_vector_type::const_iterator property_info_vector_const_iterator;

		/**
		 * Stores the reconstructed geometry and the property it belongs to.
		 *
		 * This allows us to display the reconstructed coordinates at the same time as the
		 * present-day coordinates.
		 */
		struct ReconstructedGeometryInfo
		{
			GPlatesModel::FeatureHandle::iterator d_property;
			GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type d_geometry;
			
			ReconstructedGeometryInfo(
					const GPlatesModel::FeatureHandle::iterator property,
					const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry):
				d_property(property),
				d_geometry(geometry)
			{  }
		};

		typedef std::vector<ReconstructedGeometryInfo> geometries_for_property_type;
		typedef geometries_for_property_type::const_iterator geometries_for_property_const_iterator;


		/**
		 * The Reconstruction which we will scan for RFGs from.
		 */
		const GPlatesAppLogic::Reconstruction *d_reconstruction_ptr;
		
		/**
		 * The tree widget we are populating.
		 */
		QTreeWidget *d_tree_widget_ptr;
		
		/**
		 * Used to build the QTreeWidget from QTreeWidgetItems.
		 */
		GPlatesGui::TreeWidgetBuilder d_tree_widget_builder;

		//! The focused geometry if any.
		boost::optional<GPlatesModel::FeatureHandle::iterator> d_focused_geometry;

		/**
		 * Records details about the top-level items (properties) that we are building.
		 * This allows us to add all top-level items in a single pass, after we have figured
		 * out whether the property contains geometry or not.
		 */
		property_info_vector_type d_property_info_vector;

		/**
		 * Stores the reconstructed geometries and the properties they belong to.
		 *
		 * This allows us to add the reconstructed coordinates at the same time as the
		 * present-day coordinates.
		 */
		geometries_for_property_type d_rfg_geometries;

		/**
		 * Iterates over d_reconstruction_ptr's RFGs, fills in the d_rfg_geometries table
		 * with geometry found from RFGs which belong to the given feature.
		 */
		void
		populate_rfg_geometries_for_feature(
				GPlatesModel::FeatureHandle &feature_handle);

		/**
		 * Searches the d_rfg_geometries table for geometry matching the given property.
		 */
		boost::optional<const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>
		get_reconstructed_geometry_for_property(
				const GPlatesModel::FeatureHandle::iterator property);


		void
		add_child_then_visit_value(
				const QString &name,
				const QString &value,
				GPlatesModel::PropertyValue &property_value_to_visit);

		void
		write_polygon_ring(
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_ptr);

	};

}

#endif  // GPLATES_FEATUREVISITORS_VIEWFEATUREGEOMETRIESWIDGETPOPULATOR_H
