/* $Id: MapCanvasPainter.h 4818 2009-02-13 10:05:30Z rwatson $ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2009-02-13 11:05:30 +0100 (fr, 13 feb 2009) $
 * 
 * Copyright (C) 2009 The Geological Survey of Norway
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

#ifndef GPLATES_GUI_MAPCANVASPAINTER_H
#define GPLATES_GUI_MAPCANVASPAINTER_H

#include <proj_api.h>

#include "gui/ColourTable.h"
#include "qt-widgets/MapCanvas.h"
#include "view-operations/RenderedGeometry.h"
#include "view-operations/RenderedGeometryCollection.h"
#include "view-operations/RenderedGeometryCollectionVisitor.h"


namespace GPlatesGui
{
	/**
	 * This is a Visitor to paint geometries on the map canvas.
	 */
	class MapCanvasPainter:
			public GPlatesViewOperations::ConstRenderedGeometryCollectionVisitor
	{
	public:
		explicit
		MapCanvasPainter(
			GPlatesQtWidgets::MapCanvas &canvas_,
			GPlatesViewOperations::RenderedGeometryCollection::main_layers_update_type &layers_to_visit):
			d_canvas_ptr(&canvas_),
			d_main_rendered_layers_to_visit(layers_to_visit)
		{  }
		
		virtual
		~MapCanvasPainter()
		{  
		}

		// Please keep these geometries ordered alphabetically.
#if 0
		virtual
		bool
		visit_main_rendered_layer(
            const GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
            GPlatesViewOperations::RenderedGeometryCollection::MainLayerType main_rendered_layer_type);
#endif
		virtual
		void
		visit_rendered_multi_point_on_sphere(
				const GPlatesViewOperations::RenderedMultiPointOnSphere &rendered_multi_point_on_sphere);

		virtual
		void
		visit_rendered_point_on_sphere(
				const GPlatesViewOperations::RenderedPointOnSphere &rendered_point_on_sphere);


		virtual
		void
		visit_rendered_polygon_on_sphere(
				const GPlatesViewOperations::RenderedPolygonOnSphere &rendered_polygon_on_sphere);
	
		
		virtual
		void
		visit_rendered_polyline_on_sphere(
				const GPlatesViewOperations::RenderedPolylineOnSphere &rendered_polyline_on_sphere);


	private:


		// This operator should never be defined, because we don't want to allow
		// copy-assignment.
		MapCanvasPainter &
		operator=(
				const MapCanvasPainter &);

		// FIXME: temporary variable, remove when done.
		static int s_num_vertices_drawn;

		GPlatesQtWidgets::MapCanvas *d_canvas_ptr;

		GPlatesViewOperations::RenderedGeometryCollection::main_layers_update_type d_main_rendered_layers_to_visit;

		static const float POINT_SIZE_ADJUSTMENT;
		static const float LINE_SIZE_ADJUSTMENT;
	};

}

#endif  // GPLATES_GUI_MAPCANVASPAINTER_H
