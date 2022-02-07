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

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstring> // for memset
#include <exception>
#include <limits>
#include <boost/bind.hpp>
#include <boost/utility/in_place_factory.hpp>
// Seems we need this to avoid compile problems on some CGAL versions
// (eg, 4.0.2 on Mac 10.6) for subsequent <CGAL/number_utils.h> include...
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/number_utils.h>
#include <CGAL/spatial_sort.h>
#include <CGAL/squared_distance_2.h>
#include <CGAL/Triangulation_2.h>
#include <CGAL/Delaunay_triangulation_2.h>
#include <CGAL/Triangle_2.h>
#include <QDebug>

#include "ResolvedTriangulationNetwork.h"
#include "RotationUtils.h"

#include "GeometryUtils.h"
#include "PlateVelocityUtils.h"
#include "ReconstructionTree.h"
#include "ResolvedTriangulationUtils.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "maths/AngularExtent.h"
#include "maths/CalculateVelocity.h"
#include "maths/FiniteRotation.h"
#include "maths/MathsUtils.h"
#include "maths/Real.h"

#include "utils/Profile.h"

#ifdef _MSC_VER
#ifndef copysign
#define copysign _copysign
#endif
#endif


namespace GPlatesAppLogic
{
	const double VELOCITY_DELTA_TIME = 1.0;
	const double INV_VELOCITY_DELTA_TIME = 1.0 / VELOCITY_DELTA_TIME;

	// Scale velocity values from kms/my to m/s.
	//const double VELOCITY_SCALE_KMS_MY_TO_M_S =
	//		1e-1/* kms/my -> cm/yr */ *
	//		(1.0 / 3.1536e+9)/* cm/yr to m/s */;

	// Scale 1/my -> 1/s.
	const double SCALE_PER_MY_TO_PER_SECOND =
			1e-6/* 1/my -> 1/yr */ *
			(1.0 / 3.1536e+7)/* 1/yr to 1/s */;


	//
	// CGAL typedefs.
	//
	// NOTE: Not putting these in an anonymous namespace because some compilers complain
	// about lack of external linkage when compiling CGAL.
	//

	/**
	 * Same as ResolvedTriangulation::Network::DelaunayPoint but stores a 2D point (instead of 3D)
	 * so can be spatially sorted by CGAL.
	 */
	struct DelaunayPoint2
	{

		DelaunayPoint2(
				const ResolvedTriangulation::Network::DelaunayPoint *delaunay_point_,
				const GPlatesMaths::LatLonPoint &lat_lon_point_,
				const ResolvedTriangulation::Delaunay_2::Point &point_2_) :
			delaunay_point(delaunay_point_),
			lat_lon_point(lat_lon_point_),
			point_2(point_2_)
		{  }

		//! The delaunay point information.
		const ResolvedTriangulation::Network::DelaunayPoint *delaunay_point;

		//! Lat/lon coordinates.
		GPlatesMaths::LatLonPoint lat_lon_point;

		//! The 2D projected point (azimuthal equal area projection).
		ResolvedTriangulation::Delaunay_2::Point point_2;

		struct LessX
		{
			bool
			operator()(
					const DelaunayPoint2 &lhs,
					const DelaunayPoint2 &rhs) const
			{
				return lhs.point_2.x() < rhs.point_2.x();
			}
		};

		struct LessY
		{
			bool
			operator()(
					const DelaunayPoint2 &lhs,
					const DelaunayPoint2 &rhs) const
			{
				return lhs.point_2.y() < rhs.point_2.y();
			}
		};
	};

	/**
	 * To assist CGAL::spatial_sort when sorting DelaunayPoint2 objects.
	 */
	struct DelaunayPoint2SpatialSortingTraits
	{
		typedef DelaunayPoint2 Point_2;

		typedef DelaunayPoint2::LessX Less_x_2;
		typedef DelaunayPoint2::LessY Less_y_2;

		Less_x_2
		less_x_2_object() const
		{
			return Less_x_2();
		}

		Less_y_2
		less_y_2_object() const
		{
			return Less_y_2();
		}
	};


	/**
	 * Calculate the velocity at a delaunay vertex.
	 */
	GPlatesMaths::Vector3D
	calc_delaunay_vertex_velocity(
			const ResolvedTriangulation::Delaunay_2::Vertex_handle &vertex_handle,
			const double &velocity_delta_time,
			VelocityDeltaTime::Type velocity_delta_time_type)
	{
		return vertex_handle->calc_velocity_vector(
				velocity_delta_time,
				velocity_delta_time_type);
	}


	/**
	 * Calculate the deformation at a delaunay vertex.
	 */
	ResolvedTriangulation::DeformationInfo
	calc_delaunay_vertex_deformation(
			const ResolvedTriangulation::Delaunay_2::Vertex_handle &vertex_handle)
	{
		return vertex_handle->get_deformation_info();
	}


	/**
	 * Calculate the deformed position of a delaunay vertex.
	 */
	QPointF
	calc_delaunay_vertex_deformed_point(
			const ResolvedTriangulation::Delaunay_2::Vertex_handle &vertex_handle,
			const double &time_increment,
			bool reverse_deform,
			VelocityDeltaTime::Type velocity_delta_time_type,
			const GPlatesMaths::AzimuthalEqualAreaProjection &projection)
	{
		const GPlatesMaths::FiniteRotation vertex_stage_rotation =
				vertex_handle->calc_stage_rotation(time_increment, velocity_delta_time_type);

		return projection.project_from_point_on_sphere<QPointF>(
				(reverse_deform ? vertex_stage_rotation : get_reverse(vertex_stage_rotation)) *
						vertex_handle->get_point_on_sphere());
	}
}


GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type
GPlatesAppLogic::ResolvedTriangulation::Network::get_boundary_polygon_with_rigid_block_holes() const
{
	// Create polygon if not already done so.
	if (!d_network_boundary_polygon_with_rigid_block_holes)
	{
		// Create a donut polygon version of the network boundary that includes rigid blocks as
		// interior holes if there are any.
		if (d_rigid_blocks.empty())
		{
			// No interior holes - so is the same as the boundary polygon without holes.
			d_network_boundary_polygon_with_rigid_block_holes = d_network_boundary_polygon;
		}
		else
		{
			std::vector< std::vector<GPlatesMaths::PointOnSphere> > rigid_block_interior_rings;
			rigid_block_interior_rings.reserve(d_rigid_blocks.size());

			// Iterate over the interior rigid blocks.
			rigid_block_seq_type::const_iterator rigid_blocks_iter = d_rigid_blocks.begin();
			rigid_block_seq_type::const_iterator rigid_blocks_end = d_rigid_blocks.end();
			for ( ; rigid_blocks_iter != rigid_blocks_end; ++rigid_blocks_iter)
			{
				const RigidBlock &rigid_block = *rigid_blocks_iter;

				boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> rigid_block_interior_polygon =
						GeometryUtils::get_polygon_on_sphere(
								*rigid_block.get_reconstructed_feature_geometry()->reconstructed_geometry());
				if (!rigid_block_interior_polygon)
				{
					continue;
				}

				rigid_block_interior_rings.push_back(std::vector<GPlatesMaths::PointOnSphere>());
				std::vector<GPlatesMaths::PointOnSphere> &rigid_block_interior_ring = rigid_block_interior_rings.back();
				rigid_block_interior_ring.insert(
						rigid_block_interior_ring.end(),
						rigid_block_interior_polygon.get()->exterior_ring_vertex_begin(),
						rigid_block_interior_polygon.get()->exterior_ring_vertex_end());
			}

			d_network_boundary_polygon_with_rigid_block_holes =
					GPlatesMaths::PolygonOnSphere::create_on_heap(
							d_network_boundary_polygon->exterior_ring_vertex_begin(),
							d_network_boundary_polygon->exterior_ring_vertex_end(),
							rigid_block_interior_rings.begin(),
							rigid_block_interior_rings.end());
		}
	}

	return d_network_boundary_polygon_with_rigid_block_holes.get();
}


bool
GPlatesAppLogic::ResolvedTriangulation::Network::is_point_in_network(
		const GPlatesMaths::PointOnSphere &point) const
{
	// Note that the medium and high speed point-in-polygon tests include a quick small circle
	// bounds test so we don't need to perform that test before the point-in-polygon test.

	return d_network_boundary_polygon->is_point_in_polygon(
			point,
			// Use high speed point-in-poly testing since we're being used for
			// we could be asked to test lots of points.
			// For example, very dense velocity meshes go through this path.
			GPlatesMaths::PolygonOnSphere::HIGH_SPEED_HIGH_SETUP_HIGH_MEMORY_USAGE);
}


bool
GPlatesAppLogic::ResolvedTriangulation::Network::is_point_in_deforming_region(
		const GPlatesMaths::PointOnSphere &point) const
{
	return is_point_in_network(point) && !is_point_in_a_rigid_block(point);
}


boost::optional<const GPlatesAppLogic::ResolvedTriangulation::Network::RigidBlock &>
GPlatesAppLogic::ResolvedTriangulation::Network::is_point_in_a_rigid_block(
		const GPlatesMaths::PointOnSphere &point) const
{
	// Iterate over the interior rigid blocks.
	rigid_block_seq_type::const_iterator rigid_blocks_iter = d_rigid_blocks.begin();
	rigid_block_seq_type::const_iterator rigid_blocks_end = d_rigid_blocks.end();
	for ( ; rigid_blocks_iter != rigid_blocks_end; ++rigid_blocks_iter)
	{
		const RigidBlock &rigid_block = *rigid_blocks_iter;
		if (is_point_in_rigid_block(point, rigid_block))
		{
			return rigid_block;
		}
	}

	return boost::none;
}


boost::optional<GPlatesAppLogic::ResolvedTriangulation::Network::point_location_type>
GPlatesAppLogic::ResolvedTriangulation::Network::get_point_location(
		const GPlatesMaths::PointOnSphere &point) const
{
	if (!is_point_in_network(point))
	{
		return boost::none;
	}

	// See if the point is inside any interior rigid blocks.
	boost::optional<const RigidBlock &> rigid_block = is_point_in_a_rigid_block(point);
	if (rigid_block)
	{
		return point_location_type(boost::cref(rigid_block.get()));
	}

	// If we get here then the point must be in the deforming region.

	// Project into the 2D triangulation space.
	const Delaunay_2::Point point_2 = d_projection.project_from_point_on_sphere<Delaunay_2::Point>(point);

	// Find the delaunay face containing the point.
	const Delaunay_2::Face_handle delaunay_face = get_delaunay_face_in_deforming_region(point_2);

	return point_location_type(delaunay_face);
}


bool
GPlatesAppLogic::ResolvedTriangulation::Network::is_point_in_rigid_block(
		const GPlatesMaths::PointOnSphere &point,
		const GPlatesAppLogic::ResolvedTriangulation::Network::RigidBlock &rigid_block) const
{
	boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> interior_polygon =
			GeometryUtils::get_polygon_on_sphere(
					*rigid_block.get_reconstructed_feature_geometry()->reconstructed_geometry());
	if (interior_polygon)
	{
		// Note that the medium and high speed point-in-polygon tests include a quick small circle
		// bounds test so we don't need to perform that test before the point-in-polygon test.

		if (interior_polygon.get()->is_point_in_polygon(
				point,
				// Use high speed point-in-poly testing since we're being used for
				// we could be asked to test lots of points.
				// For example, very dense velocity meshes go through this path.
				GPlatesMaths::PolygonOnSphere::HIGH_SPEED_HIGH_SETUP_HIGH_MEMORY_USAGE))
		{
			return true;
		}
	}

	return false;
}


bool
GPlatesAppLogic::ResolvedTriangulation::Network::calc_delaunay_natural_neighbor_coordinates(
		delaunay_natural_neighbor_coordinates_2_type &natural_neighbor_coordinates,
		const GPlatesMaths::PointOnSphere &point,
		Delaunay_2::Face_handle start_face_hint) const
{
	// We always classify points using 3D on-sphere tests.
	// This makes the boundary line up much better with adjacent topological polygons and also is a
	// faster test and can also prevent creation of triangulation if the point is outside the network.
	if (!is_point_in_deforming_region(point))
	{
		return false;
	}

	// Project into the 2D triangulation space.
	const Delaunay_2::Point point_2 =
			d_projection.project_from_point_on_sphere<Delaunay_2::Point>(point);

	// Get the interpolation coordinates for the projected point.
	calc_delaunay_natural_neighbor_coordinates_in_deforming_region(natural_neighbor_coordinates, point_2, start_face_hint);

	return true;
}


boost::optional<GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Face_handle>
GPlatesAppLogic::ResolvedTriangulation::Network::calc_delaunay_barycentric_coordinates(
		delaunay_coord_2_type &barycentric_coord_vertex_1,
		delaunay_coord_2_type &barycentric_coord_vertex_2,
		delaunay_coord_2_type &barycentric_coord_vertex_3,
		const GPlatesMaths::PointOnSphere &point,
		Delaunay_2::Face_handle start_face_hint) const
{
	// We always classify points using 3D on-sphere tests.
	// This makes the boundary line up much better with adjacent topological polygons and also is a
	// faster test and can also prevent creation of triangulation if the point is outside the network.
	if (!is_point_in_deforming_region(point))
	{
		return boost::none;
	}

	// Project into the 2D triangulation space.
	const Delaunay_2::Point point_2 =
			d_projection.project_from_point_on_sphere<Delaunay_2::Point>(point);

	// Get the barycentric coordinates for the projected point.
	return calc_delaunay_barycentric_coordinates_in_deforming_region(
			barycentric_coord_vertex_1,
			barycentric_coord_vertex_2,
			barycentric_coord_vertex_3,
			point_2,
			start_face_hint);
}


