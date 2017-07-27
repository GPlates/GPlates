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
#include <csetjmp> // Sometimes CGal segfaults, trap it
#include <csignal>
#include <cstring> // for memset
#include <exception>
#include <limits>
#include <boost/bind.hpp>
#include <boost/utility/in_place_factory.hpp>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/number_utils.h>
#include <CGAL/spatial_sort.h>
#include <CGAL/squared_distance_2.h>
#include <CGAL/Triangulation_2.h>
#include <CGAL/Triangulation_conformer_2.h>
#include <CGAL/Delaunay_triangulation_2.h>
#include <CGAL/Triangle_2.h>
#include <QDebug>

#include "ResolvedTriangulationNetwork.h"

#include "GeometryUtils.h"
#include "PlateVelocityUtils.h"
#include "ReconstructionTree.h"
#include "ResolvedTriangulationUtils.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "maths/CalculateVelocity.h"
#include "maths/Centroid.h"
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
	//
	// CGAL typedefs.
	//
	// NOTE: Not putting these in an anonymous namespace because some compilers complain
	// about lack of external linkage when compiling CGAL.
	//

	/**
	 * Same as ResolvedTriangulation::Network::DelaunayPoint but stores a 2D point (instead of 3D)
	 * so can be spatialy sorted by CGAL.
	 */
	struct DelaunayPoint2
	{

		DelaunayPoint2(
				const ResolvedTriangulation::Network::DelaunayPoint &delaunay_point_,
				const GPlatesMaths::LatLonPoint &lat_lon_point_,
				const ResolvedTriangulation::Delaunay_2::Point &point_2_) :
			delaunay_point(&delaunay_point_),
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


#if 0 // Not currently using being used...
	namespace
	{
	#if !defined(__WINDOWS__)
		// The old sigaction for SIGSEGV before we called sigaction().
		struct sigaction old_action;

		// Holds the data necessary for setjmp/longjmp to work.
		jmp_buf buf;


		/**
 		* Handles the SIGSEGV signal.
 		*/
		void
		handler(int signum)
		{
			// Jump past the problem call.
			/* non-zero value so that the problem call doesn't get called again */
			//qDebug() << "handler";
			longjmp( buf, 1);
		}
	#endif
	}
#endif // ...not currently using being used.
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
		return barycentric_coord_vertex_1 * delaunay_face->vertex(0)->get_deformation_info() +
				barycentric_coord_vertex_2 * delaunay_face->vertex(1)->get_deformation_info() +
				barycentric_coord_vertex_3 * delaunay_face->vertex(2)->get_deformation_info();
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
	// ...depending on 'reverse_deform'.
	//
	// However, when 'reverse_deform' is false (ie, going backward in time) we'll need to reverse our rotations to get:
	//
	//  backward in time:   reconstruction_time -> reconstruction_time + time_increment
	//  forward  in time:   reconstruction_time -> reconstruction_time - time_increment
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
			barycentric_coord_vertex_1 * deformed_point_1.x() +
			barycentric_coord_vertex_2 * deformed_point_2.x() +
			barycentric_coord_vertex_3 * deformed_point_3.x(),
			barycentric_coord_vertex_1 * deformed_point_1.y() +
			barycentric_coord_vertex_2 * deformed_point_2.y() +
			barycentric_coord_vertex_3 * deformed_point_3.y());

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
					barycentric_coord_vertex_1,
					barycentric_coord_vertex_2,
					barycentric_coord_vertex_3);

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

	d_delaunay_2 = boost::in_place(d_projection, d_reconstruction_time);

	// Improve performance by spatially sorting the delaunay points.
	// This is what is done by the CGAL overload that inserts a *range* of points into a delauany triangulation.
	//
	// Project the points to 2D space and insert into array to be spatially sorted.
	std::vector<DelaunayPoint2> delaunay_point_2_seq;
	delaunay_point_2_seq.reserve(d_build_info.delaunay_points.size());
	std::vector<DelaunayPoint>::const_iterator delaunay_points_iter = d_build_info.delaunay_points.begin();
	std::vector<DelaunayPoint>::const_iterator delaunay_points_end = d_build_info.delaunay_points.end();
	for ( ; delaunay_points_iter != delaunay_points_end; ++delaunay_points_iter)
	{
		const DelaunayPoint &delaunay_point = *delaunay_points_iter;

		// Cache our lat/lon coordinates - otherwise the projection needs to convert to lat/lon
		// internally - and so might as well only do the lat/lon conversion once for efficiency.
		const GPlatesMaths::LatLonPoint lat_lon_point = make_lat_lon_point(delaunay_point.point);

		// Project point-on-sphere to x,y space.
		const delaunay_point_2_type point_2 =
				d_projection.project_from_lat_lon<delaunay_point_2_type>(lat_lon_point);

		delaunay_point_2_seq.push_back(DelaunayPoint2(delaunay_point, lat_lon_point, point_2));
	}

	CGAL::spatial_sort(
			delaunay_point_2_seq.begin(),
			delaunay_point_2_seq.end(),
			DelaunayPoint2SpatialSortingTraits());

	// Insert the points into the delaunay triangulation.
	unsigned int vertex_index = 0;
	std::vector<DelaunayPoint2>::const_iterator delaunay_points_2_iter = delaunay_point_2_seq.begin();
	std::vector<DelaunayPoint2>::const_iterator delaunay_points_2_end = delaunay_point_2_seq.end();
	Delaunay_2::Face_handle insert_start_face;
	for ( ; delaunay_points_2_iter != delaunay_points_2_end; ++delaunay_points_2_iter)
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
					delaunay_point.plate_id,
					delaunay_point.reconstruction_tree_creator);

			// Increment vertex index since vertex handle does not refer to an existing vertex position.
			++vertex_index;
		}
		// It's possible that the returned vertex handle refers to an existing vertex.
		// This happens if the same position (presumably within an epsilon) is inserted more than once.
		// If this happens we will give preference higher plate ids in order to provide a
		// consistent insert order - this is needed because the spatial sort above can change the
		// order of vertex insertion along the topological sub-segments of the network boundary
		// for example - which can result in vertices at the intersection between two adjacent
		// sub-segments switching order of insertion from one reconstruction time to the next
		// (since both sub-segments have the same end point position) - and this can manifest
		// as randomly switching end point velocities (from one sub-segment plate id to the other).
		else if (vertex_handle->get_plate_id() < delaunay_point.plate_id)
		{
			// Reset the extra info for this vertex.
			vertex_handle->initialise(
					d_delaunay_2.get(),
					// Note: This is an existing vertex position so re-use the vertex index previously assigned...
					vertex_handle->get_vertex_index(),
					delaunay_point.point,
					delaunay_point_2.lat_lon_point,
					delaunay_point.plate_id,
					delaunay_point.reconstruction_tree_creator);
		}

		// The next insert vertex will start searching at the face of the last inserted vertex.
		insert_start_face = vertex_handle->face();
	}

	// Now that we've finished inserting vertices into the delaunay triangulation, and hence
	// finished generating triangulation faces, we can now initialise each face.
	unsigned int face_index = 0;
	Delaunay_2::Finite_faces_iterator finite_faces_2_iter = d_delaunay_2->finite_faces_begin();
	Delaunay_2::Finite_faces_iterator finite_faces_2_end = d_delaunay_2->finite_faces_end();
	for ( ; finite_faces_2_iter != finite_faces_2_end; ++finite_faces_2_iter, ++face_index)
	{
		// Determine whether current face is inside the deforming region.
		//
		// The delaunay triangulation is the convex hull around the network boundary,
		// so it includes faces outside the network boundary (and also faces inside any
		// non-deforming interior blocks).
		//
		// If the centroid of this face is inside the deforming region then true is returned.
		//
		// TODO: Note that the delaunay triangulation is *not* constrained which means some
		// delaunay faces can cross over network boundary edges or interior block edges.
		// This is something that perhaps needs to be dealt with, but currently doesn't appear
		// to be too much of a problem with current topological network datasets.
		// Compute centroid of face.
		const GPlatesMaths::PointOnSphere face_points[3] =
		{
			finite_faces_2_iter->vertex(0)->get_point_on_sphere(),
			finite_faces_2_iter->vertex(1)->get_point_on_sphere(),
			finite_faces_2_iter->vertex(2)->get_point_on_sphere()
		};
		const GPlatesMaths::PointOnSphere face_centroid(
				GPlatesMaths::Centroid::calculate_points_centroid(face_points, face_points + 3));

		// Note that we don't actually calculate the deformation strains here.
		// They'll get calculated if and when they are needed.
		finite_faces_2_iter->initialise(
				d_delaunay_2.get(),
				face_index,
				is_point_in_deforming_region(face_centroid));
	}
}


