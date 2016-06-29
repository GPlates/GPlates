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
#include <map>
#include <vector>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>

#include "GeometryCookieCutter.h"

#include "AppLogicUtils.h"
#include "GeometryUtils.h"
#include "ReconstructionGeometryUtils.h"
#include "ReconstructedFeatureGeometry.h"
#include "ReconstructMethodRegistry.h"
#include "ReconstructUtils.h"
#include "ResolvedTopologicalBoundary.h"
#include "ResolvedTopologicalLine.h"
#include "ResolvedTopologicalNetwork.h"
#include "TopologyUtils.h"

#include "global/GPlatesAssert.h"

#include "maths/ConstGeometryOnSphereVisitor.h"

#include "model/FeatureVisitor.h"


namespace GPlatesAppLogic
{
	namespace
	{
		class FeatureOrderVisitor :
				public GPlatesModel::FeatureVisitor
		{
		public:

			typedef std::map<GPlatesModel::FeatureHandle::weak_ref, unsigned int> feature_order_map_type;


			explicit
			FeatureOrderVisitor(
					feature_order_map_type &feature_order_map) :
				d_feature_order_map(feature_order_map),
				d_feature_count(0)
			{  }

		protected:

			virtual
			bool
			initialise_pre_feature_properties(
					GPlatesModel::FeatureHandle &feature_handle)
			{
				std::pair<feature_order_map_type::iterator, bool> result =
						d_feature_order_map.insert(
								feature_order_map_type::value_type(
										feature_handle.reference(),
										d_feature_count));
				if (result.second)
				{
					// A new feature was inserted into the map.
					++d_feature_count;
				}

				// We don't actually need to visit the properties.
				return false;
			}

		private:

			feature_order_map_type &d_feature_order_map;
			unsigned int d_feature_count;
		};
	}
}


GPlatesAppLogic::GeometryCookieCutter::GeometryCookieCutter(
		const double &reconstruction_time,
		boost::optional<const std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &> reconstructed_static_polygons,
		boost::optional<const std::vector<ResolvedTopologicalBoundary::non_null_ptr_type> &> resolved_topological_boundaries,
		boost::optional<const std::vector<ResolvedTopologicalNetwork::non_null_ptr_type> &> resolved_topological_networks,
		boost::optional<SortPlates> sort_plates,
		GPlatesMaths::PolygonOnSphere::PointInPolygonSpeedAndMemory partition_point_speed_and_memory) :
	d_reconstruction_time(reconstruction_time),
	d_partition_point_speed_and_memory(partition_point_speed_and_memory)
{
	// Resolved networks are added first and hence are used first (along with their interior polygons, if any)
	// during partitioning.
	if (resolved_topological_networks)
	{
		add_partitioning_resolved_topological_networks(resolved_topological_networks.get(), sort_plates);
	}

	if (resolved_topological_boundaries)
	{
		add_partitioning_resolved_topological_boundaries(resolved_topological_boundaries.get(), sort_plates);
	}

	if (reconstructed_static_polygons)
	{
		add_partitioning_reconstructed_feature_polygons(reconstructed_static_polygons.get(), sort_plates);
	}
}


