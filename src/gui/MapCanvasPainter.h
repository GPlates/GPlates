/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
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
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

#include "RenderSettings.h"
#include "TextRenderer.h"
#include "view-operations/RenderedGeometry.h"
#include "view-operations/RenderedGeometryCollection.h"
#include "view-operations/RenderedGeometryCollectionVisitor.h"


namespace GPlatesGui
{
	class Colour;
	class ColourScheme;
	class Map;

	/**
	 * This is a Visitor to paint geometries on the map canvas.
	 */
	class MapCanvasPainter:
			public GPlatesViewOperations::ConstRenderedGeometryCollectionVisitor,
			public boost::noncopyable
	{
	public:
		explicit
		MapCanvasPainter(
				Map &map,
				GPlatesGui::RenderSettings &render_settings,
				GPlatesGui::TextRenderer::ptr_to_const_type text_renderer_ptr,
				GPlatesViewOperations::RenderedGeometryCollection::main_layers_update_type &layers_to_visit,
				const double &inverse_zoom_factor,
				boost::shared_ptr<ColourScheme> colour_scheme):
			d_map(map),
			d_render_settings(render_settings),
			d_text_renderer_ptr(text_renderer_ptr),
			d_main_rendered_layers_to_visit(layers_to_visit),
			d_inverse_zoom_factor(inverse_zoom_factor),
			d_colour_scheme(colour_scheme),
			d_scale(1.0f)
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
		visit_rendered_direction_arrow(
				const GPlatesViewOperations::RenderedDirectionArrow &rendered_direction_arrow);

		virtual
		void
		visit_rendered_ellipse(
				const GPlatesViewOperations::RenderedEllipse &rendered_ellipse);

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

		virtual
		void
		visit_rendered_small_circle(
				const GPlatesViewOperations::RenderedSmallCircle &rendered_small_circle);

		virtual
		void
		visit_rendered_small_circle_arc(
			const GPlatesViewOperations::RenderedSmallCircleArc &rendered_small_circle_arc);

		virtual
		void
		visit_rendered_string(
				const GPlatesViewOperations::RenderedString &rendered_string);

		void
		set_scale(
				float scale)
		{
			d_scale = scale;
		}

	private:

		Map &d_map;

		//! Rendering flags for determining what gets shown
		RenderSettings &d_render_settings;

		//! For rendering text
		TextRenderer::ptr_to_const_type d_text_renderer_ptr;

		GPlatesViewOperations::RenderedGeometryCollection::main_layers_update_type d_main_rendered_layers_to_visit;

		const double d_inverse_zoom_factor;

		//! For assigning colours to RenderedGeometry
		boost::shared_ptr<ColourScheme> d_colour_scheme;

		//! When rendering maps that are meant to be a scaled version of another
		float d_scale;

		//! For hard-coded tweaking of the size of points
		static const float POINT_SIZE_ADJUSTMENT;
		
		//! For hard-coded tweaking of the width of lines
		static const float LINE_WIDTH_ADJUSTMENT;

	};

}

#endif  // GPLATES_GUI_MAPCANVASPAINTER_H
