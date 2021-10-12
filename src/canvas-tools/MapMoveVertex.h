/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2009 Geological Survey of Norway
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

#ifndef GPLATES_CANVASTOOLS_MAPMOVEVERTEX_H
#define GPLATES_CANVASTOOLS_MAPMOVEVERTEX_H

#include <boost/scoped_ptr.hpp>

#include "gui/MapCanvasTool.h"
#include "model/FeatureHandle.h"
#include "model/ReconstructedFeatureGeometry.h"


namespace GPlatesGui
{
	class ChooseCanvasTool;
}

namespace GPlatesQtWidgets
{
	class ViewportWindow;
}

namespace GPlatesViewOperations
{
	class ActiveGeometryOperation;
	class MoveVertexGeometryOperation;
	class GeometryOperationTarget;
	class QueryProximityThreshold;
	class RenderedGeometryCollection;
}

namespace GPlatesCanvasTools
{
	/**
	 * This is the canvas tool used to move individual vertices of geometry.
	 */
	class MapMoveVertex:
			public GPlatesGui::MapCanvasTool
	{
	public:
		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<MapMoveVertex,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<MapMoveVertex,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;

		virtual
		~MapMoveVertex();

		/**
		 * Create a MapMoveVertex instance.
		 */
		static
		const non_null_ptr_type
		create(
				GPlatesViewOperations::GeometryOperationTarget &geometry_operation_target,
				GPlatesViewOperations::ActiveGeometryOperation &active_geometry_operation,
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
				GPlatesGui::ChooseCanvasTool &choose_canvas_tool,
				const GPlatesViewOperations::QueryProximityThreshold &query_proximity_threshold,
				// Ultimately would like to remove the following arguments...
				GPlatesQtWidgets::MapCanvas &map_canvas,
				GPlatesQtWidgets::MapView &map_view,
				const GPlatesQtWidgets::ViewportWindow &view_state)
		{
			return MapMoveVertex::non_null_ptr_type(
					new MapMoveVertex(
							geometry_operation_target,
							active_geometry_operation,
							rendered_geometry_collection,
							choose_canvas_tool,
							query_proximity_threshold,
							map_canvas,
							map_view,
							view_state),
					GPlatesUtils::NullIntrusivePointerHandler());
		}
		
		
		virtual
		void
		handle_activation();


		virtual
		void
		handle_deactivation();


		virtual
		void
		handle_left_drag(
				const QPointF &initial_point_on_scene,
				bool was_on_surface,
				const QPointF &current_point_on_scene,
				bool is_on_surface,
				const QPointF &translation);

		virtual
		void
		handle_left_release_after_drag(
				const QPointF &initial_point_on_scene,
				bool was_on_surface,
				const QPointF &current_point_on_scene,
				bool is_on_surface,
				const QPointF &translation);
				
		void
		handle_move_without_drag(
				const QPointF &current_point_on_scene,
				bool is_on_surface,
				const QPointF &translation);					

	protected:
		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		MapMoveVertex(
				GPlatesViewOperations::GeometryOperationTarget &geometry_operation_target,
				GPlatesViewOperations::ActiveGeometryOperation &active_geometry_operation,
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
				GPlatesGui::ChooseCanvasTool &choose_canvas_tool,
				const GPlatesViewOperations::QueryProximityThreshold &query_proximity_threshold,
				// Ultimately would like to remove the following arguments...
				GPlatesQtWidgets::MapCanvas &map_canvas,
				GPlatesQtWidgets::MapView &map_view,
				const GPlatesQtWidgets::ViewportWindow &view_state);
	private:
		/**
		 * This is the view state used to update the viewport window status bar.
		 */
		const GPlatesQtWidgets::ViewportWindow *d_view_state_ptr;

		/**
		 * Used to set main rendered layer.
		 */
		GPlatesViewOperations::RenderedGeometryCollection *d_rendered_geometry_collection;
		
		/**
		 * Used to select target of our move vertex operation.
		 */
		GPlatesViewOperations::GeometryOperationTarget *d_geometry_operation_target;

		/**
		 * Digitise operation for moving a vertex in digitised geometry.
		 */
		boost::scoped_ptr<GPlatesViewOperations::MoveVertexGeometryOperation>
			d_move_vertex_geometry_operation;

		/**
		 * Whether or not this tool is currently in the midst of a drag.
		 */
		bool d_is_in_drag;
	};
}

#endif  // GPLATES_CANVASTOOLS_GLOBEMOVEVERTEX_H