GPlatesAppLogic::GeometryCookieCutter::GeometryCookieCutter(
		const double &reconstruction_time,
		const std::vector<ReconstructionGeometry::non_null_ptr_type> &reconstruction_geometries,
		bool group_networks_then_boundaries_then_static_polygons,
		boost::optional<SortPlates> sort_plates,
		GPlatesMaths::PolygonOnSphere::PointInPolygonSpeedAndMemory partition_point_speed_and_memory) :
	d_reconstruction_time(reconstruction_time),
	d_partition_point_speed_and_memory(partition_point_speed_and_memory)
{
	if (group_networks_then_boundaries_then_static_polygons)
	{
		std::vector<ResolvedTopologicalNetwork::non_null_ptr_type> resolved_topological_networks;
		ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type_sequence(
				reconstruction_geometries.begin(),
				reconstruction_geometries.end(),
				resolved_topological_networks);
		add_partitioning_resolved_topological_networks(resolved_topological_networks, sort_plates);

		std::vector<ResolvedTopologicalBoundary::non_null_ptr_type> resolved_topological_boundaries;
		ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type_sequence(
				reconstruction_geometries.begin(),
				reconstruction_geometries.end(),
				resolved_topological_boundaries);
		add_partitioning_resolved_topological_boundaries(resolved_topological_boundaries, sort_plates);

		std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> reconstructed_static_polygons;
		ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type_sequence(
				reconstruction_geometries.begin(),
				reconstruction_geometries.end(),
				reconstructed_static_polygons);
		add_partitioning_reconstructed_feature_polygons(reconstructed_static_polygons, sort_plates);
	}
	else
	{
		add_partitioning_reconstruction_geometries(reconstruction_geometries, sort_plates);
	}
}


