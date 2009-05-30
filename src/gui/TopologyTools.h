/**
 * \file 
 * $Revision: 4762 $
 * $Date: 2009-02-06 15:38:44 -0800 (Fri, 06 Feb 2009) $ 
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
 
#ifndef GPLATES_GUI_TOPOLOGY_TOOLS_H
#define GPLATES_GUI_TOPOLOGY_TOOLS_H

#include <QDebug>
#include <QObject>

#include <boost/optional.hpp>
#include <boost/none.hpp>

#include "global/types.h"

#include "TopologySectionsContainer.h"

#include "feature-visitors/TopologySectionsFinder.h"

#include "maths/GeometryOnSphere.h"
#include "maths/ConstGeometryOnSphereVisitor.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"

#include "model/ModelInterface.h"
#include "model/FeatureHandle.h"
#include "model/ReconstructedFeatureGeometry.h"

#include "model/PropertyValue.h"
#include "property-values/GpmlTopologicalPolygon.h"
#include "property-values/GpmlTopologicalSection.h"
#include "property-values/GpmlTopologicalPoint.h"
#include "property-values/GpmlTopologicalLineSection.h"
#include "property-values/GpmlTopologicalIntersection.h"

#include "utils/GeometryCreationUtils.h"
#include "utils/UnicodeStringUtils.h"

#include "view-operations/RenderedGeometryCollection.h"

namespace GPlatesQtWidgets
{
	class CreateFeatureDialog;
	class ViewportWindow;
	class TopologyToolsWidget;
}

namespace GPlatesGui
{
	class FeatureFocus;
}

namespace GPlatesGui
{
	class TopologyTools:
			public QObject, 
			public GPlatesMaths::ConstGeometryOnSphereVisitor
	{
		Q_OBJECT

	public:

		/** What mode the tools were started in ; NOTE this can change during tool use */
		enum CanvasToolMode
		{
			BUILD, EDIT
		};

		/**
		 * What kinds of topology to work with 
		 */
//FIXME
		enum GeometryType
		{
			PLATEPOLYGON, DEFORMING_REGION
		};

		/** simple enum to identify neighbor relation of topology sections */
		enum NeighborRelation
		{
			NONE, INTERSECT_PREV, INTERSECT_NEXT, OVERLAP_PREV, OVERLAP_NEXT, OTHER
		};


		/** Constructor */
		TopologyTools(
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
				GPlatesGui::FeatureFocus &feature_focus,
				GPlatesQtWidgets::ViewportWindow &view_state);
		
		/**
		 * Set the click point (called from canvas tool)
		 */
		void
		set_click_point( double lat, double lon );

		/**
		 * Sets the desired geometry type, d_geometry_type.
		 *
		 * This public method is used by the QUndoCommands that manipulate this widget.
		 *
		 * FIXME: If we move d_geometry_type into the QTreeWidgetItems themselves, we
		 * wouldn't need this ugly setter.
		 */
		void
		set_geometry_type(
				GeometryType geom_type)
		{
			d_geometry_type = geom_type;
		}

		/**
		 * Access the desired geometry type, d_geometry_type.
		 */
		GeometryType
		geometry_type() const
		{
			return d_geometry_type;
		}

#if 0
		/**
		 * Accessor for the Create Feature Dialog, for signal/slot connections etc.
		 */
		GPlatesQtWidgets::CreateFeatureDialog &
		create_feature_dialog()
		{
			return *d_create_feature_dialog;
		}
#endif
		

		// Please keep these geometries ordered alphabetically.

		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit_multi_point_on_sphere(
				GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere);

		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit_point_on_sphere(
				GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere);

		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit_polygon_on_sphere(
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere);

		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit_polyline_on_sphere(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere);


		/**
		 * Updates the  geometry rendered on screen.
		 */
		void
		update_geometry();

		/**
		 * From the Sections Table, create the tmp. geom. and property value items 
		 */
		void
		create_sections_from_sections_table();

		/**
		 * FIXME:
		 */
		void
		process_intersections();

		/**
		 * FIXME:
		 */
		void
		compute_intersection(
			const GPlatesMaths::PolylineOnSphere* node1_polyline,
			const GPlatesMaths::PolylineOnSphere* node2_polyline,
			NeighborRelation relation);

		/**
		 * 
		 */
		int
		find_feature_in_topology(
			GPlatesModel::FeatureHandle::weak_ref feature);

		void
		set_topology_feature_ref(
			GPlatesModel::FeatureHandle::weak_ref feature)
		{
			d_topology_feature_ref = feature;
// FIXME: 
			qDebug() << "set_topology_feature_ref()";
			show_numbers();
		}

		/**
		 * Once the feature is created from the dialog, append a boundary prop. value.
		 */
		void
		append_boundary_to_feature(
			GPlatesModel::FeatureHandle::weak_ref feature);

		void
		fill_section_vectors_from_feature_ref(
			GPlatesModel::FeatureHandle::weak_ref feature);