#if 0 // Not currently using being used...

const GPlatesAppLogic::ResolvedTriangulation::ConstrainedDelaunay_2 &
GPlatesAppLogic::ResolvedTriangulation::Network::get_constrained_delaunay_2() const
{
	if (!d_constrained_delaunay_2)
	{
		create_constrained_delaunay_2();

		// Emit a warning if triangulation is invalid, otherwise refine the triangulation.
		if (d_constrained_delaunay_2->is_valid())
		{
			if (refine_constrained_delaunay_2())
			{
				// Only make conforming if the refinement step succeeded.
				make_conforming_constrained_delaunay_2();
			}
		}
		else
		{
			qWarning() << "ResolvedTriangulation::Network: Constrained delaunay triangulation is invalid.";
		}

		// Release some build data memory since we don't need it anymore.
		std::vector<ConstrainedDelaunayGeometry> empty_constrained_delaunay_geometries;
		d_build_info.constrained_delaunay_geometries.swap(empty_constrained_delaunay_geometries);

		std::vector<GPlatesMaths::PointOnSphere> empty_scattered_points;
		d_build_info.scattered_points.swap(empty_scattered_points);

		std::vector<GPlatesMaths::PointOnSphere> empty_seed_points;
		d_build_info.seed_points.swap(empty_seed_points);
	}

	return d_constrained_delaunay_2.get();
}


