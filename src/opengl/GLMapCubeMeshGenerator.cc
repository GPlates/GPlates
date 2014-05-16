/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#include "GLMapCubeMeshGenerator.h"


GPlatesOpenGL::GLMapCubeMeshGenerator::GLMapCubeMeshGenerator(
		const GPlatesGui::MapProjection &map_projection,
		unsigned int cube_face_dimension) :
	d_cube_mesh_generator(cube_face_dimension),
	d_map_projection(map_projection)
{
}


void
GPlatesOpenGL::GLMapCubeMeshGenerator::create_cube_face_quadrant_mesh_vertices(
		std::vector<Point> &cube_face_quadrant_mesh_vertices,
		GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face,
		unsigned int quadrant_x_offset,
		unsigned int quadrant_y_offset) const
{
	const unsigned int cube_face_quadrant_dimension_in_vertex_spacing =
			get_cube_face_quadrant_dimension_in_vertex_spacing();
	const unsigned int cube_face_quadrant_dimension_in_vertex_samples =
			get_cube_face_quadrant_dimension_in_vertex_samples();

	// Create the spherical mesh vertices for the specified quadrant of the specified cube face.
	std::vector<GPlatesMaths::UnitVector3D> cube_face_quadrant_mesh_points_on_sphere;
	d_cube_mesh_generator.create_mesh_vertices(
			cube_face_quadrant_mesh_points_on_sphere,
			cube_face,
			quadrant_x_offset * cube_face_quadrant_dimension_in_vertex_spacing,
			quadrant_y_offset * cube_face_quadrant_dimension_in_vertex_spacing,
			cube_face_quadrant_dimension_in_vertex_samples,
			cube_face_quadrant_dimension_in_vertex_samples);

	cube_face_quadrant_mesh_vertices.reserve(
			cube_face_quadrant_dimension_in_vertex_samples * cube_face_quadrant_dimension_in_vertex_samples);

	// Only three cube faces (and the quadrants on them) intersect or touch the dateline.
	const bool quadrant_intersects_dateline =
			cube_face == GPlatesMaths::CubeCoordinateFrame::NEGATIVE_X ||
			cube_face == GPlatesMaths::CubeCoordinateFrame::POSITIVE_Z ||
			cube_face == GPlatesMaths::CubeCoordinateFrame::NEGATIVE_Z;

	// The cube corner that the quadrant is touching.
	const GPlatesMaths::Vector3D &corner_adjacent_to_quadrant =
			GPlatesMaths::CubeCoordinateFrame::get_cube_corner(
					GPlatesMaths::CubeCoordinateFrame::get_cube_corner_index(
							cube_face, quadrant_x_offset, quadrant_y_offset));

	// Is true if the quadrant is in the half-space of the globe with longitude range [0,180].
	// The other half space has longitude range [-180,0].
	const bool quadrant_is_in_upper_longitude_range =
			dot(corner_adjacent_to_quadrant, GPlatesMaths::UnitVector3D::yBasis()).dval() > 0;

	// Is true if the quadrant is in the hemisphere containing the dateline which is the
	// longitude ranges [90,180] and [-180,-90].
	// The other hemisphere has longitude range [-90,90].
	const bool quadrant_is_in_dateline_hemisphere =
			dot(corner_adjacent_to_quadrant, GPlatesMaths::UnitVector3D::xBasis()).dval() < 0;

	// An epsilon threshold that's enough to distinguish between adjacent mesh points - one on the
	// dateline and the adjacent one off the dateline - the '0.5' means half-way in between.
	// The sqrt(2) accounts smallest angular deviation which at the cube edge midpoints which are
	// along a diagonal on the x-z plane.
	const double dateline_test_espsilon =
			std::sin(std::atan(0.5 / (std::sqrt(2.0) * cube_face_quadrant_dimension_in_vertex_spacing)));

	const double &central_meridian_longitude = d_map_projection.central_llp().longitude();

	// Iterate over the vertices of the quadrant of the cube face.
	for (unsigned int y = 0; y < cube_face_quadrant_dimension_in_vertex_samples; ++y)
	{
		for (unsigned int x = 0; x < cube_face_quadrant_dimension_in_vertex_samples; ++x)
		{
			const GPlatesMaths::UnitVector3D &point_on_sphere =
					cube_face_quadrant_mesh_points_on_sphere[
							x + y * cube_face_quadrant_dimension_in_vertex_samples];

			// Convert to latitude/longitude.
			const GPlatesMaths::LatLonPoint lat_lon_point =
					make_lat_lon_point(GPlatesMaths::PointOnSphere(point_on_sphere));

			// The map-projected point - it's not yet projected though.
			Point2D map_point =
			{
				lat_lon_point.longitude() + central_meridian_longitude,
				lat_lon_point.latitude()
			};

			// If current point touches the dateline then we have to properly wrap to the dateline
			// before projecting onto the map.
			//
			// Exclude cube faces that don't intersect dateline.
			if (quadrant_intersects_dateline)
			{
				// For quadrants that have the dateline running along an edge of the quadrant.
				if (quadrant_is_in_dateline_hemisphere)
				{
					// Only edges of quadrant can touch the dateline.
					// This is just to avoid unnecessarily doing the dot-product epsilon test below.
					if (x == 0 ||
						y == 0 ||
						x == cube_face_quadrant_dimension_in_vertex_samples - 1 ||
						y == cube_face_quadrant_dimension_in_vertex_samples - 1)
					{
						// See if current point lies on the dateline.
						if (abs(dot(point_on_sphere, GPlatesMaths::UnitVector3D::yBasis())).dval() <
							dateline_test_espsilon)
						{
							// Change the point's longitude depending on which longitude range
							// the quadrant is in.
							map_point.x = quadrant_is_in_upper_longitude_range
									? 180 + central_meridian_longitude
									: -180 + central_meridian_longitude;
						}
					}
				}
			}

			// Map project the point.
			d_map_projection.forward_transform(map_point.x, map_point.y);

			// Store the original point-on-sphere and the map-projected point.
			const Point point = { point_on_sphere, map_point };

			cube_face_quadrant_mesh_vertices.push_back(point);
		}
	}
}


GPlatesOpenGL::GLMapCubeMeshGenerator::Point
GPlatesOpenGL::GLMapCubeMeshGenerator::create_pole_mesh_vertex(
		const double &pole_longitude,
		bool north_pole) const
{
	const double &central_meridian_longitude = d_map_projection.central_llp().longitude();

	// The map-projected point - it's not yet projected though.
	Point2D map_point =
	{
		pole_longitude + central_meridian_longitude,
		double(north_pole ? 90 : -90)
	};

	// Map project the point.
	d_map_projection.forward_transform(map_point.x, map_point.y);

	// The associated point-on-sphere is independent of the longitude at the poles.
	const GPlatesMaths::UnitVector3D point_on_sphere = north_pole
			? GPlatesMaths::UnitVector3D::zBasis()
			: -GPlatesMaths::UnitVector3D::zBasis();

	// Store the original point-on-sphere and the map-projected point.
	const Point point = { point_on_sphere, map_point };

	return point;
}
