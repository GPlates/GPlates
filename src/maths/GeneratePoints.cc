/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2016 The University of Sydney, Australia
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

#include <algorithm>
#include <cmath>
#include <set>
#include <utility>
#include <boost/generator_iterator.hpp>
#include <boost/optional.hpp>
#include <boost/random.hpp>
#include <boost/utility/in_place_factory.hpp>
#include <boost/variant.hpp>

#include "GeneratePoints.h"

#include "AngularExtent.h"
#include "GeometryDistance.h"
#include "LatLonPoint.h"
#include "MathsUtils.h"
#include "Real.h"
#include "Rotation.h"
#include "SmallCircleBounds.h"
#include "SphericalSubdivision.h"
#include "UnitVector3D.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


namespace GPlatesMaths
{
	namespace
	{
		/**
		 * Used to recurse into a Rhombic Triacontahedron to generate points
		 * (optionally within a polygon, or lat/lon extent, bounding region).
		 *
		 * This generates a more uniform distribution of points than the Hierarchical Triangular Mesh.
		 * It starts with 30 quad faces compared to 8 triangle faces (for the Hierarchical Triangular Mesh).
		 */
		class UniformPointsBuilder
		{
		public:

			/**
			 * Keeps track of the recursion depth and whether we need to test child quads against
			 * the bounds (don't have to if parent quad is completely inside).
			 */
			struct RecursionContext
			{
				RecursionContext() :
					depth(0),
					test_against_bounds(true)
				{  }

				unsigned int depth;
				bool test_against_bounds;
			};


			UniformPointsBuilder(
					std::vector<PointOnSphere> &points,
					unsigned int recursion_depth_to_generate_points,
					const double &point_random_offset) :
				d_points(points),
				d_recursion_depth_to_generate_points(recursion_depth_to_generate_points),
				d_distance_threshold(get_angular_distance_threshold(recursion_depth_to_generate_points)),
				d_bounds(boost::none)
			{
				initialise_random_offset_point_generator(point_random_offset);
			}

			UniformPointsBuilder(
					std::vector<PointOnSphere> &points,
					unsigned int recursion_depth_to_generate_points,
					const double &point_random_offset,
					const PolygonOnSphere &polygon_bounds) :
				d_points(points),
				d_recursion_depth_to_generate_points(recursion_depth_to_generate_points),
				d_distance_threshold(get_angular_distance_threshold(recursion_depth_to_generate_points)),
				d_bounds(bounds_type(polygon_bounds.get_non_null_pointer()))
			{
				initialise_random_offset_point_generator(point_random_offset);
			}

			UniformPointsBuilder(
					std::vector<PointOnSphere> &points,
					unsigned int recursion_depth_to_generate_points,
					const double &point_random_offset,
					const double &top,
					const double &bottom,
					const double &left,
					const double &right) :
				d_points(points),
				d_recursion_depth_to_generate_points(recursion_depth_to_generate_points),
				d_distance_threshold(get_angular_distance_threshold(recursion_depth_to_generate_points)),
				d_bounds(bounds_type(create_lat_lon_extend_bounds(top, bottom, left, right, d_distance_threshold)))
			{
				initialise_random_offset_point_generator(point_random_offset);
			}

