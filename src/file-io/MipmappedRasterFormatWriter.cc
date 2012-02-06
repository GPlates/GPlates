/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#include "MipmappedRasterFormatWriter.h"


namespace GPlatesFileIO
{
	namespace MipmappedRasterFormatWriterInternals
	{
		template <>
		bool
		get_nan_no_data_value<double>(
				double &no_data_value)
		{
			no_data_value = GPlatesMaths::quiet_nan<double>();
			return true;
		}

		template <>
		bool
		get_nan_no_data_value<float>(
				float &no_data_value)
		{
			no_data_value = GPlatesMaths::quiet_nan<float>();
			return true;
		}

		template <>
		bool
		get_nan_no_data_value<GPlatesGui::rgba8_t>(
				GPlatesGui::rgba8_t &no_data_value)
		{
			// Default constructor of GPlatesGui::rgba8_t does not initialise data members
			// and causes compiler error on some compilers so we need to initialise it explicitly.
			no_data_value.uint32_value = 0;
			return false;
		}
	}
}
