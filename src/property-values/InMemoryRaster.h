/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008 Geological Survey of Norway
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

#ifndef GPLATES_PROPERTYVALUES_INMEMORYRASTER_H
#define GPLATES_PROPERTYVALUES_INMEMORYRASTER_H

#include <vector>


namespace GPlatesPropertyValues
{
	/**
	 * An abstract base class for a raster which has been loaded into memory.
	 *
	 * There are a variety of ways in which a raster might be stored; derivations of this base
	 * will be used to implement different storage methods.
	 */
	class InMemoryRaster
	{
	public:
		typedef unsigned char unsigned_byte_type;

		enum ColourFormat
		{
			RgbFormat,
			RgbaFormat
		};


		InMemoryRaster()
		{  }


		virtual
		~InMemoryRaster()
		{  }

		/**
		 * Generate a texture using the data contained in a std::vector<unsinged_byte_type>.
		 */
		virtual
		void
		generate_raster(
				std::vector<unsigned_byte_type> &data,
				QSize &size,
				ColourFormat format) = 0;


		/**
		 * Generate a texture using the data pointed to by unsigned_byte_type *data.
		 */
		virtual
		void
		generate_raster(
				unsigned_byte_type *data,
				QSize &size,
				ColourFormat format) = 0;

		/**
		 * For a raster representing "scientific" data as opposed to an RGB image, this
		 * represents the minimum of the range of values used to generate colour values.
		 */
		virtual
		float
		get_min() = 0;


		/**
		 * For a raster representing "scientific" data as opposed to an RGB image, this
		 * represents the maximum of the range of values used to generate colour values.
		 */
		virtual
		float
		get_max() = 0;


		/**
		 * This returns true for an image which was generated from scientific data 
		 * (as opposed to RGB/image data), and which will therefore require a colour legend for
		 * interpretation of the colour display. 
		 */
		virtual
		bool
		corresponds_to_data() = 0;

		/**
		 * Sets the value of d_min, which contains the lowest data value used in 
		 * generating the colour scale. Note that this is *not* necessarily the minimum value 
		 * in the original data set. 
		 */ 
		virtual
		void
		set_min(
				float min) = 0;

		/**
		 * Sets the value of d_max, which contains the highest data value used in 
		 * generating the colour scale. Note that this is *not* necessarily the maximum value 
		 * in the original data set. 
		 */ 
		virtual
		void
		set_max(
				float max) = 0;


		/**
		 * Sets the value of the boolean d_corresponds_to_data. This should be set to true for 
		 * an image which was generated from scientific data 
		 * (as opposed to RGB/image data), and which will therefore require a colour legend for
		 * interpretation of the colour display.
		 */
		virtual
		void
		set_corresponds_to_data(
				bool corresponds_to_data) = 0;

		/**
		 * Sets the value of the boolean d_enabled, which determines whether or not the texture
		 * is displayed.
		 */
		virtual
		void
		set_enabled(
			bool enabled) = 0;

	private:

		// Make copy and assignment private to prevent copying/assignment
		InMemoryRaster(
			const InMemoryRaster &other);

		InMemoryRaster &
		operator=(
			const InMemoryRaster &other);

	};
}

#endif // GPLATES_PROPERTYVALUES_INMEMORYRASTER_H

