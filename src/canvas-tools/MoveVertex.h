/* $Id$ */

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

#ifndef GPLATES_CANVASTOOLS_MOVEVERTEX_H
#define GPLATES_CANVASTOOLS_MOVEVERTEX_H

#include <boost/scoped_ptr.hpp>

#include "gui/CanvasTool.h"
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
	class MoveVertexGeometryOperation;
	class GeometryBuilderToolTarget;
	class QueryProximityThreshold;
	class RenderedGeometryCollection;
	class RenderedGeometryFactory;
}

namespace GPlatesCanvasTools
{
	/**
	 * This is the canvas tool used to move individual vertices of geometry.
	 */
	class MoveVertex:
			public GPlatesGui::CanvasTool
	{
	public:
		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<MoveVertex,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<MoveVertex,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;

		virtual
		~MoveVertex();

		/**
		 * Create a MoveVertex instance.
		 */
		static
		const non_null_ptr_type
		create(
				GPlatesViewOperations::GeometryBuilderToolTarget &geom_builder_tool_target,
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
				GPlatesViewOperations::RenderedGeometryFactory &rendered_geometry_factory,
				GPlatesGui::ChooseCanvasTool &choose_canvas_tool,
				const GPlatesViewOperations::QueryProximityThreshold &query_proximity_threshold,
				// Ultimately would like to remove the following arguments...
				GPlatesGui::Globe &globe,
				GPlatesQtWidgets::GlobeCanvas &globe_canvas,
				const GPlatesQtWidgets::ViewportWindow &view_state)
		{
			return MoveVertex::non_null_ptr_type(
					new MoveVertex(
							geom_builder_tool_target,
							rendered_geometry_collection,
							rendered_geometry_factory,
							choose_canvas_tool,
							query_proximity_threshold,
							globe,
							globe_canvas,
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
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
				bool was_on_globe,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport);

		virtual
		void
		handle_left_release_after_drag(
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
				bool was_on_globe,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport);

	protected:
		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		MoveVertex(
				GPlatesViewOperations::GeometryBuilderToolTarget &geom_builder_tool_target,
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
				GPlatesViewOperations::RenderedGeometryFactory &rendered_geometry_factory,
				GPlatesGui::ChooseCanvasTool &choose_canvas_tool,
				const GPlatesViewOperations::QueryProximityThreshold &query_proximity_threshold,
				// Ultimately would like to remove the following arguments...
				GPlatesGui::Globe &globe,
				GPlatesQtWidgets::GlobeCanvas &globe_canvas,
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
		GPlatesViewOperations::GeometryBuilderToolTarget *d_geom_builder_tool_target;

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

#endif  // GPLATES_CANVASTOOLS_MOVEVERTEX_H
