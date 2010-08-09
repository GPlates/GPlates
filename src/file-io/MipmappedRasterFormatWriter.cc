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

#include "MipmappedRasterFormatWriter.h"


namespace GPlatesFileIO
{
	namespace MipmappedRasterFormatWriterInternals
	{
		template<>
		void
		write<GPlatesGui::rgba8_t>(
				QDataStream &out,
				const GPlatesGui::rgba8_t *data,
				unsigned int len)
		{
			const GPlatesGui::rgba8_t *end = data + len;
			while (data != end)
			{
				out << static_cast<quint8>(data->red)
					<< static_cast<quint8>(data->green)
					<< static_cast<quint8>(data->blue)
					<< static_cast<quint8>(data->alpha);
				++data;
			}
		}
	}
}

