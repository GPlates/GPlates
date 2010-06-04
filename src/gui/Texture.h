/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
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

#ifndef GPLATES_GUI_TEXTURE_H
#define GPLATES_GUI_TEXTURE_H

#include <QtOpenGL/qgl.h>
#include <QString>
#include <QObject>

#include "property-values/InMemoryRaster.h"

#include "utils/VirtualProxy.h"


namespace GPlatesGui
{
	/**
	 *	NUM_STRIPS_S and NUM_STRIPS_T are the number of strips that the texure is divided up into.
	 *  The edges of these strips define coordinates in texture space (s,t). 
	 *  Each of these coordinates will be mapped to a 3D coordinate on the sphere surface. 
	 * 
	 *  NUM_STRIPS_S is the number of strips in the texture's "s" direction (i.e. our longitude).
	 *  NUM_STRIPS_T is the number of strips in the texture's "t" direction (i.e. latitude). 
	 */

	const int NUM_STRIPS_S = 64;
	const int NUM_STRIPS_T = 32;

	const float LON_START = -180.0f;
	const float LON_END = 180.0f;
	const float LAT_START = -90.0f;
	const float LAT_END = 90.0f;

	const QRectF INITIAL_EXTENT(-180.,90.,360.,-180.);

	struct texture_vertex
	{
		float s;
		float t;
	};

	struct sphere_vertex
	{
		float x;
		float y;
		float z;
	};

	class Texture :
			public QObject,
			public GPlatesPropertyValues::InMemoryRaster
	{
		Q_OBJECT

	public:

		Texture() :
			d_image_data(0),
			d_image_width(0),
			d_image_height(0),
			d_texture_name(0),
			d_enabled(false),
			d_extent(INITIAL_EXTENT),
			d_should_be_remapped(false),
			d_is_loaded(false)
		{ }

		~Texture();

		/**
		 * Generates a texture using the data pointed to by unsigned_byte_type *data.
		 */
		void
		generate_raster(
				unsigned_byte_type *data,
				QSize &size,
				ColourFormat format);

		/**
		 * Generates a checker-board pattern in memory, and creates a texture from this.
		 */
		void
		generate_test_texture();


		/**
		 * Binds the texture so that it will be mapped to the sphere quadric.
		 */
		void
		paint();

		/**
		 * Returns the openGL texture name.
		 */
		GLuint
		get_texture_name();

		/**
		 * Returns the value of d_enabled, which determines whether or not the texture
		 * is displayed. 
		 */ 
		bool
		is_enabled()
		{
			return d_enabled;
		}

		/**
		 * Sets the value of the boolean d_enabled, which determines whether or not the texture
		 * is displayed.
		 */
		void
		set_enabled(
				bool enabled);
	
		/**
		 * Toggles the state of the boolean d_enabled, which determines whether or not the texture
		 * is displayed.
		 */
		void
		toggle();

		/**
		 * Set the coordinate range over which the texture will be mapped.
		 */
		void
		set_extent(
				const QRectF &rect);

		/**
		 * Return the coordinate range over which the texture is mapped.
		 */ 
		const QRectF &
		get_extent()
		{
			return d_extent;
		}

		bool
		is_loaded()
		{
			return d_is_loaded;
		}

	signals:

		//! Emitted when the raster is loaded or changed, and when the texture is enabled/disabled
		void
		texture_changed();

	private:

		/** 
		 * Maps 2D points in the texture coordinate system to 3D vertices in the sphere coordinate system. 
		 */
		void
		generate_mapping_coordinates();

		/** 
		 * Maps 2D points in the texture coordinate system to 3D vertices in the sphere coordinate system,
		 * over a region with corners given by x_start, y_start, x_end, y_end.
		 */
		void
		generate_mapping_coordinates(
				QRectF &extent);

		/**
		 * Re-maps an existing texture to the portion of the sphere surface given by extent.
		 */
		void
		remap_texture();

		/**
		 * A pointer to image data. 
		 */
		unsigned_byte_type *d_image_data;

		/**
		 * The width of the image.
		 */ 
		int d_image_width;

		/**
		 * The height of the image.
		 */
		int d_image_height;

#if 0
		/**
		 * The width of the texture, which should be a power of 2.
		 */
		int d_texture_width;

		/**
		 * The height of the texture, which should be a power of 2. 
		 */
		int d_texture_height;
#endif
		/**
		 * The openGL texture name.
		 */
		GLuint d_texture_name;

		/**
		 * Whether or not the texture is displayed.
		 */
		bool d_enabled;

		/**
		 * The vertices used for mapping the texture to the sphere. 
		 */
		texture_vertex d_texture_vertices[NUM_STRIPS_S + 1][NUM_STRIPS_T + 1];
		sphere_vertex d_sphere_vertices[NUM_STRIPS_S + 1][NUM_STRIPS_T + 1];

		/**
		 * The lat-lon extent over which the texture should be mapped.
		 */
		QRectF d_extent;
		
		/**
		 * Whether or not the texture needs to be remapped.                                                                     
		 */
		bool d_should_be_remapped;

		/**
		 * Whether a raster has been loaded.
		 */
		bool d_is_loaded;
	};


	/**
	 * Delays creation of the texture until first use.
	 */
	typedef GPlatesUtils::VirtualProxy<Texture> ProxiedTexture;
	
}

#endif // GPLATES_GUI_TEXTURE_H