			void
			visit(
					const SphericalSubdivision::RhombicTriacontahedronTraversal::Quad &quad,
					const RecursionContext &recursion_context)
			{
				RecursionContext children_recursion_context(recursion_context);

				if (d_bounds &&
					recursion_context.test_against_bounds)
				{
					const PointOnSphere quad_vertices[4] =
					{
						PointOnSphere(quad.vertex0),
						PointOnSphere(quad.vertex1),
						PointOnSphere(quad.vertex2),
						PointOnSphere(quad.vertex3)
					};

					// Create a polygon from the quad vertices.
					const PolygonOnSphere::non_null_ptr_to_const_type quad_poly =
							PolygonOnSphere::create_on_heap(quad_vertices, quad_vertices + 4);

					// See if bounds is a polygon or lat/lon extent.
					const PolygonOnSphere::non_null_ptr_to_const_type *polygon_bounds_opt =
							boost::get<PolygonOnSphere::non_null_ptr_to_const_type>(&d_bounds.get());
					if (polygon_bounds_opt)
					{
						// Polygon bounds.
						const PolygonOnSphere::non_null_ptr_to_const_type polygon_bounds = *polygon_bounds_opt;

						// The distance threshold accounts for the maximum random offset possible.
						const AngularDistance distance = minimum_distance(
								*polygon_bounds,
								*quad_poly,
								false/*polygon1_interior_is_solid*/,
								false/*polygon2_interior_is_solid*/,
								d_distance_threshold);

						// See if the quad face and polygon are further apart than the distance threshold.
						if (distance == AngularDistance::PI)
						{
							// Polygons did not intersect.
							// One might be completely inside the other.
							if (polygon_bounds->is_point_in_polygon(
									quad_poly->first_exterior_ring_vertex()/*arbitrary*/,
									PolygonOnSphere::HIGH_SPEED_HIGH_SETUP_HIGH_MEMORY_USAGE))
							{
								// Quad face (plus maximum random offset distance) is completely inside the polygon.
								// Hence all child quad faces will be too, so no need to test them.
								children_recursion_context.test_against_bounds = false;
							}
							else if (!quad_poly->is_point_in_polygon(polygon_bounds->first_exterior_ring_vertex()/*arbitrary*/))
							{
								// Quad face and polygon do not overlap, and are further apart than the distance threshold,
								// so we know that none of the points in the quad face (even with a random offset - which is
								// smaller than the distance threshold) will be inside the polygon.
								return;
							}
						}
					}
					else // lat/lon extent...
					{
						const LatLonExtent &lat_lon_extent_bounds = boost::get<LatLonExtent>(d_bounds.get());

						if (lat_lon_extent_bounds.is_inside_contracted_bounds(*quad_poly))
						{
							// Quad face is completely inside lat/lon extent (contracted by distance threshold).
							// So we know that all of the points in the quad face (even with a random offset -
							// which is smaller than the distance threshold) will be inside the lat/lon extent.
							// Hence all child quad faces will be too, so no need to test them.
							children_recursion_context.test_against_bounds = false;
						}
						else if (lat_lon_extent_bounds.is_outside_expanded_bounds(*quad_poly))
						{
							// Quad face is completely outside lat/lon extent (expanded by distance threshold).
							// So we know that none of the points in the quad face (even with a random offset -
							// which is smaller than the distance threshold) will be inside the lat/lon extent.
							return;
						}
					}
				}

				if (recursion_context.depth == d_recursion_depth_to_generate_points)
				{
					const PointOnSphere quad_vertices[4] =
					{
						PointOnSphere(quad.vertex0),
						PointOnSphere(quad.vertex1),
						PointOnSphere(quad.vertex2),
						PointOnSphere(quad.vertex3)
					};

					for (unsigned int v = 0; v < 4; ++v)
					{
						// If have already visited vertex (via adjacent quad) then continue to next vertex.
						std::pair<visited_vertices_type::iterator, bool> vertex_insert_result =
								d_visited_vertices.insert(quad_vertices[v]);
						if (!vertex_insert_result.second) // not inserted
						{
							continue;
						}

						PointOnSphere vertex(quad_vertices[v]);

						// Randomly offset the vertex if requested.
						if (d_random_offset_point_generator)
						{
							// The maximum radius offset is half the edge length of the quad.
							const double offset_radius_in_radians = 0.5 * std::acos(
									dot(quad.vertex0, quad.vertex1).dval());

							vertex = d_random_offset_point_generator->get_random_offset_point(
									vertex,
									offset_radius_in_radians,
									// The random angle *around* the point is aligned with the quad direction...
									Vector3D(quad.vertex0) - Vector3D(quad.vertex1));
						}

						// Make sure point (original or after random offset) is inside bounds (if requested).
						if (d_bounds &&
							recursion_context.test_against_bounds)
						{
							const PolygonOnSphere::non_null_ptr_to_const_type *polygon_bounds_opt =
									boost::get<PolygonOnSphere::non_null_ptr_to_const_type>(&d_bounds.get());
							if (polygon_bounds_opt)
							{
								const PolygonOnSphere::non_null_ptr_to_const_type polygon_bounds = *polygon_bounds_opt;

								if (!polygon_bounds->is_point_in_polygon(
										vertex,
										PolygonOnSphere::HIGH_SPEED_HIGH_SETUP_HIGH_MEMORY_USAGE))
								{
									// Skip the current vertex.
									continue;
								}
							}
							else // lat/lon extent...
							{
								const LatLonExtent &lat_lon_extent_bounds = boost::get<LatLonExtent>(d_bounds.get());
								if (!lat_lon_extent_bounds.contains(vertex))
								{
									// Skip the current vertex.
									continue;
								}
							}
						}

						d_points.push_back(vertex);
					}

					return;
				}

				// Recurse into the child triangles.
				++children_recursion_context.depth;
				quad.visit_children(*this, children_recursion_context);
			}

