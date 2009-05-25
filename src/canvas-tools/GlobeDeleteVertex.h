/* $Id$ */

/**
 * \file Derived @a CanvasTool to delete vertices from a temporary or focused feature geometry.
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

#ifndef GPLATES_CANVASTOOLS_GLOBEDELETEVERTEX_H
#define GPLATES_CANVASTOOLS_GLOBEDELETEVERTEX_H

#include <boost/scoped_ptr.hpp>

#include "gui/GlobeCanvasTool.h"
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
	class DeleteVertexGeometryOperation;
	class GeometryOperationTarget;
	class QueryProximityThreshold;
	class RenderedGeometryCollection;
}

namespace GPlatesCanvasTools
{
	/**
	 * This is the canvas tool used to delete vertices from geometry.
	 */
	class GlobeDeleteVertex:
			public GPlatesGui::GlobeCanvasTool
	{
	public:
		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<GlobeDeleteVertex,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GlobeDeleteVertex,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;

		virtual
		~GlobeDeleteVertex();

		/**
		 * Create a GlobeDeleteVertex instance.
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
				GPlatesGui::Globe &globe,
				GPlatesQtWidgets::GlobeCanvas &globe_canvas,
				const GPlatesQtWidgets::ViewportWindow &view_state)
		{
			return GlobeDeleteVertex::non_null_ptr_type(
					new GlobeDeleteVertex(
							geometry_operation_target,
							active_geometry_operation,
							rendered_geometry_collection,
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
		handle_left_click(
				const GPlatesMaths::PointOnSphere &click_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_click_pos_on_globe,
				bool is_on_globe);

		virtual
		void
		handle_move_without_drag(
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport);

	protected:
		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GlobeDeleteVertex(
				GPlatesViewOperations::GeometryOperationTarget &geometry_operation_target,
				GPlatesViewOperations::ActiveGeometryOperation &active_geometry_operation,
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
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
		 * Used to select target of our delete vertex operation.
		 */
		GPlatesViewOperations::GeometryOperationTarget *d_geometry_operation_target;

		/**
		 * Digitise operation for deleting a vertex from digitised or focused feature geometry.
		 */
		boost::scoped_ptr<GPlatesViewOperations::DeleteVertexGeometryOperation>
			d_delete_vertex_geometry_operation;
	};
}

#endif // GPLATES_CANVASTOOLS_GLOBEDELETEVERTEX_H
