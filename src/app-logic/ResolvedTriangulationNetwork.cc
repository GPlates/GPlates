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

// Sometimes CGal segfaults, trap it
#include <csetjmp>
#include <csignal>
#include <cstring> // for memset
#include <exception>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/number_utils.h>
#include <CGAL/spatial_sort.h>
#include <CGAL/squared_distance_2.h>
#include <CGAL/Triangulation_2.h>
#include <CGAL/Triangulation_conformer_2.h>
#include <QDebug>

#include "ResolvedTriangulationNetwork.h"

#include "GeometryUtils.h"
#include "ReconstructionTree.h"
#include "ResolvedTriangulationUtils.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "maths/CalculateVelocity.h"
#include "maths/FiniteRotation.h"

#include "utils/Profile.h"


// NOTE: use with caution: this can cause the Log window to lag during resize events.
// #define DEBUG


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
				const ResolvedTriangulation::Delaunay_2::Point &point_2_) :
			delaunay_point(&delaunay_point_),
			point_2(point_2_)
		{  }

		//! The delaunay point information.
		const ResolvedTriangulation::Network::DelaunayPoint *delaunay_point;

		//! The 2D projected point.
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
}


// TODO: Re-use one earth radius constant across of all GPlates.
const double GPlatesAppLogic::ResolvedTriangulation::Network::EARTH_RADIUS_METRES = 6371000.0;


bool
GPlatesAppLogic::ResolvedTriangulation::Network::is_point_in_network(
		const GPlatesMaths::PointOnSphere &point) const
{
	// Note that the medium and high speed point-in-polygon tests include a quick small circle
	// bounds test so we don't need to perform that test before the point-in-polygon test.

	const GPlatesMaths::PointInPolygon::Result point_in_network_boundary_result =
			d_network_boundary_polygon->is_point_in_polygon(
					point,
					// Use high speed point-in-poly testing since we're being used for
					// we could be asked to test lots of points.
					// For example, very dense velocity meshes go through this path.
					GPlatesMaths::PolygonOnSphere::HIGH_SPEED_HIGH_SETUP_HIGH_MEMORY_USAGE);

	return point_in_network_boundary_result == GPlatesMaths::PointInPolygon::POINT_INSIDE_POLYGON;
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

		boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> interior_polygon =
				GeometryUtils::get_polygon_on_sphere(
						*rigid_block.get_reconstructed_feature_geometry()->reconstructed_geometry());
		if (interior_polygon)
		{
			// Note that the medium and high speed point-in-polygon tests include a quick small circle
			// bounds test so we don't need to perform that test before the point-in-polygon test.

			const GPlatesMaths::PointInPolygon::Result point_in_rigid_block_result =
					interior_polygon.get()->is_point_in_polygon(
							point,
							// Use high speed point-in-poly testing since we're being used for
							// we could be asked to test lots of points.
							// For example, very dense velocity meshes go through this path.
							GPlatesMaths::PolygonOnSphere::HIGH_SPEED_HIGH_SETUP_HIGH_MEMORY_USAGE);

			if (point_in_rigid_block_result == GPlatesMaths::PointInPolygon::POINT_INSIDE_POLYGON)
			{
				return rigid_block;
			}
		}
	}

	return boost::none;
}


bool
GPlatesAppLogic::ResolvedTriangulation::Network::calc_delaunay_natural_neighbor_coordinates(
		delaunay_natural_neighbor_coordinates_2_type &natural_neighbor_coordinates,
		const GPlatesMaths::PointOnSphere &point) const
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
	calc_delaunay_natural_neighbor_coordinates_in_deforming_region(natural_neighbor_coordinates, point_2);

	return true;
}


boost::optional<GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Face_handle>
GPlatesAppLogic::ResolvedTriangulation::Network::calc_delaunay_barycentric_coordinates(
		delaunay_coord_2_type &barycentric_coord_vertex_1,
		delaunay_coord_2_type &barycentric_coord_vertex_2,
		delaunay_coord_2_type &barycentric_coord_vertex_3,
		const GPlatesMaths::PointOnSphere &point) const
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
			point_2);
}


boost::optional<
		std::pair<
				boost::optional<const GPlatesAppLogic::ResolvedTriangulation::Network::RigidBlock &>,
				GPlatesMaths::Vector3D> >