boost::optional<GPlatesAppLogic::ResolvedTriangulation::DeformationInfo>
GPlatesAppLogic::ResolvedTriangulation::Network::calculate_deformation(
		const GPlatesMaths::PointOnSphere &point,
		boost::optional<point_location_type> point_location) const
{
	// If already know the location of point.
	if (point_location)
	{
		Delaunay_2::Face_handle *delaunay_face = boost::get<Delaunay_2::Face_handle>(&point_location.get());
		if (delaunay_face == NULL)
		{
			return boost::none;
		}

		// Return zero strain rates for interior rigid blocks since no deformation there.
		return calculate_deformation_in_deforming_region(point, *delaunay_face);
	}

	// We always classify points using 3D on-sphere tests.
	// This makes the boundary line up much better with adjacent topological polygons and also is a
	// faster test and can also prevent creation of triangulation if the point is outside the network.
	if (!is_point_in_network(point))
	{
		return boost::none;
	}

	if (is_point_in_a_rigid_block(point))
	{
		// Return zero strain rates for interior rigid blocks since no deformation there.
		return DeformationInfo();
	}

	return calculate_deformation_in_deforming_region(point);
}


GPlatesAppLogic::ResolvedTriangulation::DeformationInfo
GPlatesAppLogic::ResolvedTriangulation::Network::calculate_deformation_in_deforming_region(
		const Delaunay_2::Point &point_2,
		Delaunay_2::Face_handle start_face_hint) const
{
	const TopologyNetworkParams::StrainRateSmoothing strain_rate_smoothing = get_strain_rate_smoothing();

	if (strain_rate_smoothing == TopologyNetworkParams::NO_SMOOTHING)
	{
		// We're not smoothing strain rates then just return the constant strain rate across the face (containing the point).
		const Delaunay_2::Face_handle face = get_delaunay_face_in_deforming_region(point_2, start_face_hint);
		return face->get_deformation_info();
	}

	if (strain_rate_smoothing == TopologyNetworkParams::BARYCENTRIC_SMOOTHING)
	{
		//
		// Smooth the strain rates using barycentric interpolation of the triangle's vertex strain rates.
		//

		// Get the interpolation coordinates for the point.
		delaunay_coord_2_type barycentric_coord_vertex_1;
		delaunay_coord_2_type barycentric_coord_vertex_2;
		delaunay_coord_2_type barycentric_coord_vertex_3;
		const Delaunay_2::Face_handle delaunay_face =
				calc_delaunay_barycentric_coordinates_in_deforming_region(
						barycentric_coord_vertex_1,
						barycentric_coord_vertex_2,
						barycentric_coord_vertex_3,
						point_2,
						start_face_hint);

		// Interpolate the deformation infos at the vertices of the triangle using the interpolation coordinates.
		return CGAL::to_double(barycentric_coord_vertex_1) * delaunay_face->vertex(0)->get_deformation_info() +
				CGAL::to_double(barycentric_coord_vertex_2) * delaunay_face->vertex(1)->get_deformation_info() +
				CGAL::to_double(barycentric_coord_vertex_3) * delaunay_face->vertex(2)->get_deformation_info();
	}

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			strain_rate_smoothing == TopologyNetworkParams::NATURAL_NEIGHBOUR_SMOOTHING,
			GPLATES_ASSERTION_SOURCE);

	//
	// Smooth the strain rates using natural neighbour interpolation of the nearby vertex strain rates.
	//

	// Get the interpolation coordinates for the point.
	delaunay_natural_neighbor_coordinates_2_type natural_neighbor_coordinates;
	calc_delaunay_natural_neighbor_coordinates_in_deforming_region(natural_neighbor_coordinates, point_2, start_face_hint);

	// Interpolate the deformation infos in the triangulation using the interpolation coordinates.
	return linear_interpolation_2(
			natural_neighbor_coordinates,
			// We don't need to cache the vertex deformations since, unlike velocities,
			// they are already cached inside the vertices...
			UncachedDataAccess<DeformationInfo>(
					get_delaunay_point_2_to_vertex_handle_map(),
					boost::bind(&calc_delaunay_vertex_deformation, _1)));
}


boost::optional<
		std::pair<
				GPlatesMaths::PointOnSphere,
				GPlatesAppLogic::ResolvedTriangulation::Network::point_location_type> >
GPlatesAppLogic::ResolvedTriangulation::Network::calculate_deformed_point(
		const GPlatesMaths::PointOnSphere &point,
		const double &time_increment,
		bool reverse_deform,
		bool use_natural_neighbour_interpolation,
		boost::optional<point_location_type> point_location) const
{
	if (!point_location &&
		!is_point_in_network(point))
	{
		return boost::none;
	}

	// Stage rotations are always forward in time as either:
	//
	//   reconstruction_time + time_increment -> reconstruction_time                  , or
	//   reconstruction_time                  -> reconstruction_time - time_increment .
	//
	// ...for 'reverse_deform' being false and true respectively (reverse deform means forward in time).
	//
	// However, when 'reverse_deform' is false (ie, going backward in time) we'll need to reverse its rotation to get:
	//
	//  backward in time:   reconstruction_time -> reconstruction_time + time_increment
	//  forward  in time:   reconstruction_time -> reconstruction_time - time_increment
	//
	// ...for 'reverse_deform' being false and true respectively (reverse deform means forward in time).
	//
	const VelocityDeltaTime::Type velocity_delta_time_type =
			reverse_deform ? VelocityDeltaTime::T_TO_T_MINUS_DELTA_T : VelocityDeltaTime::T_PLUS_DELTA_T_TO_T;

	// See if the point is inside any interior rigid blocks.
	boost::optional<const RigidBlock &> rigid_block;
	if (point_location)
	{
		const boost::reference_wrapper<const RigidBlock> *rigid_block_ptr =
				boost::get< boost::reference_wrapper<const RigidBlock> >(&point_location.get());
		if (rigid_block_ptr)
		{
			// We already know point is in a rigid block so use it.
			rigid_block = rigid_block_ptr->get();
		}
	}
	else
	{
		rigid_block = is_point_in_a_rigid_block(point);
	}
	if (rigid_block)
	{
		GPlatesMaths::FiniteRotation rigid_block_stage_rotation =
				calculate_rigid_block_stage_rotation(
						rigid_block.get(),
						time_increment,
						velocity_delta_time_type);

		// The stage rotation goes forward in time but if we are reconstructing backward
		// in time then we need to reverse the stage rotation.
		if (!reverse_deform)
		{
			rigid_block_stage_rotation = get_reverse(rigid_block_stage_rotation);
		}

		return std::make_pair(
				// Point is rigidly rotated by the interior rigid block...
				rigid_block_stage_rotation * point,
				point_location_type(boost::cref(rigid_block.get())));
	}

	// If we get here then the point must be in the deforming region.

	// Project into the 2D triangulation space.
	const Delaunay_2::Point point_2 =
			d_projection.project_from_point_on_sphere<Delaunay_2::Point>(point);

	if (use_natural_neighbour_interpolation)
	{
		Delaunay_2::Face_handle delaunay_face;
		if (point_location)
		{
			delaunay_face = boost::get<Delaunay_2::Face_handle>(point_location.get());
		}
		else
		{
			// Find the delaunay face containing the point.
			// We need to return a network position (delaunay face) and the natural neighbour interpolation
			// doesn't provide that. However it can use our delaunay face to find the coordinates faster.
			delaunay_face = get_delaunay_face_in_deforming_region(point_2);
		}

		// Get the interpolation coordinates for the point.
		delaunay_natural_neighbor_coordinates_2_type natural_neighbor_coordinates;
		calc_delaunay_natural_neighbor_coordinates_in_deforming_region(
				natural_neighbor_coordinates,
				point_2,
				delaunay_face);

		// Look for an existing map associated with the deformed point parameters.
		DelaunayVertexHandleToDeformedPointMapType &delaunay_vertex_handle_to_deformed_point_map =
				d_velocity_delta_time_to_deformed_point_map.get_value(
						std::make_pair(
								reverse_deform,
								std::make_pair(GPlatesMaths::Real(time_increment), velocity_delta_time_type)));

		// Interpolate the vertex deformed positions in the triangulation using the interpolation coordinates.
		const QPointF deformed_point_2 = linear_interpolation_2(
				natural_neighbor_coordinates,
				CachedDataAccess<DelaunayVertexHandleToDeformedPointMapType>(
						delaunay_vertex_handle_to_deformed_point_map,
						get_delaunay_point_2_to_vertex_handle_map(),
						boost::bind(&calc_delaunay_vertex_deformed_point,
								_1,
								time_increment,
								reverse_deform,
								velocity_delta_time_type,
								boost::cref(d_projection))));

		return std::make_pair(
				d_projection.unproject_to_point_on_sphere(deformed_point_2),
				point_location_type(delaunay_face));
	}
	// ...else use barycentric interpolation...

	Delaunay_2::Face_handle start_face_hint;
	if (point_location)
	{
		start_face_hint = boost::get<Delaunay_2::Face_handle>(point_location.get());
	}

	// Get the barycentric coordinates for the projected point.
	delaunay_coord_2_type barycentric_coord_vertex_1;
	delaunay_coord_2_type barycentric_coord_vertex_2;
	delaunay_coord_2_type barycentric_coord_vertex_3;
	const Delaunay_2::Face_handle delaunay_face =
			calc_delaunay_barycentric_coordinates_in_deforming_region(
					barycentric_coord_vertex_1,
					barycentric_coord_vertex_2,
					barycentric_coord_vertex_3,
					point_2,
					start_face_hint);

	// Look for an existing map associated with the velocity delta time parameters.
	DelaunayVertexHandleToDeformedPointMapType &delaunay_vertex_handle_to_deformed_point_map =
			d_velocity_delta_time_to_deformed_point_map.get_value(
					std::make_pair(
							reverse_deform,
							std::make_pair(GPlatesMaths::Real(time_increment), velocity_delta_time_type)));

	static const QPointF ZERO_POINT(0, 0);

	//
	// We deform the vertex positions (using their stage rotations), then project into 2D,
	// then interpolate 2D positions (using barycentric coordinates) and finally unproject to 3D.
	//
	// Note: The stage rotations go forward in time but if we are deforming backwards in time
	// then we need to reverse the stage rotation of each vertex.
	//

	// See if vertex value has been cached. If not then generate the value now.
	std::pair<DelaunayVertexHandleToDeformedPointMapType::iterator, bool> vertex_1_result =
			delaunay_vertex_handle_to_deformed_point_map.insert(
					std::make_pair(delaunay_face->vertex(0), ZERO_POINT));
	if (vertex_1_result.second)
	{
		vertex_1_result.first->second = calc_delaunay_vertex_deformed_point(
				vertex_1_result.first->first/*vertex_handle*/,
				time_increment,
				reverse_deform,
				velocity_delta_time_type,
				d_projection);
	}
	const QPointF &deformed_point_1 = vertex_1_result.first->second;

	// See if vertex value has been cached. If not then generate the value now.
	std::pair<DelaunayVertexHandleToDeformedPointMapType::iterator, bool> vertex_2_result =
			delaunay_vertex_handle_to_deformed_point_map.insert(
					std::make_pair(delaunay_face->vertex(1), ZERO_POINT));
	if (vertex_2_result.second)
	{
		vertex_2_result.first->second = calc_delaunay_vertex_deformed_point(
				vertex_2_result.first->first/*vertex_handle*/,
				time_increment,
				reverse_deform,
				velocity_delta_time_type,
				d_projection);
	}
	const QPointF &deformed_point_2 = vertex_2_result.first->second;

	// See if vertex value has been cached. If not then generate the value now.
	std::pair<DelaunayVertexHandleToDeformedPointMapType::iterator, bool> vertex_3_result =
			delaunay_vertex_handle_to_deformed_point_map.insert(
					std::make_pair(delaunay_face->vertex(2), ZERO_POINT));
	if (vertex_3_result.second)
	{
		vertex_3_result.first->second = calc_delaunay_vertex_deformed_point(
				vertex_3_result.first->first/*vertex_handle*/,
				time_increment,
				reverse_deform,
				velocity_delta_time_type,
				d_projection);
	}
	const QPointF &deformed_point_3 = vertex_3_result.first->second;

	// Interpolate the vertex deformed positions.
	const QPointF interpolated_deformed_point(
			CGAL::to_double(
					barycentric_coord_vertex_1 * deformed_point_1.x() +
					barycentric_coord_vertex_2 * deformed_point_2.x() +
					barycentric_coord_vertex_3 * deformed_point_3.x()),
			CGAL::to_double(
					barycentric_coord_vertex_1 * deformed_point_1.y() +
					barycentric_coord_vertex_2 * deformed_point_2.y() +
					barycentric_coord_vertex_3 * deformed_point_3.y()));

	return std::make_pair(
			d_projection.unproject_to_point_on_sphere(interpolated_deformed_point),
			point_location_type(delaunay_face));
}


boost::optional<
		std::pair<
				GPlatesMaths::FiniteRotation,
				GPlatesAppLogic::ResolvedTriangulation::Network::point_location_type> >
