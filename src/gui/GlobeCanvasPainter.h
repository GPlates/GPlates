/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
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

#ifndef GPLATES_GUI_GLOBECANVASPAINTER_H
#define GPLATES_GUI_GLOBECANVASPAINTER_H

#include "maths/ConstGeometryOnSphereVisitor.h"
#include "qt-widgets/GlobeCanvas.h"
#include "PlatesColourTable.h"


namespace GPlatesGui
{
	/**
	 * This is a Visitor to paint geometries on the globe canvas.
	 */
	class GlobeCanvasPainter:
			public GPlatesMaths::ConstGeometryOnSphereVisitor
	{
	public:
		GlobeCanvasPainter(
				GPlatesQtWidgets::GlobeCanvas &canvas_,
				PlatesColourTable::const_iterator colour_):
			d_canvas_ptr(&canvas_),
			d_colour(colour_)
		{  }

		virtual
		~GlobeCanvasPainter()
		{  }

		// Please keep these geometries ordered alphabetically.

		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit_point_on_sphere(
				GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere)
		{
			d_canvas_ptr->draw_point(point_on_sphere, d_colour);
		}

		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit_polygon_on_sphere(
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
		{
			// FIXME:  We need a 'draw_polygon' function, which will additionally need
			// to handle filled polygons.
		}

		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit_polyline_on_sphere(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
		{
			d_canvas_ptr->draw_polyline(polyline_on_sphere, d_colour);
		}

	private:
		GPlatesQtWidgets::GlobeCanvas *d_canvas_ptr;
		PlatesColourTable::const_iterator d_colour;

		// This operator should never be defined, because we don't want to allow
		// copy-assignment.
		GlobeCanvasPainter &
		operator=(
				const GlobeCanvasPainter &);
	};

}

#endif  // GPLATES_GUI_GLOBECANVASPAINTER_H
