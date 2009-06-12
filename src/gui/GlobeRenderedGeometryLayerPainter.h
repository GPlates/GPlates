/* $Id$ */

/**
 * \file Draws rendered geometries in a specific @a RenderedGeometryLayer onto 3d orthographic
 * globe using OpenGL.
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

#ifndef GPLATES_GUI_GLOBERENDEREDGEOMETRYLAYERPAINTER_H
#define GPLATES_GUI_GLOBERENDEREDGEOMETRYLAYERPAINTER_H

#include <boost/noncopyable.hpp>

#include "view-operations/RenderedGeometry.h"
#include "view-operations/RenderedGeometryVisitor.h"


namespace GPlatesViewOperations
{
	class RenderedGeometryLayer;
}

namespace GPlatesGui
{
	class NurbsRenderer;

	/**
	 * Handles drawing rendered geometries in a single layer by drawing the
	 * opaque primitives first followed by the transparent primitives.
	 */
	class GlobeRenderedGeometryLayerPainter :
			public GPlatesViewOperations::ConstRenderedGeometryVisitor,
			private boost::noncopyable
	{
	public:
		GlobeRenderedGeometryLayerPainter(
				const GPlatesViewOperations::RenderedGeometryLayer &rendered_geometry_layer,
				const double &inverse_viewport_zoom_factor,
				GPlatesGui::NurbsRenderer &nurbs_renderer) :
			d_rendered_geometry_layer(rendered_geometry_layer),
			d_nurbs_renderer(&nurbs_renderer),
			d_inverse_zoom_factor(inverse_viewport_zoom_factor),
			d_draw_opaque_primitives(true)
		{  }


		/**
		 * Draws the sequence of rendered geometries passed into constructor.
		 * NOTE: on return the state of 'glDepthMask' is GL_FALSE.
		 */
		void
		paint();

	private:
		virtual
		void
		visit_rendered_point_on_sphere(
				const GPlatesViewOperations::RenderedPointOnSphere &rendered_point_on_sphere);

		virtual
		void
		visit_rendered_multi_point_on_sphere(
				const GPlatesViewOperations::RenderedMultiPointOnSphere &rendered_multi_point_on_sphere);

		virtual
		void
		visit_rendered_polyline_on_sphere(
				const GPlatesViewOperations::RenderedPolylineOnSphere &rendered_polyline_on_sphere);

		virtual
		void
		visit_rendered_polygon_on_sphere(
				const GPlatesViewOperations::RenderedPolygonOnSphere &rendered_polygon_on_sphere);

		virtual
		void
		visit_rendered_direction_arrow(
				const GPlatesViewOperations::RenderedDirectionArrow &rendered_direction_arrow);

	private:
		const GPlatesViewOperations::RenderedGeometryLayer &d_rendered_geometry_layer;
		GPlatesGui::NurbsRenderer *const d_nurbs_renderer;
		const double d_inverse_zoom_factor;

		//! Is this the first pass (drawing opaque primitives) or second pass (transparent)?
		bool d_draw_opaque_primitives;

		//! Multiplying factor to get point size of 1.0f to look like one screen-space pixel.
		static const float POINT_SIZE_ADJUSTMENT;
		//! Multiplying factor to get line width of 1.0f to look like one screen-space pixel.
		static const float LINE_WIDTH_ADJUSTMENT;


		/**
		 * Visit each rendered geometry in our sequence.
		 */
		void
		visit_rendered_geoms();
	};
}

#endif // GPLATES_GUI_GLOBERENDEREDGEOMETRYLAYERPAINTER_H
