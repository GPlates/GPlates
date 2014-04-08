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
		get_type_as_enum<qint8>()
		{
			return INT8;
		}

		template<>
		Type
		get_type_as_enum<quint8>()
		{
			return UINT8;
		}

		template<>
		Type
		get_type_as_enum<qint16>()
		{
			return INT16;
		}

		template<>
		Type
		get_type_as_enum<quint16>()
		{
			return UINT16;
		}

		template<>
		Type
		get_type_as_enum<qint32>()
		{
			return INT32;
		}

		template<>
		Type
		get_type_as_enum<quint32>()
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


QString
GPlatesPropertyValues::RasterType::get_type_as_string(
		Type type)
{
	switch (type)
	{
	case UNINITIALISED:
		return QString("UNINITIALISED");
	case INT8:
		return QString("INT8");
	case UINT8:
		return QString("UINT8");
	case INT16:
		return QString("INT16");
	case UINT16:
		return QString("UINT16");
	case INT32:
		return QString("INT32");
	case UINT32:
		return QString("UINT32");
	case FLOAT:
		return QString("FLOAT");
	case DOUBLE:
		return QString("DOUBLE");
	case RGBA8:
		return QString("RGBA8");
	case UNKNOWN:
	default:
		return QString("UNKNOWN");
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

