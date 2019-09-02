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

//
// Workaround for a bug in <CGAL/Delaunay_triangulation_2.h>.
//
// http://cgal-discuss.949826.n4.nabble.com/Natural-neighbor-coordinates-infinite-loop-tp4659522p4664137.html
//
// By defining CGAL_DT2_USE_RECURSIVE_PROPAGATE_CONFLICTS we can use an alternative piece of code
// in that file to avoid the bug. The alternate code uses recursive propagation for all depths and
// does not switch to non-recursive code at depth 100. However there is a risk that we could exceed
// our C stack memory doing this if the conflict region is large enough (spans many faces).
// But typical deforming networks in GPlates are not really large enough for this to be a problem.
//
// In any case, this workaround is only employed for CGAL < 4.12.2 and CGAL 4.13.0.
// The bug was fixed in 4.12.2, 4.13.1 and 4.14 (according to the link above).
//
// NOTE: This workaround needs to be placed at the top of ".cc" file to prior to any direct or
//       indirect include of <CGAL/Delaunay_triangulation_2.h>.
//       It should also be placed at the top of the PCH header used for this (app-logic) project.
//
#include <CGAL/config.h>
#if (CGAL_VERSION_NR < CGAL_VERSION_NUMBER(4, 12, 2)) || (CGAL_VERSION_NR == CGAL_VERSION_NUMBER(4, 13, 0))
#	define CGAL_DT2_USE_RECURSIVE_PROPAGATE_CONFLICTS
#endif

#include "ResolvedTriangulationDelaunay2.h"

#include "ResolvedTriangulationNetwork.h"
#include "ResolvedTriangulationUtils.h"

#include "utils/Earth.h"
#include "utils/Profile.h"


namespace GPlatesAppLogic
{
	namespace ResolvedTriangulation
	{
		const double EARTH_RADIUS_METRES = 1e3 * GPlatesUtils::Earth::MEAN_RADIUS_KMS;
		const double INVERSE_EARTH_RADIUS_METRES = 1.0 / EARTH_RADIUS_METRES;
	}
}


GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Delaunay_2(
		const Network &network,
		const double &reconstruction_time) :
	d_network(network),
	d_reconstruction_time(reconstruction_time),
	// Modifications to triangulation (such as inserting vertices) continue after constructor
	// until 'set_finished_modifying_triangulation()' is called by owner class Network...
	d_finished_modifying_triangulation(false)
{
	// See if strain rate clamping requested.
	if (network.get_strain_rate_clamping().enable_clamping)
	{
		d_clamp_total_strain_rate = network.get_strain_rate_clamping().max_total_strain_rate;
	}
}


bool
GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::calc_natural_neighbor_coordinates(
		delaunay_natural_neighbor_coordinates_2_type &natural_neighbor_coordinates,
		const delaunay_point_2_type &point,
		Face_handle start_face_hint) const
{
	// Build the natural neighbor coordinates.
	CGAL::Triple< std::back_insert_iterator<delaunay_point_coordinate_vector_2_type>, delaunay_coord_2_type, bool>
			coordinate_result =
					CGAL::natural_neighbor_coordinates_2(
							*this,
							point,
							std::back_inserter(natural_neighbor_coordinates.first),
							start_face_hint);

	natural_neighbor_coordinates.second = coordinate_result.second/*norm*/;

	if (!coordinate_result.third/*in_triangulation*/)
	{
		return false;
	}

	if (natural_neighbor_coordinates.second > 0)
	{
		// Normalisation factor greater than zero - everything's fine.
		return true/*in_triangulation*/;
	}

	//
	// It appears that CGAL can trigger assertions under certain situations (at certain query points).
	// This is most likely due to us not using exact arithmetic in our delaunay triangulation
	// (currently we use 'CGAL::Exact_predicates_inexact_constructions_kernel').
	// The assertion seems to manifest as a normalisation factor of zero - so we checked for zero above.
	//
	// We get around this by converting barycentric coordinates to natural neighbour coordinates.
	//

	delaunay_coord_2_type barycentric_coord_vertex_1;
	delaunay_coord_2_type barycentric_coord_vertex_2;
	delaunay_coord_2_type barycentric_coord_vertex_3;
	const boost::optional<Face_handle> face = calc_barycentric_coordinates(
			barycentric_coord_vertex_1,
			barycentric_coord_vertex_2,
			barycentric_coord_vertex_3,
			point,
			start_face_hint);
	if (!face)
	{
		return false/*in_triangulation*/;
	}

	natural_neighbor_coordinates.first.clear();
	natural_neighbor_coordinates.first.push_back(
			std::make_pair(face.get()->vertex(0)->point(), barycentric_coord_vertex_1));
	natural_neighbor_coordinates.first.push_back(
			std::make_pair(face.get()->vertex(1)->point(), barycentric_coord_vertex_2));
	natural_neighbor_coordinates.first.push_back(
			std::make_pair(face.get()->vertex(2)->point(), barycentric_coord_vertex_3));

	natural_neighbor_coordinates.second = 1.0;

	return true/*in_triangulation*/;
}