GPlatesAppLogic::ResolvedTriangulation::Network::calculate_stage_rotation(
		const GPlatesMaths::PointOnSphere &point,
		const double &velocity_delta_time,
		VelocityDeltaTime::Type velocity_delta_time_type,
		boost::optional<point_location_type> point_location) const
{
	if (!point_location &&
		!is_point_in_network(point))
	{
		return boost::none;
	}

	// See if the point is inside any interior rigid blocks.
	boost::optional<const RigidBlock &> rigid_block;
	if (point_location)
	{
		const boost::reference_wrapper<const RigidBlock> *rigid_block_ptr =
				boost::get< boost::reference_wrapper<const RigidBlock> >(&point_location.get());
		if (rigid_block_ptr)
		{
			// We already know point is in a rigid block so use it.
			rigid_block = rigid_block_ptr->get();
		}
	}
	else
	{
		rigid_block = is_point_in_a_rigid_block(point);
	}
	if (rigid_block)
	{
		const GPlatesMaths::FiniteRotation rigid_block_stage_rotation =
				calculate_rigid_block_stage_rotation(
						rigid_block.get(),
						velocity_delta_time,
						velocity_delta_time_type);

		return std::make_pair(
				rigid_block_stage_rotation,
				point_location_type(boost::cref(rigid_block.get())));
	}

	// If we get here then the point must be in the deforming region.

	Delaunay_2::Face_handle start_face_hint;
	if (point_location)
	{
		start_face_hint = boost::get<Delaunay_2::Face_handle>(point_location.get());
	}

	// Project into the 2D triangulation space.
	const Delaunay_2::Point point_2 =
			d_projection.project_from_point_on_sphere<Delaunay_2::Point>(point);

	// Get the barycentric coordinates for the projected point.
	delaunay_coord_2_type barycentric_coord_vertex_1;
	delaunay_coord_2_type barycentric_coord_vertex_2;
	delaunay_coord_2_type barycentric_coord_vertex_3;
	const Delaunay_2::Face_handle delaunay_face =
			calc_delaunay_barycentric_coordinates_in_deforming_region(
					barycentric_coord_vertex_1,
					barycentric_coord_vertex_2,
					barycentric_coord_vertex_3,
					point_2,
					start_face_hint);

	// Look for an existing map associated with the velocity delta time parameters.
	DelaunayVertexHandleToStageRotationMapType &delaunay_vertex_handle_to_stage_rotation_map =
			d_velocity_delta_time_to_stage_rotation_map.get_value(
					std::make_pair(GPlatesMaths::Real(velocity_delta_time), velocity_delta_time_type));

	static const GPlatesMaths::FiniteRotation IDENTITY_ROTATION = GPlatesMaths::FiniteRotation::create_identity_rotation();

	// See if vertex value has been cached. If not then generate the value now.
	std::pair<DelaunayVertexHandleToStageRotationMapType::iterator, bool> vertex_1_result =
			delaunay_vertex_handle_to_stage_rotation_map.insert(
					std::make_pair(delaunay_face->vertex(0), IDENTITY_ROTATION));
	if (vertex_1_result.second)
	{
		vertex_1_result.first->second = vertex_1_result.first->first->calc_stage_rotation(
				velocity_delta_time, velocity_delta_time_type);
	}
	const GPlatesMaths::FiniteRotation &stage_rotation_1 = vertex_1_result.first->second;

	// See if vertex value has been cached. If not then generate the value now.
	std::pair<DelaunayVertexHandleToStageRotationMapType::iterator, bool> vertex_2_result =
			delaunay_vertex_handle_to_stage_rotation_map.insert(
					std::make_pair(delaunay_face->vertex(1), IDENTITY_ROTATION));
	if (vertex_2_result.second)
	{
		vertex_2_result.first->second = vertex_2_result.first->first->calc_stage_rotation(
				velocity_delta_time, velocity_delta_time_type);
	}
	const GPlatesMaths::FiniteRotation &stage_rotation_2 = vertex_2_result.first->second;

	// See if vertex value has been cached. If not then generate the value now.
	std::pair<DelaunayVertexHandleToStageRotationMapType::iterator, bool> vertex_3_result =
			delaunay_vertex_handle_to_stage_rotation_map.insert(
					std::make_pair(delaunay_face->vertex(2), IDENTITY_ROTATION));
	if (vertex_3_result.second)
	{
		vertex_3_result.first->second = vertex_3_result.first->first->calc_stage_rotation(
				velocity_delta_time, velocity_delta_time_type);
	}
	const GPlatesMaths::FiniteRotation &stage_rotation_3 = vertex_3_result.first->second;

	// Interpolate the vertex stage rotations.
	const GPlatesMaths::FiniteRotation interpolated_stage_rotation =
			GPlatesMaths::interpolate(
					stage_rotation_1,
					stage_rotation_2,
					stage_rotation_3,
					CGAL::to_double(barycentric_coord_vertex_1),
					CGAL::to_double(barycentric_coord_vertex_2),
					CGAL::to_double(barycentric_coord_vertex_3));

	return std::make_pair(
			interpolated_stage_rotation,
			point_location_type(delaunay_face));
}


boost::optional<
		std::pair<
				GPlatesMaths::Vector3D,
				boost::optional<const GPlatesAppLogic::ResolvedTriangulation::Network::RigidBlock &> > >
GPlatesAppLogic::ResolvedTriangulation::Network::calculate_velocity(
		const GPlatesMaths::PointOnSphere &point,
		const double &velocity_delta_time,
		VelocityDeltaTime::Type velocity_delta_time_type,
		boost::optional<point_location_type> point_location) const
{
	if (!point_location &&
		!is_point_in_network(point))
	{
		return boost::none;
	}

	// See if the point is inside any interior rigid blocks.
	boost::optional<const RigidBlock &> rigid_block;
	if (point_location)
	{
		const boost::reference_wrapper<const RigidBlock> *rigid_block_ptr =
				boost::get< boost::reference_wrapper<const RigidBlock> >(&point_location.get());
		if (rigid_block_ptr)
		{
			// We already know point is in a rigid block so use it.
			rigid_block = rigid_block_ptr->get();
		}
	}
	else
	{
		rigid_block = is_point_in_a_rigid_block(point);
	}
	if (rigid_block)
	{
		const GPlatesMaths::Vector3D rigid_block_velocity =
				calculate_rigid_block_velocity(
						point,
						rigid_block.get(),
						velocity_delta_time,
						velocity_delta_time_type);

		return std::make_pair(rigid_block_velocity, rigid_block);
	}

	// If we get here then the point must be in the deforming region.

	Delaunay_2::Face_handle start_face_hint;
	if (point_location)
	{
		start_face_hint = boost::get<Delaunay_2::Face_handle>(point_location.get());
	}

	// Project into the 2D triangulation space.
	const Delaunay_2::Point point_2 =
			d_projection.project_from_point_on_sphere<Delaunay_2::Point>(point);

	// Get the interpolation coordinates for the point.
	delaunay_natural_neighbor_coordinates_2_type natural_neighbor_coordinates;
	calc_delaunay_natural_neighbor_coordinates_in_deforming_region(natural_neighbor_coordinates, point_2, start_face_hint);

	// Look for an existing map associated with the velocity delta time parameters.
	DelaunayVertexHandleToVelocityMapType &delaunay_vertex_handle_to_velocity_map =
			d_velocity_delta_time_to_velocity_map.get_value(
					std::make_pair(GPlatesMaths::Real(velocity_delta_time), velocity_delta_time_type));

	// Interpolate the 3D velocity vectors in the triangulation using the interpolation coordinates.
	// Velocity 3D vectors must be interpolated (cannot interpolate velocity colat/lon).
	const GPlatesMaths::Vector3D interpolated_velocity =
			linear_interpolation_2(
					natural_neighbor_coordinates,
					CachedDataAccess<DelaunayVertexHandleToVelocityMapType>(
							delaunay_vertex_handle_to_velocity_map,
							get_delaunay_point_2_to_vertex_handle_map(),
							boost::bind(&calc_delaunay_vertex_velocity,
									_1,
									velocity_delta_time,
									velocity_delta_time_type)));

	return std::make_pair(interpolated_velocity, rigid_block/*boost::none*/);
}


const GPlatesAppLogic::ResolvedTriangulation::Delaunay_2 &
GPlatesAppLogic::ResolvedTriangulation::Network::get_delaunay_2() const
{
	if (!d_delaunay_2)
	{
		create_delaunay_2();

		// We've finished inserting vertices into the Delaunay triangulation.
		// Each vertex can now compute its deformation strain rate by area averaging the strain rates
		// of the triangles that surround it...
		d_delaunay_2->set_finished_modifying_triangulation();

		// Release some build data memory since we don't need it anymore.
		std::vector<DelaunayPoint> empty_delaunay_points;
		d_build_info.delaunay_points.swap(empty_delaunay_points);
	}

	return d_delaunay_2.get();
}


void
GPlatesAppLogic::ResolvedTriangulation::Network::create_delaunay_2() const
{
	PROFILE_FUNC();

	d_delaunay_2 = boost::in_place(*this, d_reconstruction_time);

	// Project the points to 2D space and insert into array to be spatially sorted.
	std::vector<DelaunayPoint2> delaunay_point_2_seq;
	delaunay_point_2_seq.reserve(d_build_info.delaunay_points.size());
	std::vector<DelaunayPoint>::const_iterator delaunay_points_iter = d_build_info.delaunay_points.begin();
	std::vector<DelaunayPoint>::const_iterator delaunay_points_end = d_build_info.delaunay_points.end();
	for (; delaunay_points_iter != delaunay_points_end; ++delaunay_points_iter)
	{
		const DelaunayPoint &delaunay_point = *delaunay_points_iter;

		// Cache our lat/lon coordinates - otherwise the projection needs to convert to lat/lon
		// internally - and so might as well only do the lat/lon conversion once for efficiency.
		const GPlatesMaths::LatLonPoint lat_lon_point = make_lat_lon_point(delaunay_point.point);

		// Project point-on-sphere to x,y space.
		const delaunay_point_2_type point_2 =
			d_projection.project_from_lat_lon<delaunay_point_2_type>(lat_lon_point);

		delaunay_point_2_seq.push_back(DelaunayPoint2(&delaunay_point, lat_lon_point, point_2));
	}

	// Improve performance by spatially sorting the delaunay points.
	// This is what is done by the CGAL overload that inserts a *range* of points into a delauany triangulation.
	CGAL::spatial_sort(
		delaunay_point_2_seq.begin(),
		delaunay_point_2_seq.end(),
		DelaunayPoint2SpatialSortingTraits());

	// Insert the points into the delaunay triangulation.
	unsigned int vertex_index = 0;
	std::vector<DelaunayPoint2>::const_iterator delaunay_points_2_iter = delaunay_point_2_seq.begin();
	std::vector<DelaunayPoint2>::const_iterator delaunay_points_2_end = delaunay_point_2_seq.end();
	Delaunay_2::Face_handle insert_start_face;
	for (; delaunay_points_2_iter != delaunay_points_2_end; ++delaunay_points_2_iter)
	{
		const DelaunayPoint2 &delaunay_point_2 = *delaunay_points_2_iter;
		const DelaunayPoint &delaunay_point = *delaunay_point_2.delaunay_point;

		// Insert into the triangulation.
		Delaunay_2::Vertex_handle vertex_handle =
			d_delaunay_2->insert(delaunay_point_2.point_2, insert_start_face);

		if (!vertex_handle->is_initialised())
		{
			// Set the extra info for this vertex.
			vertex_handle->initialise(
				d_delaunay_2.get(),
				vertex_index,
				delaunay_point.point,
				delaunay_point_2.lat_lon_point,
				delaunay_point.shared_source_info);

			// Increment vertex index since vertex handle does not refer to an existing vertex position.
			++vertex_index;
		}
		else
		{
			// The returned vertex handle refers to an existing vertex.
			// This happens if the same position (presumably within an epsilon) is inserted more than once.
			// So here we equally blend the source infos of the two vertices.
			//
			// Note that the spatial sort above can change the order of vertex insertion along the topological
			// sub-segments of the network boundary for example - which can result in vertices at the intersection
			// between two adjacent sub-segments switching order of insertion from one reconstruction time to the next
			// (since both sub-segments have the same end point position) - however if we equally blend the source infos
			// of both vertices then this re-ordering should not affect us - ie, we shouldn't get randomly switching
			// end point velocities (from one sub-segment plate id to the other). It's possible this could
			// still be a problem if three of more vertices coincide, because we're assuming a max of two,
			// but it is extremely unlikely for this to happen.
			//
			// Create a source info that equally interpolates between the source info of the
			// existing vertex and the source info of the vertex we're attempting to add.
			const ResolvedVertexSourceInfo::non_null_ptr_to_const_type interpolated_source_info =
				ResolvedVertexSourceInfo::create(
					// Source info of existing vertex...
					get_non_null_pointer(&vertex_handle->get_shared_source_info()),
					// Source info of new vertex...
					delaunay_point.shared_source_info,
					0.5);  // equal blending

			// Reset the extra info for this vertex.
			vertex_handle->initialise(
				d_delaunay_2.get(),
				// Note: This is an existing vertex position so re-use the vertex index previously assigned...
				vertex_handle->get_vertex_index(),
				delaunay_point.point,
				delaunay_point_2.lat_lon_point,
				// Replace source info with the interpolated source info...
				interpolated_source_info);
		}

		// The next insert vertex will start searching at the face of the last inserted vertex.
		insert_start_face = vertex_handle->face();
	}

	//
	// Note that we don't need to initialise the faces.
	//
	// They get initialised when/if they are first accessed.
	//

	// If this deforming network represents a rift then adaptively refine the
	// Delaunay triangulation by inserting new vertices along subdivided edges with
	// velocities that result in a non-uniform strain rate profile across the rift
	// (instead of a uniform/constant strain rate that would lead to constant crustal
	// thinning right across the entire rift).
	if (d_build_info.rift_params)
	{
		refine_rift_delaunay_2(d_build_info.rift_params.get(), vertex_index);
	}
}


