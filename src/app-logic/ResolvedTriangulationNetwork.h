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

#include <cmath>
#include <functional>
#include <map>
#include <utility>
#include <vector>
#include <boost/function.hpp>
#include <boost/optional.hpp>
#include <boost/ref.hpp>
#include <boost/variant.hpp>
#include <QPointF>

#include "GeometryUtils.h"
#include "ReconstructedFeatureGeometry.h"
#include "ReconstructionTreeCreator.h"
#include "ResolvedTriangulationDelaunay2.h"
#include "ResolvedVertexSourceInfo.h"
#include "TopologyNetworkParams.h"
#include "VelocityDeltaTime.h"

#include "maths/AngularExtent.h"
#include "maths/AzimuthalEqualAreaProjection.h"
#include "maths/FiniteRotation.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/UnitVector3D.h"
#include "maths/Vector3D.h"

#include "model/types.h"

#include "utils/Earth.h"
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


			//! Typedef for location of a point within network (either inside a delaunay face or a rigid block).
			typedef boost::variant<
					Delaunay_2::Face_handle,
					boost::reference_wrapper<const RigidBlock> // behaves like 'const RigidBlock &'
			> point_location_type;


			/**
			 * Information from a topological section to store in a vertex in the delaunay
			 * triangulation when it is created.
			 */
			struct DelaunayPoint
			{
			public:

				DelaunayPoint(
						const GPlatesMaths::PointOnSphere &point_,
						const ResolvedVertexSourceInfo::non_null_ptr_to_const_type &shared_source_info_) :
					point(point_),
					shared_source_info(shared_source_info_)
				{  }

				//! The reconstructed point-on-sphere associated with the delaunay vertex.
				GPlatesMaths::PointOnSphere point;

				// Shared information used to reconstruct geometry that this point came from.
				ResolvedVertexSourceInfo::non_null_ptr_to_const_type shared_source_info;
			};


			/**
			 * Feature properties if this network is a rift.
			 *
			 * A network is a rift if the network feature has rift left/right plate IDs.
			 */
			struct Rift
			{
			public:

				Rift(
						GPlatesModel::integer_plate_id_type left_plate_id_,
						GPlatesModel::integer_plate_id_type right_plate_id_,
						boost::optional<double> exponential_stretching_constant_ = boost::none,
						boost::optional<double> strain_rate_resolution_ = boost::none,
						boost::optional<GPlatesMaths::AngularExtent> edge_length_threshold_ = boost::none) :
					left_plate_id(left_plate_id_),
					right_plate_id(right_plate_id_),
					exponential_stretching_constant(exponential_stretching_constant_),
					strain_rate_resolution(strain_rate_resolution_),
					edge_length_threshold(edge_length_threshold_)
				{  }

				//! The plate IDs of the rigid un-stretched crust on either side of the rift.
				GPlatesModel::integer_plate_id_type left_plate_id;
				GPlatesModel::integer_plate_id_type right_plate_id;

				boost::optional<double> exponential_stretching_constant;
				boost::optional<double> strain_rate_resolution;
				boost::optional<GPlatesMaths::AngularExtent> edge_length_threshold;
			};


			/**
			 * Creates a @a Network.
			 */
			template <typename DelaunayPointIter, typename RigidBlockIter>
			static
			non_null_ptr_type
			create(
					const double &reconstruction_time,
					const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type &network_boundary_polygon,
					DelaunayPointIter delaunay_points_begin,
					DelaunayPointIter delaunay_points_end,
					RigidBlockIter rigid_blocks_begin,
					RigidBlockIter rigid_blocks_end,
					const TopologyNetworkParams &topology_network_params,
					boost::optional<Rift> rift = boost::none)
			{
				return non_null_ptr_type(
						new Network(
								reconstruction_time,
								network_boundary_polygon,
								delaunay_points_begin, delaunay_points_end,
								rigid_blocks_begin, rigid_blocks_end,
								topology_network_params,
								rift));
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
			 * Returns the polygon that bounds the network with the rigid blocks (if any) as interior holes.
			 */
			GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type
			get_boundary_polygon_with_rigid_block_holes() const;


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

			//! Convenient overload for 2D projected point.
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

			//! Convenient overload for 2D projected point.
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

			//! Convenient overload for 2D projected point.
			template <class Point2Type>
			boost::optional<const RigidBlock &>
			is_point_in_a_rigid_block(
					const Point2Type &point_2) const
			{
				return is_point_in_a_rigid_block(d_projection.unproject_to_point_on_sphere(point_2));
			}


			/**
			 * Returns the location of the specified 3D point within network (either inside a delaunay face or a rigid block).
			 *
			 * Return none if point is outside the network.
			 */
			boost::optional<point_location_type>
			get_point_location(
					const GPlatesMaths::PointOnSphere &point) const;

			//! Convenient overload for 2D projected point.
			template <class Point2Type>
			boost::optional<point_location_type>
			get_point_location(
					const Point2Type &point_2) const
			{
				return get_point_location(d_projection.unproject_to_point_on_sphere(point_2));
			}


			/**
			 * Returns the natural neighbor coordinates of @a point in the *delaunay* triangulation
			 * (which can then be used with different interpolation methods like linear interpolation).
			 *
			 * @a start_face_hint is an optional optimisation if you already know the delaunay face
			 * containing the point (or near the point).
			 *
			 * Returns false if @a is_point_in_deforming_region returns false for @a point so there's no
			 * need to call @a is_point_in_deforming_region first.
			 */
			bool
			calc_delaunay_natural_neighbor_coordinates(
					delaunay_natural_neighbor_coordinates_2_type &natural_neighbor_coordinates,
					const GPlatesMaths::PointOnSphere &point,
					Delaunay_2::Face_handle start_face_hint = Delaunay_2::Face_handle()) const;

			//! Convenient overload for 2D projected point.
			template <class Point2Type>
			bool
			calc_delaunay_natural_neighbor_coordinates(
					delaunay_natural_neighbor_coordinates_2_type &natural_neighbor_coordinates,
					const Point2Type &point_2,
					Delaunay_2::Face_handle start_face_hint = Delaunay_2::Face_handle()) const
			{
				return calc_delaunay_natural_neighbor_coordinates(
						natural_neighbor_coordinates,
						d_projection.unproject_to_point_on_sphere(point_2),
						start_face_hint);
			}

			/**
			 * Returns the barycentric coordinates of @a point in the delaunay triangulation along
			 * with the face containing @a point.
			 *
			 * @a start_face_hint is an optional optimisation if you already know the delaunay face
			 * containing the point (or near the point).
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
					const GPlatesMaths::PointOnSphere &point,
					Delaunay_2::Face_handle start_face_hint = Delaunay_2::Face_handle()) const;

			//! Convenient overload for 2D projected point.
			template <class Point2Type>
			boost::optional<Delaunay_2::Face_handle>
			calc_delaunay_barycentric_coordinates(
					delaunay_coord_2_type &barycentric_coord_vertex_1,
					delaunay_coord_2_type &barycentric_coord_vertex_2,
					delaunay_coord_2_type &barycentric_coord_vertex_3,
					const Point2Type &point_2,
					Delaunay_2::Face_handle start_face_hint = Delaunay_2::Face_handle()) const
			{
				return calc_delaunay_barycentric_coordinates(
						barycentric_coord_vertex_1,
						barycentric_coord_vertex_2,
						barycentric_coord_vertex_3,
						d_projection.unproject_to_point_on_sphere(point_2),
						start_face_hint);
			}


			/**
			 * Returns whether deformation strain rates are smoothed and how.
			 *
			 * Unsmoothed strain rates are constant across each face.
			 */
			TopologyNetworkParams::StrainRateSmoothing
			get_strain_rate_smoothing() const
			{
				return d_build_info.topology_network_params.get_strain_rate_smoothing();
			}

			/**
			 * Returns whether deformation strain rates are clamped (and, if so, by how much).
			 */
			TopologyNetworkParams::StrainRateClamping
			get_strain_rate_clamping() const
			{
				return d_build_info.topology_network_params.get_strain_rate_clamping();
			}

			/**
			 * Calculates the deformation at @a point in the network interpolated using
			 * natural neighbour coordinates (if @a get_strain_rate_smoothing returns NATURAL_NEIGHBOUR_SMOOTHING),
			 * or barycentric coordinates (if BARYCENTRIC_SMOOTHING), or using the constant (non-smoothed)
			 * strain rate of the face that @a point is inside (if NO_SMOOTHING).
			 *
			 * @a point_location is an optional optimisation if you already know the location of @a point
			 * (delaunay face or rigid block containing the point).
			 *
			 * Returns boost::none if the point is outside the network (if @a is_point_in_network returns false).
			 */
			boost::optional<DeformationInfo>
			calculate_deformation(
					const GPlatesMaths::PointOnSphere &point,
					boost::optional<point_location_type> point_location = boost::none) const;

			//! Convenient overload for 2D projected point.
			template <class Point2Type>
			boost::optional<DeformationInfo>
			calculate_deformation(
					const Point2Type &point_2,
					boost::optional<point_location_type> point_location = boost::none) const
			{
				return calculate_deformation(
						d_projection.unproject_to_point_on_sphere(point_2),
						point_location);
			}

			/**
			 * Same as @a calculate_deformation except assumes @a point is inside the deforming region
			 * (ie, does not check if point is outside the network or inside a rigid block).
			 *
			 * The point could be:
			 * (1) inside the deforming region (which is typically smaller than the convex hull of the delaunay triangulation), or
			 * (2) inside the convex hull of the delaunay triangulation (but outside the deforming region), or
			 * (3) outside the convex hull of the delaunay triangulation.
			 *
			 * In cases (1) and (2) the delaunay triangulation is used, and in case (3) the nearest point
			 * on the nearest edge of the convex hull is used.
			 *
			 * This is useful when tessellating the triangulation in the deforming region for visual display for example.
			 * In this case if the tessellation is done using great circle arcs (in 3D) instead of using
			 * lines (in 2D projection) then the tessellated arcs can fall slightly outside the deforming region.
			 * However we don't want to exclude them, instead we want to consider them as very close to
			 * the deforming region boundary (which uses case (2) or (3) above).
			 *
			 * Normally you should use @a calculate_deformation instead since it returns none if the
			 * point is outside the deforming region. However the above example shows a situation
			 * where calling this method is preferable.
			 */
			DeformationInfo
			calculate_deformation_in_deforming_region(
					const GPlatesMaths::PointOnSphere &point,
					Delaunay_2::Face_handle start_face_hint = Delaunay_2::Face_handle()) const
			{
				return calculate_deformation_in_deforming_region(
						d_projection.project_from_point_on_sphere<Delaunay_2::Point>(point),
						start_face_hint);
			}

			//! Overload for 2D projected point.
			template <class Point2Type>
			DeformationInfo
			calculate_deformation_in_deforming_region(
					const Point2Type &point_2,
					Delaunay_2::Face_handle start_face_hint = Delaunay_2::Face_handle()) const
			{
				return calculate_deformation_in_deforming_region(
						Delaunay_2::Point(point_2.x(), point_2.y()),
						start_face_hint);
			}

			//! Overload for 2D projected point of type 'Delaunay_2::Point'.
			DeformationInfo
			calculate_deformation_in_deforming_region(
					const Delaunay_2::Point	&point_2,
					Delaunay_2::Face_handle start_face_hint) const;


			/**
			 * Calculates the position that @a point deforms to.
			 *
			 * If @a point is inside the deforming region it will be deformed by the deformation of the
			 * triangle containing it (and nearby triangles if using natural neighbour interpolation) and
			 * that delaunay face will be returned along with the point.
			 * And if the point is inside an interior rigid block then it will be rigidly rotated with that block
			 * (and the rigid block will be returned along with it).
			 *
			 * By default the @a point is deformed backwards in time from
			 * 'reconstruction_time' to 'reconstruction_time + time_increment', where
			 * 'reconstruction_time' is the time of this network (passed into @a create).
			 * However if @a reverse_deform is true then the @a point is deformed forward in time from
			 * 'reconstruction_time' to 'reconstruction_time - time_increment.
			 *
			 * Note that @a time_increment should always be positive, regardless of @a reverse_deform.
			 *
			 * If @a use_natural_neighbour_interpolation is true and @a point lies within the deforming
			 * region, then interpolation of the natural neighbour deformed vertex positions is used,
			 * otherwise barycentric interpolation is used.
			 *
			 * @a point_location is an optional optimisation if you already know the location of @a point
			 * (delaunay face or rigid block containing the point).
			 *
			 * Returns boost::none if the point is outside the network (if @a is_point_in_network returns false).
			 */
			boost::optional< std::pair<GPlatesMaths::PointOnSphere, point_location_type> >
			calculate_deformed_point(
					const GPlatesMaths::PointOnSphere &point,
					const double &time_increment = 1.0,
					bool reverse_deform = false,
					bool use_natural_neighbour_interpolation = true,
					boost::optional<point_location_type> point_location = boost::none) const;

			//! Convenient overload for 2D projected point.
			template <class Point2Type>
			boost::optional< std::pair<GPlatesMaths::PointOnSphere, point_location_type> >
			calculate_deformed_point(
					const Point2Type &point_2,
					const double &time_increment = 1.0,
					bool reverse_deform = false,
					bool use_natural_neighbour_interpolation = true,
					boost::optional<point_location_type> point_location = boost::none) const
			{
				return calculate_deformed_point(
						d_projection.unproject_to_point_on_sphere(point_2),
						time_increment,
						reverse_deform,
						use_natural_neighbour_interpolation,
						point_location);
			}


			/**
			 * Calculates the stage rotation at @a point in the network interpolated using barycentric coordinates.
			 *
			 * Note that generally it is better to use @a calculate_velocity, rather than @a calculate_stage_rotation
			 * and then calculating velocity from that (eg, using GPlatesMaths::calculate_velocity_vector).
			 * In most situations both approaches appear to give the same results, but there are situations where they don't.
			 * For example, picture a line between two vertices V1 and V2, and the stage rotation pole R1 (of V1)
			 * is on the line and close to V1, and R1 is such that the velocity at V1 is towards North and R2 is such
			 * that the velocity at V2 is also towards North. Hence we expect the interpolated velocity at any point
			 * between V1 and V2 to also point towards North. However as we move our position from V1 to V2, and also
			 * interpolate between R1 and R2, the interpolated position will pass through the pole of R1. Since the pole
			 * of R1 is close to V1, the interpolated rotation will be similar to R1. And so the interpolated position will
			 * also pass through the pole of the interpolated rotation, and the direction that the position rotates around the
			 * interpolated rotation pole will invert from Northwards to Southwards. Therefore if we calculated the velocity
			 * from this interpolated stage rotation it would give an unexpected result (a Southwards velocity direction).
			 *
			 * If the point is inside the deforming region it will be interpolated using the delaunay triangulation
			 * (and the delaunay face will be returned along with the stage rotation).
			 * And if the point is inside an interior rigid block then the stage rotation will be
			 * calculated according to the rigid motion of that block (and the rigid block will be
			 * returned along with the stage rotation).
			 *
			 * @a point_location is an optional optimisation if you already know the location of @a point
			 * (delaunay face or rigid block containing the point).
			 *
			 * Note that the stage rotation is forward in time. It is determined by the reconstruction time of
			 * this network and @a velocity_delta_time and @a velocity_delta_time_type.
			 *
			 * Returns boost::none if the point is outside the network (if @a is_point_in_network returns false).
			 */
			boost::optional< std::pair<GPlatesMaths::FiniteRotation, point_location_type> >
			calculate_stage_rotation(
					const GPlatesMaths::PointOnSphere &point,
					const double &velocity_delta_time = 1.0,
					VelocityDeltaTime::Type velocity_delta_time_type = VelocityDeltaTime::T_PLUS_DELTA_T_TO_T,
					boost::optional<point_location_type> point_location = boost::none) const;

			//! Convenient overload for 2D projected point.
			template <class Point2Type>
			boost::optional< std::pair<GPlatesMaths::FiniteRotation, point_location_type> >
			calculate_stage_rotation(
					const Point2Type &point_2,
					const double &velocity_delta_time = 1.0,
					VelocityDeltaTime::Type velocity_delta_time_type = VelocityDeltaTime::T_PLUS_DELTA_T_TO_T,
					boost::optional<point_location_type> point_location = boost::none) const
			{
				return calculate_stage_rotation(
						d_projection.unproject_to_point_on_sphere(point_2),
						velocity_delta_time,
						velocity_delta_time_type,
						point_location);
			}


			/**
			 * Calculates the velocity at @a point in the network interpolated using natural neighbour coordinates.
			 *
			 * If the point is inside the deforming region it will be interpolated using the delaunay triangulation.
			 * And if the point is inside an interior rigid block then the velocity will be
			 * calculated according to the rigid motion of that block (and the rigid block will be
			 * returned along with the velocity).
			 *
			 * @a point_location is an optional optimisation if you already know the location of @a point
			 * (delaunay face or rigid block containing the point).
			 *
			 * Returns boost::none if the point is outside the network (if @a is_point_in_network returns false).
			 */
			boost::optional< std::pair<GPlatesMaths::Vector3D, boost::optional<const RigidBlock &> > >
			calculate_velocity(
					const GPlatesMaths::PointOnSphere &point,
					const double &velocity_delta_time = 1.0,
					VelocityDeltaTime::Type velocity_delta_time_type = VelocityDeltaTime::T_PLUS_DELTA_T_TO_T,
					boost::optional<point_location_type> point_location = boost::none) const;

			//! Convenient overload for 2D projected point.
			template <class Point2Type>
			boost::optional< std::pair<GPlatesMaths::Vector3D, boost::optional<const RigidBlock &> > >
			calculate_velocity(
					const Point2Type &point_2,
					const double &velocity_delta_time = 1.0,
					VelocityDeltaTime::Type velocity_delta_time_type = VelocityDeltaTime::T_PLUS_DELTA_T_TO_T,
					boost::optional<point_location_type> point_location = boost::none) const
			{
				return calculate_velocity(
						d_projection.unproject_to_point_on_sphere(point_2),
						velocity_delta_time,
						velocity_delta_time_type,
						point_location);
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

		private:

			/**
			 * Information used to build the internal triangulations.
			 *
			 * The information is stored in order to delay the building in case generation
			 * of triangulation(s) is never needed.
			 */
			struct BuildInfo
			{
				/**
				 * Parameters to use when this network is a rift.
				 *
				 * A network is a rift if the network feature has special rift plate IDs.
				 */
				struct RiftParams
				{
					/**
					 * Whether either/both Delaunay edge vertices are on an un-stretched side of the rift.
					 */
					enum EdgeType
					{
						ONLY_FIRST_EDGE_VERTEX_ON_UNSTRETCHED_SIDE,
						ONLY_SECOND_EDGE_VERTEX_ON_UNSTRETCHED_SIDE,
						BOTH_EDGE_VERTICES_ON_OPPOSITE_UNSTRETCHED_SIDES
					};

					RiftParams(
							const Rift &rift,
							const TopologyNetworkParams &topology_network_params) :
						left_plate_id(rift.left_plate_id),
						right_plate_id(rift.right_plate_id),
						edge_length_threshold(rift.edge_length_threshold
								? rift.edge_length_threshold.get()
								: GPlatesMaths::AngularExtent::create_from_angle(
										GPlatesMaths::convert_deg_to_rad(
												topology_network_params.get_rift_params().edge_length_threshold_degrees))),
						strain_rate_resolution(rift.strain_rate_resolution
								? rift.strain_rate_resolution.get()
								: topology_network_params.get_rift_params().strain_rate_resolution),
						exponential_stretching_constant(rift.exponential_stretching_constant
								? rift.exponential_stretching_constant.get()
								: topology_network_params.get_rift_params().exponential_stretching_constant)
					{  }

					GPlatesModel::integer_plate_id_type left_plate_id;
					GPlatesModel::integer_plate_id_type right_plate_id;

					GPlatesMaths::AngularExtent edge_length_threshold;
					double strain_rate_resolution;
					double exponential_stretching_constant;
				};


				template <typename DelaunayPointIter>
				BuildInfo(
						DelaunayPointIter delaunay_points_begin_,
						DelaunayPointIter delaunay_points_end_,
						const TopologyNetworkParams &topology_network_params_,
						boost::optional<Rift> rift_) :
					delaunay_points(delaunay_points_begin_, delaunay_points_end_),
					topology_network_params(topology_network_params_)
				{
					if (rift_)
					{
						rift_params = RiftParams(rift_.get(), topology_network_params);
					}
				}

				std::vector<DelaunayPoint> delaunay_points;
				TopologyNetworkParams topology_network_params;
				boost::optional<RiftParams> rift_params;
			};


			//! Typedef for a mapping of 2D delaunay triangulation points to delaunay vertex handles.
			typedef std::map<delaunay_point_2_type, Delaunay_2::Vertex_handle, delaunay_kernel_2_type::Less_xy_2>
					delaunay_point_2_to_vertex_handle_map_type;


			/**
			 * Functor class for accessing function values at delaunay vertices.
			 */
			template <typename DataType>
			class UncachedDataAccess :
					public std::unary_function<
							delaunay_point_2_type,
							std::pair<DataType, bool> >
			{
			public:

				typedef DataType data_type;

				typedef boost::function< data_type (Delaunay_2::Vertex_handle) > get_data_function_type;

				UncachedDataAccess(
						const delaunay_point_2_to_vertex_handle_map_type &point_2_to_vertex_handle_map,
						const get_data_function_type &get_data_function) :
					d_point_2_to_vertex_handle_map(point_2_to_vertex_handle_map),
					d_get_data_function(get_data_function)
				{  }

				std::pair<data_type, bool>
				operator()(
						const delaunay_point_2_type &point_2) const
				{
					// Lookup the vertex handle using the point - we should always be able to find one.
					delaunay_point_2_to_vertex_handle_map_type::const_iterator vertex_handle_iter =
							d_point_2_to_vertex_handle_map.find(point_2);
					if (vertex_handle_iter == d_point_2_to_vertex_handle_map.end())
					{
						return std::make_pair(data_type(), false);
					}

					const Delaunay_2::Vertex_handle &vertex_handle = vertex_handle_iter->second;

					return std::make_pair(d_get_data_function(vertex_handle), true);
				}

			private:
				const delaunay_point_2_to_vertex_handle_map_type &d_point_2_to_vertex_handle_map;
				get_data_function_type d_get_data_function;
			};


			/**
			 * Functor class for accessing, and caching, function values at delaunay vertices.
			 */
			template <class VertexHandleToDataMapType>
			class CachedDataAccess :
					public std::unary_function<
							delaunay_point_2_type,
							std::pair<typename VertexHandleToDataMapType::mapped_type, bool> >
			{
			public:

				typedef typename VertexHandleToDataMapType::mapped_type data_type;

				typedef boost::function< data_type (Delaunay_2::Vertex_handle) > get_data_function_type;

				CachedDataAccess(
						VertexHandleToDataMapType &vertex_handle_to_data_map,
						const delaunay_point_2_to_vertex_handle_map_type &point_2_to_vertex_handle_map,
						const get_data_function_type &get_data_function) :
					d_vertex_handle_to_data_map(vertex_handle_to_data_map),
					d_point_2_to_vertex_handle_map(point_2_to_vertex_handle_map),
					d_get_data_function(get_data_function)
				{  }

				std::pair<data_type, bool>
				operator()(
						const delaunay_point_2_type &point_2) const
				{
					// Lookup the vertex handle using the point - we should always be able to find one.
					delaunay_point_2_to_vertex_handle_map_type::const_iterator vertex_handle_iter =
							d_point_2_to_vertex_handle_map.find(point_2);
					if (vertex_handle_iter == d_point_2_to_vertex_handle_map.end())
					{
						return std::make_pair(data_type(), false);
					}

					const Delaunay_2::Vertex_handle &vertex_handle = vertex_handle_iter->second;

					// Lookup the data value from the vertex handle.
					// If value not yet generated for vertex handle then do so now.
					std::pair<typename VertexHandleToDataMapType::iterator, bool> data_result =
							d_vertex_handle_to_data_map.insert(
									std::make_pair(vertex_handle, data_type()));
					if (data_result.second)
					{
						// A new data entry was inserted into the map.
						// Get the data value and store it in the map.
						data_result.first->second = d_get_data_function(vertex_handle);
					}

					return std::make_pair(data_result.first->second, true);
				}

			private:
				VertexHandleToDataMapType &d_vertex_handle_to_data_map;
				const delaunay_point_2_to_vertex_handle_map_type &d_point_2_to_vertex_handle_map;
				get_data_function_type d_get_data_function;
			};


			//! Typedef for velocity delta-time parameters.
			typedef std::pair<GPlatesMaths::Real, VelocityDeltaTime::Type> velocity_delta_time_params_type;


			/**
			 * Typedef for a mapping of 2D delaunay triangulation vertex handles to velocities.
			 *
			 * NOTE: Avoid compiler warning 4503 'decorated name length exceeded' in Visual Studio 2008.
			 * Seems we get the warning, which gets (correctly) treated as error due to /WX switch,
			 * even if we disable the 4503 warning. So prevent warning by reducing name length of
			 * identifier - which we do by inheritance instead of using a typedef.
			 */
			struct DelaunayVertexHandleToVelocityMapType :
					public std::map<Delaunay_2::Vertex_handle, GPlatesMaths::Vector3D>
			{  };

			//! Typedef for a mapping of velocity delta-time parameter to 2D delaunay triangulation vertices-to-velocities maps.
			typedef GPlatesUtils::KeyValueCache<velocity_delta_time_params_type, DelaunayVertexHandleToVelocityMapType>
					velocity_delta_time_to_velocity_map_type;


			/**
			 * Typedef for a mapping of 2D delaunay triangulation vertex handles to stage rotations.
			 *
			 * NOTE: Avoid compiler warning 4503 'decorated name length exceeded' in Visual Studio 2008.
			 * Seems we get the warning, which gets (correctly) treated as error due to /WX switch,
			 * even if we disable the 4503 warning. So prevent warning by reducing name length of
			 * identifier - which we do by inheritance instead of using a typedef.
			 */
			struct DelaunayVertexHandleToStageRotationMapType :
					public std::map<Delaunay_2::Vertex_handle, GPlatesMaths::FiniteRotation>
			{  };

			//! Typedef for a mapping of velocity delta-time parameter to 2D delaunay triangulation vertices-to-stage-rotations maps.
			typedef GPlatesUtils::KeyValueCache<velocity_delta_time_params_type, DelaunayVertexHandleToStageRotationMapType>
					velocity_delta_time_to_stage_rotation_map_type;


			/**
			 * Typedef for a mapping of 2D delaunay triangulation vertex handles to deformed 2D positions.
			 *
			 * NOTE: Avoid compiler warning 4503 'decorated name length exceeded' in Visual Studio 2008.
			 * Seems we get the warning, which gets (correctly) treated as error due to /WX switch,
			 * even if we disable the 4503 warning. So prevent warning by reducing name length of
			 * identifier - which we do by inheritance instead of using a typedef.
			 */
			struct DelaunayVertexHandleToDeformedPointMapType :
					// Using 'QPointF' instead of 'Delaunay_2::Point' so that we can use natural neighbour
					// interpolation (which expects addition of points and multiplication by scalars)...
					public std::map<Delaunay_2::Vertex_handle, QPointF>
			{  };

			//! Typedef for deformed position parameters.
			typedef std::pair<bool/*reverse_deform*/, velocity_delta_time_params_type> deformed_point_params_type;

			//! Typedef for a mapping of deformed position parameters to 2D delaunay triangulation vertices-to-deformed-positions maps.
			typedef GPlatesUtils::KeyValueCache<deformed_point_params_type, DelaunayVertexHandleToDeformedPointMapType>
					velocity_delta_time_to_deformed_point_map_type;


			/**
			 * The reconstruction time this triangulation network was build at.
			 */
			double d_reconstruction_time;

			/**
			 * The polygon that bounds the network.
			 */
			GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type d_network_boundary_polygon;

			/**
			 * The polygon that bounds the network with rigid blocks (if any) as interior holes.
			 *
			 * Only computed when first accessed.
			 */
			mutable boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> d_network_boundary_polygon_with_rigid_block_holes;

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
			 * Maps delaunay vertex points to vertex handles.
			 */
			mutable boost::optional<delaunay_point_2_to_vertex_handle_map_type> d_delaunay_point_2_to_vertex_handle_map;

			/**
			 * Maps velocity delta-time parameters to velocity maps.
			 *
			 * Each velocity map in turn stores the velocities at the delaunay triangulation vertices so they
			 * can be looked up during velocity interpolation between vertices of the delaunay triangulation.
			 */
			mutable velocity_delta_time_to_velocity_map_type d_velocity_delta_time_to_velocity_map;

			/**
			 * Maps velocity delta-time parameters to stage rotation maps.
			 *
			 * Each stage rotation map in turn stores the stage rotations at the delaunay triangulation vertices so they
			 * can be looked up during stage rotation interpolation between vertices of the delaunay triangulation.
			 */
			mutable velocity_delta_time_to_stage_rotation_map_type d_velocity_delta_time_to_stage_rotation_map;

			/**
			 * Maps velocity delta-time parameters to deformed position maps.
			 *
			 * Each deformed position map in turn stores the deformed positions at the delaunay triangulation vertices so they
			 * can be looked up during deformed position interpolation between vertices of the delaunay triangulation.
			 */
			mutable velocity_delta_time_to_deformed_point_map_type d_velocity_delta_time_to_deformed_point_map;


			template <typename DelaunayPointIter, typename RigidBlockIter>
			Network(
					const double &reconstruction_time,
					const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type &network_boundary_polygon,
					DelaunayPointIter delaunay_points_begin,
					DelaunayPointIter delaunay_points_end,
					RigidBlockIter rigid_blocks_begin_,
					RigidBlockIter rigid_blocks_end_,
					const TopologyNetworkParams &topology_network_params,
					boost::optional<Rift> rift) :
				d_reconstruction_time(reconstruction_time),
				d_network_boundary_polygon(network_boundary_polygon),
				d_rigid_blocks(rigid_blocks_begin_, rigid_blocks_end_),
				d_projection(
						GPlatesMaths::PointOnSphere(network_boundary_polygon->get_boundary_centroid()),
						1e3 * GPlatesUtils::Earth::MEAN_RADIUS_KMS/*Earth radius in metres*/),
				d_build_info(delaunay_points_begin, delaunay_points_end, topology_network_params, rift),
				// Set the number of cached velocity maps (eg, for different velocity delta time parameters).
				//
				// A value of 2 is suitable since a network layer will typically be asked to use one
				// velocity delta time when rendering velocity layers while the export velocity
				// animation might override it and use another...
				d_velocity_delta_time_to_velocity_map(2/*maximum_num_values_in_cache*/),
				d_velocity_delta_time_to_stage_rotation_map(2/*maximum_num_values_in_cache*/),
				d_velocity_delta_time_to_deformed_point_map(2/*maximum_num_values_in_cache*/)
			{  }

			void
			create_delaunay_2() const;

			void
			refine_rift_delaunay_2(
					const BuildInfo::RiftParams &rift_params,
					unsigned int vertex_index) const;

			void
			refine_rift_delaunay_edge(
					std::vector<DelaunayPoint> &delaunay_edge_point_seq,
					const GPlatesMaths::PointOnSphere &first_subdivided_edge_vertex_point,
					const GPlatesMaths::PointOnSphere &second_subdivided_edge_vertex_point,
					const GPlatesMaths::real_t &first_subdivided_edge_vertex_interpolation,
					const GPlatesMaths::real_t &second_subdivided_edge_vertex_interpolation,
					const GPlatesMaths::real_t &first_subdivided_edge_vertex_twist_interpolation,
					const GPlatesMaths::real_t &second_subdivided_edge_vertex_twist_interpolation,
					const GPlatesMaths::UnitVector3D &first_edge_vertex_stage_rotation_axis,
					const GPlatesMaths::UnitVector3D &second_edge_vertex_stage_rotation_axis,
					const GPlatesMaths::real_t &first_edge_vertex_stage_rotation_angle,
					const GPlatesMaths::real_t &second_edge_vertex_stage_rotation_angle,
					const GPlatesMaths::real_t &first_edge_vertex_twist_angle,
					const GPlatesMaths::real_t &second_edge_vertex_twist_angle,
					const GPlatesMaths::UnitVector3D &edge_rotation_axis,
					const GPlatesMaths::real_t &edge_angular_extent,
					const GPlatesMaths::real_t &subdivided_edge_angular_extent,
					const GPlatesMaths::UnitVector3D &twist_axis,
					const GPlatesMaths::UnitVector3D &twist_frame_x,
					const GPlatesMaths::UnitVector3D &twist_frame_y,
					const GPlatesMaths::real_t &inv_twist_angle_between_edge_vertices,
					const GPlatesMaths::real_t &twist_velocity_gradient,
					const BuildInfo::RiftParams::EdgeType rift_edge_type,
					const BuildInfo::RiftParams &rift_params,
					const ReconstructionTreeCreator &reconstruction_tree_creator) const;


			const delaunay_point_2_to_vertex_handle_map_type &
			get_delaunay_point_2_to_vertex_handle_map() const;

			void
			create_delaunay_point_2_to_vertex_handle_map(
					delaunay_point_2_to_vertex_handle_map_type &delaunay_point_2_to_vertex_handle_map) const;

			/**
			 * Calculate the natural neighbour coordinates of the specified point.
			 *
			 * @a start_face_hint is an optional optimisation if you already know the delaunay face containing the point.
			 *
			 * We should only be called if the point is in the deforming region.
			 */
			void
			calc_delaunay_natural_neighbor_coordinates_in_deforming_region(
					delaunay_natural_neighbor_coordinates_2_type &natural_neighbor_coordinates,
					const delaunay_point_2_type &point_2,
					Delaunay_2::Face_handle start_face_hint = Delaunay_2::Face_handle()) const;

			/**
			 * Calculate the barycentric coordinates of the specified point and return the
			 * face containing the point.
			 *
			 * @a start_face_hint is an optional optimisation if you already know the delaunay face containing the point.
			 *
			 * We should only be called if the point is in the deforming region.
			 */
			Delaunay_2::Face_handle
			calc_delaunay_barycentric_coordinates_in_deforming_region(
					delaunay_coord_2_type &barycentric_coord_vertex_1,
					delaunay_coord_2_type &barycentric_coord_vertex_2,
					delaunay_coord_2_type &barycentric_coord_vertex_3,
					const delaunay_point_2_type &point_2,
					Delaunay_2::Face_handle start_face_hint = Delaunay_2::Face_handle()) const;

			/**
			 * Find the delaunay face containing the specified point.
			 *
			 * @a start_face_hint is an optional optimisation if you already know the delaunay face containing the point.
			 *
			 * We should only be called if the point is in the deforming region.
			 */
			Delaunay_2::Face_handle
			get_delaunay_face_in_deforming_region(
					const delaunay_point_2_type &point_2,
					Delaunay_2::Face_handle start_face_hint = Delaunay_2::Face_handle()) const;

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

			/**
			 * Returns true if @a point is inside @a rigid_block.
			 */
			bool
			is_point_in_rigid_block(
					const GPlatesMaths::PointOnSphere &point,
					const GPlatesAppLogic::ResolvedTriangulation::Network::RigidBlock &rigid_block) const;

			/**
			 * Returns the stage rotation for the specified rigid block.
			 */
			GPlatesMaths::FiniteRotation
			calculate_rigid_block_stage_rotation(
					const RigidBlock &rigid_block,
					const double &velocity_delta_time,
					VelocityDeltaTime::Type velocity_delta_time_type) const;

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