boost::optional<GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Face_handle>
GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::calc_barycentric_coordinates(
		delaunay_coord_2_type &barycentric_coord_vertex_1,
		delaunay_coord_2_type &barycentric_coord_vertex_2,
		delaunay_coord_2_type &barycentric_coord_vertex_3,
		const delaunay_point_2_type &point,
		Face_handle start_face_hint) const
{
	// Locate the (finite) face that the point is inside (if any).
	const boost::optional<Face_handle> face_opt = get_face_containing_point(point, start_face_hint);
	if (!face_opt)
	{
		return boost::none;
	}
	const Face_handle face = face_opt.get();

	delaunay_coord_2_type barycentric_norm;
	ResolvedTriangulation::get_barycentric_coords_2(
			point,
			face->vertex(0)->point(),
			face->vertex(1)->point(),
			face->vertex(2)->point(),
			barycentric_norm,
			barycentric_coord_vertex_1,
			barycentric_coord_vertex_2,
			barycentric_coord_vertex_3);

	return face;
}


boost::optional<GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Face_handle>
GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::get_face_containing_point(
		const delaunay_point_2_type &point,
		Face_handle start_face_hint) const
{
	// Locate the (finite) face that the point is inside (if any).
	Locate_type locate_type;
	int li;
	const Face_handle face = locate(point, locate_type, li, start_face_hint);

	if (locate_type != FACE &&
		locate_type != EDGE &&
		locate_type != VERTEX)
	{
		// Point was not inside the convex hull (delaunay triangulation).
		return boost::none;
	}

	return face;
}


GPlatesAppLogic::ResolvedTriangulation::delaunay_vector_2_type
GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::gradient_2(
		const delaunay_point_2_type &point,
		const delaunay_map_point_to_value_2_type &function_values) const
{
	typedef CGAL::Interpolation_gradient_fitting_traits_2<delaunay_kernel_2_type> Traits;

	// coordinate comp.
	delaunay_point_coordinate_vector_2_type coords;

	CGAL::Triple< std::back_insert_iterator<delaunay_point_coordinate_vector_2_type>, delaunay_coord_2_type, bool>
		coordinate_result =
			CGAL::natural_neighbor_coordinates_2(
				*this,
				point,
				std::back_inserter(coords)
			);

	const delaunay_coord_2_type &norm = coordinate_result.second;

	// Gradient Fitting
	delaunay_vector_2_type gradient_vector;

	gradient_vector =
		CGAL::sibson_gradient_fitting(
			coords.begin(),
			coords.end(),
			norm,
			point,
			CGAL::Data_access<delaunay_map_point_to_value_2_type>(function_values),
			Traits()
		);

// FIXME: test code ; remove when no longer needed
#if 0
// estimates the function gradients at all vertices of triangulation 
// that lie inside the convex hull using the coordinates computed 
// by the function natural_neighbor_coordinates_2.

	sibson_gradient_fitting_nn_2(
		*this,
		std::inserter( 
			function_gradients, 
			function_gradients.begin() ),
		CGAL::Data_access<map_point_2_to_value_type>(function_values),
		Traits()
	);
#endif

#if 0

	//Sibson interpolant: version without sqrt
	std::pair<coord_type, bool> res =
		CGAL::sibson_c1_interpolation_square(
			coords.begin(),
     		coords.end(),
			norm,
			point,
			CGAL::Data_access<map_point_2_to_value_type>(function_values),
			CGAL::Data_access<map_point_2_to_vector_2_type>(function_gradients),
			Traits());

if(res.second)
    std::cout << "   Tested interpolation on " << point
	      << " interpolation: " << res.first << " exact: "
	      << std::endl;
else
    std::cout << "C^1 Interpolation not successful." << std::endl
	      << " not all function_gradients are provided."  << std::endl
	      << " You may resort to linear interpolation." << std::endl;
	std::cout << "done" << std::endl;
#endif

	return gradient_vector;
}


