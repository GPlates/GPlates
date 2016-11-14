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

#include <cmath>
#include <set>
#include <boost/random.hpp>
#include <boost/generator_iterator.hpp>
#include <boost/optional.hpp>
#include <boost/utility/in_place_factory.hpp>

#include "GeneratePoints.h"

#include "AngularExtent.h"
#include "GeometryDistance.h"
#include "MathsUtils.h"
#include "Real.h"
#include "Rotation.h"
#include "SphericalSubdivision.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


namespace GPlatesMaths
{
	namespace
	{
		/**
		 * Used to recurse into a Rhombic Triacontahedron to generate points within a polygon.
		 *
		 * This generates a more uniform distribution of points than the Hierarchical Triangular Mesh.
		 * It starts with 30 quad faces compared to 8 triangle faces (for the Hierarchical Triangular Mesh).
		 */
		class UniformPointsInPolygonBuilder
		{
		public:

			/**
			 * Keeps track of the recursion depth and whether we need to test child quads against
			 * the polygon (don't have to if parent quad is completely inside).
			 */
			struct RecursionContext
			{
				RecursionContext() :
					depth(0),
					test_against_polygon(true)
				{  }

				unsigned int depth;
				bool test_against_polygon;
			};


			UniformPointsInPolygonBuilder(
					std::vector<PointOnSphere> &points,
					const PolygonOnSphere &polygon,
					unsigned int recursion_depth_to_generate_points,
					const double &point_random_offset,
					const AngularExtent &distance_threshold) :
				d_points(points),
				d_polygon(polygon),
				d_recursion_depth_to_generate_points(recursion_depth_to_generate_points),
				d_distance_threshold(distance_threshold)
			{
				if (!are_almost_exactly_equal(point_random_offset, 0.0))
				{
					d_random_offset_point_generator = boost::in_place(point_random_offset);
				}
			}

			void
			visit(
					const SphericalSubdivision::RhombicTriacontahedronTraversal::Quad &quad,
					const RecursionContext &recursion_context)
			{
				RecursionContext children_recursion_context(recursion_context);

				if (recursion_context.test_against_polygon)
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

					// The distance threshold accounts for the maximum random offset possible.
					const AngularDistance distance = minimum_distance(
							d_polygon,
							*quad_poly,
							false/*polygon1_interior_is_solid*/,
							false/*polygon2_interior_is_solid*/,
							d_distance_threshold);

					// See if the quad face and polygon are further apart than the distance threshold.
					if (distance == AngularDistance::PI)
					{
						// Polygons did not intersect.
						// One might be completely inside the other.
						if (d_polygon.is_point_in_polygon(
								quad_poly->first_exterior_ring_vertex()/*arbitrary*/,
								PolygonOnSphere::HIGH_SPEED_HIGH_SETUP_HIGH_MEMORY_USAGE))
						{
							// Quad face (plus maximum random offset distance) is completely inside the polygon.
							// Hence all child quad faces will be too, so no need to test them.
							children_recursion_context.test_against_polygon = false;
						}
						else if (!quad_poly->is_point_in_polygon(d_polygon.first_exterior_ring_vertex()/*arbitrary*/))
						{
							// Quad face and polygon do not overlap, and are further apart than the distance threshold,
							// so we know that none of the points in the quad face (even with a random offset - which is
							// smaller than the distance threshold) will be inside the polygon.
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
						std::pair<vertex_set_type::iterator, bool> vertex_insert_result =
								d_vertex_set.insert(quad_vertices[v]);
						if (!vertex_insert_result.second) // not inserted
						{
							continue;
						}

						// Randomly offset the vertex if requested.
						if (d_random_offset_point_generator)
						{
							// The maximum radius offset is half the edge length of the quad.
							const double offset_radius_in_radians = 0.5 * std::acos(
									dot(quad.vertex0, quad.vertex1).dval());

							const PointOnSphere random_offset_point =
									d_random_offset_point_generator->get_random_offset_point(
											quad_vertices[v],
											offset_radius_in_radians,
											// The random angle *around* the point is aligned with the quad direction...
											Vector3D(quad.vertex0) - Vector3D(quad.vertex1));

							// Make sure *offset* point (ie, after random offset) is inside polygon.
							if (recursion_context.test_against_polygon &&
								!d_polygon.is_point_in_polygon(
									random_offset_point,
									PolygonOnSphere::HIGH_SPEED_HIGH_SETUP_HIGH_MEMORY_USAGE))
							{
								// Skip the current vertex.
								continue;
							}

							d_points.push_back(random_offset_point);
						}
						else
						{
							// Make sure *original* point is inside polygon.
							if (recursion_context.test_against_polygon &&
								!d_polygon.is_point_in_polygon(
									quad_vertices[v],
									PolygonOnSphere::HIGH_SPEED_HIGH_SETUP_HIGH_MEMORY_USAGE))
							{
								// Skip the current vertex.
								continue;
							}

							d_points.push_back(quad_vertices[v]);
						}
					}

					return;
				}

				// Recurse into the child triangles.
				++children_recursion_context.depth;
				quad.visit_children(*this, children_recursion_context);
			}

		private:

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
			typedef std::set<PointOnSphere, PointOnSphereMapPredicate> vertex_set_type;


			std::vector<PointOnSphere> &d_points;
			const PolygonOnSphere &d_polygon;
			unsigned int d_recursion_depth_to_generate_points;
			AngularExtent d_distance_threshold;
			boost::optional<RandomOffsetPointGenerator> d_random_offset_point_generator;

			vertex_set_type d_vertex_set;
		};
	}
}


void
GPlatesMaths::GeneratePoints::create_uniform_points_in_polygon(
		std::vector<PointOnSphere> &points,
		const PolygonOnSphere &polygon,
		unsigned int point_density_level,
		const double &point_random_offset)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			Real(point_random_offset) >= 0.0 && Real(point_random_offset) <= 1.0,
			GPLATES_ASSERTION_SOURCE);

	// The side of a level 0 quad face of a Rhombic Triacontahedron is about 40 degrees
	// (let's assume 80 degrees to be safe).
	// The maximum radius of a random offset circle is half that length.
	// And each subdivision level reduces that by about a half.
	const AngularExtent distance_threshold = AngularExtent::create_from_angle(
			0.5 * convert_deg_to_rad(80.0) / (1 << point_density_level));

	UniformPointsInPolygonBuilder uniform_points_in_polygon_builder(
			points,
			polygon,
			point_density_level,
			point_random_offset,
			distance_threshold);

	SphericalSubdivision::RhombicTriacontahedronTraversal rhombic_triacontahedron_traversal;
	const UniformPointsInPolygonBuilder::RecursionContext recursion_context;
	rhombic_triacontahedron_traversal.visit(uniform_points_in_polygon_builder, recursion_context);
}