		private:

			/**
			 * Lat/lon bounding box.
			 */
			class LatLonExtent
			{
			public:

				LatLonExtent(
						const double &top,
						const double &bottom,
						const double &left,
						const double &right,
						const AngularExtent &distance_threshold) :
					d_lon_extent_exceeds_180(right - left > 180),
					// Regular extent...
					d_top_bound(
							PointOnSphere::south_pole.position_vector(),
							AngularExtent::create_from_angle(convert_deg_to_rad(top - -90))),
					d_bottom_bound(
							PointOnSphere::north_pole.position_vector(),
							AngularExtent::create_from_angle(convert_deg_to_rad(90 - bottom))),
					d_left_bound(
							make_point_on_sphere(LatLonPoint(0, (left + 90 > 360) ? left + 90 - 360 : left + 90)).position_vector(),
							AngularExtent::HALF_PI),
					d_right_bound(
							make_point_on_sphere(LatLonPoint(0, (right - 90 < -360) ? right - 90 + 360 : right - 90)).position_vector(),
							AngularExtent::HALF_PI),
					// Contracted extent...
					d_contracted_top_bound(d_top_bound.contract(distance_threshold)),
					d_contracted_bottom_bound(d_bottom_bound.contract(distance_threshold)),
					d_contracted_left_bound(d_left_bound.contract(distance_threshold)),
					d_contracted_right_bound(d_right_bound.contract(distance_threshold)),
					// Expanded extent...
					d_expanded_top_bound(d_top_bound.extend(distance_threshold)),
					d_expanded_bottom_bound(d_bottom_bound.extend(distance_threshold)),
					d_expanded_left_bound(d_left_bound.extend(distance_threshold)),
					d_expanded_right_bound(d_right_bound.extend(distance_threshold))
				{  }

				bool
				is_inside_contracted_bounds(
						const PolygonOnSphere &polygon) const
				{
					if (d_contracted_top_bound.test_filled_polygon(polygon) == BoundingSmallCircle::INSIDE_BOUNDS &&
						d_contracted_bottom_bound.test_filled_polygon(polygon) == BoundingSmallCircle::INSIDE_BOUNDS)
					{
						if (d_lon_extent_exceeds_180)
						{
							if (d_contracted_left_bound.test_filled_polygon(polygon) == BoundingSmallCircle::INSIDE_BOUNDS ||
								d_contracted_right_bound.test_filled_polygon(polygon) == BoundingSmallCircle::INSIDE_BOUNDS)
							{
								return true;
							}
						}
						else
						{
							if (d_contracted_left_bound.test_filled_polygon(polygon) == BoundingSmallCircle::INSIDE_BOUNDS &&
								d_contracted_right_bound.test_filled_polygon(polygon) == BoundingSmallCircle::INSIDE_BOUNDS)
							{
								return true;
							}
						}
					}

					return false;
				}

				bool
				is_outside_expanded_bounds(
						const PolygonOnSphere &polygon) const
				{
					if (d_expanded_top_bound.test_filled_polygon(polygon) == BoundingSmallCircle::OUTSIDE_BOUNDS ||
						d_expanded_bottom_bound.test_filled_polygon(polygon) == BoundingSmallCircle::OUTSIDE_BOUNDS)
					{
						return true;
					}

					if (d_lon_extent_exceeds_180)
					{
						if (d_expanded_left_bound.test_filled_polygon(polygon) == BoundingSmallCircle::OUTSIDE_BOUNDS &&
							d_expanded_right_bound.test_filled_polygon(polygon) == BoundingSmallCircle::OUTSIDE_BOUNDS)
						{
							return true;
						}
					}
					else
					{
						if (d_expanded_left_bound.test_filled_polygon(polygon) == BoundingSmallCircle::OUTSIDE_BOUNDS ||
							d_expanded_right_bound.test_filled_polygon(polygon) == BoundingSmallCircle::OUTSIDE_BOUNDS)
						{
							return true;
						}
					}

					// Note: It's possible that the polygon is outside the lat/lon extent but false is returned.
					// It's OK to be conservative here - at least we're not returning true when polygon overlaps the lat/lon extent.
					return false;
				}

