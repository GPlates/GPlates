/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008, 2009 Geological Survey of Norway
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

#ifndef GPLATES_CANVASTOOLS_COMMONCLICKGEOMETRY_H
#define GPLATES_CANVASTOOLS_COMMONCLICKGEOMETRY_H

namespace GPlatesGui
{
	class FeatureFocus;
	class FeatureTableModel;
}

namespace GPlatesMaths
{
	class PointOnSphere;
}

namespace GPlatesQtWidgets
{
	class ViewportWindow;
}

namespace GPlatesViewOperations
{
	class RenderedGeometryCollection;
}

namespace GPlatesCanvasTools
{
	class CommonClickGeometry
	{
		public:
			
		static
		void
		handle_left_click(
			const GPlatesMaths::PointOnSphere &point_on_sphere,
			double proximity_inclusion_threshold,
			const GPlatesQtWidgets::ViewportWindow &view_state,
			GPlatesGui::FeatureTableModel &clicked_table_model,
			GPlatesGui::FeatureFocus &feature_focus,
			GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection);

	};

}


#endif // GPLATES_CANVAS_TOOLS_COMMONCLICKGEOMETRY_H
