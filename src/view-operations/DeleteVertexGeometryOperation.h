/* $Id$ */

/**
 * \file 
 * Deletes a vertex in a geometry when the user clicks on it.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#ifndef GPLATES_VIEWOPERATIONS_DELETEVERTEXGEOMETRYOPERATION_H
#define GPLATES_VIEWOPERATIONS_DELETEVERTEXGEOMETRYOPERATION_H

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
	 * Deletes a vertex in @a GeometryBuilder and manages @a RenderedGeometry objects
	 * in a @a RenderedGeometryCollection layer.
	 */
	class DeleteVertexGeometryOperation :
		public GeometryOperation,
		private boost::noncopyable
	{
		Q_OBJECT

	public:
		DeleteVertexGeometryOperation(
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

		//! User has just clicked on the sphere.
		void
		left_click(
				const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere,
				const double &closeness_inclusion_threshold);

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
		 * This is used to build geometry. We delete vertices with it.
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
		 * Perform the actual delete vertex command.
		 */
		void delete_vertex(
				const GeometryBuilder::PointIndex delete_vertex_index);

		/**
		 * Returns true if user is allowed to delete a vertex.
		 * Currently returns false if there's zero or one vertex.
		 */
		bool
		allow_delete_vertex() const;

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
		add_highlight_rendered_point(
				const GeometryBuilder::PointIndex highlight_point_index);
	};
}

#endif // GPLATES_VIEWOPERATIONS_DELETEVERTEXGEOMETRYOPERATION_H