void
GPlatesAppLogic::ResolvedTriangulation::Network::refine_rift_delaunay_2(
		const BuildInfo::RiftParams &rift_params,
		unsigned int vertex_index) const
{
	PROFILE_FUNC();

	//qDebug() << "Num faces before rift refinement:" << d_delaunay_2->number_of_faces();

	// Obtain a ReconstructionTreeCreator from the first vertex in the triangulation.
	//
	// It's possible that the various topological sections making up this topological network
	// were reconstructed using different rotation layers (and hence ReconstructionTreeCreator's).
	// However that really shouldn't be the case, and if the user has set up the layers like that
	// then it is most likely been setup incorrectly. In any case it becomes ambiguous as to which
	// ReconstructionTreeCreator to use to calculate the rift stage pole.
	//
	if (d_delaunay_2->number_of_vertices() == 0)
	{
		return;
	}
	const ReconstructionTreeCreator reconstruction_tree_creator =
			d_delaunay_2->finite_vertices_begin()->get_shared_source_info().get_reconstruction_tree_creator();

	const GPlatesMaths::FiniteRotation right_plate_stage_rotation =
			PlateVelocityUtils::calculate_stage_rotation(
					rift_params.right_plate_id,
					reconstruction_tree_creator,
					d_reconstruction_time,
					VELOCITY_DELTA_TIME,
					VelocityDeltaTime::T_PLUS_DELTA_T_TO_T);

	const GPlatesMaths::FiniteRotation left_plate_stage_rotation =
			PlateVelocityUtils::calculate_stage_rotation(
					rift_params.left_plate_id,
					reconstruction_tree_creator,
					d_reconstruction_time,
					VELOCITY_DELTA_TIME,
					VelocityDeltaTime::T_PLUS_DELTA_T_TO_T);

	const ReconstructionTree::non_null_ptr_to_const_type reconstruction_tree_1 =
			reconstruction_tree_creator.get_reconstruction_tree(
					d_reconstruction_time);
	const ReconstructionTree::non_null_ptr_to_const_type reconstruction_tree_2 =
			reconstruction_tree_creator.get_reconstruction_tree(
					d_reconstruction_time + 1);

	// Get the stage pole for the rift stage rotation (from left to right plate over delta time).
	//
	// The stage pole is from 't1' to 't2', where 't1' is 't+1' and 't2' is 't'.
	const GPlatesMaths::FiniteRotation stage_pole =
			RotationUtils::get_stage_pole(
					*reconstruction_tree_2, // t1
					*reconstruction_tree_1, // t2
					rift_params.right_plate_id,
					rift_params.left_plate_id);

	// Get stage pole axis.
	if (represents_identity_rotation(stage_pole.unit_quat()))
	{
		// There's no rift stretching, so no need for rift triangulation refinement.
		return;
	}

	const GPlatesMaths::UnitVector3D stage_pole_axis =
			stage_pole.unit_quat().get_rotation_params(boost::none).axis;

	//
	// We can write "R(0->t2,A->R)" in terms of the stage rotation "R(t1->t2,L->R)" as:
	//             
	//     R(0->t2,A->R) = R(0->t2,A->L) * R(0->t2,L->R)
	//                 = R(0->t2,A->L) * R(t1->t2,L->R) * R(0->t1,L->R)
	//                 = R(0->t2,A->L) * stage_rotation * R(0->t1,L->R)
	//             
	// ...where 't1' is 't+1' and 't2' is 't' (ie, from 't1' to 't2').
	//             
	// So to get the stage pole axis of the stage rotation into the reference frame of
	// *reconstructed* geometries we need to rotate it by "R(0->t2,A->L)":
	//             
	//     reconstructed_geometry = R(0->t2,A->L) * stage_rotation * R(0->t1,L->R) * present_day_geometry
	//
	// Only then can be compare the stage pole axis with the reconstructed rift geometries in the network.
	//
	const GPlatesMaths::FiniteRotation left_plate_rotation =
			reconstruction_tree_1->get_composed_absolute_rotation(rift_params.left_plate_id);
	const GPlatesMaths::UnitVector3D twist_axis = left_plate_rotation * stage_pole_axis;


	//
	// Iterate over the edges of the Delaunay triangulation and recursively sub-divide to create
	// new triangulation vertices.
	//

	std::vector<DelaunayPoint> delaunay_edge_point_seq;

	Delaunay_2::Finite_edges_iterator finite_edges_2_iter = d_delaunay_2->finite_edges_begin();
	Delaunay_2::Finite_edges_iterator finite_edges_2_end = d_delaunay_2->finite_edges_end();
	for ( ; finite_edges_2_iter != finite_edges_2_end; ++finite_edges_2_iter)
	{
		// Get the two faces adjoining the current edge.
		const Delaunay_2::Face_handle face_handle[2] =
		{
			finite_edges_2_iter->first,
			finite_edges_2_iter->first->neighbor(finite_edges_2_iter->second)
		};

		// Iterate over both faces adjoining the current edge.
		unsigned int num_valid_faces = 0;
		for (unsigned int t = 0; t < 2; ++t)
		{
			// Skip infinite faces. These occur on convex hull edges.
			// Also skip faces with centroid outside deforming region.
			if (d_delaunay_2->is_infinite(face_handle[t]) ||
				!face_handle[t]->is_in_deforming_region())
			{
				continue;
			}

			++num_valid_faces;
		}

		// If both triangles (adjoining current edge) are outside deforming region (or an infinite face)
		// then the skip the current edge.
		if (num_valid_faces == 0)
		{
			continue;
		}

		// Get the edge vertices.
		const Delaunay_2::Vertex_handle first_edge_vertex_handle =
				finite_edges_2_iter->first->vertex(d_delaunay_2->cw(finite_edges_2_iter->second));
		const Delaunay_2::Vertex_handle second_edge_vertex_handle =
				finite_edges_2_iter->first->vertex(d_delaunay_2->ccw(finite_edges_2_iter->second));

		// Get the edge vertex positions.
		const GPlatesMaths::PointOnSphere &first_edge_vertex_point = first_edge_vertex_handle->get_point_on_sphere();
		const GPlatesMaths::PointOnSphere &second_edge_vertex_point = second_edge_vertex_handle->get_point_on_sphere();

		// If edge length is shorter than threshold distance then don't subdivide edge.
		const GPlatesMaths::AngularExtent edge_angular_extent =
				GPlatesMaths::AngularExtent::create_from_cosine(
						dot(first_edge_vertex_point.position_vector(), second_edge_vertex_point.position_vector()));
		if (edge_angular_extent.is_precisely_less_than(rift_params.edge_length_threshold))
		{
			continue;
		}
		const GPlatesMaths::UnitVector3D edge_rotation_axis = cross(
				first_edge_vertex_point.position_vector(),
				second_edge_vertex_point.position_vector()).get_normalisation();

		const ResolvedVertexSourceInfo::non_null_ptr_to_const_type first_edge_vertex_source_info =
				get_non_null_pointer(&first_edge_vertex_handle->get_shared_source_info());
		const ResolvedVertexSourceInfo::non_null_ptr_to_const_type second_edge_vertex_source_info =
				get_non_null_pointer(&second_edge_vertex_handle->get_shared_source_info());

		// Stage rotations used to generate velocities at the two edge vertices.
		const GPlatesMaths::FiniteRotation first_edge_vertex_stage_rotation =
				first_edge_vertex_source_info->get_stage_rotation(
						d_reconstruction_time, VELOCITY_DELTA_TIME, VelocityDeltaTime::T_PLUS_DELTA_T_TO_T);
		const GPlatesMaths::FiniteRotation second_edge_vertex_stage_rotation =
				second_edge_vertex_source_info->get_stage_rotation(
						d_reconstruction_time, VELOCITY_DELTA_TIME, VelocityDeltaTime::T_PLUS_DELTA_T_TO_T);

		//
		// Find out if either/both edge vertices are on an un-stretched side of the rift.
		//
		// This happens if one vertex has a stage rotation that matches either plate (left or right)
		// of the rift, and the other vertex does not match. The matching vertex has minimal stretching
		// (since near un-stretched side of rift) and un-matching vertex has maximal stretching
		// (since it is presumed to be near the centre of rifting).
		// 
		// However if both edge vertices match opposing sides of the rift then we need to create maximal
		// stretching in the *middle* of edge and minimal at either side (not yet handled below).
		// This happens when the edge crosses the entire rift and there are no vertices in the centre of the rift.
		//
		// We want minimal stretching rate at rift edge (un-stretched side of rift) and
		// maximal stretching at rift centre (that could eventually form mid-ocean ridge).
		//

		BuildInfo::RiftParams::EdgeType rift_edge_type;
		if (first_edge_vertex_stage_rotation.unit_quat() == right_plate_stage_rotation.unit_quat())
		{
			if (second_edge_vertex_stage_rotation.unit_quat() == left_plate_stage_rotation.unit_quat())
			{
				// Both edge vertices are on opposite un-stretched sides of rift.
				rift_edge_type = BuildInfo::RiftParams::BOTH_EDGE_VERTICES_ON_OPPOSITE_UNSTRETCHED_SIDES;
			}
			else if (second_edge_vertex_stage_rotation.unit_quat() == right_plate_stage_rotation.unit_quat())
			{
				// Both edge vertices are on same un-stretched side of rift.
				// Ignore edge.
				continue;
			}
			else
			{
				// Only first edge vertex is on an un-stretched side of rift.
				rift_edge_type = BuildInfo::RiftParams::ONLY_FIRST_EDGE_VERTEX_ON_UNSTRETCHED_SIDE;
			}
		}
		else if (first_edge_vertex_stage_rotation.unit_quat() == left_plate_stage_rotation.unit_quat())
		{
			if (second_edge_vertex_stage_rotation.unit_quat() == right_plate_stage_rotation.unit_quat())
			{
				// Both edge vertices are on opposite un-stretched sides of rift.
				rift_edge_type = BuildInfo::RiftParams::BOTH_EDGE_VERTICES_ON_OPPOSITE_UNSTRETCHED_SIDES;
			}
			else if (second_edge_vertex_stage_rotation.unit_quat() == left_plate_stage_rotation.unit_quat())
			{
				// Both edge vertices are on same un-stretched side of rift.
				// Ignore edge.
				continue;
			}
			else
			{
				// Only first edge vertex is on an un-stretched side of rift.
				rift_edge_type = BuildInfo::RiftParams::ONLY_FIRST_EDGE_VERTEX_ON_UNSTRETCHED_SIDE;
			}
		}
		else if (second_edge_vertex_stage_rotation.unit_quat() == right_plate_stage_rotation.unit_quat())
		{
			// Only second edge vertex is on an un-stretched side of rift.
			rift_edge_type = BuildInfo::RiftParams::ONLY_SECOND_EDGE_VERTEX_ON_UNSTRETCHED_SIDE;
		}
		else if (second_edge_vertex_stage_rotation.unit_quat() == left_plate_stage_rotation.unit_quat())
		{
			// Only second edge vertex is on an un-stretched side of rift.
			rift_edge_type = BuildInfo::RiftParams::ONLY_SECOND_EDGE_VERTEX_ON_UNSTRETCHED_SIDE;
		}
		else
		{
			// Neither vertex is on an un-stretched side of rift.
			//
			// This topological network has likely been well-constrained in this region.
			// We only want to control the strain rate in un-constrained regions that join directly
			// to the un-stretched side of rift (eg, a row of vertices along un-stretched side and
			// a row of vertices along rift axis, with no vertices/constraints in between).
			//
			// So ignore this edge.
			continue;
		}

		//
		// Decompose each vertex stage rotation into a twist component around the rift stage rotation axis and
		// a swing component around an axis orthogonal to that axis.
		//
		// This is using a modification of the twist-swing decomposition of a quaternion, see:
		//
		//   https://stackoverflow.com/questions/3684269/component-of-a-quaternion-rotation-around-an-axis/22401169#22401169
		//   http://www.euclideanspace.com/maths/geometry/rotations/for/decomposition/
		//   http://allenchou.net/2018/05/game-math-swing-twist-interpolation-sterp/
		//
		// ...where a modification is needed because the above twist-swing decomposition only guarantees
		// that the swing rotation *axis* is orthogonal to the twist rotation *axis*. However at any particular point
		// on the sphere the actual *directions/velocities* of the twist and swing rotations are not
		// guaranteed to be orthogonal which can result in the twist stage rotation being larger than we want.
		// Picture the twist rotation as the horizontal base of a triangle and the swing rotation as the second edge
		// and the actual combined twist-swing rotation as the third edge.
		// We actually want the swing edge of triangle to be vertical (orthogonal to twist edge), but with the
		// twist-swing decomposition we can get a slanted swing edge, which means the twist base edge
		// needs to be longer to result in the same third edge (combined rotation).
		//
		// So the modification to the twist-swing decomposition ensures the *angle* of the twist rotation will
		// place a rotated edge vertex on the meridian (line of constant longitude) of the twist pole at that
		// same angle. The swing component rotation then takes care of the latitude direction (of twist pole).
		//

		// Find the twist orthonormal frame where the z-axis is the rift stage pole axis and
		// the x-z plane contains the *first* edge vertex.
		const GPlatesMaths::UnitVector3D &twist_frame_z = twist_axis;
		const GPlatesMaths::UnitVector3D &first_edge_vertex = first_edge_vertex_point.position_vector();
		const GPlatesMaths::Vector3D twist_frame_y_non_normalised = cross(twist_frame_z, first_edge_vertex);
		if (twist_frame_y_non_normalised.is_zero_magnitude())
		{
			// First vertex coincides with twist axis, skip the current edge.
			continue;
		}
		const GPlatesMaths::UnitVector3D twist_frame_y = twist_frame_y_non_normalised.get_normalisation();
		const GPlatesMaths::UnitVector3D twist_frame_x = cross(twist_frame_y, twist_frame_z).get_normalisation();

		// Find the twist angle (about twist z-axis) between the two edge vertices.
		// This is the twist angle of the second vertex in the twist orthonormal frame of the first vertex.
		const GPlatesMaths::UnitVector3D &second_edge_vertex = second_edge_vertex_point.position_vector();
		const GPlatesMaths::real_t second_vertex_twist_x = dot(second_edge_vertex, twist_frame_x);
		const GPlatesMaths::real_t second_vertex_twist_y = dot(second_edge_vertex, twist_frame_y);
		if (second_vertex_twist_x == 0.0 &&
			second_vertex_twist_y == 0.0)
		{
			// Second vertex coincides with twist axis, skip the current edge.
			continue;
		}
		const GPlatesMaths::real_t twist_angle_between_edge_vertices =
				std::atan2(second_vertex_twist_y.dval(), second_vertex_twist_x.dval());

		if (twist_angle_between_edge_vertices == 0)
		{
			// There's no twist between the two edge vertices.
			// The edge is orthogonal to the rift stage pole rotation.
			// Skip the current edge.
			continue;
		}
		const GPlatesMaths::real_t inv_twist_angle_between_edge_vertices = 1.0 / twist_angle_between_edge_vertices;

		// Find the x, y and z coordinates of the *rotated* first edge vertex in the twist orthonormal frame.
		const GPlatesMaths::UnitVector3D rotated_first_edge_vertex = first_edge_vertex_stage_rotation * first_edge_vertex;
		const GPlatesMaths::real_t rotated_first_edge_vertex_twist_x = dot(rotated_first_edge_vertex, twist_frame_x);
		const GPlatesMaths::real_t rotated_first_edge_vertex_twist_y = dot(rotated_first_edge_vertex, twist_frame_y);

		// Find the twist angle (about twist z-axis) that rotates the twist frame x-z plane such that the
		// rotated x-z plane contains the *rotated* first edge vertex.
		if (rotated_first_edge_vertex_twist_x == 0.0 &&
			rotated_first_edge_vertex_twist_y == 0.0)
		{
			// Rotated first vertex coincides with twist axis, skip the current edge.
			continue;
		}
		// Note that the twist angle of the *un-rotated* first edge vertex is zero, so we don't need to subtract it.
		const GPlatesMaths::real_t first_edge_vertex_twist_angle =
				std::atan2(rotated_first_edge_vertex_twist_y.dval(), rotated_first_edge_vertex_twist_x.dval());

		// Find the x, y and z coordinates of the *rotated* second edge vertex in the twist orthonormal frame.
		const GPlatesMaths::UnitVector3D rotated_second_edge_vertex = second_edge_vertex_stage_rotation * second_edge_vertex;
		const GPlatesMaths::real_t rotated_second_edge_vertex_twist_x = dot(rotated_second_edge_vertex, twist_frame_x);
		const GPlatesMaths::real_t rotated_second_edge_vertex_twist_y = dot(rotated_second_edge_vertex, twist_frame_y);

		// Find the twist angle (about twist z-axis) that rotates the twist frame x-z plane such that the
		// rotated x-z plane contains the *rotated* second edge vertex.
		if (rotated_second_edge_vertex_twist_x == 0.0 &&
			rotated_second_edge_vertex_twist_y == 0.0)
		{
			// Rotated second vertex coincides with twist axis, skip the current edge.
			continue;
		}
		GPlatesMaths::real_t second_edge_vertex_twist_angle =
				std::atan2(rotated_second_edge_vertex_twist_y.dval(), rotated_second_edge_vertex_twist_x.dval()) -
					// Need to subtract the twist angle of the second *un-rotated* edge vertex to get the twist
					// angle between *un-rotated* and *rotated* second edge vertex.
					// The twist angle of the second edge vertex is also the twist angle between the edge vertices
					// (since the first edge vertex has a zero twist angle).
					twist_angle_between_edge_vertices;
		// Handle wraparound between -PI and PI (in atan2 angle).
		// This can happen when the *un-rotated* twist angle is near PI and the *rotated* twist angle
		// is near -PI (or vice versa) introducing an offset of 2*PI or -2*PI that must be removed.
		//
		// Normally the magnitude of the difference in angles should be less than PI since it is highly unlikely
		// that a rotation over 1My would produce such a large rotation (plates just don't move that fast).
		// If the absolute difference is greater, then we've detected wraparound. Typically a wraparound
		// difference is closer to 2*PI or -2*PI (so a threshold of PI is a good middle ground for detection).
		if (second_edge_vertex_twist_angle.is_precisely_greater_than(GPlatesMaths::PI))
		{
			second_edge_vertex_twist_angle -= 2 * GPlatesMaths::PI;
		}
		else if (second_edge_vertex_twist_angle.is_precisely_less_than(-GPlatesMaths::PI))
		{
			second_edge_vertex_twist_angle += 2 * GPlatesMaths::PI;
		}

		// Calculate the twist velocity gradient between the two edge vertices.
		// In units of (1/second) since that's our units of strain rate.
		const GPlatesMaths::real_t twist_velocity_gradient =
				INV_VELOCITY_DELTA_TIME * SCALE_PER_MY_TO_PER_SECOND/* 1/my -> 1/s */ *
				abs((second_edge_vertex_twist_angle - first_edge_vertex_twist_angle) * inv_twist_angle_between_edge_vertices);

		// Get stage rotation axis/angle at each vertex of edge.
		//
		// Note: If a rotation is identity then we'll just use any arbitrary axis and zero angle
		//       (all arbitrary axes will result in the same quaternion when the angle is zero).
		const GPlatesMaths::UnitQuaternion3D::RotationParams first_edge_vertex_stage_rotation_axis_angle =
				represents_identity_rotation(first_edge_vertex_stage_rotation.unit_quat())
				? GPlatesMaths::UnitQuaternion3D::RotationParams(GPlatesMaths::UnitVector3D::zBasis(), 0.0)
				: first_edge_vertex_stage_rotation.unit_quat().get_rotation_params(boost::none/*axis_hint*/);
		const GPlatesMaths::UnitQuaternion3D::RotationParams second_edge_vertex_stage_rotation_axis_angle =
				represents_identity_rotation(second_edge_vertex_stage_rotation.unit_quat())
				? GPlatesMaths::UnitQuaternion3D::RotationParams(GPlatesMaths::UnitVector3D::zBasis(), 0.0)
				: second_edge_vertex_stage_rotation.unit_quat().get_rotation_params(boost::none/*axis_hint*/);

		// Recursively sub-divide current Delaunay edge.
		refine_rift_delaunay_edge(
				delaunay_edge_point_seq,
				first_edge_vertex_point,
				second_edge_vertex_point,
				0.0/*first_subdivided_edge_vertex_interpolation*/,
				1.0/*second_subdivided_edge_vertex_interpolation*/,
				0.0/*first_subdivided_edge_vertex_twist_interpolation*/,
				1.0/*second_subdivided_edge_vertex_twist_interpolation*/,
				first_edge_vertex_stage_rotation_axis_angle.axis,
				second_edge_vertex_stage_rotation_axis_angle.axis,
				first_edge_vertex_stage_rotation_axis_angle.angle,
				second_edge_vertex_stage_rotation_axis_angle.angle,
				first_edge_vertex_twist_angle,
				second_edge_vertex_twist_angle,
				edge_rotation_axis,
				edge_angular_extent.get_angle()/*edge_angular_extent*/,
				edge_angular_extent.get_angle()/*subdivided_edge_angular_extent*/,
				twist_axis,
				twist_frame_x,
				twist_frame_y,
				inv_twist_angle_between_edge_vertices,
				twist_velocity_gradient,
				rift_edge_type,
				rift_params,
				reconstruction_tree_creator);
	}

	if (delaunay_edge_point_seq.empty())
	{
		return;
	}

	// Project the points to 2D space and insert into array to be spatially sorted.
	std::vector<DelaunayPoint2> delaunay_edge_point_2_seq;
	delaunay_edge_point_2_seq.reserve(delaunay_edge_point_seq.size());
	std::vector<DelaunayPoint>::const_iterator delaunay_edge_points_iter = delaunay_edge_point_seq.begin();
	std::vector<DelaunayPoint>::const_iterator delaunay_edge_points_end = delaunay_edge_point_seq.end();
	for (; delaunay_edge_points_iter != delaunay_edge_points_end; ++delaunay_edge_points_iter)
	{
		const DelaunayPoint &delaunay_edge_point = *delaunay_edge_points_iter;

		// Cache our lat/lon coordinates - otherwise the projection needs to convert to lat/lon
		// internally - and so might as well only do the lat/lon conversion once for efficiency.
		const GPlatesMaths::LatLonPoint delaunay_edge_lat_lon_point = make_lat_lon_point(delaunay_edge_point.point);

		// Project point-on-sphere to x,y space.
		const delaunay_point_2_type delaunay_edge_point_2 =
				d_projection.project_from_lat_lon<delaunay_point_2_type>(delaunay_edge_lat_lon_point);

		delaunay_edge_point_2_seq.push_back(
				DelaunayPoint2(&delaunay_edge_point, delaunay_edge_lat_lon_point, delaunay_edge_point_2));
	}

	// Improve performance by spatially sorting the delaunay points.
	// This is what is done by the CGAL overload that inserts a *range* of points into a delauany triangulation.
	CGAL::spatial_sort(
			delaunay_edge_point_2_seq.begin(),
			delaunay_edge_point_2_seq.end(),
			DelaunayPoint2SpatialSortingTraits());

	// Insert the points into the delaunay triangulation.
	std::vector<DelaunayPoint2>::const_iterator delaunay_edge_points_2_iter = delaunay_edge_point_2_seq.begin();
	std::vector<DelaunayPoint2>::const_iterator delaunay_edge_points_2_end = delaunay_edge_point_2_seq.end();
	Delaunay_2::Face_handle insert_start_face;
	for (; delaunay_edge_points_2_iter != delaunay_edge_points_2_end; ++delaunay_edge_points_2_iter)
	{
		const DelaunayPoint2 &delaunay_edge_point_2 = *delaunay_edge_points_2_iter;
		const DelaunayPoint &delaunay_edge_point = *delaunay_edge_point_2.delaunay_point;

		// Insert into the triangulation.
		Delaunay_2::Vertex_handle delaunay_point_vertex_handle =
				d_delaunay_2->insert(delaunay_edge_point_2.point_2, insert_start_face);

		if (delaunay_point_vertex_handle->is_initialised())
		{
			// Vertex handle refers to an existing vertex position.
			// Most likely the edge length is too small - which really shouldn't happen if
			// edge subdivision has a distance threshold.
			// Just ignore the current vertex.
			continue;
		}

		// Set the extra info for this vertex.
		delaunay_point_vertex_handle->initialise(
				d_delaunay_2.get(),
				vertex_index,
				delaunay_edge_point.point,
				delaunay_edge_point_2.lat_lon_point,
				delaunay_edge_point.shared_source_info);

		// Increment vertex index since vertex handle does not refer to an existing vertex position.
		++vertex_index;

		// The next insert vertex will start searching at the face of the last inserted vertex.
		insert_start_face = delaunay_point_vertex_handle->face();
	}

	//qDebug() << "          after rift refinement:" << d_delaunay_2->number_of_faces();
}


