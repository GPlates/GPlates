/* $Id$ */

/**
 * \file 
 * File specific comments.
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

#include "MipmappedRasterFormat.h"


namespace GPlatesFileIO
{
	namespace MipmappedRasterFormat
	{
		template<>
		Type
		get_type_as_enum<GPlatesGui::rgba8_t>()
		{
			return RGBA;
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
	}
}