GPlatesAppLogic::ResolvedTriangulation::Network::calculate_velocity(
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
		const GPlatesMaths::Vector3D rigid_block_velocity =
				calculate_rigid_block_velocity(point, rigid_block.get());

		return std::make_pair(rigid_block, rigid_block_velocity);
	}

	// If we get here then the point must be in the deforming region.

	// Project into the 2D triangulation space.
	const Delaunay_2::Point point_2 =
			d_projection.project_from_point_on_sphere<Delaunay_2::Point>(point);

	// Get the interpolation coordinates for the point.
	delaunay_natural_neighbor_coordinates_2_type natural_neighbor_coordinates;
	calc_delaunay_natural_neighbor_coordinates_in_deforming_region(natural_neighbor_coordinates, point_2);

	// Interpolate the 3D velocity vectors in the triangulation using the interpolation coordinates.
	// Velocity 3D vectors must be interpolated (cannot interpolate velocity colat/lon).
	const GPlatesMaths::Vector3D interpolated_velocity =
			ResolvedTriangulation::linear_interpolation_2(
					natural_neighbor_coordinates,
					CGAL::Data_access<delaunay_point_2_to_velocity_map_type>(
							get_delaunay_point_2_to_velocity_map()));

	return std::make_pair(rigid_block/*boost::none*/, interpolated_velocity);
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
GPlatesAppLogic::ResolvedTriangulation::Network::create_delaunay_2() const
{
	PROFILE_FUNC();

	d_delaunay_2 = Delaunay_2();

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

		// Project point-on-sphere to x,y space.
		const delaunay_point_2_type point_2 =
				d_projection.project_from_point_on_sphere<delaunay_point_2_type>(delaunay_point.point);

		delaunay_point_2_seq.push_back(DelaunayPoint2(delaunay_point, point_2));
	}

	CGAL::spatial_sort(
			delaunay_point_2_seq.begin(),
			delaunay_point_2_seq.end(),
			DelaunayPoint2SpatialSortingTraits());

	// Insert the points into the delaunay triangulation.
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

		// It's possible that the returned vertex handle refers to an existing vertex.
		// This happens if the same position (presumably within an epsilon) is inserted more than once.
		// If this happens we will give preference higher plate ids in order to provide a
		// consistent insert order - this is needed because the spatial sort above can change the
		// order of vertex insertion along the topological sub-segments of the network boundary
		// for example - which can result in vertices at the intersection between two adjacent
		// sub-segments switching order of insertion from one reconstruction time to the next
		// (since both sub-segments have the same end point position) - and this can manifest
		// as randomly switching end point velocities (from one sub-segment plate id to the other).
		if (!vertex_handle->is_initialised() ||
			vertex_handle->get_plate_id() < delaunay_point.plate_id)
		{
			// Set the extra info for this vertex.
			vertex_handle->initialise(
					delaunay_point.point,
					delaunay_point.plate_id,
					delaunay_point.reconstruction_tree_creator,
					d_reconstruction_time);
		}

		// The next insert vertex will start searching at the face of the last inserted vertex.
		insert_start_face = vertex_handle->face();
	}

	// Now that we've finished inserting vertices into the delaunay triangulation, and hence
	// finished generating triangulation faces, we can now initialise each face.
	int face_index = 0;
	Delaunay_2::Finite_faces_iterator finite_faces_2_iter = d_delaunay_2->finite_faces_begin();
	Delaunay_2::Finite_faces_iterator finite_faces_2_end = d_delaunay_2->finite_faces_end();
	for ( ; finite_faces_2_iter != finite_faces_2_end; ++finite_faces_2_iter, ++face_index)
	{
		finite_faces_2_iter->initialise(face_index);
	}

#ifdef DEBUG
qDebug() << "";
qDebug() << "ResolvedTriangulation::Network:             delaunay_triangulation_2 verts ="
		<< d_delaunay_2->number_of_vertices();
qDebug() << "ResolvedTriangulation::Network:             delaunay_triangulation_2 faces ="
		<< d_delaunay_2->number_of_faces();
qDebug() << "ResolvedTriangulation::Network: constrained_delaunay_triangulation_2 verts ="
		<< d_delaunay_2->number_of_vertices();
qDebug() << "ResolvedTriangulation::Network: constrained_delaunay_triangulation_2 faces ="
		<< d_delaunay_2->number_of_faces();