void
GPlatesAppLogic::ResolvedTriangulation::Network::refine_rift_delaunay_edge(
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
		const ReconstructionTreeCreator &reconstruction_tree_creator) const
{
	// Get mid-point of subdivided edge.
	const GPlatesMaths::Vector3D sum_subdivided_edge_vertex_points =
			GPlatesMaths::Vector3D(first_subdivided_edge_vertex_point.position_vector()) +
			GPlatesMaths::Vector3D(second_subdivided_edge_vertex_point.position_vector());
	if (sum_subdivided_edge_vertex_points.is_zero_magnitude())
	{
		// Edge vertices are antipodal - shouldn't be possible though.
		// Skip subdividing current edge.
		return;
	}
	const GPlatesMaths::PointOnSphere subdivided_edge_mid_point(
			sum_subdivided_edge_vertex_points.get_normalisation());

	const GPlatesMaths::real_t subdivided_edge_mid_point_vertex_twist_x =
			dot(subdivided_edge_mid_point.position_vector(), twist_frame_x);
	const GPlatesMaths::real_t subdivided_edge_mid_point_vertex_twist_y =
			dot(subdivided_edge_mid_point.position_vector(), twist_frame_y);
	if (subdivided_edge_mid_point_vertex_twist_x == 0.0 &&
		subdivided_edge_mid_point_vertex_twist_y == 0.0)
	{
		// Subdivided edge mid-point vertex coincides with twist axis, skip the current edge.
		return;
	}
	const GPlatesMaths::real_t subdivided_edge_mid_point_twist_interpolation =
			inv_twist_angle_between_edge_vertices *
			std::atan2(subdivided_edge_mid_point_vertex_twist_y.dval(), subdivided_edge_mid_point_vertex_twist_x.dval());

	if (rift_edge_type != BuildInfo::RiftParams::BOTH_EDGE_VERTICES_ON_OPPOSITE_UNSTRETCHED_SIDES)
	{
		// Interpolate from second edge vertex (instead of first edge vertex) if second edge vertex
		// is on un-stretched crust (and not first edge vertex).
		GPlatesMaths::real_t subdivided_edge_mid_point_twist_interpolation_from_unstretched_side;
		GPlatesMaths::real_t first_subdivided_edge_vertex_twist_interpolation_from_unstretched_side;
		GPlatesMaths::real_t second_subdivided_edge_vertex_twist_interpolation_from_unstretched_side;
		if (rift_edge_type == BuildInfo::RiftParams::ONLY_SECOND_EDGE_VERTEX_ON_UNSTRETCHED_SIDE)
		{
			subdivided_edge_mid_point_twist_interpolation_from_unstretched_side = 1.0 - subdivided_edge_mid_point_twist_interpolation;
			first_subdivided_edge_vertex_twist_interpolation_from_unstretched_side = 1.0 - first_subdivided_edge_vertex_twist_interpolation;
			second_subdivided_edge_vertex_twist_interpolation_from_unstretched_side = 1.0 - second_subdivided_edge_vertex_twist_interpolation;
		}
		else
		{
			subdivided_edge_mid_point_twist_interpolation_from_unstretched_side = subdivided_edge_mid_point_twist_interpolation;
			first_subdivided_edge_vertex_twist_interpolation_from_unstretched_side = first_subdivided_edge_vertex_twist_interpolation;
			second_subdivided_edge_vertex_twist_interpolation_from_unstretched_side = second_subdivided_edge_vertex_twist_interpolation;
		}

		// The linear interpolation of twist interpolation at subdivided edge mid-point (between two subdivided edge vertices).
		GPlatesMaths::real_t interpolation_within_subdivided_edge;
		if (second_subdivided_edge_vertex_twist_interpolation != first_subdivided_edge_vertex_twist_interpolation)
		{
			interpolation_within_subdivided_edge =
					(subdivided_edge_mid_point_twist_interpolation - first_subdivided_edge_vertex_twist_interpolation) /
					(second_subdivided_edge_vertex_twist_interpolation - first_subdivided_edge_vertex_twist_interpolation);
		}
		else
		{
			interpolation_within_subdivided_edge = 0.5; // Any value in range [0,1] will do.
		}

		// Adaptively sub-divide edge only if the difference between the linear interpolation of twist component of
		// strain rates (at the two edge vertices) and the actual twist component of strain rate at edge mid-point would
		// be larger than a threshold resolution (in order to adaptively fit the exponential strain rate curve across the rift).
		const GPlatesMaths::real_t delta_twist_velocity_gradient = twist_velocity_gradient *
				rift_params.exponential_stretching_constant *
				(((1 - interpolation_within_subdivided_edge) *
						std::exp(rift_params.exponential_stretching_constant * first_subdivided_edge_vertex_twist_interpolation_from_unstretched_side.dval()) +
					interpolation_within_subdivided_edge *
						std::exp(rift_params.exponential_stretching_constant * second_subdivided_edge_vertex_twist_interpolation_from_unstretched_side.dval())) -
					std::exp(rift_params.exponential_stretching_constant * subdivided_edge_mid_point_twist_interpolation_from_unstretched_side.dval())) /
				(std::exp(rift_params.exponential_stretching_constant) - 1.0);
		if (abs(delta_twist_velocity_gradient).dval() < rift_params.strain_rate_resolution)
		{
			return;
		}
	}

	const GPlatesMaths::real_t subdivided_edge_mid_point_interpolation =
			0.5 * (first_subdivided_edge_vertex_interpolation + second_subdivided_edge_vertex_interpolation);

	const GPlatesMaths::real_t rotate_first_edge_vertex_stage_rotation_angle =
			subdivided_edge_mid_point_interpolation * edge_angular_extent;
	const GPlatesMaths::real_t rotate_second_edge_vertex_stage_rotation_angle =
			-(1 - subdivided_edge_mid_point_interpolation) * edge_angular_extent;

	const GPlatesMaths::FiniteRotation rotate_first_edge_vertex_stage_rotation =
			GPlatesMaths::FiniteRotation::create(
					GPlatesMaths::UnitQuaternion3D::create_rotation(
							edge_rotation_axis,
							rotate_first_edge_vertex_stage_rotation_angle),
					boost::none);
	const GPlatesMaths::UnitVector3D rotated_first_edge_vertex_stage_rotation_axis =
			rotate_first_edge_vertex_stage_rotation * first_edge_vertex_stage_rotation_axis;
	const GPlatesMaths::FiniteRotation rotated_first_edge_vertex_stage_rotation =
			GPlatesMaths::FiniteRotation::create(
					GPlatesMaths::UnitQuaternion3D::create_rotation(
							rotated_first_edge_vertex_stage_rotation_axis,
							first_edge_vertex_stage_rotation_angle),
					boost::none);

	const GPlatesMaths::FiniteRotation rotate_second_edge_vertex_stage_rotation =
			GPlatesMaths::FiniteRotation::create(
					GPlatesMaths::UnitQuaternion3D::create_rotation(
							edge_rotation_axis,
							rotate_second_edge_vertex_stage_rotation_angle),
					boost::none);
	const GPlatesMaths::UnitVector3D rotated_second_edge_vertex_stage_rotation_axis =
			rotate_second_edge_vertex_stage_rotation * second_edge_vertex_stage_rotation_axis;
	const GPlatesMaths::FiniteRotation rotated_second_edge_vertex_stage_rotation =
			GPlatesMaths::FiniteRotation::create(
					GPlatesMaths::UnitQuaternion3D::create_rotation(
							rotated_second_edge_vertex_stage_rotation_axis,
							second_edge_vertex_stage_rotation_angle),
					boost::none);

	// Interpolate the edge vertex stage rotations at the subdivided edge mid-point.
	GPlatesMaths::FiniteRotation subdivided_edge_mid_point_stage_rotation =
			interpolate(
					rotated_first_edge_vertex_stage_rotation,
					rotated_second_edge_vertex_stage_rotation,
					subdivided_edge_mid_point_interpolation);

	// Both edge vertices are on opposite un-stretched sides of the rift.
	// We don't have a velocity constraint in the middle (axis) of the rift so
	// we just generate one ourselves. We do this by simply interpolating the velocities
	// of the two edge vertices. The two new child edges will only have one of each edge's vertices
	// on un-stretched crust and subsequent adaptive sub-division will proceed as normal.
	if (rift_edge_type != BuildInfo::RiftParams::BOTH_EDGE_VERTICES_ON_OPPOSITE_UNSTRETCHED_SIDES)
	{
		//
		// Convert the interpolated edge mid-point stage rotation from a small circle to a great circle rotation.
		//
		// This avoids issues when the small circle has a small radius and is near the edge mid-point.
		// In this case a small circle rotation would cause the stage-rotated edge mid-point to bend
		// quite tightly around that small circle causing the swing component calculation to be off.
		// We really want the point to rotate in the direction of the velocity (tangent to the small circle)
		// which means it rotates around a great circle (with rotation axis perpendicular to the point).
		//

		if (!represents_identity_rotation(subdivided_edge_mid_point_stage_rotation.unit_quat()))
		{
			GPlatesMaths::UnitQuaternion3D::RotationParams subdivided_edge_mid_point_stage_rotation_axis_angle =
					subdivided_edge_mid_point_stage_rotation.unit_quat().get_rotation_params(boost::none/*axis_hint*/);

			const GPlatesMaths::Vector3D subdivided_edge_mid_point_stage_rotation_great_circle_div_angle =
					cross(subdivided_edge_mid_point_stage_rotation_axis_angle.axis, subdivided_edge_mid_point.position_vector());
			if (subdivided_edge_mid_point_stage_rotation_great_circle_div_angle.is_zero_magnitude())
			{
				// First vertex coincides with interpolated stage rotation axis, skip the current edge.
				return;
			}
			subdivided_edge_mid_point_stage_rotation_axis_angle.angle *=
					subdivided_edge_mid_point_stage_rotation_great_circle_div_angle.magnitude();
			subdivided_edge_mid_point_stage_rotation_axis_angle.axis = cross(
					subdivided_edge_mid_point.position_vector(),
					subdivided_edge_mid_point_stage_rotation_great_circle_div_angle).get_normalisation();
			subdivided_edge_mid_point_stage_rotation = 
					GPlatesMaths::FiniteRotation::create(
							GPlatesMaths::UnitQuaternion3D::create_rotation(
									subdivided_edge_mid_point_stage_rotation_axis_angle.axis,
									subdivided_edge_mid_point_stage_rotation_axis_angle.angle),
							boost::none);
		}

		//
		// Determine the swing component of the interpolated stage rotation.
		//

		const GPlatesMaths::Vector3D subdivided_edge_mid_point_swing_rotation_axis_non_normalised =
				cross(subdivided_edge_mid_point.position_vector(), twist_axis);
		if (subdivided_edge_mid_point_swing_rotation_axis_non_normalised.is_zero_magnitude())
		{
			// First vertex coincides with twist axis, skip the current edge.
			return;
		}
		const GPlatesMaths::UnitVector3D subdivided_edge_mid_point_swing_rotation_axis =
				subdivided_edge_mid_point_swing_rotation_axis_non_normalised.get_normalisation();

		const GPlatesMaths::PointOnSphere rotated_subdivided_edge_mid_point =
				subdivided_edge_mid_point_stage_rotation * subdivided_edge_mid_point;

		const GPlatesMaths::real_t subdivided_edge_mid_point_twist_z =
				dot(subdivided_edge_mid_point.position_vector(), twist_axis);
		const GPlatesMaths::real_t rotated_subdivided_edge_mid_point_twist_z =
				dot(rotated_subdivided_edge_mid_point.position_vector(), twist_axis);
		const GPlatesMaths::real_t subdivided_edge_mid_point_swing_rotation_angle =
				asin(rotated_subdivided_edge_mid_point_twist_z) -
				asin(subdivided_edge_mid_point_twist_z);

		const GPlatesMaths::FiniteRotation subdivided_edge_mid_point_swing_rotation =
				GPlatesMaths::FiniteRotation::create(
						GPlatesMaths::UnitQuaternion3D::create_rotation(
								subdivided_edge_mid_point_swing_rotation_axis,
								subdivided_edge_mid_point_swing_rotation_angle),
						boost::none);

		// Interpolate from second edge vertex (instead of first edge vertex) if second edge vertex
		// is on un-stretched crust (and not first edge vertex).
		GPlatesMaths::real_t subdivided_edge_mid_point_twist_interpolation_from_unstretched_side;
		if (rift_edge_type == BuildInfo::RiftParams::ONLY_SECOND_EDGE_VERTEX_ON_UNSTRETCHED_SIDE)
		{
			subdivided_edge_mid_point_twist_interpolation_from_unstretched_side = 1.0 - subdivided_edge_mid_point_twist_interpolation;
		}
		else
		{
			subdivided_edge_mid_point_twist_interpolation_from_unstretched_side = subdivided_edge_mid_point_twist_interpolation;
		}

		// Use exponential interpolation for twist angle to simulate non-uniform strain rate across rift profile
		// since the twist component is around the rift stage rotation axis.
		const GPlatesMaths::real_t subdivided_edge_mid_point_twist_angle_interpolation_from_unstretched_side =
				(std::exp(rift_params.exponential_stretching_constant * subdivided_edge_mid_point_twist_interpolation_from_unstretched_side.dval()) - 1.0) /
				(std::exp(rift_params.exponential_stretching_constant) - 1.0);

		// Interpolate from second edge vertex (instead of first edge vertex) if second edge vertex
		// is on un-stretched crust (and not first edge vertex).
		GPlatesMaths::real_t subdivided_edge_mid_point_twist_angle;
		if (rift_edge_type == BuildInfo::RiftParams::ONLY_SECOND_EDGE_VERTEX_ON_UNSTRETCHED_SIDE)
		{
			subdivided_edge_mid_point_twist_angle =
					(1 - subdivided_edge_mid_point_twist_angle_interpolation_from_unstretched_side) * second_edge_vertex_twist_angle +
					subdivided_edge_mid_point_twist_angle_interpolation_from_unstretched_side * first_edge_vertex_twist_angle;
		}
		else
		{
			subdivided_edge_mid_point_twist_angle =
					(1 - subdivided_edge_mid_point_twist_angle_interpolation_from_unstretched_side) * first_edge_vertex_twist_angle +
					subdivided_edge_mid_point_twist_angle_interpolation_from_unstretched_side * second_edge_vertex_twist_angle;
		}

		const GPlatesMaths::FiniteRotation subdivided_edge_mid_point_twist_rotation =
				GPlatesMaths::FiniteRotation::create(
						GPlatesMaths::UnitQuaternion3D::create_rotation(
								twist_axis,
								subdivided_edge_mid_point_twist_angle),
						boost::none);

		// Combine the interpolated twist and swing components into the final interpolated stage rotation.
		//
		// Do the swing rotation first since that rotates towards the twist axis, and then rotates around the twist axis.
		subdivided_edge_mid_point_stage_rotation =
				compose(subdivided_edge_mid_point_twist_rotation, subdivided_edge_mid_point_swing_rotation);
	}

	// Create a vertex source info using the interpolated stage rotation.
	// This will be used to generate the velocity at the new vertex.
	const ResolvedVertexSourceInfo::non_null_ptr_to_const_type subdivided_edge_mid_point_source_info =
			ResolvedVertexSourceInfo::create(subdivided_edge_mid_point_stage_rotation, reconstruction_tree_creator);

	// Add new vertex position and source info at edge mid-point.
	delaunay_edge_point_seq.push_back(
			DelaunayPoint(subdivided_edge_mid_point, subdivided_edge_mid_point_source_info));

	// If child edge length is shorter than threshold distance then don't recurse into child edges.
	const GPlatesMaths::real_t child_subdivided_edge_angular_extent = 0.5 * subdivided_edge_angular_extent;
	if (child_subdivided_edge_angular_extent.is_precisely_less_than(rift_params.edge_length_threshold.get_angle().dval()))
	{
		return;
	}

	// Sub-divide current edges into two child edges.
	for (unsigned int child_index = 0; child_index < 2; ++child_index)
	{
		const GPlatesMaths::PointOnSphere &first_child_subdivided_edge_vertex_point =
				(child_index == 0) ? first_subdivided_edge_vertex_point : subdivided_edge_mid_point;
		const GPlatesMaths::PointOnSphere &second_child_subdivided_edge_vertex_point =
				(child_index == 0) ? subdivided_edge_mid_point : second_subdivided_edge_vertex_point;

		BuildInfo::RiftParams::EdgeType child_rift_edge_type;
		GPlatesMaths::real_t first_child_subdivided_edge_vertex_interpolation;
		GPlatesMaths::real_t second_child_subdivided_edge_vertex_interpolation;
		GPlatesMaths::real_t first_child_subdivided_edge_vertex_twist_interpolation;
		GPlatesMaths::real_t second_child_subdivided_edge_vertex_twist_interpolation;
		if (rift_edge_type == BuildInfo::RiftParams::BOTH_EDGE_VERTICES_ON_OPPOSITE_UNSTRETCHED_SIDES)
		{
			// Both edge vertices of parent edge are on opposite un-stretched sides of rift.
			if (child_index == 0)
			{
				child_rift_edge_type = BuildInfo::RiftParams::ONLY_FIRST_EDGE_VERTEX_ON_UNSTRETCHED_SIDE;
			}
			else
			{
				child_rift_edge_type = BuildInfo::RiftParams::ONLY_SECOND_EDGE_VERTEX_ON_UNSTRETCHED_SIDE;
			}

			first_child_subdivided_edge_vertex_interpolation = 0.0;
			second_child_subdivided_edge_vertex_interpolation = 1.0;

			first_child_subdivided_edge_vertex_twist_interpolation = 0.0;
			second_child_subdivided_edge_vertex_twist_interpolation = 1.0;
		}
		else
		{
			// Only one edge vertex of parent edge is on un-stretched side of rift.
			// The same child edge vertex *index* will be on un-stretched side as parent edge vertex *index*.
			// So we just propagate this down to the child.
			child_rift_edge_type = rift_edge_type;

			if (child_index == 0)
			{
				first_child_subdivided_edge_vertex_interpolation = first_subdivided_edge_vertex_interpolation;
				second_child_subdivided_edge_vertex_interpolation = subdivided_edge_mid_point_interpolation;

				first_child_subdivided_edge_vertex_twist_interpolation = first_subdivided_edge_vertex_twist_interpolation;
				second_child_subdivided_edge_vertex_twist_interpolation = subdivided_edge_mid_point_twist_interpolation;
			}
			else
			{
				first_child_subdivided_edge_vertex_interpolation = subdivided_edge_mid_point_interpolation;
				second_child_subdivided_edge_vertex_interpolation = second_subdivided_edge_vertex_interpolation;

				first_child_subdivided_edge_vertex_twist_interpolation = subdivided_edge_mid_point_twist_interpolation;
				second_child_subdivided_edge_vertex_twist_interpolation = second_subdivided_edge_vertex_twist_interpolation;
			}
		}

		refine_rift_delaunay_edge(
				delaunay_edge_point_seq,
				first_child_subdivided_edge_vertex_point,
				second_child_subdivided_edge_vertex_point,
				first_child_subdivided_edge_vertex_interpolation,
				second_child_subdivided_edge_vertex_interpolation,
				first_child_subdivided_edge_vertex_twist_interpolation,
				second_child_subdivided_edge_vertex_twist_interpolation,
				first_edge_vertex_stage_rotation_axis,
				second_edge_vertex_stage_rotation_axis,
				first_edge_vertex_stage_rotation_angle,
				second_edge_vertex_stage_rotation_angle,
				first_edge_vertex_twist_angle,
				second_edge_vertex_twist_angle,
				edge_rotation_axis,
				edge_angular_extent,
				child_subdivided_edge_angular_extent,
				twist_axis,
				twist_frame_x,
				twist_frame_y,
				inv_twist_angle_between_edge_vertices,
				twist_velocity_gradient,
				child_rift_edge_type,
				rift_params,
				reconstruction_tree_creator);
	}
}