GPlatesAppLogic::GeometryCookieCutter::GeometryCookieCutter(
		const double &reconstruction_time,
		const ReconstructMethodRegistry &reconstruct_method_registry,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &feature_collections,
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		bool group_networks_then_boundaries_then_static_polygons,
		boost::optional<SortPlates> sort_plates,
		GPlatesMaths::PolygonOnSphere::PointInPolygonSpeedAndMemory partition_point_speed_and_memory) :
	d_reconstruction_time(reconstruction_time),
	d_partition_point_speed_and_memory(partition_point_speed_and_memory)
{
	// Contains the reconstructed static polygons used for cookie-cutting.
	// Can also contain the topological section geometries referenced by topologies.
	std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> reconstructed_feature_geometries;

	const ReconstructHandle::type reconstruct_handle = ReconstructUtils::reconstruct(
			reconstructed_feature_geometries,
			reconstruction_time,
			reconstruct_method_registry,
			feature_collections,
			reconstruction_tree_creator);

	std::vector<ReconstructHandle::type> reconstruct_handles(1, reconstruct_handle);

	// Contains the resolved topological line sections referenced by topological polygons and networks.
	std::vector<ResolvedTopologicalLine::non_null_ptr_type> resolved_topological_lines;

	// Resolving topological lines generates its own reconstruct handle that will be used by
	// topological polygons and networks to find this group of resolved lines.
	const ReconstructHandle::type resolved_topological_lines_handle =
			TopologyUtils::resolve_topological_lines(
					resolved_topological_lines,
					feature_collections,
					reconstruction_tree_creator, 
					reconstruction_time,
					// Resolved topo lines use the reconstructed non-topo geometries...
					reconstruct_handles);
	reconstruct_handles.push_back(resolved_topological_lines_handle);

	// Contains the resolved topological polygons used for cookie-cutting.
	std::vector<ResolvedTopologicalBoundary::non_null_ptr_type> resolved_topological_boundaries;
	TopologyUtils::resolve_topological_boundaries(
			resolved_topological_boundaries,
			feature_collections,
			reconstruction_tree_creator, 
			reconstruction_time,
			// Resolved topo boundaries use the resolved topo lines *and* the reconstructed non-topo geometries...
			reconstruct_handles);

	// Contains the resolved topological networks used for cookie-cutting.
	// See comment in header for why a deforming region is currently used to assign plate ids.
	std::vector<ResolvedTopologicalNetwork::non_null_ptr_type> resolved_topological_networks;
	TopologyUtils::resolve_topological_networks(
			resolved_topological_networks,
			reconstruction_time,
			feature_collections,
			// Resolved topo networks use the resolved topo lines *and* the reconstructed non-topo geometries...
			reconstruct_handles);

	if (group_networks_then_boundaries_then_static_polygons)
	{
		add_partitioning_resolved_topological_networks(resolved_topological_networks, sort_plates);
		add_partitioning_resolved_topological_boundaries(resolved_topological_boundaries, sort_plates);
		add_partitioning_reconstructed_feature_polygons(reconstructed_feature_geometries, sort_plates);
	}
	else // keep same order as input feature collections (for reconstruction geometries)...
	{
		// Determine the order of features passed to us.
		FeatureOrderVisitor::feature_order_map_type feature_order_map;
		FeatureOrderVisitor visitor(feature_order_map);
		AppLogicUtils::visit_feature_collections(
				feature_collections.begin(),
				feature_collections.end(),
				visitor);

		// Order the reconstruction geometries in the same order as their associated features.
		typedef std::multimap<unsigned int, ReconstructionGeometry::non_null_ptr_type> feature_ordered_recon_geom_map_type;
		feature_ordered_recon_geom_map_type feature_ordered_recon_geoms_map;
		std::vector<ReconstructionGeometry::non_null_ptr_type> reconstruction_geometries;

		// Iterate over resolved topological networks.
		BOOST_FOREACH(
				const ResolvedTopologicalNetwork::non_null_ptr_type &resolved_topological_network,
				resolved_topological_networks)
		{
			FeatureOrderVisitor::feature_order_map_type::const_iterator feature_order_iter =
					feature_order_map.find(resolved_topological_network->get_feature_ref());
			if (feature_order_iter != feature_order_map.end())
			{
				const unsigned int feature_order = feature_order_iter->second;
				feature_ordered_recon_geoms_map.insert(
						feature_ordered_recon_geom_map_type::value_type(feature_order, resolved_topological_network));
			}
			else // couldn't find feature so just add unordered...
			{
				reconstruction_geometries.push_back(resolved_topological_network);
			}
		}

		// Iterate over resolved topological boundaries.
		BOOST_FOREACH(
				const ResolvedTopologicalBoundary::non_null_ptr_type &resolved_topological_boundary,
				resolved_topological_boundaries)
		{
			FeatureOrderVisitor::feature_order_map_type::const_iterator feature_order_iter =
					feature_order_map.find(resolved_topological_boundary->get_feature_ref());
			if (feature_order_iter != feature_order_map.end())
			{
				const unsigned int feature_order = feature_order_iter->second;
				feature_ordered_recon_geoms_map.insert(
						feature_ordered_recon_geom_map_type::value_type(feature_order, resolved_topological_boundary));
			}
			else // couldn't find feature so just add unordered...
			{
				reconstruction_geometries.push_back(resolved_topological_boundary);
			}
		}

		// Iterate over reconstructed static polygons.
		BOOST_FOREACH(
				const ReconstructedFeatureGeometry::non_null_ptr_type &reconstructed_feature_geometry,
				reconstructed_feature_geometries)
		{
			FeatureOrderVisitor::feature_order_map_type::const_iterator feature_order_iter =
					feature_order_map.find(reconstructed_feature_geometry->get_feature_ref());
			if (feature_order_iter != feature_order_map.end())
			{
				const unsigned int feature_order = feature_order_iter->second;
				feature_ordered_recon_geoms_map.insert(
						feature_ordered_recon_geom_map_type::value_type(feature_order, reconstructed_feature_geometry));
			}
			else // couldn't find feature so just add unordered...
			{
				reconstruction_geometries.push_back(reconstructed_feature_geometry);
			}
		}

		// Copy the ordered reconstruction geometries from the map into vector.
		BOOST_FOREACH(
				const feature_ordered_recon_geom_map_type::value_type &recon_geom_map_entry,
				feature_ordered_recon_geoms_map)
		{
			reconstruction_geometries.push_back(recon_geom_map_entry.second);
		}

		add_partitioning_reconstruction_geometries(reconstruction_geometries, sort_plates);
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
		boost::optional<partition_seq_type &> partitioned_inside_geometries,
		boost::optional<partitioned_geometry_seq_type &> partitioned_outside_geometries) const
{
	return partition_geometries(
			std::vector<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>(1, geometry),
			partitioned_inside_geometries,
			partitioned_outside_geometries);
}


bool
GPlatesAppLogic::GeometryCookieCutter::partition_geometries(
		const std::vector<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> &geometries,
		boost::optional<partition_seq_type &> partitioned_inside_geometries,
		boost::optional<partitioned_geometry_seq_type &> partitioned_outside_geometries) const
{
	// Return early if no partitioning polygons.
	if (d_partitioning_geometries.empty())
	{
		// There are no partitioning polygons so the input geometries go
		// to the list of geometries partitioned outside all partitioning polygons.
		if (partitioned_outside_geometries)
		{
			partitioned_outside_geometries->insert(
					partitioned_outside_geometries->end(),
					geometries.begin(),
					geometries.end());
		}
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

	// Add the geometries to be partitioned to the current list of outside geometries
	// to start off the processing chain.
	current_partitioned_outside_geometries->insert(
			current_partitioned_outside_geometries->end(),
			geometries.begin(),
			geometries.end());

	// Iterate through the partitioning polygons.
	partitioning_geometry_seq_type::const_iterator partition_iter =
			d_partitioning_geometries.begin();
	partitioning_geometry_seq_type::const_iterator partition_end =
			d_partitioning_geometries.end();
	for ( ;
		partition_iter != partition_end &&
			// If there are outside geometries then continue, otherwise there's nothing left to partition...
			!current_partitioned_outside_geometries->empty();
		++partition_iter)
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
			// Return early if no inside/outside geometries requested since we have determined
			// that the geometry is at least partially inside one of the partitioning polygons.
			if (!partitioned_inside_geometries &&
				!partitioned_outside_geometries)
			{
				return true;
			}

			if (partitioned_inside_geometries)
			{
				partitioned_inside_geometries->push_back(current_partitioned_inside_geometries);
			}

			was_geometry_partitioned = true;
		}

		// Swap the pointers to the current and next list of outside geometries.
		std::swap(
				current_partitioned_outside_geometries,
				next_partitioned_outside_geometries);
	}

	// Pass any remaining partitioned outside geometries to the caller.
	// These are not inside any of the resolved boundaries.
	if (partitioned_outside_geometries)
	{
		partitioned_outside_geometries->splice(
				partitioned_outside_geometries->end(),
				*current_partitioned_outside_geometries);
	}

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

	// Iterate through the partitioning polygons and return the first one that contains the point.
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
GPlatesAppLogic::GeometryCookieCutter::add_partitioning_reconstruction_geometries(
		const std::vector<ReconstructionGeometry::non_null_ptr_type> &reconstruction_geometries,
		boost::optional<SortPlates> sort_plates)
{
	// Create the partitioned geometries.
	// These are not grouped by reconstruction geometry type.
	AddPartitioningReconstructionGeometry visitor(*this);
	BOOST_FOREACH(const ReconstructionGeometry::non_null_ptr_type &rg, reconstruction_geometries)
	{
		rg->accept_visitor(visitor);
	}

	if (sort_plates)
	{
		sort_plates_in_partitioning_group(
				d_partitioning_geometries.begin(),
				d_partitioning_geometries.end(),
				sort_plates.get());
	}
}


void
GPlatesAppLogic::GeometryCookieCutter::add_partitioning_resolved_topological_networks(
		const std::vector<ResolvedTopologicalNetwork::non_null_ptr_type> &resolved_topological_networks,
		boost::optional<SortPlates> sort_plates)
{
	const partitioning_geometry_seq_type::size_type num_partitioning_geometries = d_partitioning_geometries.size();

	BOOST_FOREACH(const ResolvedTopologicalNetwork::non_null_ptr_type &rtn, resolved_topological_networks)
	{
		add_partitioning_resolved_topological_network(rtn);
	}

	if (sort_plates)
	{
		// Sort only the partitioning geometries just added.
		sort_plates_in_partitioning_group(
				d_partitioning_geometries.begin() + num_partitioning_geometries,
				d_partitioning_geometries.end(),
				sort_plates.get());
	}
}


void
GPlatesAppLogic::GeometryCookieCutter::add_partitioning_resolved_topological_network(
		const ResolvedTopologicalNetwork::non_null_ptr_type &rtn)
{
	// Iterate over the interior rigid blocks, if any, of the current topological network.
	const ResolvedTriangulation::Network::rigid_block_seq_type &rigid_blocks =
			rtn->get_triangulation_network().get_rigid_blocks();
	ResolvedTriangulation::Network::rigid_block_seq_type::const_iterator rigid_blocks_iter = rigid_blocks.begin();
	ResolvedTriangulation::Network::rigid_block_seq_type::const_iterator rigid_blocks_end = rigid_blocks.end();
	for ( ; rigid_blocks_iter != rigid_blocks_end; ++rigid_blocks_iter)
	{
		const ResolvedTriangulation::Network::RigidBlock &rigid_block = *rigid_blocks_iter;

		const ReconstructedFeatureGeometry::non_null_ptr_type rigid_block_rfg =
				rigid_block.get_reconstructed_feature_geometry();

		// Get the polygon geometry.
		boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> polygon =
				GeometryUtils::get_polygon_on_sphere(*rigid_block_rfg->reconstructed_geometry());
		if (!polygon)
		{
			continue;
		}

		// Add interior block as a partitioning geometry.
		d_partitioning_geometries.push_back(
				PartitioningGeometry(rigid_block_rfg, polygon.get(), d_partition_point_speed_and_memory));
	}

	// Add boundary as a partitioning geometry.
	//
	// Note: We add this after the interior blocks since the boundary should contain the interiors.
	d_partitioning_geometries.push_back(
			PartitioningGeometry(
					rtn,
					rtn->get_triangulation_network().get_boundary_polygon(),
					d_partition_point_speed_and_memory));
}


void
GPlatesAppLogic::GeometryCookieCutter::add_partitioning_resolved_topological_boundaries(
		const std::vector<ResolvedTopologicalBoundary::non_null_ptr_type> &resolved_topological_boundaries,
		boost::optional<SortPlates> sort_plates)
{
	const partitioning_geometry_seq_type::size_type num_partitioning_geometries = d_partitioning_geometries.size();

	// Create the partitioned geometries.
	BOOST_FOREACH(const ResolvedTopologicalBoundary::non_null_ptr_type &rtb, resolved_topological_boundaries)
	{
		add_partitioning_resolved_topological_boundary(rtb);
	}

	if (sort_plates)
	{
		// Sort only the partitioning geometries just added.
		sort_plates_in_partitioning_group(
				d_partitioning_geometries.begin() + num_partitioning_geometries,
				d_partitioning_geometries.end(),
				sort_plates.get());
	}
}


void
GPlatesAppLogic::GeometryCookieCutter::add_partitioning_resolved_topological_boundary(
		const ResolvedTopologicalBoundary::non_null_ptr_type &rtb)
{
	// Add it as a partitioning geometry.
	d_partitioning_geometries.push_back(
			PartitioningGeometry(rtb, rtb->resolved_topology_boundary(), d_partition_point_speed_and_memory));
}


void
GPlatesAppLogic::GeometryCookieCutter::add_partitioning_reconstructed_feature_polygons(
		const std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &reconstructed_feature_geometries,
		boost::optional<SortPlates> sort_plates)
{
	const partitioning_geometry_seq_type::size_type num_partitioning_geometries = d_partitioning_geometries.size();

	// Create the partitioned geometries.
	BOOST_FOREACH(const ReconstructedFeatureGeometry::non_null_ptr_type &rfg, reconstructed_feature_geometries)
	{
		add_partitioning_reconstructed_feature_polygon(rfg);
	}

	if (sort_plates)
	{
		// Sort only the partitioning geometries just added.
		sort_plates_in_partitioning_group(
				d_partitioning_geometries.begin() + num_partitioning_geometries,
				d_partitioning_geometries.end(),
				sort_plates.get());
	}
}


void
GPlatesAppLogic::GeometryCookieCutter::add_partitioning_reconstructed_feature_polygon(
		const ReconstructedFeatureGeometry::non_null_ptr_type &rfg)
{
	// Get the polygon geometry.
	boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> polygon =
			GeometryUtils::get_polygon_on_sphere(*rfg->reconstructed_geometry());
	if (!polygon)
	{
		return;
	}

	// Add it as a partitioning geometry.
	d_partitioning_geometries.push_back(
			PartitioningGeometry(rfg, polygon.get(), d_partition_point_speed_and_memory));
}


void
GPlatesAppLogic::GeometryCookieCutter::sort_plates_in_partitioning_group(
		const partitioning_geometry_seq_type::iterator &partitioning_group_begin,
		const partitioning_geometry_seq_type::iterator &partitioning_group_end,
		SortPlates sort_plates)
{
	switch (sort_plates)
	{
	case SORT_BY_PLATE_ID:
		// Sort the partitioning geometries added by plate id.
		std::stable_sort(
				partitioning_group_begin,
				partitioning_group_end,
				PartitioningGeometry::SortPlateIdHighestToLowest());
		break;

	case SORT_BY_PLATE_AREA:
		// Sort the partitioning geometries added by plate area.
		std::stable_sort(
				partitioning_group_begin,
				partitioning_group_end,
				PartitioningGeometry::SortPlateAreaHighestToLowest());
		break;

	default:
		// Shouldn't get here.
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
		break;
	}
}


GPlatesAppLogic::GeometryCookieCutter::PartitioningGeometry::PartitioningGeometry(
		const ReconstructionGeometry::non_null_ptr_type &reconstruction_geometry,
		const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type &partitioning_polygon,
		GPlatesMaths::PolygonOnSphere::PointInPolygonSpeedAndMemory partition_point_speed_and_memory) :
	d_reconstruction_geometry(reconstruction_geometry),
	d_polygon_intersections(
			GPlatesMaths::PolygonIntersections::create(partitioning_polygon, partition_point_speed_and_memory))
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


bool
GPlatesAppLogic::GeometryCookieCutter::PartitioningGeometry::SortPlateAreaHighestToLowest::operator()(
		const PartitioningGeometry &lhs,
		const PartitioningGeometry &rhs) const
{
	// Mirror the functionality of 'operator > (optional x, optional y)' which is:
	//   !x ? false : ( !y ? true : (*x) > (*y) )
	boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> lhs_polygon =
			ReconstructionGeometryUtils::get_boundary_polygon(lhs.d_reconstruction_geometry);
	if (!lhs_polygon)
	{
		return false;
	}

	boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> rhs_polygon =
			ReconstructionGeometryUtils::get_boundary_polygon(rhs.d_reconstruction_geometry);
	if (!rhs_polygon)
	{
		return true;
	}

	return lhs_polygon.get()->get_area().is_precisely_greater_than(
		rhs_polygon.get()->get_area().dval());
}


void
GPlatesAppLogic::GeometryCookieCutter::AddPartitioningReconstructionGeometry::visit(
		const GPlatesUtils::non_null_intrusive_ptr<reconstructed_feature_geometry_type> &rfg)
{
	d_geometry_cookie_cutter.add_partitioning_reconstructed_feature_polygon(rfg);
}


void
GPlatesAppLogic::GeometryCookieCutter::AddPartitioningReconstructionGeometry::visit(
		const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_boundary_type> &rtb)
{
	d_geometry_cookie_cutter.add_partitioning_resolved_topological_boundary(rtb);
}


void
GPlatesAppLogic::GeometryCookieCutter::AddPartitioningReconstructionGeometry::visit(
		const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_network_type> &rtn)
{
	d_geometry_cookie_cutter.add_partitioning_resolved_topological_network(rtn);
}
