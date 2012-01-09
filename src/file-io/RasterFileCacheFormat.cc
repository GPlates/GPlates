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

#include <ostream>

#include "RasterFileCacheFormat.h"


namespace GPlatesFileIO
{
	namespace RasterFileCacheFormat
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


GPlatesFileIO::RasterFileCacheFormat::UnsupportedVersion::UnsupportedVersion(
		const GPlatesUtils::CallStack::Trace &exception_source,
		quint32 unrecognised_version_) :
	Exception(exception_source),
	d_unrecognised_version(unrecognised_version_)
{
}


const char *
GPlatesFileIO::RasterFileCacheFormat::UnsupportedVersion::exception_name() const
{

	return "RasterFileCacheFormat::UnsupportedVersion";
}


void
GPlatesFileIO::RasterFileCacheFormat::UnsupportedVersion::write_message(
		std::ostream &os) const
{
	os << "unsupported version: " << d_unrecognised_version;
}
