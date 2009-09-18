/* $Id$ */

/**
 * @file 
 * File specific comments.
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

#ifndef GPLATES_GUI_GLOBE_H
#define GPLATES_GUI_GLOBE_H

#include "Colour.h"
#include "GlobeRenderedGeometryCollectionPainter.h"
#include "NurbsRenderer.h"
#include "OpaqueSphere.h"
#include "SphericalGrid.h"
#include "Texture.h"
#include "SimpleGlobeOrientation.h"
#include "TextRenderer.h"
#include "RenderSettings.h"
#include "maths/UnitVector3D.h"
#include "maths/PointOnSphere.h"
#include "maths/Rotation.h"
#include "utils/VirtualProxy.h"

#include <boost/shared_ptr.hpp>

namespace GPlatesViewOperations
{
	class RenderedGeometryCollection;
}

namespace GPlatesGui
{
	class Globe
	{
	public:
		Globe(
				GPlatesViewOperations::RenderedGeometryCollection &,
				boost::shared_ptr<TextRenderer> text_renderer_ptr);

		~Globe()
		{  }

		SimpleGlobeOrientation &
		orientation()
		{
			return d_globe_orientation;
		}

		// currently does nothing.
		void
		SetTransparency(
				bool trans = true)
		{  }

		void
		SetNewHandlePos(
				const GPlatesMaths::PointOnSphere &pos);

		void
		UpdateHandlePos(
				const GPlatesMaths::PointOnSphere &pos);

		const GPlatesMaths::PointOnSphere
		Orient(
				const GPlatesMaths::PointOnSphere &pos);

		/**
		 * Paint the globe and all the visible features and rasters on it.
		 *
		 * @param viewport_zoom_factor The magnification of the globe in the viewport window.
		 *        Value should be one when earth fills viewport and proportionately greater
		 *        than one when viewport shows only part of the globe.
		 */
		void paint(
				const double &viewport_zoom_factor);
		
		/*
		 * A special version of the globe's paint() method more suitable
		 * for vector output
		 *
		 * @param viewport_zoom_factor The magnification of the globe in the viewport window.
		 *        Value should be one when earth fills viewport and proportionately greater
		 *        than one when viewport shows only part of the globe.
		 */
		void paint_vector_output(
				const double &viewport_zoom_factor);

		void
		toggle_raster_image();

		void
		enable_raster_display();

		void
		disable_raster_display();

		Texture &
		texture()
		{
			return *d_texture;
		}

		/** 
		 * Functions to change display state variables
		 */ 
		void enable_point_display()			{ d_render_settings.show_points = true; }
		void enable_line_display()			{ d_render_settings.show_lines = true; }
		void enable_polygon_display() 		{ d_render_settings.show_polygons = true; }
		void enable_topology_display() 		{ d_render_settings.show_topology = true; }
		void enable_multipoint_display()	{ d_render_settings.show_multipoints = true; }
		void enable_arrows_display()		{ d_render_settings.show_arrows = true; }
		void enable_strings_display()		{ d_render_settings.show_strings = true; }

		void disable_point_display() 		{ d_render_settings.show_points = false; }
		void disable_line_display() 		{ d_render_settings.show_lines = false; }
		void disable_polygon_display() 		{ d_render_settings.show_polygons = false; }
		void disable_topology_display() 	{ d_render_settings.show_topology = false; }
		void disable_multipoint_display()	{ d_render_settings.show_multipoints = false; }
		void disable_arrows_display()		{ d_render_settings.show_arrows = false; }
		void diable_strings_display()		{ d_render_settings.show_strings = false; }

		void toggle_point_display()			{ d_render_settings.show_points = !d_render_settings.show_points; }
		void toggle_line_display() 			{ d_render_settings.show_lines = !d_render_settings.show_lines; }
		void toggle_polygon_display() 		{ d_render_settings.show_polygons = !d_render_settings.show_polygons; }
		void toggle_topology_display() 		{ d_render_settings.show_topology = !d_render_settings.show_topology; }
		void toggle_multipoint_display()	{ d_render_settings.show_multipoints = !d_render_settings.show_multipoints; }
		void toggle_arrows_display()		{ d_render_settings.show_arrows = !d_render_settings.show_arrows; }
		void toggle_strings_display()		{ d_render_settings.show_strings = !d_render_settings.show_strings; }

		bool point_display_is_enabled()		{ return d_render_settings.show_points; }
		bool line_display_is_enabled()		{ return d_render_settings.show_lines; }		
		bool polygon_display_is_enabled()		{ return d_render_settings.show_polygons; }
		bool topology_display_is_enabled()		{ return d_render_settings.show_topology; }
		bool multipoint_display_is_enabled()	{ return d_render_settings.show_multipoints; }
		bool arrows_display_is_enabled()		{ return d_render_settings.show_arrows; }
		bool strings_display_is_enabled()		{ return d_render_settings.show_strings; }


	private:
			
		/**
		* Flags to determine what data to show
		*/
		RenderSettings d_render_settings;
			// FIXME: be sure to synchronise default RenderSettings with ViewportWidgetUi.ui
		
		/**
		 * The collection of @a RenderedGeometry objects we need to paint.
		 */
		GPlatesViewOperations::RenderedGeometryCollection *d_rendered_geom_collection;

		/**
		 * The NurbsRenderer used to draw large GreatCircleArcs.
		 * Delay creation until it's used.
		 */
		GPlatesUtils::VirtualProxy<NurbsRenderer> d_nurbs_renderer;

		// Factory used by GPlatesUtils::VirtualProxy to create OpaqueSphere.
		class OpaqueSphereFactory
		{
		public:
			OpaqueSphereFactory(const Colour& colour) : d_colour(colour) { }
			OpaqueSphere* create() const  { return new OpaqueSphere(d_colour); }
		private:
			Colour d_colour;
		};
		/**
		 * The solid earth.
		 */
		GPlatesUtils::VirtualProxy<OpaqueSphere, OpaqueSphereFactory> d_sphere;

		/**
		 * A (single) texture to be texture-mapped over the sphere surface.
		 * Delay creation until it's used.
		 */
		GPlatesUtils::VirtualProxy<Texture> d_texture;

		/**
		 * Lines of lat and lon on surface of earth.
		 */
		SphericalGrid d_grid;

		/**
		 * The accumulated orientation of the globe.
		 */
		SimpleGlobeOrientation d_globe_orientation;

		/**
		 * Painter used to draw @a RenderedGeometry objects on the globe.
		 */
		GlobeRenderedGeometryCollectionPainter d_rendered_geom_collection_painter;

		/**
		 * One circle of latitude every 30 degrees.
		 */
		static const unsigned NUM_CIRCLES_LAT = 5;

		/**
		 * One circle of longitude every 30 degrees.
		 */
		static const unsigned NUM_CIRCLES_LON = 6;

	};
}

#endif  // GPLATES_GUI_GLOBE_H
