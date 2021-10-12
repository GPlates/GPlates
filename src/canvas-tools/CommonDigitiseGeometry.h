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

#ifndef GPLATES_CANVASTOOLS_COMMONDIGITISEGEOMETRY_H
#define GPLATES_CANVASTOOLS_COMMONDIGITISEGEOMETRY_H

#include <boost/scoped_ptr.hpp>

#include "canvas-tools/CanvasToolType.h"
#include "view-operations/GeometryType.h"

namespace GPlatesMaths
{
	class PointOnSphere;
}


namespace GPlatesViewOperations
{
	class ActiveGeometryOperation;
	class AddPointGeometryOperation;
	class GeometryOperationTarget;
}


namespace GPlatesCanvasTools
{
	class CommonDigitiseGeometry
	{
		public:
			
		static
		void
		handle_activation(
			GPlatesViewOperations::GeometryOperationTarget *geometry_operation_target,
			GPlatesViewOperations::GeometryType::Value &default_geom_type,
			GPlatesViewOperations::AddPointGeometryOperation *add_point_geometry_operation,
			GPlatesCanvasTools::CanvasToolType::Value canvas_tool_type
			);

		static
		void
		handle_left_click(
			GPlatesViewOperations::AddPointGeometryOperation *add_point_geometry_operation,
			const GPlatesMaths::PointOnSphere &point_on_sphere,
			const double &closeness_inclusion_threshold
			);

	};

}


#endif // GPLATES_CANVAS_TOOLS_COMMONDIGITISEGEOMETRY_H
