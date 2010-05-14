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
#include <boost/optional.hpp>

namespace GPlatesPropertyValues
{
	// This assumes a char is 8 bits, but that's a fairly safe assumption.
	struct rgba8_t
	{
		unsigned char red;
		unsigned char green;
		unsigned char blue;
		unsigned char alpha;
	};

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
			DOUBLE,		/**< Raster stored as array of C++ doubles */

			RGBA_8		/**< Raster stored as array of rgba8_t */
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

		template<>
		Type
		from_type<rgba8_t>();
	}

	struct WrongRasterDataTypeException {  };

	struct RasterStatistics
	{
		boost::optional<double> minimum, maximum, mean, standard_deviation;
	};

	struct GeoReferenceInfo
	{
		// This requires a bit more thought, as geo-referenced rasters don't have to
		// be lat-lon aligned.
		double min_lat, max_lat, min_lon, max_lon;

		GeoReferenceInfo() :
			min_lat(-90.0),
			max_lat(90.0),
			min_lon(-180),
			max_lon(180)
		{
		}
	};

	/**
	 * RawRaster holds the raw raster data that we read out of a raster file,
	 * before it gets processed into OpenGL textures.
	 */
	class RawRaster
	{
	public:

		RawRaster(
				unsigned int width_,
				unsigned int height_,
				RasterDataType::Type data_type_,
				unsigned char *data_ = NULL) :
			d_width(width_),
			d_height(height_),
			d_data_type(data_type_),
			d_data(data_ ? data_ :
					new unsigned char[width_ * height_ * RasterDataType::size_of(data_type_)])
		{
		}

		unsigned int
		width() const
		{
			return d_width;
		}

		unsigned int
		height() const
		{
			return d_height;
		}

		RasterDataType::Type
		data_type() const
		{
			return d_data_type;
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

		RasterStatistics &
		statistics()
		{
			return d_statistics;
		}

		const RasterStatistics &
		statistics() const
		{
			return d_statistics;
		}

		GeoReferenceInfo &
		geo_reference_info()
		{
			return d_geo_reference_info;
		}

		const GeoReferenceInfo &
		geo_reference_info() const
		{
			return d_geo_reference_info;
		}
	
	private:

		unsigned int d_width, d_height;
		RasterDataType::Type d_data_type;
		boost::scoped_array<unsigned char> d_data;
		RasterStatistics d_statistics;
		GeoReferenceInfo d_geo_reference_info;
	};
}

#endif  // GPLATES_PROPERTYVALUES_RAWRASTER_H