#endif
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
		GeometryUtils::get_geometry_points(
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

#ifdef DEBUG
qDebug() << "Number of verts after make_conforming_Delaunay_2: " << d_constrained_delaunay_2->number_of_vertices();
qDebug() << "Number of faces after make_conforming_Delaunay_2: " << d_constrained_delaunay_2->number_of_faces();
#endif

	// Then make it conforming Gabriel.
	CGAL::make_conforming_Gabriel_2(d_constrained_delaunay_2.get());

#ifdef DEBUG
qDebug() << "Number of verts after make_conforming_Gabriel_2: " << d_constrained_delaunay_2->number_of_vertices();
qDebug() << "Number of faces after make_conforming_Gabriel_2: " << d_constrained_delaunay_2->number_of_faces();
#endif
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


const GPlatesAppLogic::ResolvedTriangulation::Network::delaunay_point_2_to_velocity_map_type &
GPlatesAppLogic::ResolvedTriangulation::Network::get_delaunay_point_2_to_velocity_map() const
{
	if (!d_delaunay_point_2_to_velocity_map)
	{
		create_delaunay_point_2_to_velocity_map();
	}

	return d_delaunay_point_2_to_velocity_map.get();
}


void
GPlatesAppLogic::ResolvedTriangulation::Network::create_delaunay_point_2_to_velocity_map() const
{
	d_delaunay_point_2_to_velocity_map = delaunay_point_2_to_velocity_map_type();

	const Delaunay_2 &delaunay_2 = get_delaunay_2();

	// Iterate over the vertices of the delaunay triangulation.
	Delaunay_2::Finite_vertices_iterator finite_vertices_iter = delaunay_2.finite_vertices_begin();
	Delaunay_2::Finite_vertices_iterator finite_vertices_end = delaunay_2.finite_vertices_end();
	for ( ; finite_vertices_iter != finite_vertices_end; ++finite_vertices_iter)
	{
		// Map the triangulation vertex 2D point to its associated velocity.
		d_delaunay_point_2_to_velocity_map->insert(
				delaunay_point_2_to_velocity_map_type::value_type(
						finite_vertices_iter->point(),
						// Store velocity as 3D vectors since colat/lon cannot be interpolated...
						finite_vertices_iter->get_velocity_vector()));
	}
}


void
GPlatesAppLogic::ResolvedTriangulation::Network::calc_delaunay_natural_neighbor_coordinates_in_deforming_region(
		delaunay_natural_neighbor_coordinates_2_type &natural_neighbor_coordinates,
		const delaunay_point_2_type &point_2) const
{
	// NOTE: We should only be called if the point is in the deforming region.

	const Delaunay_2 &delaunay_2 = get_delaunay_2();

	// Get the interpolation coordinates for the point.
	if (delaunay_2.calc_natural_neighbor_coordinates(natural_neighbor_coordinates, point_2))
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
		const delaunay_point_2_type &point_2) const
{
	// NOTE: We should only be called if the point is in the deforming region.

	const Delaunay_2 &delaunay_2 = get_delaunay_2();

	// Get the barycentric coordinates for the point.
	boost::optional<Delaunay_2::Face_handle> face =
			delaunay_2.calc_barycentric_coordinates(
					barycentric_coord_vertex_1,
					barycentric_coord_vertex_2,
					barycentric_coord_vertex_3,
					point_2);
	if (face)
	{
		return face.get();
	}

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


std::pair<
		GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Vertex_handle,
		boost::optional<GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Vertex_handle> >
GPlatesAppLogic::ResolvedTriangulation::Network::get_closest_delaunay_convex_hull_edge(
		const delaunay_point_2_type &point_2) const
{
	// NOTE: We should only be called if the point is in the deforming region.

	const Delaunay_2 &delaunay_2 = get_delaunay_2();

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


GPlatesMaths::Vector3D
GPlatesAppLogic::ResolvedTriangulation::Network::calculate_rigid_block_velocity(
		const GPlatesMaths::PointOnSphere &point,
		const RigidBlock &rigid_block) const
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

	// Get the reconstruction trees to calculate velocity with.
	const ReconstructionTree::non_null_ptr_to_const_type rigid_block_recon_tree1 =
			rigid_block_rfg->get_reconstruction_tree();
	const ReconstructionTree::non_null_ptr_to_const_type rigid_block_recon_tree2 =
			rigid_block_rfg->get_reconstruction_tree_creator().get_reconstruction_tree(
					// FIXME:  Should this '1' should be user controllable? ...
					rigid_block_recon_tree1->get_reconstruction_time() + 1);

	// Get the finite rotations for this plate id.
	const GPlatesMaths::FiniteRotation &rigid_block_finite_rotation1 =
			rigid_block_recon_tree1->get_composed_absolute_rotation(rigid_block_plate_id.get()).first;
	const GPlatesMaths::FiniteRotation &rigid_block_finite_rotation2 =
			rigid_block_recon_tree2->get_composed_absolute_rotation(rigid_block_plate_id.get()).first;

	return GPlatesMaths::calculate_velocity_vector(
			point,
			rigid_block_finite_rotation1,
			rigid_block_finite_rotation2);
}