/* $Id$ */

/**
 * \file 
 * Moves points/vertices in a geometry as the user selects a vertex and drags it.
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

#ifndef GPLATES_VIEWOPERATIONS_MOVEVERTEXGEOMETRYOPERATION_H
#define GPLATES_VIEWOPERATIONS_MOVEVERTEXGEOMETRYOPERATION_H

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <QObject>

#include "GeometryBuilder.h"
#include "GeometryOperation.h"
#include "RenderedGeometryCollection.h"
#include "UndoRedo.h"


namespace GPlatesMaths
{
	class PointOnSphere;
}

namespace GPlatesGui
{
	class ChooseCanvasTool;
}

namespace GPlatesViewOperations
{
	class ActiveGeometryOperation;
	class GeometryBuilder;
	class GeometryOperationTarget;
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
		MoveVertexGeometryOperation(
				GeometryOperationTarget &geometry_operation_target,
				ActiveGeometryOperation &active_geometry_operation,
				RenderedGeometryCollection *rendered_geometry_collection,
				GPlatesGui::ChooseCanvasTool &choose_canvas_tool,
				const QueryProximityThreshold &query_proximity_threshold);

		/**
		 * Activate this operation and attach to specified @a GeometryBuilder
		 * and render into specified main rendered layer.
		 */
		virtual
		void
		activate(
				GeometryBuilder *,
				RenderedGeometryCollection::MainLayerType main_layer_type);

		//! Deactivate this operation.
		virtual
		void
		deactivate();
#if 0
		//! User has just clicked on the sphere.
		void
		start_drag(
				const GPlatesMaths::PointOnSphere &clicked_pos_on_sphere,
				const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere);
#else
		//! User has just clicked on the sphere.
		// RJW: Mods to allow us to use this from both map and globe tools. 
		void
		start_drag(
				const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere,
				const double &closeness_inclusion_threshold);
#endif
		//! User is currently in the middle of dragging the mouse.
		void
		update_drag(
				const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere);

		//! User has released mouse button after dragging.
		void
		end_drag(
				const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere);

		//! The mouse has moved but it is not a drag because mouse button is not pressed.
		void
		mouse_move(
				const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere,
				const double &closeness_inclusion_threshold);

	public slots:
		// NOTE: all signals/slots should use namespace scope for all arguments
		//       otherwise differences between signals and slots will cause Qt
		//       to not be able to connect them at runtime.

		void
		geometry_builder_stopped_updating_geometry();

	private:
		/**
		 * This is used to build geometry. We move vertices with it.
		 */
		GeometryBuilder *d_geometry_builder;

		/**
		 * Used by undo/redo.
		 */
		GeometryOperationTarget *d_geometry_operation_target;

		/**
		 * We call this when we activate/deactivate.
		 */
		ActiveGeometryOperation *d_active_geometry_operation;

		/**
		 * This is where we render our geometries and activate our render layer.
		 */
		RenderedGeometryCollection *d_rendered_geometry_collection;

		/**
		 * The main rendered layer we're currently rendering into.
		 */
		RenderedGeometryCollection::MainLayerType d_main_layer_type;

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
		GPlatesGui::ChooseCanvasTool *d_choose_canvas_tool;

		/**
		 * Used to query the proximity threshold based on position on globe.
		 */
		const QueryProximityThreshold *d_query_proximity_threshold;

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
	};
}

#endif // GPLATES_VIEWOPERATIONS_MOVEVERTEXGEOMETRYOPERATION_H
