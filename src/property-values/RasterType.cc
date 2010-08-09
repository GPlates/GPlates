/* $Id$ */

/**
 * \file 
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

#include "RasterType.h"


namespace GPlatesPropertyValues
{
	namespace RasterType
	{
		template<>
		Type
		get_type_as_enum<void>()
		{
			return UNINITIALISED;
		}

		template<>
		Type
		get_type_as_enum<boost::int8_t>()
		{
			return INT8;
		}

		template<>
		Type
		get_type_as_enum<boost::uint8_t>()
		{
			return UINT8;
		}

		template<>
		Type
		get_type_as_enum<boost::int16_t>()
		{
			return INT16;
		}

		template<>
		Type
		get_type_as_enum<boost::uint16_t>()
		{
			return UINT16;
		}

		template<>
		Type
		get_type_as_enum<boost::int32_t>()
		{
			return INT32;
		}

		template<>
		Type
		get_type_as_enum<boost::uint32_t>()
		{
			return UINT32;
		}

		template<>
		Type
		get_type_as_enum<float>()
		{
			return FLOAT;
		}

		template<>
		Type
		get_type_as_enum<double>()
		{
			return DOUBLE;
		}

		template<>
		Type
		get_type_as_enum<GPlatesGui::rgba8_t>()
		{
			return RGBA8;
		}
	}
}


bool
GPlatesPropertyValues::RasterType::is_signed_integer(
		Type type)
{
	return type == INT8 || type == INT16 || type == INT32;
}


bool
GPlatesPropertyValues::RasterType::is_unsigned_integer(
		Type type)
{
	return type == UINT8 || type == UINT16 || type == UINT32;
}


bool
GPlatesPropertyValues::RasterType::is_integer(
		Type type)
{
	return is_signed_integer(type) || is_unsigned_integer(type);
}


bool
GPlatesPropertyValues::RasterType::is_floating_point(
		Type type)
{
	return type == FLOAT || type == DOUBLE;
}