				bool
				contains(
						const PointOnSphere &point) const
				{
					if (d_top_bound.test(point) != BoundingSmallCircle::OUTSIDE_BOUNDS &&
						d_bottom_bound.test(point) != BoundingSmallCircle::OUTSIDE_BOUNDS)
					{
						if (d_lon_extent_exceeds_180)
						{
							if (d_left_bound.test(point) != BoundingSmallCircle::OUTSIDE_BOUNDS ||
								d_right_bound.test(point) != BoundingSmallCircle::OUTSIDE_BOUNDS)
							{
								return true;
							}
						}
						else
						{
							if (d_left_bound.test(point) != BoundingSmallCircle::OUTSIDE_BOUNDS &&
								d_right_bound.test(point) != BoundingSmallCircle::OUTSIDE_BOUNDS)
							{
								return true;
							}
						}
					}

					return false;
				}

			private:
				//
				// The overlap of all bounding small circles represents the interior of the lat/lon extent.
				//
				// If the longitudinal extent exceeds 180 degrees then this affects how we perform
				// our inclusion/exclusion testing.
				//
				bool d_lon_extent_exceeds_180;

				// Regular extent.
				BoundingSmallCircle d_top_bound;
				BoundingSmallCircle d_bottom_bound;
				BoundingSmallCircle d_left_bound;
				BoundingSmallCircle d_right_bound;

				// Contracted extent.
				BoundingSmallCircle d_contracted_top_bound;
				BoundingSmallCircle d_contracted_bottom_bound;
				BoundingSmallCircle d_contracted_left_bound;
				BoundingSmallCircle d_contracted_right_bound;

				// Expanded extent.
				BoundingSmallCircle d_expanded_top_bound;
				BoundingSmallCircle d_expanded_bottom_bound;
				BoundingSmallCircle d_expanded_left_bound;
				BoundingSmallCircle d_expanded_right_bound;
			};

			class RandomOffsetGenerator
			{
			public:

				RandomOffsetGenerator(
						const double &min_value,
						const double &max_value) :
					d_uniform(min_value, max_value),
					d_dice(d_rng, d_uniform)
				{  }

				double
				get_random_value()
				{
					return d_dice();
				}

			private:
				typedef boost::mt19937 rng_type;

				rng_type d_rng;
				boost::uniform_real<> d_uniform;
				boost::variate_generator< rng_type, boost::uniform_real<> > d_dice;
			};

			class RandomOffsetPointGenerator
			{
			public:

				explicit
				RandomOffsetPointGenerator(
						const double &point_random_offset) :
					d_point_random_offset(point_random_offset),
					d_random_radius_generator(0.0, 1.0),
					d_random_angle_generator(0.0, 2*PI)
				{  }

				PointOnSphere
				get_random_offset_point(
						const PointOnSphere &point,
						const double &offset_radius_in_radians,
						const Vector3D &quad_alignment)
				{
					const double random_radius = d_point_random_offset * offset_radius_in_radians *
							std::sqrt(d_random_radius_generator.get_random_value());
					const double random_angle = d_random_angle_generator.get_random_value();

					const UnitVector3D tangent =
							cross(point.position_vector(), quad_alignment).get_normalisation();

					return Rotation::create(point.position_vector(), random_angle) *
						Rotation::create(tangent, random_radius) * point;
				}

			private:
				double d_point_random_offset;
				RandomOffsetGenerator d_random_radius_generator;
				RandomOffsetGenerator d_random_angle_generator;
			};

			/**
			 * Typedef for seeing if we've already visited a vertex.
			 */
			typedef std::set<PointOnSphere, PointOnSphereMapPredicate> visited_vertices_type;

