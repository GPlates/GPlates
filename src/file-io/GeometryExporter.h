/* $Id$ */

/**
 * \file 
 * Interface for exporting GeometryOnSphere geometry.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2006, 2007, 2008 The University of Sydney, Australia
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

#ifndef GPLATES_FILEIO_GEOMETRYEXPORTER_H
#define GPLATES_FILEIO_GEOMETRYEXPORTER_H

#include "maths/GeometryOnSphere.h"

namespace GPlatesFileIO
{
	/**
	 * Interface for exporting GeometryOnSphere geometry.
	 */
	class GeometryExporter
	{
	public:
		virtual
			~GeometryExporter()
		{  }

		/**
		 * Export specified geometry.
		 *
		 * @param geometry_ptr is what's exported.
		 */
		virtual
			void
			export_geometry(
			GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_ptr) = 0;
	};
}

#endif // GPLATES_FILEIO_GEOMETRYEXPORTER_H
