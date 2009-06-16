/* $Id$ */

/**
 * \file Draws @a RenderedGeometry objects onto the globe (orthographic view).
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

#ifndef GPLATES_GUI_GLOBERENDEREDGEOMETRYPAINTER_H
#define GPLATES_GUI_GLOBERENDEREDGEOMETRYPAINTER_H

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include "GlobeRenderedGeometryLayerPainter.h"

#include "view-operations/RenderedGeometryCollectionVisitor.h"


namespace GPlatesViewOperations
{
	class RenderedGeometryCollection;
	class RenderedGeometryLayer;
}

namespace GPlatesGui
{
	// FIXME: remove all reference to Globe.
	class Globe;

	class GlobeRenderedGeometryLayerPainter;
	class NurbsRenderer;

	/**
	 * Draws rendered geometries (in a @a RenderedGeometryCollection) onto a
	 * 3d orthographic view of the globe using OpenGL.
	 */
	class GlobeRenderedGeometryCollectionPainter :
			private GPlatesViewOperations::ConstRenderedGeometryCollectionVisitor,
			private boost::noncopyable
	{
	public:
		GlobeRenderedGeometryCollectionPainter(
				const GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
				// FIXME: Remove globe hack.
				GPlatesGui::Globe *globe);


		/**
		 * Draw the rendered geometries into the depth range specified by
		 * @a depth_range_near and @a depth_range_far.
		 * The full depth range is [0,1].
		 * @a viewport_zoom_factor is used for rendering view-dependent geometries.
		 */
		void
		paint(
				const double &viewport_zoom_factor,
				GPlatesGui::NurbsRenderer &nurbs_renderer,
				double depth_range_near,
				double depth_range_far);

	private:
		virtual
		bool
		visit_rendered_geometry_layer(
				const GPlatesViewOperations::RenderedGeometryLayer &rendered_geometry_layer);

	private:
		const GPlatesViewOperations::RenderedGeometryCollection &d_rendered_geometry_collection;
		double d_current_layer_far_depth;
		double d_depth_range_per_layer;

		// FIXME: Remove this hack.
		GPlatesGui::Globe *const d_globe;

		//! Parameters that are only available when @a paint is called.
		struct PaintParams
		{
			PaintParams(
					const double &viewport_zoom_factor,
					GPlatesGui::NurbsRenderer &nurbs_renderer) :
				d_inverse_viewport_zoom_factor(1.0 / viewport_zoom_factor),
				d_nurbs_renderer(&nurbs_renderer)
			{  }

			double d_inverse_viewport_zoom_factor;
			GPlatesGui::NurbsRenderer *d_nurbs_renderer;
		};
		boost::optional<PaintParams> d_paint_params;


		/**
		 * Sets the depth range for the next rendered layer.
		 */
		void
		move_to_next_rendered_layer_depth_range_and_set();
	};
}

#endif // GPLATES_GUI_GLOBERENDEREDGEOMETRYPAINTER_H
