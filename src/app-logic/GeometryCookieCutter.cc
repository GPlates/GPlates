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

#include <algorithm>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>

#include "GeometryCookieCutter.h"

#include "ReconstructionGeometryUtils.h"
#include "ReconstructedFeatureGeometry.h"
#include "ResolvedTopologicalGeometry.h"

#include "maths/ConstGeometryOnSphereVisitor.h"

namespace
{
	/**
	 * Gets a @a PolygonOnSphere from a @a GeometryOnSphere if it is one.
	 */
	class PolygonGeometryFinder :
			public GPlatesMaths::ConstGeometryOnSphereVisitor
	{
	public:
		const boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> &
		get_polygon_on_sphere() const
		{
			return d_polygon_on_sphere;
		}

		virtual
		void
		visit_polygon_on_sphere(
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
		{
			d_polygon_on_sphere = polygon_on_sphere;
		}

	private:
		boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> d_polygon_on_sphere;
	};
}


GPlatesAppLogic::GeometryCookieCutter::GeometryCookieCutter(
		const double &reconstruction_time,
		boost::optional<const std::vector<reconstructed_feature_geometry_non_null_ptr_type> &> reconstructed_static_polygons,
		boost::optional<const std::vector<resolved_topological_geometry_non_null_ptr_type> &> resolved_topological_boundaries,
		boost::optional<const std::vector<resolved_topological_network_non_null_ptr_type> &> resolved_topological_networks) :
	d_reconstruction_time(reconstruction_time)
{
	// Resolved networks are added first and hence are used first (along with their interior polygons, if any)
	// during partitioning.
	if (resolved_topological_networks)
	{
		const partitioning_geometry_seq_type::size_type num_partitioning_geometries = d_partitioning_geometries.size();

		add_partitioning_resolved_topological_network_interior_polygons(resolved_topological_networks.get());

		// Sort the parititioning geometries just added by plate id.
		std::sort(
				d_partitioning_geometries.begin() + num_partitioning_geometries,
				d_partitioning_geometries.end(),
				PartitioningGeometry::SortPlateIdHighestToLowest());
	}

	if (resolved_topological_networks)
	{
		const partitioning_geometry_seq_type::size_type num_partitioning_geometries = d_partitioning_geometries.size();

		add_partitioning_resolved_topological_networks(resolved_topological_networks.get());

		// Sort the parititioning geometries just added by plate id.
		std::sort(
				d_partitioning_geometries.begin() + num_partitioning_geometries,
				d_partitioning_geometries.end(),
				PartitioningGeometry::SortPlateIdHighestToLowest());
	}

	if (resolved_topological_boundaries)
	{
		const partitioning_geometry_seq_type::size_type num_partitioning_geometries = d_partitioning_geometries.size();

		add_partitioning_resolved_topological_boundaries(resolved_topological_boundaries.get());

		// Sort the parititioning geometries just added by plate id.
		std::sort(
				d_partitioning_geometries.begin() + num_partitioning_geometries,
				d_partitioning_geometries.end(),
				PartitioningGeometry::SortPlateIdHighestToLowest());
	}

	if (reconstructed_static_polygons)
	{
		const partitioning_geometry_seq_type::size_type num_partitioning_geometries = d_partitioning_geometries.size();

		add_partitioning_reconstructed_feature_polygons(reconstructed_static_polygons.get());

		// Sort the parititioning geometries just added by plate id.
		std::sort(
				d_partitioning_geometries.begin() + num_partitioning_geometries,
				d_partitioning_geometries.end(),
				PartitioningGeometry::SortPlateIdHighestToLowest());
	}
}


bool
GPlatesAppLogic::GeometryCookieCutter::has_partitioning_polygons() const
{
	return !d_partitioning_geometries.empty();
}


bool
GPlatesAppLogic::GeometryCookieCutter::partition_geometry(
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry,
		partition_seq_type &partitioned_inside_geometries,
		partitioned_geometry_seq_type &partitioned_outside_geometries) const
{
	// Return early no partitioning polygons.
	if (d_partitioning_geometries.empty())
	{
		// There are no partitioning polygons so the input geometry goes
		// to the list of geometries partitioned outside all partitioning polygons.
		partitioned_outside_geometries.push_back(geometry);
		return false;
	}

	bool was_geometry_partitioned = false;

	// Keeps track of the geometries that are outside the partitioning polygon
	// being processed (and outside all partitioning polygons processed so far).
	partitioned_geometry_seq_type partitioned_outside_geometries1;
	partitioned_geometry_seq_type partitioned_outside_geometries2;
	partitioned_geometry_seq_type *current_partitioned_outside_geometries =
			&partitioned_outside_geometries1;
	partitioned_geometry_seq_type *next_partitioned_outside_geometries =
			&partitioned_outside_geometries2;

	// Add the geometry to be partitioned to the current list of outside geometries
	// to start off the processing chain.
	current_partitioned_outside_geometries->push_back(geometry);

	// Iterate through the partitioning polygons.
	partitioning_geometry_seq_type::const_iterator partition_iter =
			d_partitioning_geometries.begin();
	partitioning_geometry_seq_type::const_iterator partition_end =
			d_partitioning_geometries.end();
	for ( ; partition_iter != partition_end; ++partition_iter)
	{
		const PartitioningGeometry &partitioning_geometry = *partition_iter;

		// Geometries partitioned inside the current partitioning polygon will be stored here.
		Partition current_partitioned_inside_geometries(
				partitioning_geometry.d_reconstruction_geometry);

		// Clear the next list before we start filling it up.
		next_partitioned_outside_geometries->clear();

		// Iterate over the current sequence of outside geometries and partition them
		// using the current partitioning polygon.
		partitioned_geometry_seq_type::const_iterator outside_geometry_iter =
				current_partitioned_outside_geometries->begin();
		partitioned_geometry_seq_type::const_iterator outside_geometry_end =
				current_partitioned_outside_geometries->end();
		for ( ; outside_geometry_iter != outside_geometry_end; ++outside_geometry_iter)
		{
			const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &outside_geometry =
					*outside_geometry_iter;

			// Partition the current outside geometry against the partitioning polygon.
			// Geometry partitioned outside the current partitioning polygon get stored
			// in the sequence of outside geometries used for the next partitioning polygon.
			partitioning_geometry.d_polygon_intersections->partition_geometry(
					outside_geometry,
					current_partitioned_inside_geometries.partitioned_geometries/*inside*/,
					*next_partitioned_outside_geometries/*outside*/);
		}

		// Add the partitioned geometries to the caller's list if any were partitioned
		// into the current partitioning polygon.
		if (!current_partitioned_inside_geometries.partitioned_geometries.empty())
		{
			partitioned_inside_geometries.push_back(current_partitioned_inside_geometries);

			was_geometry_partitioned = true;
		}

		// Swap the pointers to the current and next list of outside geometries.
		std::swap(
				current_partitioned_outside_geometries,
				next_partitioned_outside_geometries);
	}

	// Pass any remaining partitioned outside geometries to the caller.
	// These are not inside any of the resolved boundaries.
	partitioned_outside_geometries.splice(
			partitioned_outside_geometries.end(),
			*current_partitioned_outside_geometries);

	return was_geometry_partitioned;
}


boost::optional<const GPlatesAppLogic::ReconstructionGeometry *>
GPlatesAppLogic::GeometryCookieCutter::partition_point(
		const GPlatesMaths::PointOnSphere &point) const
{
	// Return early if no partitioning polygons.
	if (d_partitioning_geometries.empty())
	{
		return boost::none;
	}

	// Iterate through the partitioning polygons and
	// return the first one that contains the point.
	// FIXME: Need to take care of overlapping plates by testing
	// polygons sorted by plate id because largest plate id is
	// usually furthest from anchor plate id - but even this is a bit
	// dodgy and needs a better solution.
	partitioning_geometry_seq_type::const_iterator partition_iter =
			d_partitioning_geometries.begin();
	partitioning_geometry_seq_type::const_iterator partition_end =
			d_partitioning_geometries.end();
	for ( ; partition_iter != partition_end; ++partition_iter)
	{
		const PartitioningGeometry &partitioning_geometry = *partition_iter;

		if (partitioning_geometry.d_polygon_intersections->partition_point(point) !=
			GPlatesMaths::PolygonIntersections::GEOMETRY_OUTSIDE)
		{
			return partitioning_geometry.d_reconstruction_geometry.get();
		}
	}

	return boost::none;
}


void
GPlatesAppLogic::GeometryCookieCutter::add_partitioning_resolved_topological_boundaries(
		const std::vector<resolved_topological_geometry_non_null_ptr_type> &resolved_topological_boundaries)
{
	// Create the partitioned geometries.
	BOOST_FOREACH(const ResolvedTopologicalGeometry::non_null_ptr_type &rtb, resolved_topological_boundaries)
	{
		boost::optional<ResolvedTopologicalGeometry::resolved_topology_boundary_ptr_type>
				resolved_topology_boundary_polygon = rtb->resolved_topology_boundary();
		if (resolved_topology_boundary_polygon)
		{
			d_partitioning_geometries.push_back(
					PartitioningGeometry(rtb, resolved_topology_boundary_polygon.get()));
		}
	}
}


void
GPlatesAppLogic::GeometryCookieCutter::add_partitioning_resolved_topological_networks(
		const std::vector<resolved_topological_network_non_null_ptr_type> &resolved_topological_networks)
{
	// Create the partitioned geometries.
	BOOST_FOREACH(const ResolvedTopologicalNetwork::non_null_ptr_type &rtn, resolved_topological_networks)
	{
		d_partitioning_geometries.push_back(
				PartitioningGeometry(rtn, rtn->boundary_polygon()));
	}
}


void
GPlatesAppLogic::GeometryCookieCutter::add_partitioning_resolved_topological_network_interior_polygons(
		const std::vector<resolved_topological_network_non_null_ptr_type> &resolved_topological_networks)
{
	// Create the partitioned geometries.
	BOOST_FOREACH(const ResolvedTopologicalNetwork::non_null_ptr_type &rtn, resolved_topological_networks)
	{
		// Iterate over the interior polygons, if any, of the current topological network.
		ResolvedTopologicalNetwork::interior_polygon_const_iterator interior_poly_iter = rtn->interior_polygons_begin();
		ResolvedTopologicalNetwork::interior_polygon_const_iterator interior_poly_end = rtn->interior_polygons_end();
		for ( ; interior_poly_iter != interior_poly_end; ++interior_poly_iter)
		{
			const ResolvedTopologicalNetwork::InteriorPolygon &interior_poly = *interior_poly_iter;
			const ReconstructedFeatureGeometry::non_null_ptr_type interior_poly_rfg =
					interior_poly.get_reconstructed_feature_geometry();
			GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type interior_poly_geom_on_sphere =
					interior_poly_rfg->reconstructed_geometry();

			// Get the polygon geometry.
			PolygonGeometryFinder polygon_geom_finder;
			interior_poly_geom_on_sphere->accept_visitor(polygon_geom_finder);

			// Add it as a partitioning geometry.
			if (polygon_geom_finder.get_polygon_on_sphere())
			{
				d_partitioning_geometries.push_back(
						PartitioningGeometry(
								interior_poly_rfg,
								polygon_geom_finder.get_polygon_on_sphere().get()));
			}
		}
	}
}


void
GPlatesAppLogic::GeometryCookieCutter::add_partitioning_reconstructed_feature_polygons(
		const std::vector<reconstructed_feature_geometry_non_null_ptr_type> &reconstructed_feature_geometries)
{
	typedef std::vector<reconstructed_feature_geometry_non_null_ptr_type> rfg_seq_type;

	// Find the RFGs that contain polygon geometry and add them as partitioning polygon.
	rfg_seq_type::const_iterator rfg_iter = reconstructed_feature_geometries.begin();
	rfg_seq_type::const_iterator rfg_end = reconstructed_feature_geometries.end();
	for ( ; rfg_iter != rfg_end; ++rfg_iter)
	{
		const ReconstructedFeatureGeometry::non_null_ptr_type &rfg = *rfg_iter;
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type rfg_geom_on_sphere =
				rfg->reconstructed_geometry();

		// See if it's a polygon.
		PolygonGeometryFinder polygon_geom_finder;
		rfg_geom_on_sphere->accept_visitor(polygon_geom_finder);

		// Add it as a partitioning geometry if it's a polygon.
		if (polygon_geom_finder.get_polygon_on_sphere())
		{
			d_partitioning_geometries.push_back(
					PartitioningGeometry(
							rfg,
							polygon_geom_finder.get_polygon_on_sphere().get()));
		}
	}
}


GPlatesAppLogic::GeometryCookieCutter::PartitioningGeometry::PartitioningGeometry(
		const reconstruction_geometry_non_null_ptr_type &reconstruction_geometry,
		const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type &partitioning_polygon) :
	d_reconstruction_geometry(reconstruction_geometry),
	d_polygon_intersections(
			GPlatesMaths::PolygonIntersections::create(partitioning_polygon))
{
}


bool
GPlatesAppLogic::GeometryCookieCutter::PartitioningGeometry::SortPlateIdHighestToLowest::operator()(
		const PartitioningGeometry &lhs,
		const PartitioningGeometry &rhs) const
{
	return ReconstructionGeometryUtils::get_plate_id(lhs.d_reconstruction_geometry) >
		ReconstructionGeometryUtils::get_plate_id(rhs.d_reconstruction_geometry);
}
