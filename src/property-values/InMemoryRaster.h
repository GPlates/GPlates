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
#include <boost/noncopyable.hpp>


namespace GPlatesPropertyValues
{
	/**
	 * An abstract base class for a raster which has been loaded into memory.
	 *
	 * There are a variety of ways in which a raster might be stored; derivations of this base
	 * will be used to implement different storage methods.
	 */
	class InMemoryRaster :
			public boost::noncopyable
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
		 * Generate a texture using the data pointed to by unsigned_byte_type *data.
		 */
		virtual
		void
		generate_raster(
				unsigned_byte_type *data,
				QSize &size,
				ColourFormat format) = 0;

		/**
		 * Sets the value of the boolean d_enabled, which determines whether or not the texture
		 * is displayed.
		 */
		virtual
		void
		set_enabled(
			bool enabled) = 0;
	};
}

#endif // GPLATES_PROPERTYVALUES_INMEMORYRASTER_H

