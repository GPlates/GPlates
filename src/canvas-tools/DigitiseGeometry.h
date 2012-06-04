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

#include "CanvasTool.h"

#include "view-operations/GeometryType.h"
#include "view-operations/RenderedGeometryCollection.h"


namespace GPlatesCanvasTools
{
	class GeometryOperationState;
}

namespace GPlatesGui
{
	class CanvasToolWorkflows;
}

namespace GPlatesViewOperations
{
	class AddPointGeometryOperation;
	class GeometryBuilder;
	class QueryProximityThreshold;
}

namespace GPlatesCanvasTools
{
	/**
	 * This is the canvas tool used to define new geometry.
	 */
	class DigitiseGeometry :
			public CanvasTool
	{
	public:

		/**
		 * Convenience typedef for GPlatesUtils::non_null_intrusive_ptr<DigitiseGeometry>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<DigitiseGeometry> non_null_ptr_type;

		static
		const non_null_ptr_type
		create(
				const status_bar_callback_type &status_bar_callback,
				GPlatesViewOperations::GeometryType::Value geom_type,
				GPlatesViewOperations::GeometryBuilder &geometry_builder,
				GPlatesCanvasTools::GeometryOperationState &geometry_operation_state,
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
				GPlatesViewOperations::RenderedGeometryCollection::MainLayerType main_rendered_layer_type,
				GPlatesGui::CanvasToolWorkflows &canvas_tool_workflows,
				const GPlatesViewOperations::QueryProximityThreshold &query_proximity_threshold)
		{
			return new DigitiseGeometry(
					status_bar_callback,
					geom_type,
					geometry_builder,
					geometry_operation_state,
					rendered_geometry_collection,
					main_rendered_layer_type,
					canvas_tool_workflows,
					query_proximity_threshold);
		}

		virtual
		~DigitiseGeometry();
		
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
		 * Create a DigitiseGeometry instance.
		 */
		DigitiseGeometry(
				const status_bar_callback_type &status_bar_callback,
				GPlatesViewOperations::GeometryType::Value geom_type,
				GPlatesViewOperations::GeometryBuilder &geometry_builder,
				GPlatesCanvasTools::GeometryOperationState &geometry_operation_state,
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
				GPlatesViewOperations::RenderedGeometryCollection::MainLayerType main_rendered_layer_type,
				GPlatesGui::CanvasToolWorkflows &canvas_tool_workflows,
				const GPlatesViewOperations::QueryProximityThreshold &query_proximity_threshold);	

		/**
		 * This is the type of geometry this particular DigitiseGeometry tool should default to.
		 */
		GPlatesViewOperations::GeometryType::Value d_default_geom_type;

		GPlatesViewOperations::GeometryBuilder &d_geometry_builder;

		/**
		 * Digitise operation for adding a point to digitised geometry.
		 */
		boost::scoped_ptr<GPlatesViewOperations::AddPointGeometryOperation> d_add_point_geometry_operation;
	};
}

#endif  // GPLATES_CANVASTOOLS_DIGITISEGEOMETRY_H
