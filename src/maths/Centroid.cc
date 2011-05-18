/* $Id$ */
 
/**
 * \file 
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

#include "Centroid.h"

#include "utils/Profile.h"


boost::optional<GPlatesMaths::UnitVector3D>
GPlatesMaths::Centroid::calculate_centroid(
		const PolygonOnSphere &polygon,
		const double &min_average_point_magnitude)
{
	//PROFILE_BLOCK("calculate_centroid");

	const Vector3D summed_points_position =
			calculate_sum_points(polygon.vertex_begin(), polygon.vertex_end());

	// If the magnitude of the summed vertex position is zero then all the points averaged
	// to zero and hence we cannot get a centroid point.
	// This most likely happens when the vertices roughly form a great circle arc.
	// If the average magnitude of the summed vertex position is too small then it means
	// all the points averaged to a small vector.
	// When the magnitude gets too small divert to a more expensive but more accurate test.
	const double min_summed_point_magnitude = polygon.number_of_vertices() * min_average_point_magnitude;
	if (summed_points_position.magSqrd() > min_summed_point_magnitude * min_summed_point_magnitude)
	{
		return summed_points_position.get_normalisation();
	}
	// Divert to more expensive, but more accurate test...

	//PROFILE_BLOCK("calculate_centroid: more accurate");

	const boost::optional<UnitVector3D> spherical_centroid = calculate_spherical_centroid(polygon);

	if (!spherical_centroid)
	{
		return boost::none;
	}

	return spherical_centroid.get();
}
