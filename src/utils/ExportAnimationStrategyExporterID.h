/* $Id$ */

/**
 * \file validate filename template.
 * $Revision$
 * $Date$
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

#ifndef GPLATES_UTILS_EXPORTANIMATIONSTRATEGYEXPORTERID_H
#define GPLATES_UTILS_EXPORTANIMATIONSTRATEGYEXPORTERID_H

namespace GPlatesUtils
{	
	enum Exporter_ID
	{
		ZERO_ID,
		RECONSTRUCTED_GEOMETRIES_GMT,
		RECONSTRUCTED_GEOMETRIES_SHAPEFILE,
		PROJECTED_GEOMETRIES_SVG,
		MESH_VELOCITIES_GPML,
		RESOLVED_TOPOLOGIES_GMT,
		RELATIVE_ROTATION_CSV_COMMA,
		RELATIVE_ROTATION_CSV_SEMICOLON,
		RELATIVE_ROTATION_CSV_TAB,
		EQUIVALENT_ROTATION_CSV_COMMA,
		EQUIVALENT_ROTATION_CSV_SEMICOLON,
		EQUIVALENT_ROTATION_CSV_TAB,
		ROTATION_PARAMS_CSV_COMMA,
		ROTATION_PARAMS_CSV_SEMICOLON,
		ROTATION_PARAMS_CSV_TAB,
		RASTER_BMP,
		RASTER_JPG,
		RASTER_JPEG,
		RASTER_PNG,
		RASTER_PPM,
		RASTER_TIFF,
		RASTER_XBM,
		RASTER_XPM,
		FLOWLINES_GMT,
		FLOWLINES_SHAPEFILE,
		MOTION_PATHS_GMT,
		MOTION_PATHS_SHAPEFILE,
		INVALID_ID=999
	};
}

#endif // GPLATES_UTILS_EXPORTANIMATIONSTRATEGYEXPORTERID_H



