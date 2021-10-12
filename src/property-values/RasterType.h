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

#ifndef GPLATES_PROPERTYVALUES_RASTERTYPE_H
#define GPLATES_PROPERTYVALUES_RASTERTYPE_H

#include <boost/cstdint.hpp>

#include "gui/Colour.h"


namespace GPlatesPropertyValues
{
	namespace RasterType
	{
		/**
		 * An enumeration of data types that can be found in rasters.
		 */
		enum Type
		{
			UNINITIALISED,
			INT8,
			UINT8,
			INT16,
			UINT16,
			INT32,
			UINT32,
			FLOAT,
			DOUBLE,
			RGBA8,
			UNKNOWN
		};

		template<typename T>
		Type
		get_type_as_enum()
		{
			return UNKNOWN;
		}

		template<>
		Type
		get_type_as_enum<void>();

		template<>
		Type
		get_type_as_enum<boost::int8_t>();

		template<>
		Type
		get_type_as_enum<boost::uint8_t>();

		template<>
		Type
		get_type_as_enum<boost::int16_t>();

		template<>
		Type
		get_type_as_enum<boost::uint16_t>();

		template<>
		Type
		get_type_as_enum<boost::int32_t>();

		template<>
		Type
		get_type_as_enum<boost::uint32_t>();

		template<>
		Type
		get_type_as_enum<float>();

		template<>
		Type
		get_type_as_enum<double>();

		template<>
		Type
		get_type_as_enum<GPlatesGui::rgba8_t>();

		bool
		is_signed_integer(
				Type type);

		bool
		is_unsigned_integer(
				Type type);

		bool
		is_integer(
				Type type);

		bool
		is_floating_point(
				Type type);
	}
}

#endif  // GPLATES_PROPERTYVALUES_RASTERTYPE_H