const GPlatesMaths::AzimuthalEqualAreaProjection &
GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::get_projection() const
{
	return d_network.get_projection();
}


bool
GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::is_point_in_deforming_region(
		const GPlatesMaths::PointOnSphere &point) const
{
	return d_network.is_point_in_deforming_region(point);
}


GPlatesAppLogic::ResolvedTriangulation::DeformationInfo
GPlatesAppLogic::ResolvedTriangulation::calculate_face_deformation_info(
		const Delaunay_2 &delaunay_2,
		const double &theta1,
		const double &theta2,
		const double &theta3,
		const double &theta_centroid,
		const double &phi1,
		const double &phi2,
		const double &phi3,
		const double &phi_centroid,
		const double &utheta1,
		const double &utheta2,
		const double &utheta3,
		const double &utheta_centroid,
		const double &uphi1,
		const double &uphi2,
		const double &uphi3,
		const double &uphi_centroid)
{
	//
	// Compute spatial gradients of the velocity.
	//
	// The spatial gradients of velocity in 2D lat/lon space.
	// NOTE: Might not be the best space to calculate gradients.
	//

	// Velocity theta-component is co-latitude (positive when away from the North pole).
	// Velocity phi-component is positive when East.
	// Velocity units are in metres/second.

	//
	// Barycentric interpolation of lat/lon position between three vertices is:
	//
	//   theta = b1*theta1 + b2*theta2 + b3*theta3
	//   phi   = b1*phi1   + b2*phi2   + b3*phi3
	//
	// ...where (see 'ResolvedTriangulation::get_barycentric_coords_2()') ...
	// 
	//   b0 =  (theta2 - theta1) * (phi3 - phi1) - (theta3 - theta1) * (phi2 - phi1)
	//   b1 = ((theta2 -  theta) * (phi3 -  phi) - (theta3 -  theta) * (phi2 -  phi)) / b0 
	//   b2 = ((theta3 -  theta) * (phi1 -  phi) - (theta1 -  theta) * (phi3 -  phi)) / b0
	//   b3 = ((theta1 -  theta) * (phi2 -  phi) - (theta2 -  theta) * (phi1 -  phi)) / b0 
	//
	// Barycentric interpolation of velocity between three vertices is:
	//
	//   utheta = b1(theta,phi)*utheta1 + b2(theta,phi)*utheta2 + b3(theta,phi)*utheta3
	//   uphi   = b1(theta,phi)*uphi1   + b2(theta,phi)*uphi2   + b3(theta,phi)*uphi3
	//
	// The spatial gradients of velocity are:
	//
	//   dutheta_dtheta = d(b1)/dtheta * utheta1 + d(b2)/dtheta * utheta2 + d(b3)/dtheta * utheta3
	//   dutheta_dphi   = d(b1)/dphi * utheta1   + d(b2)/dphi * utheta2   + d(b3)/dphi * utheta3
	//   duphi_dtheta   = d(b1)/dtheta * uphi1   + d(b2)/dtheta * uphi2   + d(b3)/dtheta * uphi3
	//   duphi_dphi     = d(b1)/dphi * uphi1     + d(b2)/dphi * uphi2     + d(b3)/dphi * uphi3
	//
	// ...where...
	//
	//   d(b1)/dtheta = (phi2 - phi3) / b0
	//   d(b1)/dphi = (theta3 - theta2) / b0
	//   d(b2)/dtheta = (phi3 - phi1) / b0
	//   d(b2)/dphi = (theta1 - theta3) / b0
	//   d(b3)/dtheta = (phi1 - phi2) / b0
	//   d(b3)/dphi = (theta2 - theta1) / b0
	//
	// ...resulting in...
	//
	//   dutheta_dtheta = [(phi2 - phi3) * utheta1     + (phi3 - phi1) * utheta2     + (phi1 - phi2) * utheta3]     / b0
	//   dutheta_dphi   = [(theta3 - theta2) * utheta1 + (theta1 - theta3) * utheta2 + (theta2 - theta1) * utheta3] / b0
	//   duphi_dtheta   = [(phi2 - phi3) * uphi1       + (phi3 - phi1) * uphi2       + (phi1 - phi2) * uphi3]       / b0
	//   duphi_dphi     = [(theta3 - theta2) * uphi1   + (theta1 - theta3) * uphi2   + (theta2 - theta1) * uphi3]   / b0
	//
	const double b0 =  (theta2 - theta1) * (phi3 - phi1) - (theta3 - theta1) * (phi2 - phi1);

	// Avoid divide-by-zero.
	if (b0 > -GPlatesMaths::EPSILON &&
		b0 < GPlatesMaths::EPSILON)
	{
		// Unable to calculate velocity spatial gradients - use default values of zero.
		return DeformationInfo();
	}
	const double inv_b0 = 1.0 / b0;

	const double dutheta_dtheta = ((phi2 - phi3) * utheta1 + (phi3 - phi1) * utheta2 + (phi1 - phi2) * utheta3) * inv_b0;
	const double dutheta_dphi = ((theta3 - theta2) * utheta1 + (theta1 - theta3) * utheta2 + (theta2 - theta1) * utheta3) * inv_b0;
	const double duphi_dtheta = ((phi2 - phi3) * uphi1 + (phi3 - phi1) * uphi2 + (phi1 - phi2) * uphi3) * inv_b0;
	const double duphi_dphi = ((theta3 - theta2) * uphi1 + (theta1 - theta3) * uphi2 + (theta2 - theta1) * uphi3) * inv_b0;

	// Range of 'theta_centroid' is [0, PI] from North to South pole.
	const double sin_theta_centroid = std::sin(theta_centroid);

	// Avoid divide-by-zero.
	// Range of 'sin_theta_centroid' is [0, 1].
	if (sin_theta_centroid < GPlatesMaths::EPSILON)
	{
		// Unable to calculate strain rate - use default values of zero.
		return DeformationInfo();
	}
	const double inv_sin_theta_centroid = 1.0 / sin_theta_centroid;
	const double cos_theta_centroid = std::cos(theta_centroid);

	//
	// Convert spatial gradients of velocity from 2D lat/lon coordinates into
	// spherical coordinates (ignoring radial direction).
	//
	// This is the velocity spatial gradient tensor L in
	// chapter 4 of "Introduction to the mechanics of a continuous medium" by Malvern)
	// which is defined in spherical coordinates in appendix II in equation
	// (equation II.4.S8 - but note it has a typo - incorrectly specifies cot(phi) instead of cot(theta)).
	const double ugrad_theta_theta = INVERSE_EARTH_RADIUS_METRES * dutheta_dtheta;
	const double ugrad_theta_phi = INVERSE_EARTH_RADIUS_METRES * inv_sin_theta_centroid * (
			dutheta_dphi -
			uphi_centroid * cos_theta_centroid);
	const double ugrad_phi_theta = INVERSE_EARTH_RADIUS_METRES * duphi_dtheta;
	const double ugrad_phi_phi = INVERSE_EARTH_RADIUS_METRES * inv_sin_theta_centroid * (
			duphi_dphi +
			cos_theta_centroid * utheta_centroid);

	// Velocity gradient units are in 1/second.
	const DeformationStrainRate deformation_strain_rate(
			ugrad_theta_theta,
			ugrad_theta_phi,
			ugrad_phi_theta,
			ugrad_phi_phi);

	//
	// Clamp the total strain rate (2nd invariant) to a maximum value if requested.
	//
	// Note that the geodesy definition of second invariant in Kreemer et al. 2014 is
	// equivalent to the total strain rate. It is invariant with coordinate transformations and
	// in principal axes is equal to sqrt(D1^2 + D2^2) which is the norm of the total strain rate.
	//
	const boost::optional<double> clamp_total_strain_rate =
			delaunay_2.get_clamp_total_strain_rate();
	if (clamp_total_strain_rate)
	{
		// The total strain rate.
		const double total_strain_rate = deformation_strain_rate.get_strain_rate_second_invariant();

		// Scale down all strain rate quantities such that the total strain rate
		// equals the maximum allowed value.
		if (total_strain_rate > clamp_total_strain_rate.get())
		{
			const double strain_rate_scale = clamp_total_strain_rate.get() / total_strain_rate;

			return DeformationInfo(strain_rate_scale * deformation_strain_rate);
		}
	}

	return DeformationInfo(deformation_strain_rate);
}
