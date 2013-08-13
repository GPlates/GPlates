/* $Id$ */

/**
 * \file 
 * Helper class used to build geometry used to create a new feature.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#include <boost/none.hpp>

#include "InternalGeometryBuilder.h"
#include "utils/GeometryCreationUtils.h"


GPlatesViewOperations::InternalGeometryBuilder::InternalGeometryBuilder(
		GeometryBuilder *geometry_builder,
		GPlatesMaths::GeometryType::Value geom_type):
	d_desired_geometry_type(geom_type),
	d_geometry_opt_ptr(boost::none),
	d_actual_geometry_type(GPlatesMaths::GeometryType::NONE),
	d_update(false)
{
}

void
GPlatesViewOperations::InternalGeometryBuilder::update() const
{
	// Return if don't need updating.
	if (!d_update)
	{
		return;
	}

	//
	// Rebuild our cached GeometryOnSphere and update the actual geometry type created.
	//

	d_actual_geometry_type = GPlatesMaths::GeometryType::NONE;
	create_geometry_on_sphere(d_desired_geometry_type);

	// Finished updating - don't need to do again until internal state is modified again.
	d_update = false;
}

void
GPlatesViewOperations::InternalGeometryBuilder::create_geometry_on_sphere(
		GPlatesMaths::GeometryType::Value geom_type) const
{
	GPlatesUtils::GeometryConstruction::GeometryConstructionValidity validity;

	switch (geom_type)
	{
	case GPlatesMaths::GeometryType::POINT:
		d_geometry_opt_ptr = GPlatesUtils::create_point_on_sphere(d_point_seq, validity);
		if (validity == GPlatesUtils::GeometryConstruction::VALID)
		{
			d_actual_geometry_type = GPlatesMaths::GeometryType::POINT;
		}
		break;

	case GPlatesMaths::GeometryType::MULTIPOINT:
		if (d_point_seq.size() > 1)
		{
			d_geometry_opt_ptr = GPlatesUtils::create_multipoint_on_sphere(d_point_seq, validity);
			if (validity == GPlatesUtils::GeometryConstruction::VALID)
			{
				d_actual_geometry_type = GPlatesMaths::GeometryType::MULTIPOINT;
			}
			else if (validity == GPlatesUtils::GeometryConstruction::INVALID_INSUFFICIENT_POINTS)
			{
				create_geometry_on_sphere(GPlatesMaths::GeometryType::POINT);
			}
		}
		else
		{
			create_geometry_on_sphere(GPlatesMaths::GeometryType::POINT);
		}
		break;

	case GPlatesMaths::GeometryType::POLYLINE:
		d_geometry_opt_ptr = GPlatesUtils::create_polyline_on_sphere(d_point_seq, validity);
		if (validity == GPlatesUtils::GeometryConstruction::VALID)
		{
			d_actual_geometry_type = GPlatesMaths::GeometryType::POLYLINE;
		}
		else if (validity == GPlatesUtils::GeometryConstruction::INVALID_INSUFFICIENT_POINTS)
		{
			create_geometry_on_sphere(GPlatesMaths::GeometryType::POINT);
		}
		break;

	case GPlatesMaths::GeometryType::POLYGON:
		d_geometry_opt_ptr = GPlatesUtils::create_polygon_on_sphere(d_point_seq, validity);
		if (validity == GPlatesUtils::GeometryConstruction::VALID)
		{
			d_actual_geometry_type = GPlatesMaths::GeometryType::POLYGON;
		}
		else if (validity == GPlatesUtils::GeometryConstruction::INVALID_INSUFFICIENT_POINTS)
		{
			create_geometry_on_sphere(GPlatesMaths::GeometryType::POLYLINE);
		}
		break;

	default:
		break;
	}
}
