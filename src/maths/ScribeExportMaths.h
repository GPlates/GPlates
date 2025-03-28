/**
 * Copyright (C) 2023 The University of Sydney, Australia
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

#ifndef GPLATES_DATA_MINING_SCRIBEEXPORTMATHS_H
#define GPLATES_DATA_MINING_SCRIBEEXPORTMATHS_H

#include "MultiPointOnSphere.h"
#include "PointOnSphere.h"
#include "PolygonOnSphere.h"
#include "PolylineOnSphere.h"


/**
 * Scribe export registered classes/types in the 'property-values' source sub-directory.
 *
 * See "ScribeExportRegistration.h" for more details.
 *
 *******************************************************************************
 * WARNING: Changing the string ids will break backward/forward compatibility. *
 *******************************************************************************
 */
#define SCRIBE_EXPORT_MATHS \
		\
		(((GPlatesMaths::MultiPointOnSphere), \
			"GPlatesMaths::MultiPointOnSphere")) \
		\
		(((GPlatesMaths::PointGeometryOnSphere), \
			"GPlatesMaths::PointGeometryOnSphere")) \
		\
		(((GPlatesMaths::PolygonOnSphere), \
			"GPlatesMaths::PolygonOnSphere")) \
		\
		(((GPlatesMaths::PolylineOnSphere), \
			"GPlatesMaths::PolylineOnSphere")) \
		\


#endif // GPLATES_DATA_MINING_SCRIBEEXPORTMATHS_H
