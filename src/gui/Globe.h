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
#include "maths/UnitVector3D.h"
#include "maths/PointOnSphere.h"
#include "maths/Rotation.h"
#include "utils/VirtualProxy.h"


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
				GPlatesViewOperations::RenderedGeometryCollection &);

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

	private:
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
