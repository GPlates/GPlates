/**
 * \file 
 * $Revision$
 * $Date$ 
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
 
#ifndef GPLATES_QTWIDGETS_PLATECLOSUREWIDGET_H
#define GPLATES_QTWIDGETS_PLATECLOSUREWIDGET_H

#include <QDebug>
#include <QWidget>
#include <QTreeWidget>
#include <QUndoStack>
#include <boost/optional.hpp>
#include <boost/none.hpp>
#include "PlateClosureWidgetUi.h"

#include "global/types.h"

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

#include "view-operations/RenderedGeometryCollection.h"

namespace GPlatesViewOperations
{
	class RenderedGeometryFactory;
}

// An effort to reduce the dependency spaghetti currently plaguing the GUI.
namespace GPlatesGui
{
	class FeatureFocus;
}

namespace GPlatesQtWidgets
{
	// Forward declaration to avoid the GUI header spaghetti as much as possible.
	// class ExportCoordinatesDialog;
	class CreateFeatureDialog;
	class ViewportWindow;

	class PlateClosureWidget:
			public QWidget, 
			public GPlatesMaths::ConstGeometryOnSphereVisitor,
			protected Ui_PlateClosureWidget
	{
		Q_OBJECT

	public:

		/**
		 * What kinds of geometry the PlateClosureWidget can be configured for.
		 */
		enum GeometryType
		{
			PLATEPOLYGON,
			DEFORMINGPLATE
		};

		/** simple enum to identify neighbor relation */
		enum NeighborRelation
		{
			NONE, INTERSECT_PREV, INTERSECT_NEXT, OVERLAP_PREV, OVERLAP_NEXT, OTHER
		};
		

		explicit
		PlateClosureWidget(
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
				GPlatesViewOperations::RenderedGeometryFactory &rendered_geom_factory,
				GPlatesGui::FeatureFocus &feature_focus,
				GPlatesModel::ModelInterface &model_interface,
				ViewportWindow &view_state_,
				QWidget *parent_ = NULL);
		
		/**
		 * Set the click point (called from canvas tool)
		 */
		void
		set_click_point( double lat, double lon );

		/**
		 * Updates the temporary geometry rendered on screen.
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
		 * Once the feature is created from the dialog, append a boundary prop. value.
		 */
		void
		append_boundary_to_feature(
			GPlatesModel::FeatureHandle::weak_ref feature);

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

		/**
		 * Accessor for the Create Feature Dialog, for signal/slot connections etc.
		 */
		CreateFeatureDialog &
		create_feature_dialog()
		{
			return *d_create_feature_dialog;
		}
		

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


	public slots:
		
		/**
		 * Resets all fields to their defaults.
		 */
		void
		clear_widgets();

		void
		fill_widgets(
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
		display_feature_not_on_boundary(
			GPlatesModel::FeatureHandle::weak_ref feature_ref,
			GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type associated_rfg);

		/**
		 * Configures widgets to accept new geometry of a specific type.
		 */
		void
		initialise_geometry(
				GeometryType geom_type);

		/**
		 * Configures widgets for a new geometry type while preserving the
		 * points that are currently being digitised.
		 * 
		 * Triggered when the user switches to a different PlateClosure CanvasTool.
		 */
		void
		change_geometry_type(
				GeometryType geom_type);

		/**
		 * Draw the temporary geometry (if there is one) on the screen.
		 */
		void
		draw_all_layers_clear();

		void
		draw_all_layers();

		void 
		draw_temporary_geometry();

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

		void
		draw_click_points();

		void
		draw_click_point();

	private slots:

		/**
		 * The slot that gets called when the user clicks "New Topology"
		 */
		void
		handle_new_topology();

		/**
		 * The slot that gets called when the user clicks "Edit Topology"
		 */
		void
		handle_edit_topology();

		/**
		 * The slot that gets called when the user clicks "Use Coordinates in Reverse".
		 */
		void
		handle_use_coordinates_in_reverse();

		/**
		 * The slot that gets called when the user clicks "Choose Feature".
		 */
		void
		handle_append_feature();

		/**
		 * The slot that gets called when the user clicks "Remove Feature".
		 */
		void
		handle_remove_feature();

		void
		handle_insert_after();

		void
		handle_insert_before();

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
		 * Used to create @a RenderedGeometry objects.
		 */
		GPlatesViewOperations::RenderedGeometryFactory *d_rendered_geom_factory;


		/**
		 * Rendered geometry layers to draw into 
		 */
		GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type
			d_temporary_geometry_layer_ptr,
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
		 * The model
		 */ 
		GPlatesModel::ModelInterface *d_model_interface;

		/**
		 * The View State is used to access the digitisation layer in the globe in the
		 * globe canvas.
		 */
		ViewportWindow *d_view_state_ptr;

		/**
		 * The dialog the user sees when they hit the Create button.
		 * Memory managed by Qt.
		 */
		CreateFeatureDialog *d_create_feature_dialog;
		
		/**
		 * What kind of geometry are we -supposed- to be digitising?
		 * Note that what we actually get when the user hits Create may be
		 * different (A LineString with only one point?! That's unpossible.)
		 */
		GeometryType d_geometry_type;


		/**
		 * What kind of geometry did we successfully build last?
		 * 
		 * This may be boost::none if the digitisation widget has no
		 * (valid) point data yet.
		 */
		boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> 
			d_geometry_opt_ptr;

		/**
		* place holders for the widget data 
		*/
		QString d_first_coord;
		QString d_last_coord;


		std::vector<GPlatesMaths::PointOnSphere> d_vertex_list;
		

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
		* thise d_ vars keep track of the widget's current state as data is transfered from
		* the Clicked Table to the Sections Table
		*/
		bool d_use_reverse;


		// collection of intersection points
		std::vector<GPlatesMaths::PointOnSphere> d_intersection_vertex_list;

		GPlatesMaths::real_t d_closeness;

		double d_click_point_lat;
		double d_click_point_lon;
		const GPlatesMaths::PointOnSphere *d_click_point_ptr;

		/**
		 * These vectors are sychronized to the 'Topology Sections' table with d_tmp_index
		 */
		std::vector<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type> 
			d_section_ptrs;
		std::vector<GPlatesModel::FeatureId> d_section_ids;
		std::vector<std::pair<double, double> > d_section_click_points;
		std::vector<bool> d_section_reverse_flags;


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

		// end points for currently focused feature 
		std::vector<GPlatesMaths::PointOnSphere> d_focus_head_end_points; 
		std::vector<GPlatesMaths::PointOnSphere> d_focus_tail_end_points; 

		/*
		 * Set by display_feature, used by append_boundary_to_feature()
		 * Used when editing a topology 
        */

		GPlatesModel::FeatureHandle::weak_ref d_focused_feature_ref;
		GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type d_focused_rfg;

		GPlatesModel::ReconstructionGeometry::maybe_null_ptr_type d_focused_geometry;

		/*
		 * Set by  FIXME:
		 */ 
		GPlatesModel::FeatureHandle::weak_ref d_topology_feature_ref;

		/*
		 * When a feature is selected , and on the topology boundary this is set
		 * Used to access the d_section_FOO vectors
		 */
		int d_focused_index;

		//
		// private functions 
		//
		void
		create_child_rendered_layers();

		void
		show_numbers();
	};
}

#endif  // GPLATES_QTWIDGETS_PLATECLOSUREWIDGET_H

