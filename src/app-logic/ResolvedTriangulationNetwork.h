/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
 * Copyright (C) 2012 2013 California Institute of Technology 
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

#ifndef GPLATES_APP_LOGIC_RESOLVEDTRIANGULATIONNETWORK_H
#define GPLATES_APP_LOGIC_RESOLVEDTRIANGULATIONNETWORK_H

#include <map>
#include <utility>
#include <vector>
#include <boost/optional.hpp>

#include "ReconstructedFeatureGeometry.h"
#include "ReconstructionTreeCreator.h"
#include "ResolvedTriangulationConstrainedDelaunay2.h"
#include "ResolvedTriangulationDelaunay2.h"
#include "ResolvedTriangulationDelaunay3.h"
#include "VelocityDeltaTime.h"

#include "maths/AzimuthalEqualAreaProjection.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/Vector3D.h"

#include "utils/KeyValueCache.h"
#include "utils/ReferenceCount.h"


namespace GPlatesAppLogic
{
	namespace ResolvedTriangulation
	{
		/**
		 * The central access point for resolved topological network triangulations.
		 *
		 * This class ensures the same 3D <--> 2D projection is used for the internal
		 * delaunay and constrained delaunay triangulations.
		 *
		 * It also optimises by only building the triangulations if they are actually needed.
		 * This is done mainly by providing an intersection test to see if geometry intersects the
		 * internal triangulation(s) thereby avoiding the need to access the internal triangulation(s)
		 * unless geometry intersection is detected.
		 */
		class Network :
				public GPlatesUtils::ReferenceCount<Network>
		{
		public:

			//! A convenience typedef for a shared pointer to a non-const @a Network.
			typedef GPlatesUtils::non_null_intrusive_ptr<Network> non_null_ptr_type;

			//! A convenience typedef for a shared pointer to a const @a Network.
			typedef GPlatesUtils::non_null_intrusive_ptr<const Network> non_null_ptr_to_const_type;


			/**
			 * An interior rigid block in the network.
			 *
			 * These blocks are not part of the deforming region since the interior of the block is rigid.
			 */
			class RigidBlock
			{
			public:
				explicit
				RigidBlock(
						const ReconstructedFeatureGeometry::non_null_ptr_type &rfg) :
					d_rfg(rfg)
				{  }

				/**
				 * The reconstructed feature geometry.
				 */
				ReconstructedFeatureGeometry::non_null_ptr_type
				get_reconstructed_feature_geometry() const
				{
					return d_rfg;
				}

			private:
				ReconstructedFeatureGeometry::non_null_ptr_type d_rfg;
			};

			//! Typedef for a sequence of @a RigidBlock objects.
			typedef std::vector<RigidBlock> rigid_block_seq_type;


			/**
			 * Information from a topological section to store in a vertex in the delaunay
			 * triangulation when it is created.
			 */
			struct DelaunayPoint
			{
			public:

				DelaunayPoint(
						const GPlatesMaths::PointOnSphere &point_,
						GPlatesModel::integer_plate_id_type plate_id_,
						const ReconstructionTreeCreator &reconstruction_tree_creator_) :
					point(point_),
					plate_id(plate_id_),
					reconstruction_tree_creator(reconstruction_tree_creator_)
				{  }

				//! The reconstructed point-on-sphere associated with the delaunay vertex.
				GPlatesMaths::PointOnSphere point;

				//! The plate id associated with the delaunay vertex.
				GPlatesModel::integer_plate_id_type plate_id;

				/**
				 * The rotation tree generator used to create reconstruction trees for the
				 * ReconstructedFeatureGeometry associated with the delaunay vertex.
				 *
				 * This (shared) reconstruction tree creator is stored instead of the
				 * ReconstructedFeatureGeometry itself in order to save memory.
				 */
				ReconstructionTreeCreator reconstruction_tree_creator;
			};


			/**
			 * Geometry that forms the constraints of the constrained delaunay triangulation.
			 */
			struct ConstrainedDelaunayGeometry
			{
			public:

				ConstrainedDelaunayGeometry(
						const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry_,
						bool constrain_begin_and_end_points_) :
					geometry(geometry_),
					constrain_begin_and_end_points(constrain_begin_and_end_points_)
				{  }

				//! The geometry that forms the constraints of the constrained delaunay triangulation.
				GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry;

				//! Whether to close the loop between begin and end points by forming a constraint.
				bool constrain_begin_and_end_points;
			};


			/**
			 * Creates a @a Network.
			 */
			template <typename DelaunayPointIter,
					typename ConstrainedDelaunayGeometryIter,
					typename ScatteredPointOnSphereIter,
					typename SeedPointOnSphereIter,
					typename RigidBlockIter>
			static
			non_null_ptr_type
			create(
					const double &reconstruction_time,
					const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type &network_boundary_polygon,
					DelaunayPointIter delaunay_points_begin,
					DelaunayPointIter delaunay_points_end,
					ConstrainedDelaunayGeometryIter constrained_delaunay_geometries_begin,
					ConstrainedDelaunayGeometryIter constrained_delaunay_geometries_end,
					ScatteredPointOnSphereIter scattered_points_begin,
					ScatteredPointOnSphereIter scattered_points_end,
					SeedPointOnSphereIter seed_points_begin,
					SeedPointOnSphereIter seed_points_end,
					RigidBlockIter rigid_blocks_begin,
					RigidBlockIter rigid_blocks_end,
					const double &refined_mesh_shape_factor,
					const double &refined_mesh_max_edge)
			{
				return non_null_ptr_type(
						new Network(
								reconstruction_time,
								network_boundary_polygon,
								delaunay_points_begin, delaunay_points_end,
								constrained_delaunay_geometries_begin, constrained_delaunay_geometries_end,
								scattered_points_begin, scattered_points_end,
								seed_points_begin, seed_points_end,
								rigid_blocks_begin, rigid_blocks_end,
								refined_mesh_shape_factor,
								refined_mesh_max_edge));
			}


			/**
			 * Returns the projection used by this triangulation to convert from 3D points to
			 * 2D points and vice versa.
			 */
			const GPlatesMaths::AzimuthalEqualAreaProjection &
			get_projection() const
			{
				return d_projection;
			}


			/**
			 * Returns the polygon that bounds the network.
			 */
			GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type
			get_boundary_polygon() const
			{
				return d_network_boundary_polygon;
			}


			/**
			 * Returns the rigid blocks inside the network.
			 */
			const rigid_block_seq_type &
			get_rigid_blocks() const
			{
				return d_rigid_blocks;
			}


			/**
			 * Returns true if the specified 3D point is inside the network boundary (PolygonOnSphere).
			 *
			 * This includes any interior rigid blocks.
			 *
			 * NOTE: This does not rely on the network triangulation (delaunay or constrained delaunay)
			 * and hence does not force the internal triangulations to be generated.
			 */
			bool
			is_point_in_network(
					const GPlatesMaths::PointOnSphere &point) const;

			//! Convenient overload.
			template <class Point2Type>
			bool
			is_point_in_network(
					const Point2Type &point_2) const
			{
				return is_point_in_network(d_projection.unproject_to_point_on_sphere(point_2));
			}


			/**
			 * Returns true if the specified 3D point is inside the network boundary (PolygonOnSphere)
			 * but outside any interior rigid blocks (also PolygonOnSphere's).
			 *
			 * NOTE: This does not rely on the network triangulation (delaunay or constrained delaunay)
			 * and hence does not force the internal triangulations to be generated.
			 */
			bool
			is_point_in_deforming_region(
					const GPlatesMaths::PointOnSphere &point) const;

			//! Convenient overload.
			template <class Point2Type>
			bool
			is_point_in_deforming_region(
					const Point2Type &point_2) const
			{
				return is_point_in_deforming_region(d_projection.unproject_to_point_on_sphere(point_2));
			}


			/**
			 * Returns true if the specified 3D point is inside any interior rigid blocks (PolygonOnSphere's).
			 *
			 * NOTE: This does not rely on the network triangulation (delaunay or constrained delaunay)
			 * and hence does not force the internal triangulations to be generated.
			 */
			boost::optional<const RigidBlock &>
			is_point_in_a_rigid_block(
					const GPlatesMaths::PointOnSphere &point) const;

			//! Convenient overload.
			template <class Point2Type>
			boost::optional<const RigidBlock &>
			is_point_in_a_rigid_block(
					const Point2Type &point_2) const
			{
				return is_point_in_a_rigid_block(d_projection.unproject_to_point_on_sphere(point_2));
			}


			/**
			 * Returns the natural neighbor coordinates of @a point in the *delaunay* triangulation
			 * (which can then be used with different interpolation methods like linear interpolation).
			 *
			 * Returns false if @a is_point_in_deforming_region returns false for @a point so there's no
			 * need to call @a is_point_in_deforming_region first.
			 */
			bool
			calc_delaunay_natural_neighbor_coordinates(
					delaunay_natural_neighbor_coordinates_2_type &natural_neighbor_coordinates,
					const GPlatesMaths::PointOnSphere &point) const;

			//! Convenient overload.
			template <class Point2Type>
			bool
			calc_delaunay_natural_neighbor_coordinates(
					delaunay_natural_neighbor_coordinates_2_type &natural_neighbor_coordinates,
					const Point2Type &point_2) const
			{
				return calc_delaunay_natural_neighbor_coordinates(
						natural_neighbor_coordinates,
						d_projection.unproject_to_point_on_sphere(point_2));
			}

			/**
			 * Returns the barycentric coordinates of @a point in the delaunay triangulation along
			 * with the face containing @a point.
			 *
			 * The coordinates sum to 1.0.
			 *
			 * Returns boost::none if @a is_point_in_deforming_region returns false for @a point so
			 * there's no need to call @a is_point_in_deforming_region first.
			 */
			boost::optional<Delaunay_2::Face_handle>
			calc_delaunay_barycentric_coordinates(
					delaunay_coord_2_type &barycentric_coord_vertex_1,
					delaunay_coord_2_type &barycentric_coord_vertex_2,
					delaunay_coord_2_type &barycentric_coord_vertex_3,
					const GPlatesMaths::PointOnSphere &point) const;

			//! Convenient overload.
			template <class Point2Type>
			bool
			calc_delaunay_barycentric_coordinates(
					delaunay_coord_2_type &barycentric_coord_vertex_1,
					delaunay_coord_2_type &barycentric_coord_vertex_2,
					delaunay_coord_2_type &barycentric_coord_vertex_3,
					const Point2Type &point_2) const
			{
				return calc_delaunay_barycentric_coordinates(
						barycentric_coord_vertex_1,
						barycentric_coord_vertex_2,
						barycentric_coord_vertex_3,
						d_projection.unproject_to_point_on_sphere(point_2));
			}


			/**
			 * Calculates the velocity at @a point in the network.
			 *
			 * If the point is inside the deforming region it will be interpolated using the
			 * delaunay triangulation.
			 * And if the point is inside an interior rigid block then the velocity will be
			 * calculated according to the rigid motion of that block (and the rigid block will be
			 * returned along with the velocity).
			 *
			 * Returns boost::none if the point is outside the network (if @a is_point_in_network returns false).
			 */
			boost::optional< std::pair<boost::optional<const RigidBlock &>, GPlatesMaths::Vector3D> >
			calculate_velocity(
					const GPlatesMaths::PointOnSphere &point,
					const double &velocity_delta_time = 1.0,
					VelocityDeltaTime::Type velocity_delta_time_type = VelocityDeltaTime::T_PLUS_DELTA_T_TO_T) const;

			//! Convenient overload.
			template <class Point2Type>
			boost::optional< std::pair<boost::optional<const RigidBlock &>, GPlatesMaths::Vector3D> >
			calculate_velocity(
					const Point2Type &point_2,
					const double &velocity_delta_time = 1.0,
					VelocityDeltaTime::Type velocity_delta_time_type = VelocityDeltaTime::T_PLUS_DELTA_T_TO_T) const
			{
				return calculate_velocity(
						d_projection.unproject_to_point_on_sphere(point_2),
						velocity_delta_time,
						velocity_delta_time_type);
			}


			/**
			 * Gets, or creates, 2D delaunay triangulation.
			 *
			 * The returned triangulation is *const* so that it cannot be modified such as
			 * inserting more vertices (because vertices and faces need to be initialised properly
			 * with the correct per-vertex or per-face data).
			 *
			 * NOTE: Creates delaunay triangulation if it hasn't yet been created.
			 * This enables the optimisation whereby the triangulation is not generated
			 * if it is never needed (accessed).
			 */
			const Delaunay_2 &
			get_delaunay_2() const;


			/**
			 * Gets, or creates, 2D constrained delaunay triangulation.
			 *
			 * The returned triangulation is *const* so that it cannot be modified such as
			 * inserting more vertices.
			 *
			 * NOTE: Creates constrained delaunay triangulation if it hasn't yet been created.
			 * This enables the optimisation whereby the triangulation is not generated
			 * if it is never needed (accessed).
			 */
			const ConstrainedDelaunay_2 &
			get_constrained_delaunay_2() const;


			/**
			 * Gets, or creates, 3D delaunay triangulation.
			 *
			 * The returned triangulation is *const* so that it cannot be modified such as
			 * inserting more vertices.
			 *
			 * NOTE: Creates constrained delaunay triangulation if it hasn't yet been created.
			 * This enables the optimisation whereby the triangulation is not generated
			 * if it is never needed (accessed).
			 */
			const Delaunay_3 &
			get_delaunay_3() const;

		private:

			/**
			 * Information used to build the internal triangulations.
			 *
			 * The information is stored in order to delay the building in case generation
			 * of triangulation(s) is never needed.
			 */
			struct BuildInfo
			{
				template <typename DelaunayPointIter,
						typename ConstrainedDelaunayGeometryIter,
						typename SeedPointOnSphereIter,
						typename ScatteredPointOnSphereIter>
				BuildInfo(
						DelaunayPointIter delaunay_points_begin_,
						DelaunayPointIter delaunay_points_end_,
						ConstrainedDelaunayGeometryIter constrained_delaunay_geometries_begin_,
						ConstrainedDelaunayGeometryIter constrained_delaunay_geometries_end_,
						ScatteredPointOnSphereIter scattered_points_begin_,
						ScatteredPointOnSphereIter scattered_points_end_,
						SeedPointOnSphereIter seed_points_begin_,
						SeedPointOnSphereIter seed_points_end_,
						const double &refined_mesh_shape_factor_,
						const double &refined_mesh_max_edge_) :
					delaunay_points(delaunay_points_begin_, delaunay_points_end_),
					constrained_delaunay_geometries(
							constrained_delaunay_geometries_begin_,
							constrained_delaunay_geometries_end_),
					scattered_points(scattered_points_begin_, scattered_points_end_),
					seed_points(seed_points_begin_, seed_points_end_),
					refined_mesh_shape_factor(refined_mesh_shape_factor_),
					refined_mesh_max_edge(refined_mesh_max_edge_)
				{  }

				std::vector<DelaunayPoint> delaunay_points;
				std::vector<ConstrainedDelaunayGeometry> constrained_delaunay_geometries;
				std::vector<GPlatesMaths::PointOnSphere> scattered_points;
				std::vector<GPlatesMaths::PointOnSphere> seed_points;

				double refined_mesh_shape_factor;
				double refined_mesh_max_edge;
			};


			//! Typedef for a mapping of 2D delaunay triangulation points to velocities.
			typedef std::map<delaunay_point_2_type, GPlatesMaths::Vector3D, delaunay_kernel_2_type::Less_xy_2>
					delaunay_point_2_to_velocity_map_type;
			/**
			 *
			 * NOTE: Avoid compiler warning 4503 'decorated name length exceeded' in Visual Studio 2008.
			 * Seems we get the warning, which gets (correctly) treated as error due to /WX switch,
			 * even if we disable the 4503 warning. So prevent warning by reducing name length of
			 * identifier - which we do by inheritance instead of using a typedef.
			 */
			struct DelaunayPoint2ToVelocityMapContainer :
					public delaunay_point_2_to_velocity_map_type
			{  };

			typedef std::pair<GPlatesMaths::Real, VelocityDeltaTime::Type> velocity_delta_time_params_type;

			//! Typedef for a mapping of velocity delta-time parameter to 2D delaunay triangulation points-to-velocities maps.
			typedef GPlatesUtils::KeyValueCache<velocity_delta_time_params_type, DelaunayPoint2ToVelocityMapContainer>
					velocity_delta_time_to_velocity_map_type;


			/**
			 * The reconstruction time this triangulation network was build at.
			 */
			double d_reconstruction_time;

			/**
			 * The polygon that bounds the network.
			 */
			GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type d_network_boundary_polygon;

			/**
			 * The rigid blocks inside the network.
			 */
			rigid_block_seq_type d_rigid_blocks;

			//! Used to project from 3D to 2D (for 2D triangulation).
			GPlatesMaths::AzimuthalEqualAreaProjection d_projection;

			/**
			 * Information used to build the internal triangulation (building delayed in case not needed).
			 */
			mutable BuildInfo d_build_info;

			/**
			 * 2D delaunay triangulation is only built if it's needed.
			 */
			mutable boost::optional<Delaunay_2> d_delaunay_2;

			/**
			 * 2D constrained delaunay triangulation is only built if it's needed.
			 */
			mutable boost::optional<ConstrainedDelaunay_2> d_constrained_delaunay_2;

			/**
			 * 3D delaunay triangulation is only built if it's needed.
			 */
			mutable boost::optional<Delaunay_3> d_delaunay_3;

			/**
			 * Maps velocity delta-time parameters to velocity maps.
			 *
			 * Each velocity map in turn stores the velocities at the delaunay triangulation points so they
			 * can be looked up during velocity interpolation between vertices of the delaunay triangulation.
			 */
			mutable velocity_delta_time_to_velocity_map_type d_velocity_delta_time_to_velocity_map;


			static const double EARTH_RADIUS_METRES;


			template <typename DelaunayPointIter,
					typename ConstrainedDelaunayGeometryIter,
					typename ScatteredPointOnSphereIter,
					typename SeedPointOnSphereIter,
					typename RigidBlockIter>
			Network(
					const double &reconstruction_time,
					const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type &network_boundary_polygon,
					DelaunayPointIter delaunay_points_begin,
					DelaunayPointIter delaunay_points_end,
					ConstrainedDelaunayGeometryIter constrained_delaunay_geometries_begin,
					ConstrainedDelaunayGeometryIter constrained_delaunay_geometries_end,
					ScatteredPointOnSphereIter scattered_points_begin_,
					ScatteredPointOnSphereIter scattered_points_end_,
					SeedPointOnSphereIter seed_points_begin,
					SeedPointOnSphereIter seed_points_end,
					RigidBlockIter rigid_blocks_begin,
					RigidBlockIter rigid_blocks_end,
					const double &refined_mesh_shape_factor,
					const double &refined_mesh_max_edge) :
				d_reconstruction_time(reconstruction_time),
				d_network_boundary_polygon(network_boundary_polygon),
				d_rigid_blocks(rigid_blocks_begin, rigid_blocks_end),
				d_projection(
						GPlatesMaths::PointOnSphere(network_boundary_polygon->get_boundary_centroid()),
						EARTH_RADIUS_METRES),
				d_build_info(
						delaunay_points_begin, delaunay_points_end,
						constrained_delaunay_geometries_begin, constrained_delaunay_geometries_end,
						scattered_points_begin_, scattered_points_end_,
						seed_points_begin, seed_points_end,
						refined_mesh_shape_factor, refined_mesh_max_edge),
				// Set the number of cached velocity maps (eg, for different velocity delta time parameters).
				//
				// A value of 2 is suitable since a network layer will typically be asked to use one
				// velocity delta time when rendering velocity layers while the export velocity
				// animation might override it and use another...
				d_velocity_delta_time_to_velocity_map(2/*maximum_num_values_in_cache*/)
			{  }

			void
			create_delaunay_2() const;

            void
			compute_spherical_delaunay_2() const;

            void
			smooth_delaunay_2() const;

			void
			create_constrained_delaunay_2() const;

			void
			insert_geometries_into_constrained_delaunay_2() const;

			void
			insert_scattered_points_into_constrained_delaunay_2(
					bool constrain_all_points) const;

			void
			insert_vertex_into_constrained_delaunay_2(
					const GPlatesMaths::PointOnSphere &point_on_sphere,
					std::vector<ConstrainedDelaunay_2::Vertex_handle> &vertex_handles) const;

			bool
			refine_constrained_delaunay_2() const;

			void
			make_conforming_constrained_delaunay_2() const;

			void
			create_delaunay_3() const;

			const delaunay_point_2_to_velocity_map_type &
			get_delaunay_point_2_to_velocity_map(
					const double &velocity_delta_time,
					VelocityDeltaTime::Type velocity_delta_time_type) const;

			void
			create_delaunay_point_2_to_velocity_map(
					delaunay_point_2_to_velocity_map_type &delaunay_point_2_to_velocity_map,
					const double &velocity_delta_time,
					VelocityDeltaTime::Type velocity_delta_time_type) const;

			/**
			 * Calculate the natural neighbour coorinates of the specified point.
			 *
			 * We should only be called if the point is in the deforming region.
			 */
			void
			calc_delaunay_natural_neighbor_coordinates_in_deforming_region(
					delaunay_natural_neighbor_coordinates_2_type &natural_neighbor_coordinates,
					const delaunay_point_2_type &point_2) const;

			/**
			 * Calculate the barycentric coordinates of the specified point and return the
			 * face containing the point.
			 *
			 * We should only be called if the point is in the deforming region.
			 */
			Delaunay_2::Face_handle
			calc_delaunay_barycentric_coordinates_in_deforming_region(
					delaunay_coord_2_type &barycentric_coord_vertex_1,
					delaunay_coord_2_type &barycentric_coord_vertex_2,
					delaunay_coord_2_type &barycentric_coord_vertex_3,
					const delaunay_point_2_type &point_2) const;

			/**
			 * Find the delaunay convex hull edge that is closest to the specified point
			 * (where one of the edge end points is also the closest triangulation vertex to the
			 * specified point).
			 *
			 * Note that the specified point is assumed to be outside the delaunay triangulation.
			 *
			 * Returns the edge end vertices - the second vertex is boost::none if unable to find closest edge.
			 */
			std::pair<
					Delaunay_2::Vertex_handle/*closest vertex*/,
					boost::optional<Delaunay_2::Vertex_handle>/*end vertex of closest edge*/ >
			get_closest_delaunay_convex_hull_edge(
					const delaunay_point_2_type &point_2) const;

			GPlatesMaths::Vector3D
			calculate_rigid_block_velocity(
					const GPlatesMaths::PointOnSphere &point,
					const RigidBlock &rigid_block,
					const double &velocity_delta_time,
					VelocityDeltaTime::Type velocity_delta_time_type) const;
		};
	}
}

#endif // GPLATES_APP_LOGIC_RESOLVEDTRIANGULATIONNETWORK_H