const GPlatesAppLogic::ResolvedTriangulation::Network::delaunay_point_2_to_vertex_handle_map_type &
GPlatesAppLogic::ResolvedTriangulation::Network::get_delaunay_point_2_to_vertex_handle_map() const
{
	if (!d_delaunay_point_2_to_vertex_handle_map)
	{
		d_delaunay_point_2_to_vertex_handle_map = delaunay_point_2_to_vertex_handle_map_type();
		create_delaunay_point_2_to_vertex_handle_map(d_delaunay_point_2_to_vertex_handle_map.get());
	}

	return d_delaunay_point_2_to_vertex_handle_map.get();
}


void
GPlatesAppLogic::ResolvedTriangulation::Network::create_delaunay_point_2_to_vertex_handle_map(
		delaunay_point_2_to_vertex_handle_map_type &delaunay_point_2_to_vertex_handle_map) const
{
	const Delaunay_2 &delaunay_2 = get_delaunay_2();

	// Iterate over the vertices of the delaunay triangulation.
	Delaunay_2::Finite_vertices_iterator finite_vertices_iter = delaunay_2.finite_vertices_begin();
	Delaunay_2::Finite_vertices_iterator finite_vertices_end = delaunay_2.finite_vertices_end();
	for ( ; finite_vertices_iter != finite_vertices_end; ++finite_vertices_iter)
	{
		// Map the triangulation vertex 2D point to its associated velocity.
		delaunay_point_2_to_vertex_handle_map.insert(
				delaunay_point_2_to_vertex_handle_map_type::value_type(
						finite_vertices_iter->point(),
						finite_vertices_iter));
	}
}