#if 0
		void
		fill_section_table_from_topology_sections();
#endif

		void
		fill_topology_sections_from_section_table();


	public slots:
		
		void
		clear_widgets();

		void
		fill_widgets(
			GPlatesModel::FeatureHandle::weak_ref feature_ref,
			GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type associated_rfg);

		void
		handle_reconstruction_time_change( double t );

		void
		set_focus(
			GPlatesModel::FeatureHandle::weak_ref feature_ref,
			GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type associated_rfg);

		void
		display_feature(
			GPlatesModel::FeatureHandle::weak_ref feature_ref,
			GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type associated_rfg);

		void
		display_feature_focus_modified(
			GPlatesModel::FeatureHandle::weak_ref feature_ref,
			GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type associated_rfg);

		void
		display_feature_topology(
			GPlatesModel::FeatureHandle::weak_ref feature_ref,
			GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type associated_rfg);

		void
		display_feature_on_boundary(
			GPlatesModel::FeatureHandle::weak_ref feature_ref,
			GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type associated_rfg);

		void
		display_feature_off_boundary(
			GPlatesModel::FeatureHandle::weak_ref feature_ref,
			GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type associated_rfg);

		void
		handle_shift_left_click(
			const GPlatesMaths::PointOnSphere &click_pos_on_globe,
			const GPlatesMaths::PointOnSphere &oriented_click_pos_on_globe,
			bool is_on_globe);


		/** Slots for signals from TopologySectionsContainer */
		void
		cleared();

		void
		insertion_point_moved(
			GPlatesGui::TopologySectionsContainer::size_type new_index);
		
	
		void
		entry_removed(
			GPlatesGui::TopologySectionsContainer::size_type deleted_index);
	
		void
		entries_inserted(
			GPlatesGui::TopologySectionsContainer::size_type inserted_index,
			GPlatesGui::TopologySectionsContainer::size_type quantity);

		void
		entries_modified(
			GPlatesGui::TopologySectionsContainer::size_type modified_index_begin,
			GPlatesGui::TopologySectionsContainer::size_type modified_index_end);

		/**
		 * Draw the temporary geometry (if there is one) on the screen.
		 */
		void
		draw_all_layers_clear();

		void
		draw_all_layers();

		void 
		draw_topology_geometry();

		void 
		draw_focused_geometry();

		void 
		draw_focused_geometry_end_points();

		void
		draw_segments();

		void
		draw_end_points();

		void
		draw_intersection_points();

#if 0
		void
		draw_click_points();
#endif

		void
		draw_click_point();

		void
		activate( CanvasToolMode );

		void
		deactivate();

		void
		connect_to_focus_signals( bool state );

		void
		connect_to_topology_sections_container_signals( bool state );