const GPlatesAppLogic::ResolvedTriangulation::Delaunay_3 &
GPlatesAppLogic::ResolvedTriangulation::Network::get_delaunay_3() const
{
	if (!d_delaunay_3)
	{
		create_delaunay_3();
	}

	return d_delaunay_3.get();
}


void
GPlatesAppLogic::ResolvedTriangulation::Network::create_constrained_delaunay_2() const
{
	PROFILE_FUNC();

	d_constrained_delaunay_2 = ConstrainedDelaunay_2();

	// Insert the geometries forming linear constraints along their length.
	insert_geometries_into_constrained_delaunay_2();

	// 2D + C
	// Do NOT constrain every point to ever other point.
	insert_scattered_points_into_constrained_delaunay_2(false/*constrain_all_points*/);

#if 0
// NOTE : these are examples of how to add points to all the triangulation types
// 2D ; 2D + C ; 3D , using all_network_points ; save them for reference
	
	// 2D + C
	d_constrained_delaunay_2->insert_points(
		all_network_points.begin(), 
		all_network_points.end(),
		true);
#endif
}


void
GPlatesAppLogic::ResolvedTriangulation::Network::insert_geometries_into_constrained_delaunay_2() const
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_constrained_delaunay_2,
			GPLATES_ASSERTION_SOURCE);

	if (d_build_info.constrained_delaunay_geometries.empty())
	{
		return;
	}

	// Iterate over the constraint geometries.
	std::vector<ConstrainedDelaunayGeometry>::const_iterator constrained_delaunay_geometries_iter =
			d_build_info.constrained_delaunay_geometries.begin();
	std::vector<ConstrainedDelaunayGeometry>::const_iterator constrained_delaunay_geometries_end =
			d_build_info.constrained_delaunay_geometries.end();
	for ( ;
		constrained_delaunay_geometries_iter != constrained_delaunay_geometries_end;
		++constrained_delaunay_geometries_iter)
	{
		const ConstrainedDelaunayGeometry &constrained_delaunay_geometry =
				*constrained_delaunay_geometries_iter;

		// Get the points in the current geometry.
		std::vector<GPlatesMaths::PointOnSphere> geometry_points;
		GeometryUtils::get_geometry_exterior_points(
				*constrained_delaunay_geometry.geometry,
				geometry_points);

		std::vector<GPlatesMaths::PointOnSphere>::const_iterator points_begin = geometry_points.begin();
		std::vector<GPlatesMaths::PointOnSphere>::const_iterator points_end = geometry_points.end();

		// If no points then skip to the next geometry.
		if (points_begin == points_end)
		{
			continue;
		}

		std::vector<ConstrainedDelaunay_2::Vertex_handle> vertex_handles;

		// Double check for identical start and end points 
		if ( *points_begin == *(points_end - 1) )
		{
			points_end = points_end - 1; // disregard duplicate point
		}

		// Loop over the points, project them, and add them to the triangulation
		for (std::vector<GPlatesMaths::PointOnSphere>::const_iterator points_iter = points_begin;
			points_iter != points_end;
			++points_iter)
		{
			insert_vertex_into_constrained_delaunay_2(*points_iter, vertex_handles);
		}
		//qDebug() << "after loop vertex_handles.size() " << vertex_handles.size();

		// Now work out the constraints 
		// Do not add a constraint for a single point
		if ( points_begin == (points_end - 1) )
		{
			//qDebug() << "Cannot insert a constraint for a single point";
			continue;
		}

		std::vector<ConstrainedDelaunay_2::Vertex_handle>::iterator v_end;

		// set the end bound of the loop
		if (constrained_delaunay_geometry.constrain_begin_and_end_points)
		{
			// loop all the way to the end of the list; 
			// v2 will be reset to vertex_handles.begin() below
			v_end = vertex_handles.end();
		}
		else
		{
			// loop to one less than the end
			v_end = --(vertex_handles.end());
		}

		// qDebug() << "Loop over the vertex handles, insert constrained edges";

		// Loop over the vertex handles, insert constrained edges
		for (std::vector<ConstrainedDelaunay_2::Vertex_handle>::iterator v1_iter = vertex_handles.begin();
			v1_iter != v_end;
			++v1_iter)
		{
			// get the next vertex handle
			std::vector<ConstrainedDelaunay_2::Vertex_handle>::iterator v2_iter = v1_iter + 1;

			// Check for constraint on begin and end points
			if ( constrained_delaunay_geometry.constrain_begin_and_end_points )
			{
				// close the loop
				if (v2_iter == vertex_handles.end() ) 
				{ 
					//qDebug() << "Close the loop: v2_iter == vertex_handles.end()";
					v2_iter = vertex_handles.begin(); 
				} 
			}
			// else no need to reset v2

			// get the vertex handles and insert the constraint 
			ConstrainedDelaunay_2::Vertex_handle v1 = *v1_iter;
			ConstrainedDelaunay_2::Vertex_handle v2 = *v2_iter;

			// Double check on constraining the same point to itself 
			if (v1 == v2) 
			{
				// qDebug() << "Cannot insert_constraint: v1 == v2";
				continue;
			}

			try
			{
				//qDebug() << "before insert_constraint(v1,v2)";
				d_constrained_delaunay_2->insert_constraint(v1, v2);
			}
			catch (std::exception &exc)
			{
				qWarning() << exc.what();
			}
			catch (...)
			{
				qWarning() << "insert_points: Unknown error";
				qWarning() << "insert_constraint(v1,v2) failed!";
			}
		}
	}
}