void
GPlatesAppLogic::ResolvedTriangulation::Network::calc_delaunay_natural_neighbor_coordinates_in_deforming_region(
		delaunay_natural_neighbor_coordinates_2_type &natural_neighbor_coordinates,
		const delaunay_point_2_type &point_2,
		Delaunay_2::Face_handle start_face_hint) const
{
	// NOTE: We should only be called if the point is in the deforming region.

	const Delaunay_2 &delaunay_2 = get_delaunay_2();

	// Get the interpolation coordinates for the point.
	if (delaunay_2.calc_natural_neighbor_coordinates(natural_neighbor_coordinates, point_2, start_face_hint))
	{
		return;
	}

	natural_neighbor_coordinates.first.clear();

	// If we get here then the point is inside the network when testing against polygons on the
	// *3D* sphere but is outside the *2D* delaunay triangulation (convex hull).
	// This can happen due to numerical tolerances or the fact that a straight line in projected
	// 2D space does not map to a great circle arc on the sphere (we're not using a gnomic projection).
	// In that latter case a point that it just inside the network boundary polygon-on-sphere can
	// get projected to a 2D point that is just outside the 2D convex hull.
	//
	// The current solution is to find the closest position along the nearest edge of the 2D convex
	// hull (and interpolate the endpoint vertices of that edge).
	// We do this by first finding the closest vertex and then iterating over its incident
	// vertices to find the closest edge (edge is between nearest vertex and incident vertex).
	// This should work since we're at the edge of the convex hull and the test point should
	// be relatively close to an edge of the convex hull.

	// Find the closest edge incident to the nearest vertex.
	std::pair< Delaunay_2::Vertex_handle, boost::optional<Delaunay_2::Vertex_handle> > closest_edge =
			get_closest_delaunay_convex_hull_edge(point_2);

	const Delaunay_2::Vertex_handle closest_vertex = closest_edge.first;
	boost::optional<Delaunay_2::Vertex_handle> closest_edge_end_vertex = closest_edge.second;

	if (closest_edge_end_vertex)
	{
		// Calculate the interpolation coefficients of the nearest and next nearest vertices
		// relative to the test point (this assumes the test point is relatively close to the edge).
		const delaunay_coord_2_type closest_vertex_distance =
				CGAL::sqrt(
						CGAL::to_double(CGAL::squared_distance(point_2, closest_vertex->point())));
		const delaunay_coord_2_type closest_edge_end_vertex_distance =
				CGAL::sqrt(
						CGAL::to_double(CGAL::squared_distance(point_2, closest_edge_end_vertex.get()->point())));

		// Note that the distances are swapped relative to their vertices so that interpolation
		// coefficients are largest when distance to respective vertex is smallest.
		natural_neighbor_coordinates.first.push_back(
				std::make_pair(closest_vertex->point(), closest_edge_end_vertex_distance));
		natural_neighbor_coordinates.first.push_back(
				std::make_pair(closest_edge_end_vertex.get()->point(), closest_vertex_distance));
		natural_neighbor_coordinates.second = closest_vertex_distance + closest_edge_end_vertex_distance;

		//qDebug() << "The point "
		//	<< d_projection.unproject_to_lat_lon(point_2)
		//	<< " slipped through the cracks between the topological network boundary on 3D sphere "
		//	<< "and the 2D projected delaunay triangulation - so interpolating nearest delaunay edge "
		//	<< "for natural neighbour coordinates.";
	}
	else
	{
		// Unable to find the next closest vertex so make the natural neighbour coordinates reference
		// the closest vertex solely by having one coordinate of weight 1.0 and a normalisation factor of 1.0.
		natural_neighbor_coordinates.first.push_back(
				std::make_pair(closest_vertex->point(), 1));
		natural_neighbor_coordinates.second = 1;

		// This will probably never happen but emit a debug message to at least record the
		// fact that this event has happened in case someone checks the logs.
		qDebug() << "The point "
			<< d_projection.unproject_to_lat_lon(QPointF(CGAL::to_double(point_2.x()), CGAL::to_double(point_2.y())))
			<< " slipped through the cracks between the topological network boundary on 3D sphere "
			<< "and the 2D projected delaunay triangulation - and could not find nearest delaunay edge "
			<< "- so using nearest delaunay vertex for natural neighbour coordinates.";
	}
}


GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Face_handle
GPlatesAppLogic::ResolvedTriangulation::Network::calc_delaunay_barycentric_coordinates_in_deforming_region(
		delaunay_coord_2_type &barycentric_coord_vertex_1,
		delaunay_coord_2_type &barycentric_coord_vertex_2,
		delaunay_coord_2_type &barycentric_coord_vertex_3,
		const delaunay_point_2_type &point_2,
		Delaunay_2::Face_handle start_face_hint) const
{
	// NOTE: We should only be called if the point is in the deforming region.

	const Delaunay_2 &delaunay_2 = get_delaunay_2();

	// Get the barycentric coordinates for the point.
	boost::optional<Delaunay_2::Face_handle> face =
			delaunay_2.calc_barycentric_coordinates(
					barycentric_coord_vertex_1,
					barycentric_coord_vertex_2,
					barycentric_coord_vertex_3,
					point_2,
					start_face_hint);
	if (face)
	{
		return face.get();
	}

	// If we get here then the point is inside the network when testing against polygons on the
	// *3D* sphere but is outside the *2D* delaunay triangulation (convex hull).
	// This can happen due to numerical tolerances or the fact that a straight line in projected
	// 2D space does not map to a great circle arc on the sphere (we're not using a gnomic projection).
	// In that latter case a point that is just inside the network boundary polygon-on-sphere can
	// get projected to a 2D point that is just outside the 2D convex hull.
	//
	// The current solution is to find the closest position along the nearest edge of the 2D convex
	// hull (and interpolate the endpoint vertices of that edge).
	// We do this by first finding the closest vertex and then iterating over its incident
	// vertices to find the closest edge (edge is between nearest vertex and incident vertex).
	// This should work since we're at the edge of the convex hull and the test point should
	// be relatively close to an edge of the convex hull.

	// Find the closest edge incident to the nearest vertex.
	std::pair< Delaunay_2::Vertex_handle, boost::optional<Delaunay_2::Vertex_handle> > closest_edge =
			get_closest_delaunay_convex_hull_edge(point_2);

	const Delaunay_2::Vertex_handle closest_vertex = closest_edge.first;
	boost::optional<Delaunay_2::Vertex_handle> closest_edge_end_vertex = closest_edge.second;

	// Since the point is outside the face, some of the barycentric coordinates would be
	// negative, but they would still all sum to 1.0. Even so we'll ensure they are all
	// positive by choosing a point on the closest edge of the face.

	if (closest_edge_end_vertex)
	{
		// Find the (finite) face containing the closest edge.
		const Delaunay_2::Face_circulator incident_face_circulator_start =
				delaunay_2.incident_faces(closest_vertex);
		Delaunay_2::Face_circulator incident_face_circulator = incident_face_circulator_start;
		do
		{
			// Ignore the infinite face - we're at the edge of the convex hull so one (or two?)
			// adjacent face(s) will be the infinite face.
			if (delaunay_2.is_infinite(incident_face_circulator))
			{
				continue;
			}

			if (incident_face_circulator->has_vertex(closest_edge_end_vertex.get()))
			{
				face = incident_face_circulator;
				break;
			}
		}
		while (++incident_face_circulator != incident_face_circulator_start);

		if (face)
		{
			// Calculate the interpolation coefficients of the nearest and next nearest vertices
			// relative to the test point (this assumes the test point is relatively close to the edge).
			const delaunay_coord_2_type closest_vertex_distance =
					CGAL::sqrt(
							CGAL::to_double(CGAL::squared_distance(point_2, closest_vertex->point())));
			const delaunay_coord_2_type closest_edge_end_vertex_distance =
					CGAL::sqrt(
							CGAL::to_double(CGAL::squared_distance(point_2, closest_edge_end_vertex.get()->point())));

			// Calculate the barycentric weights (sum to 1.0).
			const delaunay_coord_2_type closest_vertex_weight = closest_edge_end_vertex_distance /
					(closest_vertex_distance + closest_edge_end_vertex_distance);
			const delaunay_coord_2_type closest_edge_end_vertex_weight = closest_vertex_distance /
					(closest_vertex_distance + closest_edge_end_vertex_distance);

			// Get the face's vertex indices.
			const int closest_vertex_index = face.get()->index(closest_vertex);
			const int closest_edge_end_vertex_index = face.get()->index(closest_edge_end_vertex.get());

			if (closest_vertex_index == 0)
			{
				barycentric_coord_vertex_1 = closest_vertex_weight;
				if (closest_edge_end_vertex_index == 1)
				{
					barycentric_coord_vertex_2 = closest_edge_end_vertex_weight;
					barycentric_coord_vertex_3 = 0;
				}
				else // closest_edge_end_vertex_index == 2
				{
					barycentric_coord_vertex_3 = closest_edge_end_vertex_weight;
					barycentric_coord_vertex_2 = 0;
				}
			}
			else if (closest_vertex_index == 1)
			{
				barycentric_coord_vertex_2 = closest_vertex_weight;
				if (closest_edge_end_vertex_index == 2)
				{
					barycentric_coord_vertex_3 = closest_edge_end_vertex_weight;
					barycentric_coord_vertex_1 = 0;
				}
				else // closest_edge_end_vertex_index == 0
				{
					barycentric_coord_vertex_1 = closest_edge_end_vertex_weight;
					barycentric_coord_vertex_3 = 0;
				}
			}
			else // closest_vertex_index == 2
			{
				barycentric_coord_vertex_3 = closest_vertex_weight;
				if (closest_edge_end_vertex_index == 0)
				{
					barycentric_coord_vertex_1 = closest_edge_end_vertex_weight;
					barycentric_coord_vertex_2 = 0;
				}
				else // closest_edge_end_vertex_index == 1
				{
					barycentric_coord_vertex_2 = closest_edge_end_vertex_weight;
					barycentric_coord_vertex_1 = 0;
				}
			}

			//qDebug() << "The point "
			//	<< d_projection.unproject_to_lat_lon(point_2)
			//	<< " slipped through the cracks between the topological network boundary on 3D sphere "
			//	<< "and the 2D projected delaunay triangulation - so interpolating nearest delaunay edge "
			//	<< "for barycentric coordinates.";

			return face.get();
		}
	}

	// Unable to find the next closest vertex so just choose any face incident to the closest vertex.
	face = closest_vertex->face();

	// Get the face's closest vertex index.
	const int closest_vertex_index = face.get()->index(closest_vertex);

	if (closest_vertex_index == 0)
	{
		barycentric_coord_vertex_1 = 1;
		barycentric_coord_vertex_2 = 0;
		barycentric_coord_vertex_3 = 0;
	}
	else if (closest_vertex_index == 1)
	{
		barycentric_coord_vertex_2 = 1;
		barycentric_coord_vertex_3 = 0;
		barycentric_coord_vertex_1 = 0;
	}
	else // closest_vertex_index == 2
	{
		barycentric_coord_vertex_3 = 1;
		barycentric_coord_vertex_1 = 0;
		barycentric_coord_vertex_2 = 0;
	}

	// This will probably never happen but emit a debug message to at least record the
	// fact that this event has happened in case someone checks the logs.
	qDebug() << "The point "
		<< d_projection.unproject_to_lat_lon(QPointF(CGAL::to_double(point_2.x()), CGAL::to_double(point_2.y())))
		<< " slipped through the cracks between the topological network boundary on 3D sphere "
		<< "and the 2D projected delaunay triangulation - and could not find nearest delaunay edge "
		<< "- so using nearest delaunay vertex for barycentric coordinates.";

	return face.get();
}


GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Face_handle
GPlatesAppLogic::ResolvedTriangulation::Network::get_delaunay_face_in_deforming_region(
		const delaunay_point_2_type &point_2,
		Delaunay_2::Face_handle start_face_hint) const
{
	// NOTE: We should only be called if the point is in the deforming region.

	const Delaunay_2 &delaunay_2 = get_delaunay_2();

	// Get the barycentric coordinates for the point.
	boost::optional<Delaunay_2::Face_handle> face =
			delaunay_2.get_face_containing_point(point_2, start_face_hint);
	if (face)
	{
		return face.get();
	}

	// If we get here then the point is inside the network when testing against polygons on the
	// *3D* sphere but is outside the *2D* delaunay triangulation (convex hull).
	// This can happen due to numerical tolerances or the fact that a straight line in projected
	// 2D space does not map to a great circle arc on the sphere (we're not using a gnomic projection).
	// In that latter case a point that is just inside the network boundary polygon-on-sphere can
	// get projected to a 2D point that is just outside the 2D convex hull.
	//
	// The current solution is to find the nearest edge of the 2D convex hull and returning the
	// finite face adjacent to it.

	// Find the closest edge incident to the nearest vertex.
	std::pair< Delaunay_2::Vertex_handle, boost::optional<Delaunay_2::Vertex_handle> > closest_edge =
			get_closest_delaunay_convex_hull_edge(point_2);

	const Delaunay_2::Vertex_handle closest_vertex = closest_edge.first;
	boost::optional<Delaunay_2::Vertex_handle> closest_edge_end_vertex = closest_edge.second;

	// Since the point is outside the face, some of the barycentric coordinates would be
	// negative, but they would still all sum to 1.0. Even so we'll ensure they are all
	// positive by choosing a point on the closest edge of the face.

	if (closest_edge_end_vertex)
	{
		// Find the (finite) face containing the closest edge.
		const Delaunay_2::Face_circulator incident_face_circulator_start =
				delaunay_2.incident_faces(closest_vertex);
		Delaunay_2::Face_circulator incident_face_circulator = incident_face_circulator_start;
		do
		{
			// Ignore the infinite face - we're at the edge of the convex hull so one (or two?)
			// adjacent face(s) will be the infinite face.
			if (delaunay_2.is_infinite(incident_face_circulator))
			{
				continue;
			}

			if (incident_face_circulator->has_vertex(closest_edge_end_vertex.get()))
			{
				return incident_face_circulator;
			}
		}
		while (++incident_face_circulator != incident_face_circulator_start);
	}

	// Unable to find the next closest vertex so just choose any face incident to the closest vertex.
	return closest_vertex->face();
}


std::pair<
		GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Vertex_handle,
		boost::optional<GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Vertex_handle> >
GPlatesAppLogic::ResolvedTriangulation::Network::get_closest_delaunay_convex_hull_edge(
		const delaunay_point_2_type &point_2) const
{
	// NOTE: We should only be called if the point is in the deforming region.

	const Delaunay_2 &delaunay_2 = get_delaunay_2();

	// If we get here then the point is inside the deforming region of the network when testing against
	// polygons (boundary and interiors) on the *3D* sphere but is outside the *2D* delaunay triangulation (convex hull).
	// This can happen due to numerical tolerances or the fact that a straight line in projected
	// 2D space does not map to a great circle arc on the sphere (we're not using a gnomic projection).
	// In that latter case a point that it just inside the network boundary polygon-on-sphere can
	// get projected to a 2D point that is just outside the 2D convex hull.
	//
	// The current solution is to find the closest position along the nearest edge of the 2D convex
	// hull (and interpolate the endpoint vertices of that edge).
	// We do this by first finding the closest vertex and then iterating over its incident
	// vertices to find the closest edge (edge is between nearest vertex and incident vertex).
	// This should work since we're at the edge of the convex hull and the test point should
	// be relatively close to an edge of the convex hull.
	const Delaunay_2::Vertex_handle closest_vertex = delaunay_2.nearest_vertex(point_2);

	// Find the closest edge incident to the nearest vertex.
	boost::optional<Delaunay_2::Vertex_handle> closest_edge_end_vertex;
	boost::optional<delaunay_coord_2_type> closest_edge_squared_distance;
	const Delaunay_2::Vertex_circulator incident_vertex_circulator_start =
			delaunay_2.incident_vertices(closest_vertex);
	Delaunay_2::Vertex_circulator incident_vertex_circulator = incident_vertex_circulator_start;
	do
	{
		const Delaunay_2::Vertex_handle incident_vertex = incident_vertex_circulator;

		// Ignore the infinite vertex - we're at the edge of the convex hull so one adjacent
		// vertex will be the infinite vertex.
		if (delaunay_2.is_infinite(incident_vertex))
		{
			continue;
		}

		// Calculate squared distance to current edge.
		const Delaunay_2::Segment edge_segment(closest_vertex->point(), incident_vertex->point());
		const delaunay_coord_2_type edge_squared_distance = CGAL::squared_distance(point_2, edge_segment);

		if (!closest_edge_squared_distance ||
			CGAL::compare(edge_squared_distance, closest_edge_squared_distance.get()) == CGAL::SMALLER)
		{
			closest_edge_squared_distance = edge_squared_distance;
			closest_edge_end_vertex = incident_vertex;
		}
	}
	while (++incident_vertex_circulator != incident_vertex_circulator_start);

	return std::make_pair(closest_vertex, closest_edge_end_vertex);
}


GPlatesMaths::FiniteRotation
GPlatesAppLogic::ResolvedTriangulation::Network::calculate_rigid_block_stage_rotation(
		const ResolvedTriangulation::Network::RigidBlock &rigid_block,
		const double &velocity_delta_time,
		VelocityDeltaTime::Type velocity_delta_time_type) const
{
	ReconstructedFeatureGeometry::non_null_ptr_type rigid_block_rfg =
			rigid_block.get_reconstructed_feature_geometry();

	// Get the rigid block plate id.
	// If we can't get a reconstruction plate ID then we'll just use plate id zero (spin axis)
	// which can still give a non-identity rotation if the anchor plate id is non-zero.
	boost::optional<GPlatesModel::integer_plate_id_type> rigid_block_plate_id =
			rigid_block_rfg->reconstruction_plate_id();
	if (!rigid_block_plate_id)
	{
		rigid_block_plate_id = 0;
	}

	// Calculate the stage rotation for this plate id.
	return PlateVelocityUtils::calculate_stage_rotation(
			rigid_block_plate_id.get(),
			rigid_block_rfg->get_reconstruction_tree_creator(),
			rigid_block_rfg->get_reconstruction_time(),
			velocity_delta_time,
			velocity_delta_time_type);
}


GPlatesMaths::Vector3D
GPlatesAppLogic::ResolvedTriangulation::Network::calculate_rigid_block_velocity(
		const GPlatesMaths::PointOnSphere &point,
		const RigidBlock &rigid_block,
		const double &velocity_delta_time,
		VelocityDeltaTime::Type velocity_delta_time_type) const
{
	ReconstructedFeatureGeometry::non_null_ptr_type rigid_block_rfg =
			rigid_block.get_reconstructed_feature_geometry();

	// Get the rigid block plate id.
	// If we can't get a reconstruction plate ID then we'll just use plate id zero (spin axis)
	// which can still give a non-identity rotation if the anchor plate id is non-zero.
	boost::optional<GPlatesModel::integer_plate_id_type> rigid_block_plate_id =
			rigid_block_rfg->reconstruction_plate_id();
	if (!rigid_block_plate_id)
	{
		rigid_block_plate_id = 0;
	}

	// Calculate the velocity for this plate id.
	return PlateVelocityUtils::calculate_velocity_vector(
			point,
			rigid_block_plate_id.get(),
			rigid_block_rfg->get_reconstruction_tree_creator(),
			rigid_block_rfg->get_reconstruction_time(),
			velocity_delta_time,
			velocity_delta_time_type);
}
