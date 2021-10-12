/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2009 Geological Survey of Norway.
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

#ifndef GPLATES_CANVASTOOLS_MAPDIGITISEGEOMETRY_H
#define GPLATES_CANVASTOOLS_MAPDIGITISEGEOMETRY_H

#include <boost/scoped_ptr.hpp>

#include "canvas-tools/CanvasToolType.h"
#include "gui/MapCanvasTool.h"
#include "view-operations/GeometryType.h"


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
	class AddPointGeometryOperation;
	class GeometryOperationTarget;
	class QueryProximityThreshold;
	class RenderedGeometryCollection;
}

namespace GPlatesCanvasTools
{
	/**
	 * This is the canvas tool used to define new geometry.
	 */
	class MapDigitiseGeometry:
			public GPlatesGui::MapCanvasTool
	{
	public:
		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<MapDigitiseGeometry,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<MapDigitiseGeometry,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;

		virtual
		~MapDigitiseGeometry();

		/**
		 * Create a MapDigitiseGeometry instance.
		 *
		 * FIXME: Clean up unused parameters.
		 */
		static
		const non_null_ptr_type
		create(
				GPlatesViewOperations::GeometryType::Value geom_type,
				GPlatesViewOperations::GeometryOperationTarget &geom_operation_target,
				GPlatesViewOperations::ActiveGeometryOperation &active_geometry_operation,
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
				GPlatesGui::ChooseCanvasTool &choose_canvas_tool,
				GPlatesCanvasTools::CanvasToolType::Value canvas_tool_type,
				const GPlatesViewOperations::QueryProximityThreshold &query_proximity_threshold,
				// Ultimately would like to remove the following arguments...
				GPlatesQtWidgets::MapCanvas &map_canvas,
				GPlatesQtWidgets::MapView &map_view,
				const GPlatesQtWidgets::ViewportWindow &view_state);
		
		
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

	protected:
		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		MapDigitiseGeometry(
				GPlatesViewOperations::GeometryType::Value geom_type,
				GPlatesViewOperations::GeometryOperationTarget &geometry_operation_target,
				GPlatesViewOperations::ActiveGeometryOperation &active_geometry_operation,
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
				GPlatesGui::ChooseCanvasTool &choose_canvas_tool,
				GPlatesCanvasTools::CanvasToolType::Value canvas_tool_type,
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
		 * Used to select target of our add point operation.
		 */
		GPlatesViewOperations::GeometryOperationTarget *d_geometry_operation_target;

		/**
		* The type of this canvas tool.
		*/
		GPlatesCanvasTools::CanvasToolType::Value d_canvas_tool_type;

		/**
		 * This is the type of geometry this particular MapDigitiseGeometry tool
		 * should default to.
		 */
		GPlatesViewOperations::GeometryType::Value d_default_geom_type;

		/**
		 * Digitise operation for adding a point to digitised geometry.
		 */
		boost::scoped_ptr<GPlatesViewOperations::AddPointGeometryOperation>
			d_add_point_geometry_operation;
	};
}

#endif  // GPLATES_CANVASTOOLS_MAPDIGITISEGEOMETRY_H
