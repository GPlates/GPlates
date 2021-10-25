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


GPlatesPropertyValues::SpatialReferenceSystem::non_null_ptr_to_const_type
GPlatesPropertyValues::SpatialReferenceSystem::get_WGS84()
{
	static boost::optional<non_null_ptr_to_const_type> OGR_WGS84;

	if (!OGR_WGS84)
	{
		OGRSpatialReference wgs84;
		wgs84.SetWellKnownGeogCS("WGS84");
		OGR_WGS84 = create(wgs84);
	}

	return OGR_WGS84.get();
}


GPlatesPropertyValues::SpatialReferenceSystem::SpatialReferenceSystem(
		const OGRSpatialReference &ogr_srs) :
	d_ogr_srs(new OGRSpatialReference(ogr_srs)/*copy constructor clones SRS*/)
{
}


GPlatesPropertyValues::SpatialReferenceSystem::~SpatialReferenceSystem()
{
	// boost::scoped_ptr destructor needs complete type.
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