void
GPlatesAppLogic::ResolvedTriangulation::Network::insert_scattered_points_into_constrained_delaunay_2(
		bool constrain_all_points) const
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_constrained_delaunay_2,
			GPLATES_ASSERTION_SOURCE);

	if (d_build_info.scattered_points.empty())
	{
		return;
	}

	//qDebug() << "insert_scattered_points";
	//qDebug() << "Constrain all points? " << constrain_all_points;

	std::vector<ConstrainedDelaunay_2::Vertex_handle> vertex_handles;

	// Loop over the points, project them, and add them to the triangulation.
	std::vector<GPlatesMaths::PointOnSphere>::const_iterator scattered_points_iter =
			d_build_info.scattered_points.begin();
	std::vector<GPlatesMaths::PointOnSphere>::const_iterator scattered_points_end =
			d_build_info.scattered_points.end();
	for ( ; scattered_points_iter != scattered_points_end; ++scattered_points_iter)
	{
		insert_vertex_into_constrained_delaunay_2(*scattered_points_iter, vertex_handles);
	}
	//qDebug() << "insert_scattered_points: vertex_handles.size() " << vertex_handles.size();

	if (!constrain_all_points) 
	{
		//qDebug() << "do not insert_constraints:";
		return;
	}

	// Now work out the constraints 
	//qDebug() << "insert_constraints:";

	std::vector<ConstrainedDelaunay_2::Vertex_handle>::iterator v_end = vertex_handles.end();

	// Loop over the vertex handles, insert constrained edges
	for (std::vector<ConstrainedDelaunay_2::Vertex_handle>::iterator v1_iter = vertex_handles.begin();
		v1_iter != v_end;
		++v1_iter)
	{
		ConstrainedDelaunay_2::Vertex_handle v1 = *v1_iter;

		for (std::vector<ConstrainedDelaunay_2::Vertex_handle>::iterator v2_iter = vertex_handles.begin();
			v2_iter != v_end;
			++v2_iter)
		{
			// get the vertex handles and insert the constraint 
			ConstrainedDelaunay_2::Vertex_handle v2 = *v2_iter;

			// Double check on constraining the same point to itself 
			if (v1 == v2) 
			{
				//qDebug() << "Cannot insert_constraint: v1 == v2";
				continue;
			}

			try
			{
				//qDebug() << "before insert_constraint: v1 != v2";
				d_constrained_delaunay_2->insert_constraint(v1, v2);
			}
			catch (std::exception &exc)
			{
				qWarning() << exc.what();
			}
			catch (...)
			{
				qWarning() << "insert_scattered_points: Unknown error";
				qWarning() << "insert_constraint(v1,v2) failed!";
			}
		}
	}
}