			typedef boost::variant<LatLonExtent, PolygonOnSphere::non_null_ptr_to_const_type> bounds_type;


			static
			AngularExtent
			get_angular_distance_threshold(
					unsigned int recursion_depth_to_generate_points)
			{
				// The side of a level 0 quad face of a Rhombic Triacontahedron is about 40 degrees
				// (let's assume 80 degrees to be safe).
				// The maximum radius of a random offset circle is half that length.
				// And each subdivision level reduces that by about a half...
				return AngularExtent::create_from_angle(
						0.5 * convert_deg_to_rad(80.0) / (1 << recursion_depth_to_generate_points));
			}

			static
			LatLonExtent
			create_lat_lon_extend_bounds(
					double top,
					double bottom,
					double left,
					double right,
					const AngularExtent &distance_threshold)
			{
				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						top <= 90.0 && top >= -90 &&
							bottom <= 90.0 && bottom >= -90,
						GPLATES_ASSERTION_SOURCE);
				if (top < bottom)
				{
					std::swap(top, bottom);
				}

				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						Real(right) >= -360.0 && Real(right) <= 360 &&
							Real(left) >= -360.0 && Real(left) <= 360,
						GPLATES_ASSERTION_SOURCE);
				if (right < left)
				{
					std::swap(right, left);
				}

				// Keep distance between left and right to 360 degrees or less.
				if (right > left + 360)
				{
					right -= 360;
				}

				return LatLonExtent(top, bottom, left, right, distance_threshold);
			}

			void
			initialise_random_offset_point_generator(
					const double &point_random_offset)
			{
				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						Real(point_random_offset) >= 0.0 && Real(point_random_offset) <= 1.0,
						GPLATES_ASSERTION_SOURCE);

				if (!are_almost_exactly_equal(point_random_offset, 0.0))
				{
					d_random_offset_point_generator = boost::in_place(point_random_offset);
				}
			}


			std::vector<PointOnSphere> &d_points;
			unsigned int d_recursion_depth_to_generate_points;
			AngularExtent d_distance_threshold;
			boost::optional<RandomOffsetPointGenerator> d_random_offset_point_generator;
			boost::optional<bounds_type> d_bounds;

			visited_vertices_type d_visited_vertices;
		};
	}
}


void
GPlatesMaths::GeneratePoints::create_global_uniform_points(
		std::vector<PointOnSphere> &points,
		unsigned int point_density_level,
		const double &point_random_offset)
{
	UniformPointsBuilder uniform_points_builder(
			points,
			point_density_level,
			point_random_offset);

	SphericalSubdivision::RhombicTriacontahedronTraversal rhombic_triacontahedron_traversal;
	const UniformPointsBuilder::RecursionContext recursion_context;
	rhombic_triacontahedron_traversal.visit(uniform_points_builder, recursion_context);
}


void
GPlatesMaths::GeneratePoints::create_uniform_points_in_lat_lon_extent(
		std::vector<PointOnSphere> &points,
		unsigned int point_density_level,
		const double &point_random_offset,
		const double &top,    // Max lat.
		const double &bottom, // Min lat.
		const double &left,   // Min lon.
		const double &right)  // Max lon.
{
	UniformPointsBuilder uniform_points_builder(
			points,
			point_density_level,
			point_random_offset,
			top,
			bottom,
			left,
			right);

	SphericalSubdivision::RhombicTriacontahedronTraversal rhombic_triacontahedron_traversal;
	const UniformPointsBuilder::RecursionContext recursion_context;
	rhombic_triacontahedron_traversal.visit(uniform_points_builder, recursion_context);
}


void
GPlatesMaths::GeneratePoints::create_uniform_points_in_polygon(
		std::vector<PointOnSphere> &points,
		unsigned int point_density_level,
		const double &point_random_offset,
		const PolygonOnSphere &polygon)
{
	UniformPointsBuilder uniform_points_builder(
			points,
			point_density_level,
			point_random_offset,
			polygon);

	SphericalSubdivision::RhombicTriacontahedronTraversal rhombic_triacontahedron_traversal;
	const UniformPointsBuilder::RecursionContext recursion_context;
	rhombic_triacontahedron_traversal.visit(uniform_points_builder, recursion_context);
}
