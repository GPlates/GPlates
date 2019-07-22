/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2014 The University of Sydney, Australia
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

#include <boost/optional.hpp>
#include <ogr_spatialref.h>

#include "SpatialReferenceSystem.h"

#include "global/GdalVersion.h"


GPlatesPropertyValues::SpatialReferenceSystem::non_null_ptr_to_const_type
GPlatesPropertyValues::SpatialReferenceSystem::get_WGS84()
{
	static boost::optional<non_null_ptr_to_const_type> OGR_WGS84;

	if (!OGR_WGS84)
	{
		OGRSpatialReference wgs84;
		wgs84.SetWellKnownGeogCS("WGS84");
#if GPLATES_GDAL_VERSION_NUM >= GPLATES_GDAL_COMPUTE_VERSION(3,0,0)
		// GDAL >= 3.0 introduced a data-axis-to-CRS-axis mapping (that breaks backward compatibility).
		// We need to set it to behave the same as before GDAL 3.0 (ie, longitude first, latitude second).
		wgs84.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
#endif
		OGR_WGS84 = create(wgs84);
	}

	return OGR_WGS84.get();
}


GPlatesPropertyValues::SpatialReferenceSystem::SpatialReferenceSystem(
		const OGRSpatialReference &ogr_srs) :
	d_ogr_srs(
			// Ensure allocation/deallocation happens in the OGR memory heap on Windows platforms
			// since apparently Windows uses a separate heap per DLL...
			//
			// See http://lists.osgeo.org/pipermail/gdal-dev/2006-March/008204.html
			//
			static_cast<OGRSpatialReference *>(OSRNewSpatialReference(NULL)),
			OGRSpatialReferenceReleaser())
{
	*d_ogr_srs = ogr_srs; // Assignment operator clones SRS.
}


GPlatesPropertyValues::SpatialReferenceSystem::~SpatialReferenceSystem()
{
}


bool
GPlatesPropertyValues::SpatialReferenceSystem::is_geographic() const
{
	return d_ogr_srs->IsGeographic();
}


bool
GPlatesPropertyValues::SpatialReferenceSystem::is_projected() const
{
	return d_ogr_srs->IsProjected();
}

bool
GPlatesPropertyValues::SpatialReferenceSystem::is_wgs84() const
{
	return d_ogr_srs->IsSame(&(get_WGS84()->get_ogr_srs()));
}


void
GPlatesPropertyValues::SpatialReferenceSystem::OGRSpatialReferenceReleaser::operator()(
		OGRSpatialReference *ogr_srs)
{
	// Decrement reference count (which destroys if count reaches zero).
	// Our clients may have incremented reference count when our OGRSpatialReference was
	// exposed via 'SpatialReferenceSystem::get_ogr_srs()'.
	OSRRelease(ogr_srs);
}