void
GPlatesAppLogic::ResolvedTriangulation::Network::insert_vertex_into_constrained_delaunay_2(
		const GPlatesMaths::PointOnSphere &point_on_sphere,
		std::vector<ConstrainedDelaunay_2::Vertex_handle> &vertex_handles) const
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_constrained_delaunay_2,
			GPLATES_ASSERTION_SOURCE);

	const constrained_delaunay_point_2_type point_2 =
			d_projection.project_from_point_on_sphere<constrained_delaunay_point_2_type>(point_on_sphere);

	//const GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(point_on_sphere);
	//qDebug() << "before insert_vertex llp = " << llp;
	//qDebug() << "before insert_vertex vertex_handles.size() " << vertex_handles.size();
	//qDebug() << "before insert_vertex point = " << point.x() << "," << point.y();

#if !defined(__WINDOWS__)
	// Set a handler for SIGSEGV in case the call does a segfault.
	struct sigaction action;
	std::memset(&action, 0, sizeof(action)); // sizeof(struct sigaction) is platform dependent.
	action.sa_handler = &handler;
	sigaction(SIGSEGV, &action, &old_action);

	// Try and call 
	if (!setjmp( buf ))
	{
#endif
		// The first time setjmp() is called, it returns 0. If call segfaults,
		// we longjmp back to the if statement, but with a non-zero value.
		//qDebug() << " call insert ";
		ConstrainedDelaunay_2::Vertex_handle vh = d_constrained_delaunay_2->insert( point_2 );
		vertex_handles.push_back( vh );
	
#if !defined(__WINDOWS__)
	}
	//qDebug() << " after insert ";
	// Restore the old sigaction whether or not we segfaulted.
	sigaction(SIGSEGV, &old_action, NULL);
#endif
}


