/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, Geological Survey of Norway
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
#include "property-values/InMemoryRaster.h"





namespace GPlatesGui {

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


	struct raster_data
	{
		std::vector<GLubyte> &data;
		int width;
		int height;
		GLuint format;
	} ;

	class Texture:
			public GPlatesPropertyValues::InMemoryRaster
	{
	public:
		Texture():
			d_image_data(0),
			d_image_width(0),
			d_image_height(0),
			d_texture_width(0),
			d_texture_height(0),
			d_texture_name(0),
			d_enabled(false),
			d_min(0),
			d_max(0),
			d_corresponds_to_data(false)
			{}

		~Texture();


		/**
		 * Generates a texture using the data contained in a std::vector<unsinged_byte_type>.
		 * This is used for data loaded via GDAL. 
		 */
		void
		generate_raster(
			std::vector<unsigned_byte_type> &data,
			int width,
			int height,
			ColourFormat format);

		/**
		 * Generates a texture using the data contained in a std::vector<unsinged_byte_type>, and
		 * which will be mapped to the coordinates given by x_start, y_start, x_end, y_end. 
		 * This is used for data loaded via GDAL.
		 */
		void
		generate_raster(
			std::vector<unsigned_byte_type> &data,
			float x_start,
			float y_start,
			float x_end,
			float y_end,
			int width,
			int height,
			ColourFormat format);

		/**
		 * Generates a texture using the data pointed to by unsigned_byte_type *data.
		 * This is used for images loaded via Qt's QImage class.
		 */
		void
		generate_raster(
			unsigned_byte_type *data,
			int width,
			int height,
			ColourFormat format);


		/**
		 * Generates a checker-board pattern in memory, and creates a texture from this.
		 */
		void
		generate_test_texture();

		/**
		 * Loads a 24-bit TGA file and creates a texture. Assumes the loaded image represents 
		 * the entire globe, and that the leftmost edge of the image is -180 degrees longitude. 
		 */
		void
		generate_test_texture2();

		/**
		 * Loads a 24-bit TGA file and generates a texture from it. Any existing texture will
		 * be replaced by the new texture.
		 */
		void
		load_TGA_file(QString &filename);

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
		is_enabled(){
			return d_enabled;
		}

		/**
		 * This returns true for an image which was generated from scientific data 
		 * (as opposed to RGB/image data), and which will therefore require a colour legend for
		 * interpretation of the colour display. 
		 */
		bool
		corresponds_to_data(){
			return d_corresponds_to_data;
		}

		/**
		 * Sets the value of the boolean d_enabled, which determines whether or not the texture
		 * is displayed.
		 */
		void
		set_enabled(
			bool enabled){
			d_enabled = enabled;
		}
	
		/**
		 * Toggles the state of the boolean d_enabled, which determines whether or not the texture
		 * is displayed.
		 */
		void
		toggle();

		/**
		 * For a raster representing "scientific" data as opposed to an RGB image, this
		 * represents the minimum of the range of values used to generate colour values.
		 */
		float
		get_min(){
			return d_min;
		}

		/**
		 * For a raster representing "scientific" data as opposed to an RGB image, this
		 * represents the maximum of the range of values used to generate colour values.
		 */
		float
		get_max(){
				return d_max;
		}

		/**
		 * Sets the value of d_min, which contains the lowest data value used in 
		 * generating the colour scale. Note that this is *not* necessarily the minimum value 
		 * in the original data set. 
		 */ 
		void
		set_min(
			float min){
			d_min = min;
		}

		/**
		 * Sets the value of d_max, which contains the highest data value used in 
		 * generating the colour scale. Note that this is *not* necessarily the maximum value 
		 * in the original data set. 
		 */ 
		void
		set_max(
			float max){
			d_max = max;
		}

		/**
		 * Sets the value of the boolean d_corresponds_to_data. This should be set to true for 
		 * an image which was generated from scientific data 
		 * (as opposed to RGB/image data), and which will therefore require a colour legend for
		 * interpretation of the colour display.
		 */
		void
		set_corresponds_to_data(
			bool data){
			d_corresponds_to_data = data;
		}

	private:

		// Make copy and assignment private to prevent copying/assignment
		Texture(
			const Texture &other);

		Texture &
		operator=(
			const Texture &other);

		/** 
		 * Maps 2D points in the texture coordinate system to 3D vertices on the sphere coordinate system. 
		 */
		void
		generate_mapping_coordinates();

		/** 
		 * Maps 2D points in the texture coordinate system to 3D vertices on the sphere coordinate system,
		 * over a region with corners given by x_start, y_start, x_end, y_end.
		 */
		void
		generate_mapping_coordinates(
			float x_start,
			float y_start,
			float x_end,
			float y_end);

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

		/**
		 * The width of the texture, which should be a power of 2.
		 */
		int d_texture_width;

		/**
		 * The height of the texture, which should be a power of 2. 
		 */
		int d_texture_height;

		/**
		 * The openGL texture name.
		 */
		GLuint d_texture_name;

		/**
		 * Whether or not the texture is displayed.
		 */
		bool d_enabled;

		/**
		 * The minimum data value used in 
		 * generating the colour scale. Note that this is *not* necessarily the minimum value 
		 * in the original data set. 
		 */ 
		float d_min;

		/**
		 * The maximum data value used in 
		 * generating the colour scale. Note that this is *not* necessarily the maximum value 
		 * in the original data set. 
		 */ 
		float d_max;

		/**
		 * This should be true for an image which was generated from scientific data 
		 * (as opposed to RGB/image data), and which will therefore require a colour legend for
		 * interpretation of the colour display. 
		 */
		bool d_corresponds_to_data;

		/**
		 * The vertices used for mapping the texture to the sphere. 
		 */
		texture_vertex d_texture_vertices[NUM_STRIPS_S + 1][NUM_STRIPS_T + 1];
		sphere_vertex d_sphere_vertices[NUM_STRIPS_S + 1][NUM_STRIPS_T + 1];

	};

}

#endif // GPLATES_GUI_TEXTURE_H

