/* $Id$ */

/**
 * \file 
 * Contains the definition of the RawRaster class.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

#ifndef GPLATES_PROPERTYVALUES_RAWRASTER_H
#define GPLATES_PROPERTYVALUES_RAWRASTER_H

#include <boost/cstdint.hpp>
#include <boost/scoped_array.hpp>

namespace GPlatesPropertyValues
{
	// A note on types:
	//  - The exact-width integer types are said to be optional, but they should be
	//    present on all platforms that we're interested in. See the documentation
	//    for boost/cstdint.hpp for more information.
	//  - GDAL defines two data types, a 32-bit and a 64-bit float. Although there
	//    is no guarantee that 'float' is 32-bit and 'double' is 64-bit, that is
	//    the assumption that GDAL seems to make, and for the platforms that we are
	//    interested in, that is a fairly safe assumption.
	//  - With the FLOAT and DOUBLE types below, we don't specify the width; they
	//    mean whatever C++ says is 'float' and 'double' on a particular platform.

	namespace RasterDataType
	{
		enum Type
		{
			INT_8,		/**< Raster stored as array of boost::int8_t */
			UINT_8,		/**< Raster stored as array of boost::uint8_t */

			INT_16,		/**< Raster stored as array of boost::int16_t */
			UINT_16,	/**< Raster stored as array of boost::uint16_t */

			INT_32,		/**< Raster stored as array of boost::int32_t */
			UINT_32,	/**< Raster stored as array of boost::uint32_t */

			FLOAT,		/**< Raster stored as array of C++ floats */
			DOUBLE		/**< Raster stored as array of C++ doubles */
		};

		size_t
		size_of(
				Type data_type);

		template<typename T>
		Type
		from_type();

		template<>
		Type
		from_type<boost::int8_t>();

		template<>
		Type
		from_type<boost::uint8_t>();

		template<>
		Type
		from_type<boost::int16_t>();

		template<>
		Type
		from_type<boost::uint16_t>();

		template<>
		Type
		from_type<boost::int32_t>();

		template<>
		Type
		from_type<boost::uint32_t>();

		template<>
		Type
		from_type<float>();

		template<>
		Type
		from_type<double>();
	}

	struct WrongRasterDataTypeException {  };

	class RawRaster
	{
	public:

		RawRaster(
				int width,
				int height,
				RasterDataType::Type data_type) :
			d_data(new unsigned char[width * height * RasterDataType::size_of(data_type)])
		{
		}

		template<class T>
		T *
		get() const
		{
			if (RasterDataType::from_type<T> == d_data_type)
			{
				return static_cast<T *>(d_data);
			}
			else
			{
				throw WrongRasterDataTypeException();
			}
		}
	
	private:

		boost::scoped_array<unsigned char> d_data;
		RasterDataType::Type d_data_type;
	};
}

#endif  // GPLATES_PROPERTYVALUES_RAWRASTER_H
