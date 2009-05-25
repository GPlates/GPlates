/* $Id$ */

/**
 * \file Derived @a CanvasTool to insert vertices into temporary or focused feature geometry.
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

#ifndef GPLATES_CANVASTOOLS_MAPINSERTVERTEX_H
#define GPLATES_CANVASTOOLS_MAPINSERTVERTEX_H

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
	class InsertVertexGeometryOperation;
	class GeometryOperationTarget;
	class QueryProximityThreshold;
	class RenderedGeometryCollection;
}

namespace GPlatesCanvasTools
{
	/**
	 * This is the canvas tool used to insert vertices into geometry.
	 */
	class MapInsertVertex:
			public GPlatesGui::MapCanvasTool
	{
	public:
		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<MapInsertVertex,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<MapInsertVertex,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;

		virtual
		~MapInsertVertex();

		/**
		 * Create a MapInsertVertex instance.
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
			return MapInsertVertex::non_null_ptr_type(
					new MapInsertVertex(
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
		handle_left_click(
				const QPointF &click_point_on_scene,
				bool is_on_surface);

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
		handle_move_without_drag(
				const QPointF &current_point_on_scene,
				bool is_on_surface,
				const QPointF &translation);

	protected:
		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		MapInsertVertex(
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
		 * Used to select target of our insert vertex operation.
		 */
		GPlatesViewOperations::GeometryOperationTarget *d_geometry_operation_target;

		/**
		 * Digitise operation for inserting a vertex into digitised or focused feature geometry.
		 */
		boost::scoped_ptr<GPlatesViewOperations::InsertVertexGeometryOperation>
			d_insert_vertex_geometry_operation;
	};
}

#endif // GPLATES_CANVASTOOLS_MAPINSERTVERTEX_H