bool
GPlatesAppLogic::ResolvedTriangulation::Network::refine_constrained_delaunay_2() const
{
	PROFILE_FUNC();

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_constrained_delaunay_2,
			GPLATES_ASSERTION_SOURCE);

	// 2D + C + Mesh
	ConstrainedDelaunayMesher_2 constrained_mesher(d_constrained_delaunay_2.get());
	ConstrainedDelaunayMesher_2::Criteria constrained_criteria(
			d_build_info.refined_mesh_shape_factor,
			d_build_info.refined_mesh_max_edge);
	constrained_mesher.set_criteria(constrained_criteria);

	// Insert the mesh seed points.
	if (!d_build_info.seed_points.empty())
	{
		// Project the seed points.
		std::vector<constrained_delaunay_point_2_type> seed_point_2_seq;
		seed_point_2_seq.reserve(d_build_info.seed_points.size());
		std::vector<GPlatesMaths::PointOnSphere>::const_iterator seed_points_iter =
				d_build_info.seed_points.begin();
		std::vector<GPlatesMaths::PointOnSphere>::const_iterator seed_points_end =
				d_build_info.seed_points.end();
		for ( ; seed_points_iter != seed_points_end; ++seed_points_iter)
		{
			seed_point_2_seq.push_back(
					d_projection.project_from_point_on_sphere<constrained_delaunay_point_2_type>(
							*seed_points_iter));
		}

		// Only mesh those bounded connected components that do not include a seed point.
		constrained_mesher.clear_seeds();
		constrained_mesher.set_seeds(
			seed_point_2_seq.begin(), 
			seed_point_2_seq.end(),
			false/*mark*/);
	}

	// Try to refine the triangulation.
	try 
	{
		constrained_mesher.refine_mesh();
	}
	catch (std::exception &exc)
	{
		qWarning() << "ResolvedTriangulation::Network: unable to refine constrained delaunay triangulation: "
			<< exc.what();
		return false;
	}
	catch ( ... )
	{
		qWarning() << "ResolvedTriangulation::Network: unable to refine  constrained delaunay triangulation: "
				<< "unknown error";
		return false;
	}

	return true;
}


void
GPlatesAppLogic::ResolvedTriangulation::Network::make_conforming_constrained_delaunay_2() const
{
	PROFILE_FUNC();

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_constrained_delaunay_2,
			GPLATES_ASSERTION_SOURCE);

	// Make it conforming Delaunay.
	CGAL::make_conforming_Delaunay_2(d_constrained_delaunay_2.get());

	// Then make it conforming Gabriel.
	CGAL::make_conforming_Gabriel_2(d_constrained_delaunay_2.get());
}


void
GPlatesAppLogic::ResolvedTriangulation::Network::create_delaunay_3() const
{
	d_delaunay_3 = Delaunay_3();

#if 0
// NOTE : these are examples of how to add points to all the triangulation types
// 2D ; 2D + C ; 3D , using all_network_points ; save them for reference

	std::vector<point_3_type> cgal_points;

	// Loop over the points and convert them.
	std::vector<GPlatesMaths::PointOnSphere>::const_iterator points_iter = d_build_info.points.begin();
	std::vector<GPlatesMaths::PointOnSphere>::const_iterator points_end = d_build_info.points.end();
	for ( ; points_iter != points_end; ++points_iter)
	{
		cgal_points.push_back(convert_point_on_sphere_to_point_3(*points_iter));
	}

	// Build the Triangulation.
	d_delaunay_3->insert( cgal_points.begin(), cgal_points.end() );
#endif
}

#endif // ...not currently using being used.


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
				CGAL::sqrt(CGAL::squared_distance(point_2, closest_vertex->point()));
		const delaunay_coord_2_type closest_edge_end_vertex_distance =
				CGAL::sqrt(CGAL::squared_distance(point_2, closest_edge_end_vertex.get()->point()));

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
			<< d_projection.unproject_to_lat_lon(point_2)
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
					CGAL::sqrt(CGAL::squared_distance(point_2, closest_vertex->point()));
			const delaunay_coord_2_type closest_edge_end_vertex_distance =
					CGAL::sqrt(CGAL::squared_distance(point_2, closest_edge_end_vertex.get()->point()));

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
		<< d_projection.unproject_to_lat_lon(point_2)
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
