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

#ifndef GPLATES_CANVASTOOLS_DIGITISEGEOMETRY_H
#define GPLATES_CANVASTOOLS_DIGITISEGEOMETRY_H

#include <boost/scoped_ptr.hpp>

#include "canvas-tools/CanvasToolType.h"
#include "CanvasTool.h"
#include "view-operations/GeometryType.h"


namespace GPlatesGui
{
	class ChooseCanvasTool;
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
	class DigitiseGeometry:
			public CanvasTool
	{
	public:

		virtual
		~DigitiseGeometry();

		/**
		 * Create a DigitiseGeometry instance.
		 */
		DigitiseGeometry(
				GPlatesViewOperations::GeometryType::Value geom_type,
				GPlatesViewOperations::GeometryOperationTarget &geometry_operation_target,
				GPlatesViewOperations::ActiveGeometryOperation &active_geometry_operation,
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
				GPlatesGui::ChooseCanvasTool &choose_canvas_tool,
				GPlatesCanvasTools::CanvasToolType::Value canvas_tool_type,
				const GPlatesViewOperations::QueryProximityThreshold &query_proximity_threshold);		
		
		virtual
		void
		handle_activation();


		virtual
		void
		handle_deactivation();

		virtual
		void
		handle_left_click(
				const GPlatesMaths::PointOnSphere &point_on_sphere,
				bool is_on_earth,
				double proximity_inclusion_threshold);
		
	private:

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
		 * This is the type of geometry this particular DigitiseGeometry tool
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

#endif  // GPLATES_CANVASTOOLS_DIGITISEGEOMETRY_H
