/* $Id$ */

/**
 * \file 
 * Moves points/vertices in a geometry as the user selects a vertex and drags it.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
 * Copyright (C) 2009, 2010 Geological Survey of Norway
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

#ifndef GPLATES_VIEWOPERATIONS_MOVEVERTEXGEOMETRYOPERATION_H
#define GPLATES_VIEWOPERATIONS_MOVEVERTEXGEOMETRYOPERATION_H

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <QObject>

#include "maths/ConstGeometryOnSphereVisitor.h"
#include "app-logic/ReconstructionGeometry.h"
#include "model/types.h"
#include "GeometryBuilder.h"
#include "GeometryOperation.h"
#include "RenderedGeometryCollection.h"
#include "RenderedGeometryFactory.h"
#include "RenderedGeometryParameters.h"
#include "RenderedGeometryVisitor.h"
#include "RenderedMultiReconstructionGeometry.h"
#include "RenderedReconstructionGeometry.h"
#include "UndoRedo.h"


namespace GPlatesCanvasTools
{
	class GeometryOperationState;
	class ModifyGeometryState;
}

namespace GPlatesMaths
{
	class PointOnSphere;
}

namespace GPlatesGui
{
	class CanvasToolWorkflows;
	class FeatureFocus;
}

namespace GPlatesViewOperations
{
	class GeometryBuilder;
	class QueryProximityThreshold;
	class RenderedGeometryCollection;
	class RenderedGeometryLayer;
	struct RenderedGeometryProximityHit;

	/**
	 * Moves a vertex in @a GeometryBuilder and adds @a RenderedGeometry objects
	 * to @a RenderedGeometryCollection.
	 */
	class MoveVertexGeometryOperation :
		public GeometryOperation,
		private boost::noncopyable
	{
		Q_OBJECT

	public:
		
		/**
		 * Visitor to find a rendered geometry's reconstruction geometry.
		 */
		class ReconstructionGeometryFinder:
			public ConstRenderedGeometryVisitor
		{
		public:

			ReconstructionGeometryFinder()
			{  }

			virtual
			void
			visit_rendered_reconstruction_geometry(
				const RenderedReconstructionGeometry &rendered_reconstruction_geometry)
			{
				d_rendered_reconstruction_geometry.reset(
					rendered_reconstruction_geometry.get_reconstruction_geometry());
			}

			virtual
			void
			visit_rendered_multi_reconstruction_geometry(
				const RenderedMultiReconstructionGeometry &rendered_multi_reconstruction_geometry)
			{
				d_rendered_reconstruction_geometry.reset(
					rendered_multi_reconstruction_geometry.get_reconstruction_geometries().front());
			}

			boost::optional<GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type>
			get_reconstruction_geometry()
			{
				return d_rendered_reconstruction_geometry;
			}

		private:
				
			boost::optional<GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type>
				d_rendered_reconstruction_geometry;

		};		
	
		/**
		 * A visitor to add rendered geometries to the points and lines layers provided in the constructor.                                                                     
		 */
		class RenderedGeometryLayerFiller:
			public GPlatesMaths::ConstGeometryOnSphereVisitor
		{
		public:
			RenderedGeometryLayerFiller(
				GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type points_layer,
				GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type lines_layer):
				d_points_layer(points_layer),
				d_lines_layer(lines_layer)
				{
				
				}
			
			void
			visit_point_on_sphere(
				GPlatesMaths::PointGeometryOnSphere::non_null_ptr_to_const_type point_on_sphere)
			{
				RenderedGeometry rendered_geometry = RenderedGeometryFactory::create_rendered_point_on_sphere(
												point_on_sphere->position(),
												GeometryOperationParameters::NOT_IN_FOCUS_COLOUR);
				d_points_layer->add_rendered_geometry(rendered_geometry);
			}
			
			void
			visit_multi_point_on_sphere(
				GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere)
			{
				RenderedGeometry rendered_geometry = RenderedGeometryFactory::create_rendered_multi_point_on_sphere(
											multi_point_on_sphere,
											GeometryOperationParameters::NOT_IN_FOCUS_COLOUR);
				d_points_layer->add_rendered_geometry(rendered_geometry);
			}		
			
			void
			visit_polyline_on_sphere(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
			{
				RenderedGeometry rendered_geometry = RenderedGeometryFactory::create_rendered_polyline_on_sphere(
											polyline_on_sphere,
											GeometryOperationParameters::NOT_IN_FOCUS_COLOUR,
											GeometryOperationParameters::SECONDARY_LINE_WIDTH_HINT);
				d_lines_layer->add_rendered_geometry(rendered_geometry);
			}
			
			void
			visit_polygon_on_sphere(
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
			{
				RenderedGeometry rendered_geometry = RenderedGeometryFactory::create_rendered_polygon_on_sphere(
											polygon_on_sphere,
											GeometryOperationParameters::NOT_IN_FOCUS_COLOUR,
											GeometryOperationParameters::SECONDARY_LINE_WIDTH_HINT);
				d_lines_layer->add_rendered_geometry(rendered_geometry);
			}				
		
			
		private:

			GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type d_points_layer;				
			GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type d_lines_layer;		
		};

		MoveVertexGeometryOperation(
				GeometryBuilder &geometry_builder,
				GPlatesCanvasTools::GeometryOperationState &geometry_operation_state,
				GPlatesCanvasTools::ModifyGeometryState &modify_geometry_state,
				RenderedGeometryCollection &rendered_geometry_collection,
				RenderedGeometryCollection::MainLayerType main_rendered_layer_type,
				GPlatesGui::CanvasToolWorkflows &canvas_tool_workflows,
				const QueryProximityThreshold &query_proximity_threshold,
				GPlatesGui::FeatureFocus &feature_focus);

		//! Activate this operation.
		virtual
		void
		activate();

		//! Deactivate this operation.
		virtual
		void
		deactivate();

		void
		left_press(
				const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere,
				const double &closeness_inclusion_threshold);		
				

		//! User has just clicked and dragged on the sphere.
		void
		start_drag(
				const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere,
				const double &closeness_inclusion_threshold);

		//! User is currently in the middle of dragging the mouse.
		void
		update_drag(
				const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere);

		//! User has released mouse button after dragging.
		void
		end_drag(
				const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere);
				
		/**
		 * User has released the mouse without a drag.                                                                      
		 */
		void
		release_click();

		//! The mouse has moved but it is not a drag because mouse button is not pressed.
		void
		mouse_move(
				const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere,
				const double &closeness_inclusion_threshold);

	public Q_SLOTS:
		// NOTE: all signals/slots should use namespace scope for all arguments
		//       otherwise differences between signals and slots will cause Qt
		//       to not be able to connect them at runtime.

		void
		geometry_builder_stopped_updating_geometry();
		
		
	private Q_SLOTS:
	
		/**
		 * This will transfer any user-provided move-nearby-vertex information from the task panel tab.
		 */
		void
		handle_snap_vertices_setup_changed(
				bool should_check_nearby_vertices,
				double threshold,
				bool should_use_plate_id,
				GPlatesModel::integer_plate_id_type plate_id);
	
	private:
		
		/**
		 * This is used to build geometry. We move vertices with it.
		 */
		GeometryBuilder &d_geometry_builder;

		/**
		 * We call this when we activate/deactivate.
		 */
		GPlatesCanvasTools::GeometryOperationState &d_geometry_operation_state;

		/**
		 * This is where we render our geometries and activate our render layer.
		 */
		RenderedGeometryCollection &d_rendered_geometry_collection;

		/**
		 * The main rendered layer we're currently rendering into.
		 */
		RenderedGeometryCollection::MainLayerType d_main_rendered_layer_type;

		/**
		 * Rendered geometry layer used for lines.
		 */
		RenderedGeometryCollection::child_layer_owner_ptr_type d_lines_layer_ptr;

		/**
		 * Rendered geometry layer used for points.
		 */
		RenderedGeometryCollection::child_layer_owner_ptr_type d_points_layer_ptr;

		/**
		 * Rendered geometry layer used for the single highlighted point (the point
		 * that the mouse cursor is currently hovering over if any).
		 */
		RenderedGeometryCollection::child_layer_owner_ptr_type d_highlight_point_layer_ptr;

		/**
		 * Used by undo/redo to make sure appropriate tool is active
		 * when the undo/redo happens.
		 */
		GPlatesGui::CanvasToolWorkflows &d_canvas_tool_workflows;

		/**
		 * Used to query the proximity threshold based on position on globe.
		 */
		const QueryProximityThreshold &d_query_proximity_threshold;

		/**
		 * Unique command id used to merge move vertex commands together.
		 */
		UndoRedo::CommandId d_move_vertex_command_id;

		/**
		 * Index of vertex selected by user.
		 */
		unsigned int d_selected_vertex_index;

		/**
		 * Has the user selected a vertex.
		 */
		bool d_is_vertex_selected;
		
		/**
		 * Is the user hovering over a vertex                                                                     
		 */
		bool d_is_vertex_highlighted;		
		
		/**
		 * Does the user want to check nearby vertices of other geometries                                                                     
		 */
		bool d_should_check_nearby_vertices;
		
		/**
		 *  Does the user want to filter other geometries by plate-id                                                                     
		 */
		bool d_should_use_plate_id_filter;
		
		/**
		 * Proximity threshold (degrees of arc)for checking nearby vertices.                                                                     
		 */
		double d_nearby_vertex_threshold;
		
		/**
		 * Used to retrieve focused geometry when snapping vertices.
		 */
		GPlatesGui::FeatureFocus &d_feature_focus;
		
		/**
		 * Plate-id provided by user for restricting nearby features to check                                                                   
		 */
		boost::optional<GPlatesModel::integer_plate_id_type> d_filter_plate_id; 
		
		
		
		
		/**
		 * Perform the actual move vertex command.
		 */
		void
		move_vertex(
				const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere,
				bool is_intermediate_move);

		/**
		 * Test proximity to the points (at vertices) to the position on sphere and
		 * return closest point if at least one point was close enough, otherwise false.
		 */
		boost::optional<RenderedGeometryProximityHit>
		test_proximity_to_points(
				const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere,
				const double &closeness_inclusion_threshold);

		void
		connect_to_geometry_builder_signals();

		void
		disconnect_from_geometry_builder_signals();

		void
		create_rendered_geometry_layers();

		/**
		 * Update all @a RenderedGeometry objects.
		 */
		void
		update_rendered_geometries();

		void
		update_rendered_geometry(
				GeometryBuilder::GeometryIndex geom_index);

		void
		update_rendered_polyline_on_sphere(
				GeometryBuilder::GeometryIndex geom_index);

		void
		update_rendered_polygon_on_sphere(
				GeometryBuilder::GeometryIndex geom_index);

		void
		add_rendered_points(
				GeometryBuilder::GeometryIndex geom_index);

		void
		add_rendered_lines_for_polygon_on_sphere(
				GeometryBuilder::GeometryIndex geom_index);

		void
		add_rendered_lines_for_polyline_on_sphere(
				GeometryBuilder::GeometryIndex geom_index);

		void
		update_highlight_rendered_point(
				const GeometryBuilder::PointIndex highlight_point_index);
		
		/**
		 * Checks for nearby vertices in other geometries, and sends any results to the geometry builder.
		 */		
		void
		update_secondary_geometries(
				const GPlatesMaths::PointOnSphere &point_on_sphere);
		
		/**
		 * Adds any secondary geometries in the geometry_builder to the appropriate rendered layers.
		 */
		void
		update_rendered_secondary_geometries();
		
		
		/**
		 * Highlight any secondary geometry vertices which might be moved.                                                                    
		 */
		void
		update_highlight_secondary_vertices();
	};
}

#endif // GPLATES_VIEWOPERATIONS_MOVEVERTEXGEOMETRYOPERATION_H
