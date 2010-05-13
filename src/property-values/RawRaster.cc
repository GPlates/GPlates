/* $Id$ */

/**
 * \file 
 * Contains the implementation of the RawRaster class.
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

#include "RawRaster.h"

namespace GPlatesPropertyValues
{
	namespace RasterDataType
	{
		template<>
		Type
		from_type<boost::int8_t>()
		{
			return INT_8;
		}

		template<>
		Type
		from_type<boost::uint8_t>()
		{
			return UINT_8;
		}

		template<>
		Type
		from_type<boost::int16_t>()
		{
			return INT_16;
		}

		template<>
		Type
		from_type<boost::uint16_t>()
		{
			return UINT_16;
		}

		template<>
		Type
		from_type<boost::int32_t>()
		{
			return INT_32;
		}

		template<>
		Type
		from_type<boost::uint32_t>()
		{
			return UINT_32;
		}

		template<>
		Type
		from_type<float>()
		{
			return FLOAT;
		}

		template<>
		Type
		from_type<double>()
		{
			return DOUBLE;
		}
	}
}


size_t
GPlatesPropertyValues::RasterDataType::size_of(
		Type data_type)
{
	switch (data_type)
	{
		case INT_8:
			return sizeof(boost::int8_t);

		case UINT_8:
			return sizeof(boost::uint8_t);

		case INT_16:
			return sizeof(boost::int16_t);

		case UINT_16:
			return sizeof(boost::uint16_t);

		case INT_32:
			return sizeof(boost::int32_t);

		case UINT_32:
			return sizeof(boost::uint32_t);

		case FLOAT:
			return sizeof(float);

		case DOUBLE:
			return sizeof(double);

		default:
			return 0;
	}
}