// // // 
// // // 
// // // 
		/**
		 * The slot that gets called when the user clicks "Add Focused Feature".
		 */
		void
		handle_add_feature();

		void
		handle_remove_all_sections();

		/**
		 * The slot that gets called when the user clicks "Remove Feature".
		 */
		void
		handle_remove_feature();

		void
		handle_insert_feature(int index);

		/**
		 * The slot that gets called when the user clicks "Clear".
		 */
		void
		handle_clear();


		/**
		 * The slot that gets called when the user clicks "Apply".
		 */
		void
		handle_apply();

		/**
		 * The slot that gets called when the user clicks "Cancel".
		 */
		void
		handle_cancel();

	private:

		/**
		 * Used to draw rendered geometries.
		 */
		GPlatesViewOperations::RenderedGeometryCollection *d_rendered_geom_collection;

		/**
		 * Rendered geometry layers to draw into 
		 */
		GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type
			d_topology_geometry_layer_ptr,
			d_focused_feature_layer_ptr,
			d_segments_layer_ptr,
			d_end_points_layer_ptr,
			d_intersection_points_layer_ptr,
			d_click_points_layer_ptr;


		/**
		 * This is our reference to the Feature Focus, which we use to let the rest of the
		 * application know what the user just clicked on.
		 */
		GPlatesGui::FeatureFocus *d_feature_focus_ptr;

		/**
		 * The View State is used to access the digitisation layer in the globe in the
		 * globe canvas.
		 */
		GPlatesQtWidgets::ViewportWindow *d_view_state_ptr;

		/**
		 * What kind of geometry are we -supposed- to be digitising?
		 * Note that what we actually get when the user hits Create may be
		 * different (A LineString with only one point?! That's unpossible.)
		 */
		GeometryType d_geometry_type;


		/*
		* pointer to the TopologySectionsContainer in ViewportWindow.
		*/
		GPlatesGui::TopologySectionsContainer *d_topology_sections_container_ptr;

		/**
		* pointer to the TopologyToolsWidget in Task Panel
		*/
		GPlatesQtWidgets::TopologyToolsWidget *d_topology_tools_widget_ptr;


	
		/** Keeps track of what canvas tool mode activted this object */
		CanvasToolMode d_mode;	


		/**
		 * These d_tmp_ vars are all set by the canvas tool, or the widget
		 * and used during interation around the Sections Table
		 * as the code bounces between visitor functions and intersection processing functions.
		 */
		int d_tmp_index;
		int d_tmp_sections_size;
		int d_tmp_prev_index;
		int d_tmp_next_index;

		// These control the behavior of the geom. visitors
		bool d_is_active;
		bool d_in_edit;
			
		// These control the behavior of the geom. visitors
		bool d_visit_to_check_type;
		bool d_visit_to_create_properties;
		bool d_visit_to_get_focus_end_points;

		// These get set during the visit 
		GPlatesGlobal::FeatureTypes d_tmp_feature_type;

		bool d_tmp_index_use_reverse;
		bool d_tmp_process_intersections;


		std::vector<GPlatesMaths::PointOnSphere> d_tmp_index_vertex_list;

		std::vector<GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type> 
			d_source_geometry_property_delegate_ptrs;

		GPlatesModel::FeatureId d_tmp_index_fid;

		QString d_tmp_property_name;
		QString d_tmp_value_type;

		int d_num_intersections_with_prev;
		int d_num_intersections_with_next;


		/**
		* this d_ vars keep track of the widget's current state as data is transfered from
		* the Clicked Table to the Sections Table
		*/
		bool d_use_reverse;


		// collection of intersection points
		std::vector<GPlatesMaths::PointOnSphere> d_intersection_vertex_list;

		GPlatesMaths::real_t d_closeness;

		double d_click_point_lat;
		double d_click_point_lon;
		const GPlatesMaths::PointOnSphere *d_click_point_ptr;

		// end_points for currently focused feature 
		std::vector<GPlatesMaths::PointOnSphere> d_feature_focus_head_points; 
		std::vector<GPlatesMaths::PointOnSphere> d_feature_focus_tail_points; 

		/*
		 * This index is set when the feature focus references a feature on the boundary.
		 * Used to access the d_section_FOO vectors during Add/Remove/Insert etc. operations
		 */
		int d_section_feature_focus_index;


		/**
		 * These vectors are sychronized to the 'Topology Sections' table 
		 * via the d_section_feature_focus_index.
		 */
		std::vector<GPlatesModel::FeatureId> d_section_ids;
		std::vector<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type> 
			d_section_ptrs;
		std::vector<std::pair<double, double> > d_section_click_points;
		std::vector<bool> d_section_reverse_flags;

		/**
		 * a collection of TopologySectionsContainer::TableRow structs
		 */
		GPlatesGui::TopologySectionsContainer::container_type d_topology_sections;


		// collection of end points for all boundary features
		std::vector<GPlatesMaths::PointOnSphere> d_head_end_points;
		std::vector<GPlatesMaths::PointOnSphere> d_tail_end_points; 

		// collection of intersection points for all boundary features
		std::vector<GPlatesMaths::PointOnSphere> d_intersection_points;
		
		// collection of sub-segments for all boundary features
		std::vector<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> 
			d_segments;

		// collection of sub-segments for insert operation
		std::vector<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> 
			d_insert_segments;

		/**
		 * An odered collection of all the vertices
		 */
		std::vector<GPlatesMaths::PointOnSphere> d_topology_vertices;

		/**
		 * The d_vertex_list gets processed into this geometry; may be boost::none 
		 */
		boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> 
			d_topology_geometry_opt_ptr;

		/*
		 * 
		 */ 
		GPlatesModel::FeatureHandle::weak_ref d_topology_feature_ref;

		GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type d_topology_feature_rfg;


		/*
		 * These variables keep track during insert operations
		 */
		int d_insertion_point;
		int d_insert_index;
		GPlatesModel::FeatureHandle::weak_ref d_insert_feature_ref;
		GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type d_insert_feature_rfg;

		//
		// private functions 
		//
		void
		create_child_rendered_layers();

		void
		show_numbers();

		/** sets d_topology_geometry_opt_ptr */
		void
		create_geometry_from_vertex_list(
			std::vector<GPlatesMaths::PointOnSphere> &points,
			GPlatesGui::TopologyTools::GeometryType target_geom_type,
			GPlatesUtils::GeometryConstruction::GeometryConstructionValidity &validity);
	};
}

#endif  // GPLATES_GUI_TOPOLOGY_TOOLS_H


