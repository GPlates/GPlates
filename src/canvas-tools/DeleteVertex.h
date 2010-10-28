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

#ifndef GPLATES_CANVASTOOLS_DELETEVERTEX_H
#define GPLATES_CANVASTOOLS_DELETEVERTEX_H

#include <boost/scoped_ptr.hpp>

#include "CanvasTool.h"

#include "model/FeatureHandle.h"


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
	class DeleteVertex :
			public CanvasTool
	{
	public:

		virtual
		~DeleteVertex();

		/**
		 * Create a DeleteVertex instance.
		 */
		DeleteVertex(
				GPlatesViewOperations::GeometryOperationTarget &geometry_operation_target,
				GPlatesViewOperations::ActiveGeometryOperation &active_geometry_operation,
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
				GPlatesGui::ChooseCanvasTool &choose_canvas_tool,
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

		virtual
		void
		handle_move_without_drag(
				const GPlatesMaths::PointOnSphere &point_on_sphere,
				bool is_on_earth,
				double proximity_inclusion_threshold);

	private:

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

#endif // GPLATES_CANVASTOOLS_DELETEVERTEX_H
