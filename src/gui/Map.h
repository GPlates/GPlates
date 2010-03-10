/* $Id$ */

/**
 * @file 
 * Contains the definition of the Map class.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_MAP_H
#define GPLATES_GUI_MAP_H

#include <boost/shared_ptr.hpp>

#include "MapProjection.h"
#include "TextRenderer.h"
#include "gui/ViewportZoom.h"
#include "view-operations/RenderedGeometryCollection.h"

namespace GPlatesGui
{
	class ColourScheme;
	class RenderSettings;

	/**
	 * Holds the state for MapCanvas/MapView (analogous to the Globe class).
	 */
	class Map
	{
	public:

		Map(
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
				RenderSettings &render_settings,
				ViewportZoom &viewport_zoom,
				boost::shared_ptr<ColourScheme> colour_scheme);

		MapProjection &
		projection();

		ProjectionType
		projection_type();

		void
		set_projection_type(
				GPlatesGui::ProjectionType projection_type_);

		double
		central_meridian();

		void
		set_central_meridian(
				double central_meridian_);

		void
		set_text_renderer(
				TextRenderer::ptr_to_const_type text_renderer_ptr);

		//! Set the background colour and draw the lat-lon grid.
		void
		draw_background();

		//! Draws the contents of the RenderedGeometryLayers.
		void
		paint(
				float scale);

		void
		set_update_type(
				GPlatesViewOperations::RenderedGeometryCollection::main_layers_update_type update_type);

	private:

		//! Draw lat-lon grid lines on the canvas, at 30-degree intervals. 
		void
		draw_grid_lines();

		//! To do map projections
		MapProjection d_projection;

		//! A pointer to the state's RenderedGeometryCollection
		GPlatesViewOperations::RenderedGeometryCollection *d_rendered_geometry_collection;
		GPlatesViewOperations::RenderedGeometryCollection::main_layers_update_type d_update_type;

		//! Used for rendering text
		GPlatesGui::TextRenderer::ptr_to_const_type d_text_renderer_ptr;

		//! Flags to determine what data to show
		GPlatesGui::RenderSettings &d_render_settings;

		//! For zoom-dependent rendered objects.                                                                     
		GPlatesGui::ViewportZoom &d_viewport_zoom;		
		
		//! For giving colour to RenderedGeometry
		boost::shared_ptr<GPlatesGui::ColourScheme> d_colour_scheme;
	};
}

#endif  // GPLATES_GUI_MAP_H
