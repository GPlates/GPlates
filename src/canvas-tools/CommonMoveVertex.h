/* $Id$ */

/**
 * \file Derived @a CanvasTool to move vertices of temporary or focused feature geometry.
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

#ifndef GPLATES_CANVASTOOLS_COMMONMOVEVERTEX_H
#define GPLATES_CANVASTOOLS_COMMONMOVEVERTEX_H

namespace GPlatesMaths
{
	class PointOnSphere;
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
	class CommonMoveVertex
	{
		public:

		static
		void
		handle_activation(
			GPlatesViewOperations::GeometryOperationTarget *geometry_operation_target,
			GPlatesViewOperations::MoveVertexGeometryOperation *move_vertex_geometry_operation);

		static
		void
		handle_left_drag(
			bool &is_in_drag,
			GPlatesViewOperations::MoveVertexGeometryOperation *move_vertex_geometry_operation,
			const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
			const double &closeness_inclusion_threshold,
			const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe);

	};

}

#endif  // GPLATES_CANVASTOOLS_COMMONMOVEVERTEX_H
