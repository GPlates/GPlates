/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
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

#include <utility>
#include <vector>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include <QFileInfo>
#include <QString>

#include "PyTopologicalSnapshot.h"

#include "PyFeatureCollectionFunctionArgument.h"
#include "PyGeometriesOnSphere.h"
#include "PyRotationModel.h"
#include "PythonConverterUtils.h"
#include "PythonExtractUtils.h"
#include "PythonHashDefVisitor.h"
#include "PythonPickle.h"
#include "PythonUtils.h"
#include "PythonVariableFunctionArguments.h"

#include "app-logic/PlateBoundaryStats.h"
#include "app-logic/PlateVelocityUtils.h"
#include "app-logic/ReconstructedFeatureGeometry.h"
#include "app-logic/ReconstructContext.h"
#include "app-logic/ReconstructHandle.h"
#include "app-logic/ReconstructionGeometryUtils.h"
#include "app-logic/ReconstructMethodRegistry.h"
#include "app-logic/ResolvedTopologicalLine.h"
#include "app-logic/ResolvedTopologicalBoundary.h"
#include "app-logic/ResolvedTopologicalNetwork.h"
#include "app-logic/ResolvedTopologicalSection.h"
#include "app-logic/ResolvedTopologicalSharedSubSegment.h"
#include "app-logic/TopologyInternalUtils.h"
#include "app-logic/TopologyUtils.h"
#include "app-logic/VelocityDeltaTime.h"
#include "app-logic/VelocityUnits.h"

#include "file-io/FeatureCollectionFileFormatRegistry.h"
#include "file-io/File.h"
#include "file-io/ReadErrorAccumulation.h"
#include "file-io/ReconstructionGeometryExportImpl.h"
#include "file-io/ResolvedTopologicalGeometryExport.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "maths/FiniteRotation.h"
#include "maths/PolygonOrientation.h"

#include "model/FeatureCollectionHandle.h"
#include "model/types.h"

#include "property-values/GeoTimeInstant.h"

#include "scribe/Scribe.h"

#include "utils/Earth.h"


namespace bp = boost::python;


namespace GPlatesApi
{
	/**
	 * This is called directly from Python via 'TopologicalSnapshot.__init__()'.
	 */
	TopologicalSnapshot::non_null_ptr_type
	topological_snapshot_create(
			const TopologicalFeatureCollectionSequenceFunctionArgument &topological_features,
			const RotationModelFunctionArgument &rotation_model_argument,
			const GPlatesPropertyValues::GeoTimeInstant &reconstruction_time,
			boost::optional<GPlatesModel::integer_plate_id_type> anchor_plate_id,
			boost::optional<ResolveTopologyParameters::non_null_ptr_to_const_type> resolve_topology_parameters)
	{
		// Time must not be distant past/future.
		if (!reconstruction_time.is_real())
		{
			PyErr_SetString(PyExc_ValueError,
					"Time values cannot be distant-past (float('inf')) or distant-future (float('-inf')).");
			bp::throw_error_already_set();
		}

		return TopologicalSnapshot::create(
				topological_features,
				rotation_model_argument,
				reconstruction_time.value(),
				anchor_plate_id,
				resolve_topology_parameters);
	}

	/**
	 * This is called directly from Python via 'TopologicalSnapshot.get_resolved_topologies()'.
	 */
	bp::list
	topological_snapshot_get_resolved_topologies(
			TopologicalSnapshot::non_null_ptr_type topological_snapshot,
			ResolveTopologyType::flags_type resolve_topology_types,
			bool same_order_as_topological_features)
	{
		// Resolved topology type flags must correspond to existing flags.
		if ((resolve_topology_types & ~ResolveTopologyType::ALL_RESOLVE_TOPOLOGY_TYPES) != 0)
		{
			PyErr_SetString(PyExc_ValueError, "Unknown bit flag specified in resolve topology types.");
			bp::throw_error_already_set();
		}

		const std::vector<GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_type> resolved_topologies =
				topological_snapshot->get_resolved_topologies(
						resolve_topology_types,
						same_order_as_topological_features);

		bp::list resolved_topologies_list;
		for (auto resolved_topology : resolved_topologies)
		{
			resolved_topologies_list.append(resolved_topology);
		}

		return resolved_topologies_list;
	}

	/**
	 * This is called directly from Python via 'TopologicalSnapshot.export_resolved_topologies()'.
	 */
	void
	topological_snapshot_export_resolved_topologies(
			TopologicalSnapshot::non_null_ptr_type topological_snapshot,
			const FilePathFunctionArgument &export_file_name,
			ResolveTopologyType::flags_type resolve_topology_types,
			bool wrap_to_dateline,
			boost::optional<GPlatesMaths::PolygonOrientation::Orientation> force_boundary_orientation)
	{
		// Resolved topology type flags must correspond to existing flags.
		if ((resolve_topology_types & ~ResolveTopologyType::ALL_RESOLVE_TOPOLOGY_TYPES) != 0)
		{
			PyErr_SetString(PyExc_ValueError, "Unknown bit flag specified in resolve topology types.");
			bp::throw_error_already_set();
		}

		topological_snapshot->export_resolved_topologies(
				export_file_name,
				resolve_topology_types,
				wrap_to_dateline,
				force_boundary_orientation);
	}

	/**
	 * This is called directly from Python via 'TopologicalSnapshot.get_resolved_topological_sections()'.
	 */
	bp::list
	topological_snapshot_get_resolved_topological_sections(
			TopologicalSnapshot::non_null_ptr_type topological_snapshot,
			ResolveTopologyType::flags_type resolve_topological_section_types,
			bool same_order_as_topological_features)
	{
		// Resolved topological section type flags must correspond to BOUNDARY and/or NETWORK.
		if ((resolve_topological_section_types & ~ResolveTopologyType::BOUNDARY_AND_NETWORK_RESOLVE_TOPOLOGY_TYPES) != 0)
		{
			PyErr_SetString(PyExc_ValueError, "Bit flags specified in resolve topological section types must be "
					"ResolveTopologyType.BOUNDARY and/or ResolveTopologyType.NETWORK.");
			bp::throw_error_already_set();
		}

		const std::vector<GPlatesAppLogic::ResolvedTopologicalSection::non_null_ptr_type> resolved_topological_sections =
				topological_snapshot->get_resolved_topological_sections(
						resolve_topological_section_types,
						same_order_as_topological_features);

		bp::list resolved_topological_sections_list;
		for (auto resolved_topological_section : resolved_topological_sections)
		{
			resolved_topological_sections_list.append(resolved_topological_section);
		}

		return resolved_topological_sections_list;
	}

	/**
	 * This is called directly from Python via 'TopologicalSnapshot.export_resolved_topological_sections()'.
	 */
	void
	topological_snapshot_export_resolved_topological_sections(
			TopologicalSnapshot::non_null_ptr_type topological_snapshot,
			const FilePathFunctionArgument &export_file_name,
			ResolveTopologyType::flags_type resolve_topological_section_types,
			bool export_topological_line_sub_segments,
			bool wrap_to_dateline)
	{
		// Resolved topology type flags must correspond to existing flags.
		if ((resolve_topological_section_types & ~ResolveTopologyType::BOUNDARY_AND_NETWORK_RESOLVE_TOPOLOGY_TYPES) != 0)
		{
			PyErr_SetString(PyExc_ValueError, "Bit flags specified in resolve topological section types must be "
					"ResolveTopologyType.BOUNDARY and/or ResolveTopologyType.NETWORK.");
			bp::throw_error_already_set();
		}

		topological_snapshot->export_resolved_topological_sections(
				export_file_name,
				resolve_topological_section_types,
				export_topological_line_sub_segments,
				wrap_to_dateline);
	}

	/**
	 * Calculate plate boundary stats at uniformly spaced points along resolved topological sections.
	 */
	bp::object
	topological_snapshot_calculate_plate_boundary_statistics(
			TopologicalSnapshot::non_null_ptr_type topological_snapshot,
			const double &uniform_point_spacing_radians,
			boost::optional<double> first_uniform_point_spacing_radians,
			const double &velocity_delta_time,
			GPlatesAppLogic::VelocityDeltaTime::Type velocity_delta_time_type,
			GPlatesAppLogic::VelocityUnits::Value velocity_units,
			const double &earth_radius_in_kms,
			bool include_network_boundaries,
			bp::object boundary_section_filter_object,
			bool return_shared_sub_segment_dict)
	{
		if (uniform_point_spacing_radians <= 0)
		{
			PyErr_SetString(PyExc_ValueError, "'uniform_point_spacing_radians' should be positive");
			bp::throw_error_already_set();
		}

		// Velocity delta time must be positive.
		if (velocity_delta_time <= 0)
		{
			PyErr_SetString(PyExc_ValueError, "Velocity delta time must be positive.");
			bp::throw_error_already_set();
		}

		// Get the resolved topological sections.
		//
		// Note: We include networks in the resolved topological sections regardless of the value of
		//       *include_network_boundaries* because we still need to discover the left/right networks
		//       sharing a plate boundary (and our first attempt at doing this is via the resolved topologies
		//       sharing the resolved topological section).
		std::vector<GPlatesAppLogic::ResolvedTopologicalSection::non_null_ptr_type> resolved_topological_sections =
				topological_snapshot->get_resolved_topological_sections(
						ResolveTopologyType::BOUNDARY_AND_NETWORK_RESOLVE_TOPOLOGY_TYPES);

		// If a boundary section filter object was specified then filter the resolved topological sections,
		// otherwise accept them all.
		if (boundary_section_filter_object != bp::object()/*Py_None*/)
		{
			std::vector<GPlatesAppLogic::ResolvedTopologicalSection::non_null_ptr_type> filtered_resolved_topological_sections;

			// See if filter object is a feature type.
			bp::extract<GPlatesModel::FeatureType> extract_feature_type(boundary_section_filter_object);
			if (extract_feature_type.check())
			{
				// Extract the allowed feature type.
				const GPlatesModel::FeatureType allowed_feature_type = extract_feature_type();

				// Filter the resolved topological sections.
				for (const auto &resolved_topological_section : resolved_topological_sections)
				{
					// Should be valid due to GPlatesApi::ResolvedTopologicalSectionWrapper.
					if (resolved_topological_section->get_feature_ref().is_valid())
					{
						const GPlatesModel::FeatureType feature_type = resolved_topological_section->get_feature_ref()->feature_type();

						// See if the feature type matches the allowed feature type.
						if (feature_type == allowed_feature_type)
						{
							filtered_resolved_topological_sections.push_back(resolved_topological_section);
						}
					}
				}
			}
			// Else attempt to extract a sequence of feature types...
			else if (PythonExtractUtils::check_sequence<GPlatesModel::FeatureType>(boundary_section_filter_object))
			{
				// Extract the allowed feature types.
				std::vector<GPlatesModel::FeatureType> allowed_feature_types;
				PythonExtractUtils::extract_sequence(allowed_feature_types, boundary_section_filter_object);

				// Filter the resolved topological sections.
				for (const auto &resolved_topological_section : resolved_topological_sections)
				{
					// Should be valid due to GPlatesApi::ResolvedTopologicalSectionWrapper.
					if (resolved_topological_section->get_feature_ref().is_valid())
					{
						const GPlatesModel::FeatureType feature_type = resolved_topological_section->get_feature_ref()->feature_type();

						// See if the feature type matches one of the allowed feature types.
						if (std::find(allowed_feature_types.begin(), allowed_feature_types.end(), feature_type) !=
							allowed_feature_types.end())
						{
							filtered_resolved_topological_sections.push_back(resolved_topological_section);
						}
					}
				}
			}
			else  // Filter must be a callable predicate...
			{
				// Filter the resolved topological sections.
				for (const auto &resolved_topological_section : resolved_topological_sections)
				{
					// Pass the resolved topological section to the callable predicate.
					if (bp::extract<bool>(boundary_section_filter_object(resolved_topological_section)))
					{
						filtered_resolved_topological_sections.push_back(resolved_topological_section);
					}
				}
			}

			resolved_topological_sections.swap(filtered_resolved_topological_sections);
		}

		// Get the resolved topological boundaries (rigid plates).
		//
		// Need to convert pointers-to-non-const to pointers-to-const.
		const std::vector<GPlatesAppLogic::ResolvedTopologicalBoundary::non_null_ptr_to_const_type> resolved_topological_boundaries(
				topological_snapshot->get_resolved_topological_boundaries().begin(),
				topological_snapshot->get_resolved_topological_boundaries().end());
		// Get the resolved topological networks (deforming regions).
		//
		// Need to convert pointers-to-non-const to pointers-to-const.
		const std::vector<GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_to_const_type> resolved_topological_networks(
				topological_snapshot->get_resolved_topological_networks().begin(),
				topological_snapshot->get_resolved_topological_networks().end());

		// Calculate the plate boundary statistics.
		std::map<
				GPlatesAppLogic::ResolvedTopologicalSharedSubSegment::non_null_ptr_type,
				std::vector<GPlatesAppLogic::PlateBoundaryStat>
		> plate_boundary_stats;
		GPlatesAppLogic::calculate_plate_boundary_stats(
				plate_boundary_stats,
				resolved_topological_sections,
				resolved_topological_boundaries,
				resolved_topological_networks,
				topological_snapshot->get_reconstruction_time(),
				uniform_point_spacing_radians,
				first_uniform_point_spacing_radians,
				velocity_delta_time,
				velocity_delta_time_type,
				velocity_units,
				earth_radius_in_kms,
				include_network_boundaries);
		
		// If we should group plate boundary stats (dict value) by their shared sub-segments (dict key).
		if (return_shared_sub_segment_dict)
		{
			bp::dict shared_sub_segment_dict;

			// Add a list of plate boundary stats for each shared sub-segment to the dict.
			for (const auto &shared_sub_segment_plate_boundary_stats : plate_boundary_stats)
			{
				bp::list shared_sub_segment_plate_boundary_stats_list;
				for (const auto &plate_boundary_stat : shared_sub_segment_plate_boundary_stats.second)
				{
					shared_sub_segment_plate_boundary_stats_list.append(plate_boundary_stat);
				}

				shared_sub_segment_dict[shared_sub_segment_plate_boundary_stats.first] =
						shared_sub_segment_plate_boundary_stats_list;
			}

			return shared_sub_segment_dict;
		}

		// One big list of plate boundary stats (not grouped by shared sub-segment).
		bp::list plate_boundary_stats_list;
		for (const auto &shared_sub_segment_plate_boundary_stats : plate_boundary_stats)
		{
			for (const auto &plate_boundary_stat : shared_sub_segment_plate_boundary_stats.second)
			{
				plate_boundary_stats_list.append(plate_boundary_stat);
			}
		}

		return plate_boundary_stats_list;
	}

	namespace
	{
		/**
		 * Find location of point in resolved topological networks and boundaries.
		 */
		GPlatesAppLogic::TopologyPointLocation
		get_point_location_in_resolved_topologies(
				const GPlatesMaths::PointOnSphere &point,
				boost::optional<std::vector<GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_type>> &resolved_topological_networks,
				boost::optional<std::vector<GPlatesAppLogic::ResolvedTopologicalBoundary::non_null_ptr_type>> &resolved_topological_boundaries)
		{
			// See if point is inside any topological networks.
			if (resolved_topological_networks)
			{
				const auto resolved_networks_begin = resolved_topological_networks->begin();
				const auto resolved_networks_end = resolved_topological_networks->end();
				for (auto resolved_networks_iter = resolved_networks_begin;
					resolved_networks_iter != resolved_networks_end;
					++resolved_networks_iter)
				{
					// NOTE: Don't use a reference here
					//       (otherwise wrong resolved network will be returned due to std::swap below).
					const auto resolved_topological_network = *resolved_networks_iter;

					if (boost::optional<GPlatesAppLogic::ResolvedTriangulation::Network::PointLocation> point_location =
						resolved_topological_network->get_triangulation_network().get_point_location(point))
					{
						// The next point is probably in the same resolved network so make it the first one to be tested next time.
						if (resolved_networks_iter != resolved_networks_begin)
						{
							std::swap(*resolved_networks_begin, *resolved_networks_iter);
						}

						return GPlatesAppLogic::TopologyPointLocation(resolved_topological_network, point_location.get());
					}
				}
			}

			// See if point is inside any topological boundaries.
			if (resolved_topological_boundaries)
			{
				const auto resolved_boundaries_begin = resolved_topological_boundaries->begin();
				const auto resolved_boundaries_end = resolved_topological_boundaries->end();
				for (auto resolved_boundaries_iter = resolved_boundaries_begin;
					resolved_boundaries_iter != resolved_boundaries_end;
					++resolved_boundaries_iter)
				{
					// NOTE: Don't use a reference here
					//       (otherwise wrong resolved boundary will be returned due to std::swap below).
					const auto resolved_topological_boundary = *resolved_boundaries_iter;

					if (resolved_topological_boundary->resolved_topology_boundary()->is_point_in_polygon(point))
					{
						// The next point is probably in the same resolved boundary so make it the first one to be tested next time.
						if (resolved_boundaries_iter != resolved_boundaries_begin)
						{
							std::swap(*resolved_boundaries_begin, *resolved_boundaries_iter);
						}

						return GPlatesAppLogic::TopologyPointLocation(resolved_topological_boundary);
					}
				}
			}

			// Point is not located inside any resolved boundaries/networks.
			return GPlatesAppLogic::TopologyPointLocation();
		}
	}

	bp::list
	topological_snapshot_get_point_locations(
			TopologicalSnapshot::non_null_ptr_type topological_snapshot,
			PointSequenceFunctionArgument point_seq,
			ResolveTopologyType::flags_type resolve_topology_types)
	{
		bp::list point_locations_list;

		// Resolved topology type flags must correspond to BOUNDARY and/or NETWORK.
		if ((resolve_topology_types & ~ResolveTopologyType::BOUNDARY_AND_NETWORK_RESOLVE_TOPOLOGY_TYPES) != 0)
		{
			PyErr_SetString(PyExc_ValueError, "Bit flags specified in resolve topology types must be "
					"ResolveTopologyType.BOUNDARY and/or ResolveTopologyType.NETWORK.");
			bp::throw_error_already_set();
		}

		// Get the resolved topological networks (if requested).
		boost::optional<std::vector<GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_type>> resolved_topological_networks;
		if ((resolve_topology_types & ResolveTopologyType::NETWORK) != 0)
		{
			resolved_topological_networks = topological_snapshot->get_resolved_topological_networks();
		}
		// Get the resolved topological boundaries (if requested).
		boost::optional<std::vector<GPlatesAppLogic::ResolvedTopologicalBoundary::non_null_ptr_type>> resolved_topological_boundaries;
		if ((resolve_topology_types & ResolveTopologyType::BOUNDARY) != 0)
		{
			resolved_topological_boundaries = topological_snapshot->get_resolved_topological_boundaries();
		}

		// Iterate over the sequence of points.
		for (const auto &point : point_seq.get_points())
		{
			// Find which resolved topological network or boundary (if any) contains the point.
			const GPlatesAppLogic::TopologyPointLocation point_location =
					get_point_location_in_resolved_topologies(
							point,
							resolved_topological_networks,
							resolved_topological_boundaries);

			point_locations_list.append(point_location);
		}

		return point_locations_list;
	}

	namespace
	{
		/**
		 * Find location, and calculate velocity, of point in resolved topological networks and boundaries.
		 *
		 * Returns none if point is not in any resolved topological networks or boundaries.
		 */
		boost::optional<std::pair<GPlatesMaths::Vector3D, GPlatesAppLogic::TopologyPointLocation>>
		get_point_velocity_in_resolved_topologies(
				const GPlatesMaths::PointOnSphere &point,
				const RotationModel::non_null_ptr_type &rotation_model,
				const double &reconstruction_time,
				boost::optional<std::vector<GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_type>> &resolved_topological_networks,
				boost::optional<std::vector<GPlatesAppLogic::ResolvedTopologicalBoundary::non_null_ptr_type>> &resolved_topological_boundaries,
				const double &velocity_delta_time,
				GPlatesAppLogic::VelocityDeltaTime::Type velocity_delta_time_type,
				GPlatesAppLogic::VelocityUnits::Value velocity_units,
				const double &earth_radius_in_kms,
				std::map<GPlatesModel::integer_plate_id_type, GPlatesMaths::FiniteRotation> &resolved_boundary_stage_rotation_map)
		{
			// See if point is inside any topological networks.
			if (resolved_topological_networks)
			{
				const auto resolved_networks_begin = resolved_topological_networks->begin();
				const auto resolved_networks_end = resolved_topological_networks->end();
				for (auto resolved_networks_iter = resolved_networks_begin;
					resolved_networks_iter != resolved_networks_end;
					++resolved_networks_iter)
				{
					// NOTE: Don't use a reference here
					//       (otherwise wrong resolved network will be returned due to std::swap below).
					const auto resolved_topological_network = *resolved_networks_iter;

					boost::optional< std::pair<GPlatesMaths::Vector3D, GPlatesAppLogic::ResolvedTriangulation::Network::PointLocation> >
							velocity = resolved_topological_network->get_triangulation_network().calculate_velocity(
									point,
									velocity_delta_time,
									velocity_delta_time_type,
									velocity_units,
									earth_radius_in_kms);
					if (velocity)
					{
						// The next point is probably in the same resolved network so make it the first one to be tested next time.
						if (resolved_networks_iter != resolved_networks_begin)
						{
							std::swap(*resolved_networks_begin, *resolved_networks_iter);
						}

						return std::make_pair(
								velocity->first,
								GPlatesAppLogic::TopologyPointLocation(resolved_topological_network, velocity->second));
					}
				}
			}

			// See if point is inside any topological boundaries.
			if (resolved_topological_boundaries)
			{
				const auto resolved_boundaries_begin = resolved_topological_boundaries->begin();
				const auto resolved_boundaries_end = resolved_topological_boundaries->end();
				for (auto resolved_boundaries_iter = resolved_boundaries_begin;
					resolved_boundaries_iter != resolved_boundaries_end;
					++resolved_boundaries_iter)
				{
					// NOTE: Don't use a reference here
					//       (otherwise wrong resolved boundary will be returned due to std::swap below).
					const auto resolved_topological_boundary = *resolved_boundaries_iter;

					if (resolved_topological_boundary->resolved_topology_boundary()->is_point_in_polygon(point))
					{
						// Get the plate ID from resolved boundary.
						//
						// If we can't get a reconstruction plate ID then we'll just use plate id zero (spin axis)
						// which can still give a non-identity rotation if the anchor plate id is non-zero.
						boost::optional<GPlatesModel::integer_plate_id_type> resolved_boundary_plate_id =
								resolved_topological_boundary->plate_id();
						if (!resolved_boundary_plate_id)
						{
							resolved_boundary_plate_id = 0;
						}

						boost::optional<GPlatesMaths::FiniteRotation> stage_rotation;

						// See if we've already calculated a stage rotation for the current plate ID.
						auto stage_rotation_map_iter = resolved_boundary_stage_rotation_map.find(resolved_boundary_plate_id.get());
						if (stage_rotation_map_iter != resolved_boundary_stage_rotation_map.end())
						{
							stage_rotation = stage_rotation_map_iter->second;
						}
						else // calculate stage rotation and insert into the map...
						{
							stage_rotation = GPlatesAppLogic::PlateVelocityUtils::calculate_stage_rotation(
									resolved_boundary_plate_id.get(),
									rotation_model->get_reconstruction_tree_creator(),
									reconstruction_time,
									velocity_delta_time,
									velocity_delta_time_type);

							// Insert stage rotation into the map.
							resolved_boundary_stage_rotation_map.insert(
								{ resolved_boundary_plate_id.get() , stage_rotation.get() });
						}

						const GPlatesMaths::Vector3D velocity =
							GPlatesAppLogic::PlateVelocityUtils::calculate_velocity_vector(
									point,
									stage_rotation.get(),
									velocity_delta_time,
									velocity_units,
									earth_radius_in_kms);

						// The next point is probably in the same resolved boundary so make it the first one to be tested next time.
						if (resolved_boundaries_iter != resolved_boundaries_begin)
						{
							std::swap(*resolved_boundaries_begin, *resolved_boundaries_iter);
						}

						return std::make_pair(
								velocity,
								GPlatesAppLogic::TopologyPointLocation(resolved_topological_boundary));
					}
				}
			}

			// Point is not located inside any resolved boundaries/networks.
			return boost::none;
		}
	}

	bp::object
	topological_snapshot_get_point_velocities(
			TopologicalSnapshot::non_null_ptr_type topological_snapshot,
			PointSequenceFunctionArgument point_seq,
			ResolveTopologyType::flags_type resolve_topology_types,
			const double &velocity_delta_time,
			GPlatesAppLogic::VelocityDeltaTime::Type velocity_delta_time_type,
			GPlatesAppLogic::VelocityUnits::Value velocity_units,
			const double &earth_radius_in_kms,
			bool return_point_locations)
	{
		bp::list point_velocities_list;

		boost::optional<bp::list> point_locations_list;
		if (return_point_locations)
		{
			point_locations_list = bp::list();
		}

		// Resolved topology type flags must correspond to BOUNDARY and/or NETWORK.
		if ((resolve_topology_types & ~ResolveTopologyType::BOUNDARY_AND_NETWORK_RESOLVE_TOPOLOGY_TYPES) != 0)
		{
			PyErr_SetString(PyExc_ValueError, "Bit flags specified in resolve topology types must be "
					"ResolveTopologyType.BOUNDARY and/or ResolveTopologyType.NETWORK.");
			bp::throw_error_already_set();
		}

		// Get the resolved topological networks (if requested).
		boost::optional<std::vector<GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_type>> resolved_topological_networks;
		if ((resolve_topology_types & ResolveTopologyType::NETWORK) != 0)
		{
			resolved_topological_networks = topological_snapshot->get_resolved_topological_networks();
		}
		// Get the resolved topological boundaries (if requested).
		boost::optional<std::vector<GPlatesAppLogic::ResolvedTopologicalBoundary::non_null_ptr_type>> resolved_topological_boundaries;
		if ((resolve_topology_types & ResolveTopologyType::BOUNDARY) != 0)
		{
			resolved_topological_boundaries = topological_snapshot->get_resolved_topological_boundaries();
		}

		// Keep track of the stage rotations of resolved boundaries as we encounter them.
		// This is an optimisation since many points can be inside the same resolved boundary.
		std::map<GPlatesModel::integer_plate_id_type, GPlatesMaths::FiniteRotation> resolved_boundary_stage_rotation_map;

		// Iterate over the sequence of points.
		for (const auto &point : point_seq.get_points())
		{
			// Find which resolved topological network or boundary (if any) contains the point.
			//
			// Note: This is none if the point is not inside any resolved boundaries/networks. 
			const boost::optional<std::pair<GPlatesMaths::Vector3D, GPlatesAppLogic::TopologyPointLocation>> point_velocity_and_location =
					get_point_velocity_in_resolved_topologies(
							point,
							topological_snapshot->get_rotation_model(),
							topological_snapshot->get_reconstruction_time(),
							resolved_topological_networks,
							resolved_topological_boundaries,
							velocity_delta_time,
							velocity_delta_time_type,
							velocity_units,
							earth_radius_in_kms,
							resolved_boundary_stage_rotation_map);

			if (point_velocity_and_location)
			{
				point_velocities_list.append(point_velocity_and_location->first);
				if (return_point_locations)
				{
					point_locations_list->append(point_velocity_and_location->second);
				}
			}
			else
			{
				// Point is not located inside any resolved boundaries/networks.
				point_velocities_list.append(bp::object()/*Py_None*/);
				if (return_point_locations)
				{
					point_locations_list->append(GPlatesAppLogic::TopologyPointLocation());
				}
			}
		}

		if (return_point_locations)
		{
			return bp::make_tuple(point_velocities_list, point_locations_list.get());
		}
		else
		{
			return point_velocities_list;
		}
	}

	namespace
	{
		/**
		 * Find location, and calculate strain rate, of point in resolved topological networks and boundaries.
		 *
		 * Returns none if point is not in any resolved topological networks or boundaries.
		 */
		boost::optional<std::pair<GPlatesAppLogic::DeformationStrainRate, GPlatesAppLogic::TopologyPointLocation>>
		get_point_strain_rate_in_resolved_topologies(
				const GPlatesMaths::PointOnSphere &point,
				boost::optional<std::vector<GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_type>> &resolved_topological_networks,
				boost::optional<std::vector<GPlatesAppLogic::ResolvedTopologicalBoundary::non_null_ptr_type>> &resolved_topological_boundaries)
		{
			// See if point is inside any topological networks.
			if (resolved_topological_networks)
			{
				const auto resolved_networks_begin = resolved_topological_networks->begin();
				const auto resolved_networks_end = resolved_topological_networks->end();
				for (auto resolved_networks_iter = resolved_networks_begin;
					resolved_networks_iter != resolved_networks_end;
					++resolved_networks_iter)
				{
					// NOTE: Don't use a reference here
					//       (otherwise wrong resolved network will be returned due to std::swap below).
					const auto resolved_topological_network = *resolved_networks_iter;

					boost::optional<std::pair<
							GPlatesAppLogic::ResolvedTriangulation::DeformationInfo,
							GPlatesAppLogic::ResolvedTriangulation::Network::PointLocation> >
									deformation_info = resolved_topological_network->get_triangulation_network().calculate_deformation(point);
					if (deformation_info)
					{
						// The next point is probably in the same resolved network so make it the first one to be tested next time.
						if (resolved_networks_iter != resolved_networks_begin)
						{
							std::swap(*resolved_networks_begin, *resolved_networks_iter);
						}

						return std::make_pair(
								deformation_info->first.get_strain_rate(),
								GPlatesAppLogic::TopologyPointLocation(resolved_topological_network, deformation_info->second));
					}
				}
			}

			// See if point is inside any topological boundaries.
			if (resolved_topological_boundaries)
			{
				const auto resolved_boundaries_begin = resolved_topological_boundaries->begin();
				const auto resolved_boundaries_end = resolved_topological_boundaries->end();
				for (auto resolved_boundaries_iter = resolved_boundaries_begin;
					resolved_boundaries_iter != resolved_boundaries_end;
					++resolved_boundaries_iter)
				{
					// NOTE: Don't use a reference here
					//       (otherwise wrong resolved boundary will be returned due to std::swap below).
					const auto resolved_topological_boundary = *resolved_boundaries_iter;

					if (resolved_topological_boundary->resolved_topology_boundary()->is_point_in_polygon(point))
					{
						// The next point is probably in the same resolved boundary so make it the first one to be tested next time.
						if (resolved_boundaries_iter != resolved_boundaries_begin)
						{
							std::swap(*resolved_boundaries_begin, *resolved_boundaries_iter);
						}

						// Return zero deformation (since inside a rigid plate).
						return std::make_pair(
								GPlatesAppLogic::DeformationStrainRate(),
								GPlatesAppLogic::TopologyPointLocation(resolved_topological_boundary));
					}
				}
			}

			// Point is not located inside any resolved boundaries/networks.
			return boost::none;
		}
	}

	bp::object
	topological_snapshot_get_point_strain_rates(
			TopologicalSnapshot::non_null_ptr_type topological_snapshot,
			PointSequenceFunctionArgument point_seq,
			ResolveTopologyType::flags_type resolve_topology_types,
			bool return_point_locations)
	{
		bp::list point_strain_rates_list;

		boost::optional<bp::list> point_locations_list;
		if (return_point_locations)
		{
			point_locations_list = bp::list();
		}

		// Resolved topology type flags must correspond to BOUNDARY and/or NETWORK.
		if ((resolve_topology_types & ~ResolveTopologyType::BOUNDARY_AND_NETWORK_RESOLVE_TOPOLOGY_TYPES) != 0)
		{
			PyErr_SetString(PyExc_ValueError, "Bit flags specified in resolve topology types must be "
					"ResolveTopologyType.BOUNDARY and/or ResolveTopologyType.NETWORK.");
			bp::throw_error_already_set();
		}

		// Get the resolved topological networks (if requested).
		boost::optional<std::vector<GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_type>> resolved_topological_networks;
		if ((resolve_topology_types & ResolveTopologyType::NETWORK) != 0)
		{
			resolved_topological_networks = topological_snapshot->get_resolved_topological_networks();
		}
		// Get the resolved topological boundaries (if requested).
		boost::optional<std::vector<GPlatesAppLogic::ResolvedTopologicalBoundary::non_null_ptr_type>> resolved_topological_boundaries;
		if ((resolve_topology_types & ResolveTopologyType::BOUNDARY) != 0)
		{
			resolved_topological_boundaries = topological_snapshot->get_resolved_topological_boundaries();
		}

		// Iterate over the sequence of points.
		for (const auto &point : point_seq.get_points())
		{
			// Find which resolved topological network or boundary (if any) contains the point.
			//
			// Note: This is none if the point is not inside any resolved boundaries/networks. 
			const boost::optional<std::pair<GPlatesAppLogic::DeformationStrainRate, GPlatesAppLogic::TopologyPointLocation>> point_strain_rate_and_location =
					get_point_strain_rate_in_resolved_topologies(
							point,
							resolved_topological_networks,
							resolved_topological_boundaries);

			if (point_strain_rate_and_location)
			{
				point_strain_rates_list.append(point_strain_rate_and_location->first);
				if (return_point_locations)
				{
					point_locations_list->append(point_strain_rate_and_location->second);
				}
			}
			else
			{
				// Point is not located inside any resolved boundaries/networks.
				point_strain_rates_list.append(bp::object()/*Py_None*/);
				if (return_point_locations)
				{
					point_locations_list->append(GPlatesAppLogic::TopologyPointLocation());
				}
			}
		}

		if (return_point_locations)
		{
			return bp::make_tuple(point_strain_rates_list, point_locations_list.get());
		}
		else
		{
			return point_strain_rates_list;
		}
	}

	namespace
	{
		/**
		 * Reconstruct points in resolved topological networks and boundaries.
		 */
		boost::optional<GPlatesMaths::PointOnSphere>
		reconstruct_point_in_resolved_topologies(
				const GPlatesMaths::PointOnSphere &point,
				const double &initial_time,
				const double &final_time,
				const double &time_increment,
				bool reverse_reconstruct,
				const RotationModel::non_null_ptr_type &rotation_model,
				boost::optional<std::vector<GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_type>> &resolved_topological_networks,
				boost::optional<std::vector<GPlatesAppLogic::ResolvedTopologicalBoundary::non_null_ptr_type>> &resolved_topological_boundaries,
				bool use_natural_neighbour_interpolation,
				bool return_input_if_not_intersect,
				std::map<GPlatesModel::integer_plate_id_type, GPlatesMaths::FiniteRotation> &resolved_boundary_stage_rotation_map)
		{
			// See if point is inside any topological networks.
			if (resolved_topological_networks)
			{
				const auto resolved_networks_begin = resolved_topological_networks->begin();
				const auto resolved_networks_end = resolved_topological_networks->end();
				for (auto resolved_networks_iter = resolved_networks_begin;
					resolved_networks_iter != resolved_networks_end;
					++resolved_networks_iter)
				{
					// NOTE: Don't use a reference here
					//       (helps avoid using wrong resolved network in case std::swap below is moved higher up).
					const auto resolved_topological_network = *resolved_networks_iter;

					boost::optional<std::pair<
							GPlatesMaths::PointOnSphere,
							GPlatesAppLogic::ResolvedTriangulation::Network::PointLocation> >
									deformed_point_result = resolved_topological_network->get_triangulation_network().calculate_deformed_point(
											point,
											time_increment,
											reverse_reconstruct,
											use_natural_neighbour_interpolation);
					if (deformed_point_result)
					{
						// The next point is probably in the same resolved network so make it the first one to be tested next time.
						if (resolved_networks_iter != resolved_networks_begin)
						{
							std::swap(*resolved_networks_begin, *resolved_networks_iter);
						}

						return deformed_point_result->first;
					}
				}
			}

			// See if point is inside any topological boundaries.
			if (resolved_topological_boundaries)
			{
				const auto resolved_boundaries_begin = resolved_topological_boundaries->begin();
				const auto resolved_boundaries_end = resolved_topological_boundaries->end();
				for (auto resolved_boundaries_iter = resolved_boundaries_begin;
					resolved_boundaries_iter != resolved_boundaries_end;
					++resolved_boundaries_iter)
				{
					// NOTE: Don't use a reference here
					//       (helps avoid using wrong resolved boundary in case std::swap below is moved higher up).
					const auto resolved_topological_boundary = *resolved_boundaries_iter;

					// See if point is inside the resolved topological boundary.
					if (resolved_topological_boundary->resolved_topology_boundary()->is_point_in_polygon(point))
					{
						// Get the plate ID from resolved boundary.
						//
						// If we can't get a reconstruction plate ID then we'll just use plate id zero (spin axis)
						// which can still give a non-identity rotation if the anchor plate id is non-zero.
						boost::optional<GPlatesModel::integer_plate_id_type> resolved_boundary_plate_id =
								resolved_topological_boundary->plate_id();
						if (!resolved_boundary_plate_id)
						{
							resolved_boundary_plate_id = 0;
						}

						boost::optional<GPlatesMaths::FiniteRotation> stage_rotation;

						// See if we've already calculated a stage rotation for the current plate ID.
						auto stage_rotation_map_iter = resolved_boundary_stage_rotation_map.find(resolved_boundary_plate_id.get());
						if (stage_rotation_map_iter != resolved_boundary_stage_rotation_map.end())
						{
							stage_rotation = stage_rotation_map_iter->second;
						}
						else // calculate stage rotation and insert into the map...
						{
							//
							// Delegate to 'PlateVelocityUtils::calculate_stage_rotation()' since it adjusts the
							// stage rotation time interval if one of the times goes negative or if the rotation
							// file only has rotations up to time 't', but not time 't+dt'.
							//

							if (reverse_reconstruct) // forward in time ...
							{
								// Forward stage rotation from 'initial_time' to 'final_time'.
								stage_rotation = GPlatesAppLogic::PlateVelocityUtils::calculate_stage_rotation(
										resolved_boundary_plate_id.get(),
										rotation_model->get_reconstruction_tree_creator(),
										initial_time,
										// Must be positive...
										time_increment/*velocity_delta_time*/,
										GPlatesAppLogic::VelocityDeltaTime::T_TO_T_MINUS_DELTA_T/*velocity_delta_time_type*/);
							}
							else // backward in time ...
							{
								// Backward stage rotation from 'initial_time' to 'final_time'.
								//
								// Note: Need to reverse rotation from forward-in-time to backward-in-time.
								stage_rotation = GPlatesMaths::get_reverse(
										GPlatesAppLogic::PlateVelocityUtils::calculate_stage_rotation(
												resolved_boundary_plate_id.get(),
												rotation_model->get_reconstruction_tree_creator(),
												initial_time,
												// Must be positive...
												time_increment/*velocity_delta_time*/,
												GPlatesAppLogic::VelocityDeltaTime::T_PLUS_DELTA_T_TO_T/*velocity_delta_time_type*/));
							}

							// Insert stage rotation into the map.
							resolved_boundary_stage_rotation_map.insert(
								{ resolved_boundary_plate_id.get() , stage_rotation.get() });
						}

						// The next point is probably in the same resolved boundary so make it the first one to be tested next time.
						if (resolved_boundaries_iter != resolved_boundaries_begin)
						{
							std::swap(*resolved_boundaries_begin, *resolved_boundaries_iter);
						}

						// Return reconstructed point.
						return stage_rotation.get() * point;
					}
				}
			}

			// Point is not located inside any resolved boundaries/networks.

			if (return_input_if_not_intersect)
			{
				return point;
			}

			return boost::none;
		}
	}

	bp::object
	topological_snapshot_reconstruct_points(
			TopologicalSnapshot::non_null_ptr_type topological_snapshot,
			PointSequenceFunctionArgument point_seq,
			const GPlatesPropertyValues::GeoTimeInstant &reconstruction_time,
			ResolveTopologyType::flags_type resolve_topology_types,
			bool use_natural_neighbour_interpolation,
			bool return_input_if_not_intersect)
	{
		bp::list reconstructed_points;

		// Reconstruction time must not be distant past/future.
		if (!reconstruction_time.is_real())
		{
			PyErr_SetString(PyExc_ValueError,
					"Reconstruction time cannot be distant-past (float('inf')) or distant-future (float('-inf')).");
			bp::throw_error_already_set();
		}

		// The initial time is the time we're reconstructing *from*.
		const double initial_time = topological_snapshot->get_reconstruction_time();
		// The final time is the time we're reconstructing *to*.
		const double final_time = reconstruction_time.value();

		bool reverse_reconstruct;
		double time_increment;
		if (initial_time > final_time)
		{
			// Final time is *younger* than the initial time.
			// So we are reverse reconstructing (going forward in time).
			reverse_reconstruct = true;
			// The time increment should always be positive.
			time_increment = initial_time - final_time;
		}
		else
		{
			// Final time is *older* than the initial time.
			// So we are reconstructing (going backward in time).
			reverse_reconstruct = false;
			// The time increment should always be positive.
			time_increment = final_time - initial_time;
		}

		// Resolved topology type flags must correspond to BOUNDARY and/or NETWORK.
		if ((resolve_topology_types & ~ResolveTopologyType::BOUNDARY_AND_NETWORK_RESOLVE_TOPOLOGY_TYPES) != 0)
		{
			PyErr_SetString(PyExc_ValueError, "Bit flags specified in resolve topology types must be "
					"ResolveTopologyType.BOUNDARY and/or ResolveTopologyType.NETWORK.");
			bp::throw_error_already_set();
		}

		// Get the resolved topological networks (if requested).
		boost::optional<std::vector<GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_type>> resolved_topological_networks;
		if ((resolve_topology_types & ResolveTopologyType::NETWORK) != 0)
		{
			resolved_topological_networks = topological_snapshot->get_resolved_topological_networks();
		}
		// Get the resolved topological boundaries (if requested).
		boost::optional<std::vector<GPlatesAppLogic::ResolvedTopologicalBoundary::non_null_ptr_type>> resolved_topological_boundaries;
		if ((resolve_topology_types & ResolveTopologyType::BOUNDARY) != 0)
		{
			resolved_topological_boundaries = topological_snapshot->get_resolved_topological_boundaries();
		}

		// Keep track of the stage rotations of resolved boundaries as we encounter them.
		// This is an optimisation since many points can be inside the same resolved boundary.
		std::map<GPlatesModel::integer_plate_id_type, GPlatesMaths::FiniteRotation> resolved_boundary_stage_rotation_map;

		// Iterate over the sequence of points.
		for (const auto &point : point_seq.get_points())
		{
			// Reconstruct the point using the resolved topological network or boundary (if any) containing the point.
			const boost::optional<GPlatesMaths::PointOnSphere> reconstructed_point =
					reconstruct_point_in_resolved_topologies(
							point,
							initial_time,
							final_time,
							time_increment,
							reverse_reconstruct,
							topological_snapshot->get_rotation_model(),
							resolved_topological_networks,
							resolved_topological_boundaries,
							use_natural_neighbour_interpolation,
							return_input_if_not_intersect,
							resolved_boundary_stage_rotation_map);

			reconstructed_points.append(reconstructed_point);
		}

		return reconstructed_points;
	}

	/**
	 * Returns the boundary feature.
	 *
	 * The feature reference could be invalid.
	 * It should normally be valid though so we don't document that Py_None could be returned to the caller.
	 */
	boost::optional<GPlatesModel::FeatureHandle::non_null_ptr_type>
	plate_boundary_statistic_get_boundary_feature(
			const GPlatesAppLogic::PlateBoundaryStat &plate_boundary_statistic)
	{
		// The feature reference could be invalid. It should normally be valid though.
		const GPlatesModel::FeatureHandle::weak_ref boundary_feature_ref =
				plate_boundary_statistic.get_boundary_feature();
		if (!boundary_feature_ref.is_valid())
		{
			return boost::none;
		}

		return GPlatesModel::FeatureHandle::non_null_ptr_type(boundary_feature_ref.handle_ptr());
	}

	// Convert UnitVector3D to Vector3D.
	GPlatesMaths::Vector3D
	plate_boundary_statistic_get_boundary_normal(
			const GPlatesAppLogic::PlateBoundaryStat &plate_boundary_statistic)
	{
		return GPlatesMaths::Vector3D(plate_boundary_statistic.get_boundary_normal());
	}

	double
	plate_boundary_statistic_get_convergence_velocity_magnitude(
			const GPlatesAppLogic::PlateBoundaryStat &plate_boundary_statistic)
	{
		return plate_boundary_statistic.get_convergence_velocity_magnitude(false/*return_signed_magnitude*/);
	}

	double
	plate_boundary_statistic_get_convergence_velocity_signed_magnitude(
			const GPlatesAppLogic::PlateBoundaryStat &plate_boundary_statistic)
	{
		return plate_boundary_statistic.get_convergence_velocity_magnitude(true/*return_signed_magnitude*/);
	}


	TopologicalSnapshot::non_null_ptr_type
	TopologicalSnapshot::create(
			const TopologicalFeatureCollectionSequenceFunctionArgument &topological_features,
			const RotationModelFunctionArgument &rotation_model_argument,
			const double &reconstruction_time,
			boost::optional<GPlatesModel::integer_plate_id_type> anchor_plate_id,
			boost::optional<ResolveTopologyParameters::non_null_ptr_to_const_type> default_resolve_topology_parameters)
	{
		// Extract the rotation model from the function argument and adapt it to a new one that has 'anchor_plate_id'
		// as its default (which if none, then uses default anchor plate of extracted rotation model instead).
		// This ensures we will reconstruct topological sections using the correct anchor plate.
		RotationModel::non_null_ptr_type rotation_model = RotationModel::create(
				rotation_model_argument.get_rotation_model(),
				1/*reconstruction_tree_cache_size*/,
				anchor_plate_id);

		// Get the topological files.
		std::vector<GPlatesFileIO::File::non_null_ptr_type> topological_files;
		topological_features.get_files(topological_files);

		// Get the associated resolved topology parameters.
		std::vector<boost::optional<ResolveTopologyParameters::non_null_ptr_to_const_type>> resolve_topology_parameters;
		topological_features.get_resolve_topology_parameters(resolve_topology_parameters);

		// If no resolve topology parameters specified then use default values.
		if (!default_resolve_topology_parameters)
		{
			default_resolve_topology_parameters = ResolveTopologyParameters::create();
		}

		return non_null_ptr_type(
				new TopologicalSnapshot(
						rotation_model,
						topological_files,
						resolve_topology_parameters,
						default_resolve_topology_parameters.get(),
						reconstruction_time));
	}
	
	TopologicalSnapshot::TopologicalSnapshot(
			const std::vector<GPlatesAppLogic::ResolvedTopologicalLine::non_null_ptr_type> &resolved_topological_lines,
			const std::vector<GPlatesAppLogic::ResolvedTopologicalBoundary::non_null_ptr_type> &resolved_topological_boundaries,
			const std::vector<GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_type> &resolved_topological_networks,
			const RotationModel::non_null_ptr_type &rotation_model,
			const std::vector<GPlatesFileIO::File::non_null_ptr_type> &topological_files,
			const std::vector<boost::optional<ResolveTopologyParameters::non_null_ptr_to_const_type>> &resolve_topology_parameters,
			ResolveTopologyParameters::non_null_ptr_to_const_type default_resolve_topology_parameters,
			const double &reconstruction_time) :
		d_rotation_model(rotation_model),
		d_topological_files(topological_files),
		d_resolve_topology_parameters(resolve_topology_parameters),
		d_default_resolve_topology_parameters(default_resolve_topology_parameters),
		d_reconstruction_time(reconstruction_time),
		d_resolved_topological_lines(resolved_topological_lines),
		d_resolved_topological_boundaries(resolved_topological_boundaries),
		d_resolved_topological_networks(resolved_topological_networks)
	{
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				d_resolve_topology_parameters.size() == d_topological_files.size(),
				GPLATES_ASSERTION_SOURCE);
	}

	TopologicalSnapshot::TopologicalSnapshot(
			const RotationModel::non_null_ptr_type &rotation_model,
			const std::vector<GPlatesFileIO::File::non_null_ptr_type> &topological_files,
			const std::vector<boost::optional<ResolveTopologyParameters::non_null_ptr_to_const_type>> &resolve_topology_parameters,
			ResolveTopologyParameters::non_null_ptr_to_const_type default_resolve_topology_parameters,
			const double &reconstruction_time) :
		d_rotation_model(rotation_model),
		d_topological_files(topological_files),
		d_resolve_topology_parameters(resolve_topology_parameters),
		d_default_resolve_topology_parameters(default_resolve_topology_parameters),
		d_reconstruction_time(reconstruction_time)
	{
		initialise_resolved_topologies();
	}

	void
	TopologicalSnapshot::initialise_resolved_topologies()
	{
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				d_resolve_topology_parameters.size() == d_topological_files.size(),
				GPLATES_ASSERTION_SOURCE);

		// Clear the data members we're about to initialise in case this function called during transcribing.
		d_resolved_topological_lines.clear();
		d_resolved_topological_boundaries.clear();
		d_resolved_topological_networks.clear();
		// Also clear any caches.
		for (auto &resolved_topological_sections : d_resolved_topological_sections)
		{
			resolved_topological_sections = boost::none;
		}

		// Extract topological feature collection weak refs from their files.
		std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> topological_feature_collections;
		for (const auto &topological_file : d_topological_files)
		{
			topological_feature_collections.push_back(
					topological_file->get_reference().get_feature_collection());
		}

		// Find the topological section feature IDs referenced by any topological features at the reconstruction time.
		//
		// This is an optimisation that avoids unnecessary reconstructions. Only those topological sections referenced
		// by topologies that exist at the reconstruction time are reconstructed.
		std::set<GPlatesModel::FeatureId> topological_sections_referenced;
		for (const auto &topological_feature_collection : topological_feature_collections)
		{
			GPlatesAppLogic::TopologyInternalUtils::find_topological_sections_referenced(
					topological_sections_referenced,
					topological_feature_collection,
					boost::none/*topology_geometry_type*/,
					d_reconstruction_time);
		}

		// Contains the topological section regular geometries referenced by topologies.
		std::vector<GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type> reconstructed_feature_geometries;

		// Generate RFGs only for the referenced topological sections.
		GPlatesAppLogic::ReconstructMethodRegistry reconstruct_method_registry;
		GPlatesAppLogic::ReconstructContext reconstruct_context(reconstruct_method_registry);
		reconstruct_context.set_features(topological_feature_collections);
		const GPlatesAppLogic::ReconstructHandle::type reconstruct_handle =
				reconstruct_context.get_reconstructed_topological_sections(
						reconstructed_feature_geometries,
						topological_sections_referenced,
						reconstruct_context.create_context_state(
								GPlatesAppLogic::ReconstructMethodInterface::Context(
										GPlatesAppLogic::ReconstructParams(),
										d_rotation_model->get_reconstruction_tree_creator())),
						d_reconstruction_time);

		// All reconstruct handles used to find topological sections (referenced by topological boundaries/networks).
		std::vector<GPlatesAppLogic::ReconstructHandle::type> topological_sections_reconstruct_handles(1, reconstruct_handle);

		// Resolved topological line sections are referenced by topological boundaries and networks.
		//
		// Resolving topological lines generates its own reconstruct handle that will be used by
		// topological boundaries and networks to find this group of resolved lines.
		const GPlatesAppLogic::ReconstructHandle::type resolved_topological_lines_handle =
				GPlatesAppLogic::TopologyUtils::resolve_topological_lines(
						d_resolved_topological_lines,
						topological_feature_collections,
						d_rotation_model->get_reconstruction_tree_creator(),
						d_reconstruction_time,
						// Resolved topo lines use the reconstructed non-topo geometries...
						topological_sections_reconstruct_handles
	// NOTE: We need to generate all resolved topological lines, not just those referenced by resolved boundaries/networks,
	//       because the user may later explicitly request the resolved topological lines (or explicitly export them)...
#if 0
						// Only those topo lines references by resolved boundaries/networks...
						, topological_sections_referenced
#endif
				);

		topological_sections_reconstruct_handles.push_back(resolved_topological_lines_handle);

		// Resolve topological boundaries.
		GPlatesAppLogic::TopologyUtils::resolve_topological_boundaries(
				d_resolved_topological_boundaries,
				topological_feature_collections,
				d_rotation_model->get_reconstruction_tree_creator(),
				d_reconstruction_time,
				// Resolved topo boundaries use the resolved topo lines *and* the reconstructed non-topo geometries...
				topological_sections_reconstruct_handles);

		//
		// Resolve topological networks.
		//
		// The resolve topology parameters currently only affect the resolving of *networks*.
		//
		// Each feature collection can have a different resolve topology parameters so resolve them separately.
		const unsigned int num_feature_collections = topological_feature_collections.size();
		for (unsigned int feature_collection_index = 0; feature_collection_index < num_feature_collections; ++feature_collection_index)
		{
			auto topological_feature_collection = topological_feature_collections[feature_collection_index];
			auto resolve_topology_parameters = d_resolve_topology_parameters[feature_collection_index];

			// If current feature collection did not specify resolve topology parameters then use the default parameters.
			if (!resolve_topology_parameters)
			{
				resolve_topology_parameters = d_default_resolve_topology_parameters;
			}

			GPlatesAppLogic::TopologyUtils::resolve_topological_networks(
					d_resolved_topological_networks,
					d_reconstruction_time,
					std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>(1, topological_feature_collection),
					// Resolved topo networks use the resolved topo lines *and* the reconstructed non-topo geometries...
					topological_sections_reconstruct_handles,
					resolve_topology_parameters.get()->get_topology_network_params());
		}
	}

	std::vector<GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_type>
	TopologicalSnapshot::get_resolved_topologies(
			ResolveTopologyType::flags_type resolve_topology_types,
			bool same_order_as_topological_features) const
	{
		// Gather all the resolved topologies to output.
		std::vector<GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_type> resolved_topologies;

		if ((resolve_topology_types & ResolveTopologyType::LINE) != 0)
		{
			resolved_topologies.insert(
					resolved_topologies.end(),
					d_resolved_topological_lines.begin(),
					d_resolved_topological_lines.end());
		}

		if ((resolve_topology_types & ResolveTopologyType::BOUNDARY) != 0)
		{
			resolved_topologies.insert(
					resolved_topologies.end(),
					d_resolved_topological_boundaries.begin(),
					d_resolved_topological_boundaries.end());
		}

		if ((resolve_topology_types & ResolveTopologyType::NETWORK) != 0)
		{
			resolved_topologies.insert(
					resolved_topologies.end(),
					d_resolved_topological_networks.begin(),
					d_resolved_topological_networks.end());
		}

		if (same_order_as_topological_features)
		{
			// Sort the resolved topologies in the order of the features in the topological files(and the order across files).
			resolved_topologies = sort_resolved_topologies(resolved_topologies);
		}

		return resolved_topologies;
	}

	void
	TopologicalSnapshot::export_resolved_topologies(
			const FilePathFunctionArgument &export_file_path,
			ResolveTopologyType::flags_type resolve_topology_types,
			bool wrap_to_dateline,
			boost::optional<GPlatesMaths::PolygonOrientation::Orientation> force_boundary_orientation) const
	{
		const QString export_file_name = export_file_path.get_file_path();

		// Get the resolved topologies.
		const std::vector<GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_type> resolved_topologies =
				get_resolved_topologies(
						resolve_topology_types,
						// We don't need to sort the resolved topologies because the following export will do that...
						false/*same_order_as_topological_features*/);

		// Convert resolved topologies to raw pointers.
		std::vector<const GPlatesAppLogic::ReconstructionGeometry *> resolved_topology_ptrs;
		resolved_topology_ptrs.reserve(resolved_topologies.size());
		for (const auto &resolved_topology : resolved_topologies)
		{
			resolved_topology_ptrs.push_back(resolved_topology.get());
		}

		// Get the sequence of topological files as File pointers.
		std::vector<const GPlatesFileIO::File::Reference *> topological_file_ptrs;
		for (const auto &topological_file : d_topological_files)
		{
			topological_file_ptrs.push_back(&topological_file->get_reference());
		}

		// Get the sequence of reconstruction files (if any) from the rotation model.
		std::vector<GPlatesFileIO::File::non_null_ptr_type> reconstruction_files;
		d_rotation_model->get_files(reconstruction_files);

		// Convert the sequence of reconstruction files to File pointers.
		std::vector<const GPlatesFileIO::File::Reference *> reconstruction_file_ptrs;
		for (const auto &reconstruction_file : reconstruction_files)
		{
			reconstruction_file_ptrs.push_back(&reconstruction_file->get_reference());
		}

		GPlatesFileIO::FeatureCollectionFileFormat::Registry file_format_registry;
		const GPlatesFileIO::ResolvedTopologicalGeometryExport::Format format =
				GPlatesFileIO::ResolvedTopologicalGeometryExport::get_export_file_format(
						QFileInfo(export_file_name),
						file_format_registry);

		// The API docs state that dateline wrapping should be ignored except for Shapefile.
		//
		// For example, we don't want to pollute real-world data with dateline vertices when
		// using GMT software (since it can handle 3D globe data, whereas ESRI handles only 2D).
		if (format != GPlatesFileIO::ResolvedTopologicalGeometryExport::SHAPEFILE)
		{
			wrap_to_dateline = false;
		}

		// Export the resolved topologies.
		GPlatesFileIO::ResolvedTopologicalGeometryExport::export_resolved_topological_geometries(
					export_file_name,
					format,
					resolved_topology_ptrs,
					topological_file_ptrs,
					reconstruction_file_ptrs,
					get_anchor_plate_id(),
					d_reconstruction_time,
					// Shapefiles do not support topological features but they can support
					// regular features (as topological sections) so if exporting to Shapefile and
					// there's only *one* input topological *sections* file then its shapefile attributes
					// will get copied to output...
					true/*export_single_output_file*/,
					false/*export_per_input_file*/, // We only generate a single output file.
					false/*export_output_directory_per_input_file*/, // We only generate a single output file.
					force_boundary_orientation,
					wrap_to_dateline);
	}

	std::vector<GPlatesAppLogic::ResolvedTopologicalSection::non_null_ptr_type>
	TopologicalSnapshot::get_resolved_topological_sections(
			ResolveTopologyType::flags_type resolve_topological_section_types,
			bool same_order_as_topological_features) const
	{
		// Array index zero corresponds to an empty 'resolve_topological_section_types' where no sections are returned.
		unsigned int array_index = 0;

		if ((resolve_topological_section_types & ResolveTopologyType::BOUNDARY_AND_NETWORK_RESOLVE_TOPOLOGY_TYPES) ==
			ResolveTopologyType::BOUNDARY_AND_NETWORK_RESOLVE_TOPOLOGY_TYPES)
		{
			// BOUNDARY and NETWORK
			array_index = 1;
		}
		else if ((resolve_topological_section_types & ResolveTopologyType::BOUNDARY) == ResolveTopologyType::BOUNDARY)
		{
			// BOUNDARY only
			array_index = 2;
		}
		else if ((resolve_topological_section_types & ResolveTopologyType::NETWORK) == ResolveTopologyType::NETWORK)
		{
			// NETWORK only
			array_index = 3;
		}
		// else array_index = 0

		// Find the sections if they've not already been cached.
		if (!d_resolved_topological_sections[array_index])
		{
			d_resolved_topological_sections[array_index] = find_resolved_topological_sections(resolve_topological_section_types);
		}

		// Copy the cached sections in case we need to sort them next.
		// Note that if no sorting is done then this copy and the returned copy should get combined into one copy by the compiler.
		std::vector<GPlatesAppLogic::ResolvedTopologicalSection::non_null_ptr_type> resolved_topological_sections =
				d_resolved_topological_sections[array_index].get();

		if (same_order_as_topological_features)
		{
			// Sort the resolved topological sections in the order of the features in the topological files(and the order across files).
			resolved_topological_sections = sort_resolved_topological_sections(resolved_topological_sections);
		}

		return resolved_topological_sections;
	}

	void
	TopologicalSnapshot::export_resolved_topological_sections(
			const FilePathFunctionArgument &export_file_path,
			ResolveTopologyType::flags_type resolve_topological_section_types,
			bool export_topological_line_sub_segments,
			bool wrap_to_dateline) const
	{
		const QString export_file_name = export_file_path.get_file_path();

		// Get the resolved topological sections.
		const std::vector<GPlatesAppLogic::ResolvedTopologicalSection::non_null_ptr_type> resolved_topological_sections =
				get_resolved_topological_sections(
						resolve_topological_section_types,
						// We don't need to sort the resolved topological sections because the following export will do that...
						false/*same_order_as_topological_features*/);

		// Converts to raw pointers.
		std::vector<const GPlatesAppLogic::ResolvedTopologicalSection *> resolved_topological_section_ptrs;
		resolved_topological_section_ptrs.reserve(resolved_topological_sections.size());
		for (auto resolved_topological_section : resolved_topological_sections)
		{
			resolved_topological_section_ptrs.push_back(resolved_topological_section.get());
		}

		// Get the sequence of topological files as File pointers.
		std::vector<const GPlatesFileIO::File::Reference *> topological_file_ptrs;
		for (const auto &topological_file : d_topological_files)
		{
			topological_file_ptrs.push_back(&topological_file->get_reference());
		}

		// Get the sequence of reconstruction files (if any) from the rotation model.
		std::vector<GPlatesFileIO::File::non_null_ptr_type> reconstruction_files;
		d_rotation_model->get_files(reconstruction_files);

		// Convert the sequence of reconstruction files to File pointers.
		std::vector<const GPlatesFileIO::File::Reference *> reconstruction_file_ptrs;
		for (const auto &reconstruction_file : reconstruction_files)
		{
			reconstruction_file_ptrs.push_back(&reconstruction_file->get_reference());
		}

		GPlatesFileIO::FeatureCollectionFileFormat::Registry file_format_registry;
		const GPlatesFileIO::ResolvedTopologicalGeometryExport::Format format =
				GPlatesFileIO::ResolvedTopologicalGeometryExport::get_export_file_format(
						QFileInfo(export_file_name),
						file_format_registry);

		// The API docs state that dateline wrapping should be ignored except for Shapefile.
		//
		// For example, we don't want to pollute real-world data with dateline vertices when
		// using GMT software (since it can handle 3D globe data, whereas ESRI handles only 2D).
		if (format != GPlatesFileIO::ResolvedTopologicalGeometryExport::SHAPEFILE)
		{
			wrap_to_dateline = false;
		}

		// Export the resolved topological sections.
		GPlatesFileIO::ResolvedTopologicalGeometryExport::export_resolved_topological_sections(
					export_file_name,
					format,
					resolved_topological_section_ptrs,
					topological_file_ptrs,
					reconstruction_file_ptrs,
					get_anchor_plate_id(),
					d_reconstruction_time,
					// If exporting to Shapefile and there's only *one* input reconstructable file then
					// shapefile attributes in input reconstructable file will get copied to output...
					true/*export_single_output_file*/,
					false/*export_per_input_file*/, // We only generate a single output file.
					false/*export_output_directory_per_input_file*/, // We only generate a single output file.
					export_topological_line_sub_segments,
					wrap_to_dateline);
	}

	std::vector<GPlatesAppLogic::ResolvedTopologicalSection::non_null_ptr_type>
	TopologicalSnapshot::find_resolved_topological_sections(
			ResolveTopologyType::flags_type resolve_topological_section_types) const
	{
		//
		// Find the shared resolved topological sections from the resolved topological boundaries and/or networks.
		//
		// If no boundaries or networks were requested for some reason then there will be no shared
		// resolved topological sections and we'll get an empty list or an exported file with no features in it.
		//

		// Include resolved topological *boundaries* if requested...
		std::vector<GPlatesAppLogic::ResolvedTopologicalBoundary::non_null_ptr_to_const_type> resolved_topological_boundaries;
		if ((resolve_topological_section_types & ResolveTopologyType::BOUNDARY) != 0)
		{
			resolved_topological_boundaries.insert(
					resolved_topological_boundaries.end(),
					d_resolved_topological_boundaries.begin(),
					d_resolved_topological_boundaries.end());
		}

		// Include resolved topological *networks* if requested...
		std::vector<GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_to_const_type> resolved_topological_networks;
		if ((resolve_topological_section_types & ResolveTopologyType::NETWORK) != 0)
		{
			resolved_topological_networks.insert(
					resolved_topological_networks.end(),
					d_resolved_topological_networks.begin(),
					d_resolved_topological_networks.end());
		}

		std::vector<GPlatesAppLogic::ResolvedTopologicalSection::non_null_ptr_type> resolved_topological_sections;
		GPlatesAppLogic::TopologyUtils::find_resolved_topological_sections(
				resolved_topological_sections,
				resolved_topological_boundaries,
				resolved_topological_networks);

		return resolved_topological_sections;
	}

	std::vector<GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_type>
	TopologicalSnapshot::sort_resolved_topologies(
			const std::vector<GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_type> &resolved_topologies) const
	{
		// Get the sequence of topological files as File pointers.
		std::vector<const GPlatesFileIO::File::Reference *> topological_file_ptrs;
		for (auto topological_file : d_topological_files)
		{
			topological_file_ptrs.push_back(&topological_file->get_reference());
		}

		// Converts resolved topologies to raw pointers.
		std::vector<const GPlatesAppLogic::ReconstructionGeometry *> resolved_topology_ptrs;
		resolved_topology_ptrs.reserve(resolved_topologies.size());
		for (auto resolved_topology : resolved_topologies)
		{
			resolved_topology_ptrs.push_back(resolved_topology.get());
		}

		//
		// Order the resolved topologies according to the order of the features in the feature collections.
		//

		// Get the list of active topological feature collection files that contain
		// the features referenced by the ReconstructionGeometry objects.
		GPlatesFileIO::ReconstructionGeometryExportImpl::feature_handle_to_collection_map_type feature_to_collection_map;
		GPlatesFileIO::ReconstructionGeometryExportImpl::populate_feature_handle_to_collection_map(
				feature_to_collection_map,
				topological_file_ptrs);

		// Group the ReconstructionGeometry objects by their feature.
		typedef GPlatesFileIO::ReconstructionGeometryExportImpl::FeatureGeometryGroup<
				GPlatesAppLogic::ReconstructionGeometry> feature_geometry_group_type;
		std::list<feature_geometry_group_type> grouped_recon_geoms_seq;
		GPlatesFileIO::ReconstructionGeometryExportImpl::group_reconstruction_geometries_with_their_feature(
				grouped_recon_geoms_seq,
				resolved_topology_ptrs,
				feature_to_collection_map);

		//
		// Add to the ordered sequence of resolved topologies.
		//
		
		// The sorted sequence to return;
		std::vector<GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_type> sorted_resolved_topologies;
		sorted_resolved_topologies.reserve(resolved_topologies.size());

		for (const feature_geometry_group_type &feature_geom_group : grouped_recon_geoms_seq)
		{
			const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref = feature_geom_group.feature_ref;
			if (!feature_ref.is_valid())
			{
				continue;
			}

			// Iterate through the reconstruction geometries of the current feature.
			for (auto const_rg_ptr : feature_geom_group.recon_geoms)
			{
				GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type const_rg(const_rg_ptr);

				sorted_resolved_topologies.push_back(
						// Need to pass ReconstructionGeometry::non_null_ptr_type to Python...
						GPlatesUtils::const_pointer_cast<GPlatesAppLogic::ReconstructionGeometry>(const_rg));
			}
		}

		return sorted_resolved_topologies;
	}

	std::vector<GPlatesAppLogic::ResolvedTopologicalSection::non_null_ptr_type>
	TopologicalSnapshot::sort_resolved_topological_sections(
			const std::vector<GPlatesAppLogic::ResolvedTopologicalSection::non_null_ptr_type> &resolved_topological_sections) const
	{
		// Get the sequence of topological files as File pointers.
		std::vector<const GPlatesFileIO::File::Reference *> topological_file_ptrs;
		for (auto topological_file : d_topological_files)
		{
			topological_file_ptrs.push_back(&topological_file->get_reference());
		}

		// We need to determine which resolved topological sections belong to which feature group
		// so we know which sections to write out which output file.
		typedef std::map<
				const GPlatesAppLogic::ReconstructionGeometry *,
				const GPlatesAppLogic::ResolvedTopologicalSection *>
						recon_geom_to_resolved_section_map_type;
		recon_geom_to_resolved_section_map_type recon_geom_to_resolved_section_map;

		// List of the resolved topological section ReconstructionGeometry's.
		// We'll use these to determine which features/collections each section came from.
		std::vector<const GPlatesAppLogic::ReconstructionGeometry *> resolved_topological_section_recon_geom_ptrs;

		for (const auto &resolved_topological_section : resolved_topological_sections)
		{
			const GPlatesAppLogic::ReconstructionGeometry *resolved_topological_section_recon_geom_ptr =
					resolved_topological_section->get_reconstruction_geometry().get();

			recon_geom_to_resolved_section_map.insert(
					recon_geom_to_resolved_section_map_type::value_type(
							resolved_topological_section_recon_geom_ptr,
							resolved_topological_section.get()));

			resolved_topological_section_recon_geom_ptrs.push_back(
					resolved_topological_section_recon_geom_ptr);
		}

		//
		// Order the resolved topological sections according to the order of the features in the feature collections.
		//

		// Get the list of active topological feature collection files that contain
		// the features referenced by the ReconstructionGeometry objects.
		GPlatesFileIO::ReconstructionGeometryExportImpl::feature_handle_to_collection_map_type feature_to_collection_map;
		GPlatesFileIO::ReconstructionGeometryExportImpl::populate_feature_handle_to_collection_map(
				feature_to_collection_map,
				topological_file_ptrs);

		// Group the ReconstructionGeometry objects by their feature.
		typedef GPlatesFileIO::ReconstructionGeometryExportImpl::FeatureGeometryGroup<
				GPlatesAppLogic::ReconstructionGeometry> feature_geometry_group_type;
		std::list<feature_geometry_group_type> grouped_recon_geoms_seq;
		GPlatesFileIO::ReconstructionGeometryExportImpl::group_reconstruction_geometries_with_their_feature(
				grouped_recon_geoms_seq,
				resolved_topological_section_recon_geom_ptrs,
				feature_to_collection_map);

		//
		// Add to the ordered sequence of resolved topological sections.
		//

		// The sorted sequence to return;
		std::vector<GPlatesAppLogic::ResolvedTopologicalSection::non_null_ptr_type> sorted_resolved_topological_sections;
		sorted_resolved_topological_sections.reserve(resolved_topological_sections.size());

		for (const feature_geometry_group_type &feature_geom_group : grouped_recon_geoms_seq)
		{
			const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref = feature_geom_group.feature_ref;
			if (!feature_ref.is_valid())
			{
				continue;
			}

			// Iterate through the reconstruction geometries of the current feature and write to output.
			for (const GPlatesAppLogic::ReconstructionGeometry *recon_geom : feature_geom_group.recon_geoms)
			{
				auto resolved_section_iter = recon_geom_to_resolved_section_map.find(recon_geom);
				if (resolved_section_iter != recon_geom_to_resolved_section_map.end())
				{
					const GPlatesAppLogic::ResolvedTopologicalSection::non_null_ptr_to_const_type
							const_resolved_section(resolved_section_iter->second);

					sorted_resolved_topological_sections.push_back(
							// Need to pass ResolvedTopologicalSection::non_null_ptr_type to Python...
							GPlatesUtils::const_pointer_cast<GPlatesAppLogic::ResolvedTopologicalSection>(
									const_resolved_section));
				}
			}
		}

		return sorted_resolved_topological_sections;
	}


	GPlatesScribe::TranscribeResult
	TopologicalSnapshot::transcribe_construct_data(
			GPlatesScribe::Scribe &scribe,
			GPlatesScribe::ConstructObject<TopologicalSnapshot> &topological_snapshot)
	{
		if (scribe.is_saving())
		{
			save_construct_data(scribe, topological_snapshot.get_object());
		}
		else // loading
		{
			GPlatesScribe::LoadRef<RotationModel::non_null_ptr_type> rotation_model;
			std::vector<GPlatesFileIO::File::non_null_ptr_type> topological_files;
			std::vector<boost::optional<ResolveTopologyParameters::non_null_ptr_to_const_type>> resolve_topology_parameters;
			GPlatesScribe::LoadRef<ResolveTopologyParameters::non_null_ptr_to_const_type> default_resolve_topology_parameters;
			double reconstruction_time;
			if (!load_construct_data(
					scribe,
					rotation_model,
					topological_files,
					resolve_topology_parameters,
					default_resolve_topology_parameters,
					reconstruction_time))
			{
				return scribe.get_transcribe_result();
			}

			// Create the topological model.
			topological_snapshot.construct_object(
					rotation_model,
					topological_files,
					resolve_topology_parameters,
					default_resolve_topology_parameters,
					reconstruction_time);
		}

		return GPlatesScribe::TRANSCRIBE_SUCCESS;
	}


	GPlatesScribe::TranscribeResult
	TopologicalSnapshot::transcribe(
			GPlatesScribe::Scribe &scribe,
			bool transcribed_construct_data)
	{
		if (!transcribed_construct_data)
		{
			if (scribe.is_saving())
			{
				save_construct_data(scribe, *this);
			}
			else // loading
			{
				GPlatesScribe::LoadRef<RotationModel::non_null_ptr_type> rotation_model;
				GPlatesScribe::LoadRef<ResolveTopologyParameters::non_null_ptr_to_const_type> default_resolve_topology_parameters;
				if (!load_construct_data(
						scribe,
						rotation_model,
						d_topological_files,
						d_resolve_topology_parameters,
						default_resolve_topology_parameters,
						d_reconstruction_time))
				{
					return scribe.get_transcribe_result();
				}
				d_rotation_model = rotation_model.get();
				d_default_resolve_topology_parameters = default_resolve_topology_parameters.get();

				// Initialise resolved topologies (based on the construct parameters we just loaded).
				//
				// Note: The existing resolved topologies in 'this' topological snapshot must be old data
				//       because 'transcribed_construct_data' is false (ie, it was not transcribed) and so 'this'
				//       object must've been created first (using unknown constructor arguments) and *then* transcribed.
				initialise_resolved_topologies();
			}
		}

		return GPlatesScribe::TRANSCRIBE_SUCCESS;
	}


	void
	TopologicalSnapshot::save_construct_data(
			GPlatesScribe::Scribe &scribe,
			const TopologicalSnapshot &topological_snapshot)
	{
		// Save the rotation model.
		scribe.save(TRANSCRIBE_SOURCE, topological_snapshot.d_rotation_model, "rotation_model");

		const GPlatesScribe::ObjectTag files_tag("files");

		// Save number of topological files.
		const unsigned int num_files = topological_snapshot.d_topological_files.size();
		scribe.save(TRANSCRIBE_SOURCE, num_files, files_tag.sequence_size());

		// Save the topological files (feature collections and their filenames).
		for (unsigned int file_index = 0; file_index < num_files; ++file_index)
		{
			const auto feature_collection_file = topological_snapshot.d_topological_files[file_index];

			const GPlatesModel::FeatureCollectionHandle::non_null_ptr_type feature_collection(
					feature_collection_file->get_reference().get_feature_collection().handle_ptr());
			const QString filename =
					feature_collection_file->get_reference().get_file_info().get_qfileinfo().absoluteFilePath();

			scribe.save(TRANSCRIBE_SOURCE, feature_collection, files_tag[file_index]("feature_collection"));
			scribe.save(TRANSCRIBE_SOURCE, filename, files_tag[file_index]("filename"));
		}

		// Save the resolved topology parameters.
		scribe.save(TRANSCRIBE_SOURCE, topological_snapshot.d_resolve_topology_parameters, "resolve_topology_parameters");
		scribe.save(TRANSCRIBE_SOURCE, topological_snapshot.d_default_resolve_topology_parameters, "default_resolve_topology_parameters");

		// Save the reconstruction time.
		scribe.save(TRANSCRIBE_SOURCE, topological_snapshot.d_reconstruction_time, "reconstruction_time");
	}


	bool
	TopologicalSnapshot::load_construct_data(
			GPlatesScribe::Scribe &scribe,
			GPlatesScribe::LoadRef<RotationModel::non_null_ptr_type> &rotation_model,
			std::vector<GPlatesFileIO::File::non_null_ptr_type> &topological_files,
			const std::vector<boost::optional<ResolveTopologyParameters::non_null_ptr_to_const_type>> &resolve_topology_parameters,
			GPlatesScribe::LoadRef<ResolveTopologyParameters::non_null_ptr_to_const_type> &default_resolve_topology_parameters,
			double &reconstruction_time)
	{
		// Load the rotation model.
		rotation_model = scribe.load<RotationModel::non_null_ptr_type>(TRANSCRIBE_SOURCE, "rotation_model");
		if (!rotation_model.is_valid())
		{
			return false;
		}

		const GPlatesScribe::ObjectTag files_tag("files");

		// Number of topological files.
		unsigned int num_files;
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, num_files, files_tag.sequence_size()))
		{
			return false;
		}

		// Load the topological files (feature collections and their filenames).
		for (unsigned int file_index = 0; file_index < num_files; ++file_index)
		{
			GPlatesScribe::LoadRef<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type> feature_collection =
					scribe.load<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type>(
							TRANSCRIBE_SOURCE,
							files_tag[file_index]("feature_collection"));
			if (!feature_collection.is_valid())
			{
				return false;
			}

			QString filename;
			if (!scribe.transcribe(TRANSCRIBE_SOURCE, filename, files_tag[file_index]("filename")))
			{
				return false;
			}

			topological_files.push_back(
					GPlatesFileIO::File::create_file(GPlatesFileIO::FileInfo(filename), feature_collection));
		}

		// Load the resolved topology parameters.
		//
		// Note: These must be loaded in the same order they were saved (see 'save_construct_data':
		//       per-file 'resolve_topology_parameters' first, then 'default_resolve_topology_parameters').
		//       The order matters for the "raw lane" (used by pickling) which is positional - there
		//       are no per-object tags to look up, so the load order must mirror the save order.
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, resolve_topology_parameters, "resolve_topology_parameters"))
		{
			return false;
		}
		default_resolve_topology_parameters = scribe.load<ResolveTopologyParameters::non_null_ptr_to_const_type>(
				TRANSCRIBE_SOURCE, "default_resolve_topology_parameters");
		if (!default_resolve_topology_parameters.is_valid())
		{
			return false;
		}

		// Load the reconstruction time.
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, reconstruction_time, "reconstruction_time"))
		{
			return false;
		}

		return true;
	}
}

	
void
export_topological_snapshot()
{
	// An enumeration nested within 'pygplates' (ie, current) module.
	bp::enum_<GPlatesApi::ResolveTopologyType::Value>("ResolveTopologyType")
			.value("line", GPlatesApi::ResolveTopologyType::LINE)
			.value("boundary", GPlatesApi::ResolveTopologyType::BOUNDARY)
			.value("network", GPlatesApi::ResolveTopologyType::NETWORK);


	//
	// PlateBoundaryStatistic - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<GPlatesAppLogic::PlateBoundaryStat>(
					"PlateBoundaryStatistic",
					"Statistics at a point *on* a plate boundary.\n"
					"\n"
					"  .. seealso:: :ref:`pygplates_primer_plate_boundary_statistics` in the *Primer* documentation.\n"
					"\n"
					"PlateBoundaryStatistics are equality (``==``, ``!=``) comparable (but not hashable - cannot be used as a key in a ``dict``).\n"
					"\n"
					".. versionadded:: 0.47\n",
					bp::no_init)
		.add_property("shared_sub_segment",
				&GPlatesAppLogic::PlateBoundaryStat::get_shared_sub_segment,
				"Shared sub-segment containing the :attr:`boundary point <boundary_point>`.\n"
				"\n"
				"  :type: ResolvedTopologicalSharedSubSegment\n"
				"\n"
				"  .. note:: Another way to get the shared sub-segment is to call :meth:`TopologicalSnapshot.calculate_plate_boundary_statistics` "
				"with ``return_shared_sub_segment_dict=True`` (which associates each shared sub-segment with a list of boundary point statistics).\n"
				"\n"
				"  .. seealso:: :attr:`boundary_feature`\n"
				"\n"
				".. versionadded:: 1.0\n")
		.add_property("boundary_feature",
				&GPlatesApi::plate_boundary_statistic_get_boundary_feature,
				"Boundary feature associated with the :attr:`boundary point <boundary_point>`.\n"
				"\n"
				"  :type: Feature\n"
				"\n"
				"  If the :attr:`shared sub-segment <shared_sub_segment>` containing the :attr:`boundary point <boundary_point>` "
				"is from a :class:`ReconstructedFeatureGeometry` then the returned feature matches the shared sub-segment's feature. "
				"However, if the shared sub-segment is from a :class:`ResolvedTopologicalLine` then the returned feature matches one of "
				"the resolved topological line's :meth:`sub-segments <ResolvedTopologicalLine.get_line_sub_segments>` (the one containing the boundary point).\n"
				"\n"
				"  .. note:: An example where this is useful is along a deforming trench line when you need to know the reconstruction plate ID "
				"associated with the sub-segment of the trench line that the :attr:`boundary point <boundary_point>` is on, since that plate ID "
				"more accurately represents motion of the trench near the boundary point.\n"
				"\n"
				"  .. seealso:: :attr:`shared_sub_segment`\n"
				"\n"
				".. versionadded:: 1.0\n")
		.add_property("boundary_point",
				bp::make_function(&GPlatesAppLogic::PlateBoundaryStat::get_boundary_point, bp::return_value_policy<bp::copy_const_reference>()),
				"Position of the point on a plate boundary.\n"
				"\n"
				"  :type: PointOnSphere\n"
				"\n"
				"  .. seealso:: :attr:`boundary_length` and :attr:`boundary_normal`\n")
		.add_property("boundary_length",
				&GPlatesAppLogic::PlateBoundaryStat::get_boundary_length,
				"Length (in radians) subtended on the plate boundary (at the :attr:`boundary point <boundary_point>`).\n"
				"\n"
				"  :type: float\n"
				"\n"
				"  .. seealso:: :attr:`boundary_point` and :attr:`boundary_normal`\n")
		.add_property("boundary_normal",
				&GPlatesApi::plate_boundary_statistic_get_boundary_normal,
				"Normal to the plate boundary (at the :attr:`boundary point <boundary_point>`).\n"
				"\n"
				"  :type: Vector3D\n"
				"\n"
				"  .. note:: This is the unit-length normal of the :class:`great circle arc <GreatCircleArc>` segment (that the :attr:`boundary point <boundary_point>` is located on). "
				"And, as such, the normal is to the *left* of the segment (when following the vertices of the "
				":class:`shared sub-segment <ResolvedTopologicalSharedSubSegment>` that the :attr:`boundary point <boundary_point>` is located on).\n"
				"\n"
				"  .. seealso:: :attr:`boundary_point` and :attr:`boundary_length`\n")
		.add_property("boundary_normal_azimuth",
				&GPlatesAppLogic::PlateBoundaryStat::get_boundary_normal_azimuth,
				"Clockwise (East-wise) angle in radians (in the range :math:`[0, 2\\pi]`) from North to the :attr:`plate boundary normal <boundary_normal>` "
				"(at the :attr:`boundary point <boundary_point>`).\n"
				"\n"
				"  :type: float\n"
				"\n"
				"  This is the equivalent of:\n"
				"  ::\n"
				"\n"
				"    local_cartesian = pygplates.LocalCartesian(plate_boundary_stat.boundary_point)\n"
				"    _, azimuth, _ = local_cartesian.from_geocentric_to_magnitude_azimuth_inclination(plate_boundary_stat.boundary_normal)\n"
				"\n"
				"  .. seealso:: :attr:`boundary_normal`\n")
		.add_property("boundary_velocity",
				bp::make_function(&GPlatesAppLogic::PlateBoundaryStat::get_boundary_velocity, bp::return_value_policy<bp::copy_const_reference>()),
				"Velocity vector of the plate boundary (at the :attr:`boundary point <boundary_point>`).\n"
				"\n"
				"  :type: Vector3D\n"
				"\n"
				"  .. note:: The velocity units are determined by the call to :meth:`TopologicalSnapshot.calculate_plate_boundary_statistics`.\n"
				"\n"
				"  This is the velocity of the plate boundary itself. In other words, the velocity of the :class:`topological section <ResolvedTopologicalSection>` "
				"that contributes to the plate boundary at the :attr:`boundary point <boundary_point>`.\n"
				"\n"
				"  .. seealso:: :attr:`left_plate_velocity` and :attr:`right_plate_velocity`\n")
		.add_property("boundary_velocity_magnitude",
				&GPlatesAppLogic::PlateBoundaryStat::get_boundary_velocity_magnitude,
				"Magnitude of velocity vector of the plate boundary (at the :attr:`boundary point <boundary_point>`).\n"
				"\n"
				"  :type: float\n"
				"\n"
				"  This is the equivalent of:\n"
				"  ::\n"
				"\n"
				"    plate_boundary_stat.boundary_velocity.get_magnitude()\n"
				"\n"
				"  .. note:: The velocity units are determined by the call to :meth:`TopologicalSnapshot.calculate_plate_boundary_statistics`.\n"
				"\n"
				"  .. seealso:: :attr:`boundary_velocity`\n")
		.add_property("boundary_velocity_obliquity",
				&GPlatesAppLogic::PlateBoundaryStat::get_boundary_velocity_obliquity,
				"Obliquity (in radians) of velocity vector of the plate boundary (at the :attr:`boundary point <boundary_point>`).\n"
				"\n"
				"  :type: float\n"
				"\n"
				"  .. note:: Returns zero if the :attr:`boundary velocity magnitude <boundary_velocity_magnitude>` is zero.\n"
				"\n"
				"  This is the angle of the :attr:`boundary velocity vector <boundary_velocity>` relative to the :attr:`boundary normal <boundary_normal>`. "
				"It is in the range :math:`[-\\pi, \\pi]` with positive values representing clockwise angles (and negative representing counter-clockwise).\n"
				"\n"
				"  Since the :attr:`boundary normal <boundary_normal>` is to the *left*, an obliquity angle satisfying :math:`\\lvert obliquity \\rvert < \\frac{\\pi}{2}` "
				"represents movement towards the *left* plate and an angle satisfying :math:`\\lvert obliquity \\rvert > \\frac{\\pi}{2}` represents movement towards *right* plate.\n"
				"\n"
				"  .. seealso:: :attr:`boundary_velocity`\n")
		.add_property("boundary_velocity_orthogonal",
				&GPlatesAppLogic::PlateBoundaryStat::get_boundary_velocity_orthogonal,
				"Orthogonal component (in direction of boundary normal) of velocity vector of the plate boundary (at the :attr:`boundary point <boundary_point>`).\n"
				"\n"
				"  :type: float\n"
				"\n"
				"  This is the equivalent of:\n"
				"  ::\n"
				"\n"
				"    boundary_velocity_orthogonal = (plate_boundary_stat.boundary_velocity_magnitude *\n"
				"                                    math.cos(plate_boundary_stat.boundary_velocity_obliquity))\n"
				"\n"
				"  .. note:: The velocity units are determined by the call to :meth:`TopologicalSnapshot.calculate_plate_boundary_statistics`.\n"
				"\n"
				"  .. seealso:: :attr:`boundary_velocity`\n")
		.add_property("boundary_velocity_parallel",
				&GPlatesAppLogic::PlateBoundaryStat::get_boundary_velocity_parallel,
				"Parallel component (in direction along boundary line) of velocity vector of the plate boundary (at the :attr:`boundary point <boundary_point>`).\n"
				"\n"
				"  :type: float\n"
				"\n"
				"  This is the equivalent of:\n"
				"  ::\n"
				"\n"
				"    boundary_velocity_parallel = (plate_boundary_stat.boundary_velocity_magnitude *\n"
				"                                  math.sin(plate_boundary_stat.boundary_velocity_obliquity))\n"
				"\n"
				"  .. note:: The velocity units are determined by the call to :meth:`TopologicalSnapshot.calculate_plate_boundary_statistics`.\n"
				"\n"
				"  .. seealso:: :attr:`boundary_velocity`\n")
		.add_property("left_plate",
				bp::make_function(&GPlatesAppLogic::PlateBoundaryStat::get_left_plate, bp::return_value_policy<bp::copy_const_reference>()),
				"The left plate (at the :attr:`boundary point <boundary_point>`).\n"
				"\n"
				"  :type: TopologyPointLocation\n"
				"\n"
				"  .. note:: :meth:`TopologyPointLocation.not_located_in_resolved_topology` will return ``True`` if there is no plate (or network) to the left "
				"(when following the vertices of the :class:`shared sub-segment <ResolvedTopologicalSharedSubSegment>` that the "
				":attr:`boundary point <boundary_point>` is located on).\n"
				"\n"
				"  To get the polygon boundary of the left resolved topological :class:`plate <ResolvedTopologicalBoundary>` or "
				":class:`network <ResolvedTopologicalNetwork>` (or ``None`` if neither):\n"
				"  ::\n"
				"\n"
				"    left_plate = plate_boundary_stat.left_plate\n"
				"    if left_plate.located_in_resolved_boundary():\n"
				"        left_topology_boundary = left_plate.located_in_resolved_boundary().get_resolved_boundary()\n"
				"    elif left_plate.located_in_resolved_network():\n"
				"        left_topology_boundary = left_plate.located_in_resolved_network().get_resolved_boundary()\n"
				"    else:\n"
				"        left_topology_boundary = None\n"
				"\n"
				"  If both a left :class:`plate <ResolvedTopologicalBoundary>` and a left :class:`network <ResolvedTopologicalNetwork>` share the plate boundary "
				"(at the :attr:`boundary point <boundary_point>`) then the :class:`network <ResolvedTopologicalNetwork>` is returned. This is because a network "
				"typically overlays its underlying plate. The same applies if there are *multiple* left plates and a single overlayed left network. However, if there are "
				"multiple overlaying left networks then it is undefined which network is returned (the topological model was likely constructed incorrectly in this case). "
				"Furthermore, if a left plate shares the plate boundary but an overlaying network does not (eg, the network crosses the plate boundary rather than sharing "
				"a boundary with it) then the left plate is returned (the network is not discovered in this case).\n"
				"\n"
				"  .. seealso:: :attr:`right_plate`\n")
		.add_property("right_plate",
				bp::make_function(&GPlatesAppLogic::PlateBoundaryStat::get_right_plate, bp::return_value_policy<bp::copy_const_reference>()),
				"The right plate (at the :attr:`boundary point <boundary_point>`).\n"
				"\n"
				"  :type: TopologyPointLocation\n"
				"\n"
				"  .. note:: :meth:`TopologyPointLocation.not_located_in_resolved_topology` will return ``True`` if there is no plate (or network) to the right "
				"(when following the vertices of the :class:`shared sub-segment <ResolvedTopologicalSharedSubSegment>` that the "
				":attr:`boundary point <boundary_point>` is located on).\n"
				"\n"
				"  To get the polygon boundary of the right resolved topological :class:`plate <ResolvedTopologicalBoundary>` or "
				":class:`network <ResolvedTopologicalNetwork>` (or ``None`` if neither):\n"
				"  ::\n"
				"\n"
				"    right_plate = plate_boundary_stat.right_plate\n"
				"    if right_plate.located_in_resolved_boundary():\n"
				"        right_topology_boundary = right_plate.located_in_resolved_boundary().get_resolved_boundary()\n"
				"    elif right_plate.located_in_resolved_network():\n"
				"        right_topology_boundary = right_plate.located_in_resolved_network().get_resolved_boundary()\n"
				"    else:\n"
				"        right_topology_boundary = None\n"
				"\n"
				"  If both a right :class:`plate <ResolvedTopologicalBoundary>` and a right :class:`network <ResolvedTopologicalNetwork>` share the plate boundary "
				"(at the :attr:`boundary point <boundary_point>`) then the :class:`network <ResolvedTopologicalNetwork>` is returned. This is because a network "
				"typically overlays its underlying plate. The same applies if there are *multiple* right plates and a single overlayed right network. However, if there are "
				"multiple overlaying right networks then it is undefined which network is returned (the topological model was likely constructed incorrectly in this case). "
				"Furthermore, if a right plate shares the plate boundary but an overlaying network does not (eg, the network crosses the plate boundary rather than sharing "
				"a boundary with it) then the right plate is returned (the network is not discovered in this case).\n"
				"\n"
				"  .. seealso:: :attr:`left_plate`\n")
		.add_property("left_plate_velocity",
				bp::make_function(&GPlatesAppLogic::PlateBoundaryStat::get_left_plate_velocity, bp::return_value_policy<bp::copy_const_reference>()),
				"Velocity vector of the left plate (at the :attr:`boundary point <boundary_point>`).\n"
				"\n"
				"  :type: Vector3D or None\n"
				"\n"
				"  Returns ``None`` if there is no plate (or network) to the left (when following the vertices of the "
				":class:`shared sub-segment <ResolvedTopologicalSharedSubSegment>` that the :attr:`boundary point <boundary_point>` is located on). "
				"See :attr:`left_plate` for details on how the left plate is determined.\n"
				"\n"
				"  .. note:: The velocity units are determined by the call to :meth:`TopologicalSnapshot.calculate_plate_boundary_statistics`.\n"
				"\n"
				"  .. seealso:: :attr:`right_plate_velocity` and :attr:`boundary_velocity`\n")
		.add_property("left_plate_velocity_magnitude",
				&GPlatesAppLogic::PlateBoundaryStat::get_left_plate_velocity_magnitude,
				"Magnitude of velocity vector of the left plate (at the :attr:`boundary point <boundary_point>`).\n"
				"\n"
				"  :type: float\n"
				"\n"
				"  .. note:: Returns ``float('nan')`` if there is no plate (or network) to the left (when following the vertices of the "
				":class:`shared sub-segment <ResolvedTopologicalSharedSubSegment>` that the :attr:`boundary point <boundary_point>` is located on).\n"
				"\n"
				"  This is the equivalent of:\n"
				"  ::\n"
				"\n"
				"    if plate_boundary_stat.left_plate_velocity:\n"
				"        left_plate_velocity_magnitude = plate_boundary_stat.left_plate_velocity.get_magnitude()\n"
				"    else:\n"
				"        left_plate_velocity_magnitude = float('nan')\n"
				"\n"
				"  .. note:: The velocity units are determined by the call to :meth:`TopologicalSnapshot.calculate_plate_boundary_statistics`.\n"
				"\n"
				"  .. seealso:: :attr:`left_plate_velocity`\n")
		.add_property("left_plate_velocity_obliquity",
				&GPlatesAppLogic::PlateBoundaryStat::get_left_plate_velocity_obliquity,
				"Obliquity (in radians) of velocity vector of the left plate (at the :attr:`boundary point <boundary_point>`).\n"
				"\n"
				"  :type: float\n"
				"\n"
				"  .. note:: Returns ``float('nan')`` if there is no plate (or network) to the left (when following the vertices of the "
				":class:`shared sub-segment <ResolvedTopologicalSharedSubSegment>` that the :attr:`boundary point <boundary_point>` is located on).\n"
				"\n"
				"  .. note:: Returns zero if the :attr:`left plate velocity magnitude <left_plate_velocity_magnitude>` is zero.\n"
				"\n"
				"  This is the angle of the :attr:`left plate velocity vector <left_plate_velocity>` relative to the :attr:`boundary normal <boundary_normal>`. "
				"It is in the range :math:`[-\\pi, \\pi]` with positive values representing clockwise angles (and negative representing counter-clockwise).\n"
				"\n"
				"  Since the :attr:`boundary normal <boundary_normal>` is to the *left*, an obliquity angle satisfying :math:`\\lvert obliquity \\rvert < \\frac{\\pi}{2}` "
				"represents movement of the left plate *away* from the boundary and an angle satisfying :math:`\\lvert obliquity \\rvert > \\frac{\\pi}{2}` represents "
				"movement *towards* the boundary.\n"
				"\n"
				"  .. seealso:: :attr:`left_plate_velocity`\n")
		.add_property("left_plate_velocity_orthogonal",
				&GPlatesAppLogic::PlateBoundaryStat::get_left_plate_velocity_orthogonal,
				"Orthogonal component (in direction of boundary normal) of velocity vector of the left plate (at the :attr:`boundary point <boundary_point>`).\n"
				"\n"
				"  :type: float\n"
				"\n"
				"  .. note:: Returns ``float('nan')`` if there is no plate (or network) to the left (when following the vertices of the "
				":class:`shared sub-segment <ResolvedTopologicalSharedSubSegment>` that the :attr:`boundary point <boundary_point>` is located on).\n"
				"\n"
				"  This is the equivalent of:\n"
				"  ::\n"
				"\n"
				"    if plate_boundary_stat.left_plate_velocity:\n"
				"        left_plate_velocity_orthogonal = (plate_boundary_stat.left_plate_velocity_magnitude *\n"
				"                                          math.cos(plate_boundary_stat.left_plate_velocity_obliquity))\n"
				"    else:\n"
				"        left_plate_velocity_orthogonal = float('nan')\n"
				"\n"
				"  .. note:: The velocity units are determined by the call to :meth:`TopologicalSnapshot.calculate_plate_boundary_statistics`.\n"
				"\n"
				"  .. seealso:: :attr:`left_plate_velocity`\n")
		.add_property("left_plate_velocity_parallel",
				&GPlatesAppLogic::PlateBoundaryStat::get_left_plate_velocity_parallel,
				"Parallel component (in direction along boundary line) of velocity vector of the left plate (at the :attr:`boundary point <boundary_point>`).\n"
				"\n"
				"  :type: float\n"
				"\n"
				"  .. note:: Returns ``float('nan')`` if there is no plate (or network) to the left (when following the vertices of the "
				":class:`shared sub-segment <ResolvedTopologicalSharedSubSegment>` that the :attr:`boundary point <boundary_point>` is located on).\n"
				"\n"
				"  This is the equivalent of:\n"
				"  ::\n"
				"\n"
				"    if plate_boundary_stat.left_plate_velocity:\n"
				"        left_plate_velocity_parallel = (plate_boundary_stat.left_plate_velocity_magnitude *\n"
				"                                        math.sin(plate_boundary_stat.left_plate_velocity_obliquity))\n"
				"    else:\n"
				"        left_plate_velocity_parallel = float('nan')\n"
				"\n"
				"  .. note:: The velocity units are determined by the call to :meth:`TopologicalSnapshot.calculate_plate_boundary_statistics`.\n"
				"\n"
				"  .. seealso:: :attr:`left_plate_velocity`\n")
		.add_property("right_plate_velocity",
				bp::make_function(&GPlatesAppLogic::PlateBoundaryStat::get_right_plate_velocity, bp::return_value_policy<bp::copy_const_reference>()),
				"Velocity vector of the right plate (at the :attr:`boundary point <boundary_point>`).\n"
				"\n"
				"  :type: Vector3D or None\n"
				"\n"
				"  Returns ``None`` if there is no plate (or network) to the right (when following the vertices of the "
				":class:`shared sub-segment <ResolvedTopologicalSharedSubSegment>` that the :attr:`boundary point <boundary_point>` is located on). "
				"See :attr:`right_plate` for details on how the right plate is determined.\n"
				"\n"
				"  .. note:: The velocity units are determined by the call to :meth:`TopologicalSnapshot.calculate_plate_boundary_statistics`.\n"
				"\n"
				"  .. seealso:: :attr:`left_plate_velocity` and :attr:`boundary_velocity`\n")
		.add_property("right_plate_velocity_magnitude",
				&GPlatesAppLogic::PlateBoundaryStat::get_right_plate_velocity_magnitude,
				"Magnitude of velocity vector of the right plate (at the :attr:`boundary point <boundary_point>`).\n"
				"\n"
				"  :type: float\n"
				"\n"
				"  .. note:: Returns ``float('nan')`` if there is no plate (or network) to the right (when following the vertices of the "
				":class:`shared sub-segment <ResolvedTopologicalSharedSubSegment>` that the :attr:`boundary point <boundary_point>` is located on).\n"
				"\n"
				"  This is the equivalent of:\n"
				"  ::\n"
				"\n"
				"    if plate_boundary_stat.right_plate_velocity:\n"
				"        right_plate_velocity_magnitude = plate_boundary_stat.right_plate_velocity.get_magnitude()\n"
				"    else:\n"
				"        right_plate_velocity_magnitude = float('nan')\n"
				"\n"
				"  .. note:: The velocity units are determined by the call to :meth:`TopologicalSnapshot.calculate_plate_boundary_statistics`.\n"
				"\n"
				"  .. seealso:: :attr:`right_plate_velocity`\n")
		.add_property("right_plate_velocity_obliquity",
				&GPlatesAppLogic::PlateBoundaryStat::get_right_plate_velocity_obliquity,
				"Obliquity (in radians) of velocity vector of the right plate (at the :attr:`boundary point <boundary_point>`).\n"
				"\n"
				"  :type: float\n"
				"\n"
				"  .. note:: Returns ``float('nan')`` if there is no plate (or network) to the right (when following the vertices of the "
				":class:`shared sub-segment <ResolvedTopologicalSharedSubSegment>` that the :attr:`boundary point <boundary_point>` is located on).\n"
				"\n"
				"  .. note:: Returns zero if the :attr:`right plate velocity magnitude <right_plate_velocity_magnitude>` is zero.\n"
				"\n"
				"  This is the angle of the :attr:`right plate velocity vector <right_plate_velocity>` relative to the :attr:`boundary normal <boundary_normal>`. "
				"It is in the range :math:`[-\\pi, \\pi]` with positive values representing clockwise angles (and negative representing counter-clockwise).\n"
				"\n"
				"  Since the :attr:`boundary normal <boundary_normal>` is to the *left*, an obliquity angle satisfying :math:`\\lvert obliquity \\rvert < \\frac{\\pi}{2}` "
				"represents movement of the right plate *towards* the boundary and an angle satisfying :math:`\\lvert obliquity \\rvert > \\frac{\\pi}{2}` represents "
				"movement *away* from the boundary.\n"
				"\n"
				"  .. seealso:: :attr:`right_plate_velocity`\n")
		.add_property("right_plate_velocity_orthogonal",
				&GPlatesAppLogic::PlateBoundaryStat::get_right_plate_velocity_orthogonal,
				"Orthogonal component (in direction of boundary normal) of velocity vector of the right plate (at the :attr:`boundary point <boundary_point>`).\n"
				"\n"
				"  :type: float\n"
				"\n"
				"  .. note:: Returns ``float('nan')`` if there is no plate (or network) to the right (when following the vertices of the "
				":class:`shared sub-segment <ResolvedTopologicalSharedSubSegment>` that the :attr:`boundary point <boundary_point>` is located on).\n"
				"\n"
				"  This is the equivalent of:\n"
				"  ::\n"
				"\n"
				"    if plate_boundary_stat.right_plate_velocity:\n"
				"        right_plate_velocity_orthogonal = (plate_boundary_stat.right_plate_velocity_magnitude *\n"
				"                                           math.cos(plate_boundary_stat.right_plate_velocity_obliquity))\n"
				"    else:\n"
				"        right_plate_velocity_orthogonal = float('nan')\n"
				"\n"
				"  .. note:: The velocity units are determined by the call to :meth:`TopologicalSnapshot.calculate_plate_boundary_statistics`.\n"
				"\n"
				"  .. seealso:: :attr:`right_plate_velocity`\n")
		.add_property("right_plate_velocity_parallel",
				&GPlatesAppLogic::PlateBoundaryStat::get_right_plate_velocity_parallel,
				"Parallel component (in direction along boundary line) of velocity vector of the right plate (at the :attr:`boundary point <boundary_point>`).\n"
				"\n"
				"  :type: float\n"
				"\n"
				"  .. note:: Returns ``float('nan')`` if there is no plate (or network) to the right (when following the vertices of the "
				":class:`shared sub-segment <ResolvedTopologicalSharedSubSegment>` that the :attr:`boundary point <boundary_point>` is located on).\n"
				"\n"
				"  This is the equivalent of:\n"
				"  ::\n"
				"\n"
				"    if plate_boundary_stat.right_plate_velocity:\n"
				"        right_plate_velocity_parallel = (plate_boundary_stat.right_plate_velocity_magnitude *\n"
				"                                         math.sin(plate_boundary_stat.right_plate_velocity_obliquity))\n"
				"    else:\n"
				"        right_plate_velocity_parallel = float('nan')\n"
				"\n"
				"  .. note:: The velocity units are determined by the call to :meth:`TopologicalSnapshot.calculate_plate_boundary_statistics`.\n"
				"\n"
				"  .. seealso:: :attr:`right_plate_velocity`\n")
		.add_property("left_plate_strain_rate",
				&GPlatesAppLogic::PlateBoundaryStat::get_left_plate_strain_rate,
				"Strain rate of the left plate (at the :attr:`boundary point <boundary_point>`).\n"
				"\n"
				"  :type: StrainRate\n"
				"\n"
				"  Returns ``pygplates.StrainRate.zero`` (no deformation) if there's no left deforming network (eg, there's just a rigid plate with no "
				"deforming network overlaid on top) or if :attr:`boundary point <boundary_point>` is inside an interior rigid block of the left deforming network. "
				"See :attr:`left_plate` for details on how the left plate is determined.\n"
				"\n"
				"  .. note:: Strain rate in a deforming network is calculated from the spatial gradients of velocity where the velocities are calculated over "
				"a 1 Myr time interval and using the *equatorial* Earth radius :class:`pygplates.Earth.equatorial_radius_in_kms <Earth>`.\n"
				"\n"
				"  .. seealso:: :attr:`left_plate` and :attr:`left_plate_velocity`\n")
		.add_property("right_plate_strain_rate",
				&GPlatesAppLogic::PlateBoundaryStat::get_right_plate_strain_rate,
				"Strain rate of the right plate (at the :attr:`boundary point <boundary_point>`).\n"
				"\n"
				"  :type: StrainRate\n"
				"\n"
				"  Returns ``pygplates.StrainRate.zero`` (no deformation) if there's no right deforming network (eg, there's just a rigid plate with no "
				"deforming network overlaid on top) or if :attr:`boundary point <boundary_point>` is inside an interior rigid block of the right deforming network. "
				"See :attr:`right_plate` for details on how the right plate is determined.\n"
				"\n"
				"  .. note:: Strain rate in a deforming network is calculated from the spatial gradients of velocity where the velocities are calculated over "
				"a 1 Myr time interval and using the *equatorial* Earth radius :class:`pygplates.Earth.equatorial_radius_in_kms <Earth>`.\n"
				"\n"
				"  .. seealso:: :attr:`right_plate` and :attr:`right_plate_velocity`\n")
		.add_property("convergence_velocity",
				&GPlatesAppLogic::PlateBoundaryStat::get_convergence_velocity,
				"Convergence velocity vector (at the :attr:`boundary point <boundary_point>`).\n"
				"\n"
				"  :type: Vector3D or None\n"
				"\n"
				"  This is the velocity of the *right* plate relative to the *left* plate.\n"
				"\n"
				"  .. note:: Returns ``None`` if there is no plate (or network) on the left or no plate (or network) on the right (when following the vertices of the "
				":class:`shared sub-segment <ResolvedTopologicalSharedSubSegment>` that the :attr:`boundary point <boundary_point>` is located on).\n"
				"\n"
				"  .. note:: The velocity units are determined by the call to :meth:`TopologicalSnapshot.calculate_plate_boundary_statistics`.\n"
				"\n"
				"  .. seealso:: :attr:`left_plate_velocity` and :attr:`right_plate_velocity`\n")
		.add_property("convergence_velocity_magnitude",
				&GPlatesApi::plate_boundary_statistic_get_convergence_velocity_magnitude,
				"Magnitude of convergence velocity vector (at the :attr:`boundary point <boundary_point>`).\n"
				"\n"
				"  :type: float\n"
				"\n"
				"  .. note:: Returns ``float('nan')`` if there is no plate (or network) on the left or no plate (or network) on the right (when following the vertices of the "
				":class:`shared sub-segment <ResolvedTopologicalSharedSubSegment>` that the :attr:`boundary point <boundary_point>` is located on).\n"
				"\n"
				"  .. note:: Returns zero if the :attr:`convergence velocity <convergence_velocity>` has :meth:`zero magnitude <Vector3D.is_zero_magnitude>`.\n"
				"\n"
				"  The magnitude is always positive (or zero or ``float('nan')``).\n"
				"\n"
				"  This is the equivalent of:\n"
				"  ::\n"
				"\n"
				"    if plate_boundary_stat.convergence_velocity:\n"
				"        if plate_boundary_stat.convergence_velocity.is_zero_magnitude():\n"
				"            convergence_velocity_magnitude = 0.0\n"
				"        else:\n"
				"            convergence_velocity_magnitude = plate_boundary_stat.convergence_velocity.get_magnitude()\n"
				"    else:\n"
				"        convergence_velocity_magnitude = float('nan')\n"
				"\n"
				"  .. note:: The velocity units are determined by the call to :meth:`TopologicalSnapshot.calculate_plate_boundary_statistics`.\n"
				"\n"
				"  .. seealso:: :attr:`convergence_velocity_signed_magnitude` and :attr:`convergence_velocity`\n")
		.add_property("convergence_velocity_signed_magnitude",
				&GPlatesApi::plate_boundary_statistic_get_convergence_velocity_signed_magnitude,
				"Signed magnitude of convergence velocity vector (at the :attr:`boundary point <boundary_point>`).\n"
				"\n"
				"  :type: float\n"
				"\n"
				"  .. note:: Returns ``float('nan')`` if there is no plate (or network) on the left or no plate (or network) on the right (when following the vertices of the "
				":class:`shared sub-segment <ResolvedTopologicalSharedSubSegment>` that the :attr:`boundary point <boundary_point>` is located on).\n"
				"\n"
				"  .. note:: Returns zero if the :attr:`convergence velocity <convergence_velocity>` has :meth:`zero magnitude <Vector3D.is_zero_magnitude>`.\n"
				"\n"
				"  The *signed* magnitude is positive if the plates are *converging* and negative if they're *diverging*. Otherwise it's zero or ``float('nan')``.\n"
				"\n"
				"  This is the equivalent of:\n"
				"  ::\n"
				"\n"
				"    convergence_velocity_signed_magnitude = plate_boundary_stat.convergence_velocity_magnitude\n"
				"    if (not math.isnan(convergence_velocity_signed_magnitude) and\n"
				"        abs(plate_boundary_stat.convergence_obliquity) > math.pi/2):\n"
				"        convergence_velocity_signed_magnitude = -convergence_velocity_signed_magnitude\n"
				"\n"
				"  .. note:: The velocity units are determined by the call to :meth:`TopologicalSnapshot.calculate_plate_boundary_statistics`.\n"
				"\n"
				"  .. seealso:: :attr:`convergence_velocity_magnitude` and :attr:`convergence_velocity`\n")
		.add_property("convergence_velocity_obliquity",
				&GPlatesAppLogic::PlateBoundaryStat::get_convergence_velocity_obliquity,
				"Obliquity (in radians) of the convergence velocity vector (at the :attr:`boundary point <boundary_point>`).\n"
				"\n"
				"  :type: float\n"
				"\n"
				"  .. note:: Returns ``float('nan')`` if there is no plate (or network) on the left or no plate (or network) on the right (when following the vertices of the "
				":class:`shared sub-segment <ResolvedTopologicalSharedSubSegment>` that the :attr:`boundary point <boundary_point>` is located on).\n"
				"\n"
				"  .. note:: Returns zero if the :attr:`convergence velocity magnitude <convergence_velocity_magnitude>` is zero.\n"
				"\n"
				"  This is the angle of the :attr:`convergence velocity vector <convergence_velocity>` relative to the :attr:`boundary normal <boundary_normal>`. "
				"It is in the range :math:`[-\\pi, \\pi]` with positive values representing clockwise angles (and negative representing counter-clockwise).\n"
				"\n"
				"  Since the :attr:`boundary normal <boundary_normal>` is to the *left* and the :attr:`convergence velocity <convergence_velocity>` is the "
				"velocity of the *right* plate relative to the *left* plate, an obliquity angle satisfying :math:`\\lvert obliquity \\rvert < \\frac{\\pi}{2}` "
				"represents *convergence* and an angle satisfying :math:`\\lvert obliquity \\rvert > \\frac{\\pi}{2}` represents *divergence*.\n"
				"\n"
				"  .. seealso:: :attr:`convergence_velocity`\n")
		.add_property("convergence_velocity_orthogonal",
				&GPlatesAppLogic::PlateBoundaryStat::get_convergence_velocity_orthogonal,
				"Orthogonal component (in direction of boundary normal) of convergence velocity vector (at the v).\n"
				"\n"
				"  :type: float\n"
				"\n"
				"  .. note:: Returns ``float('nan')`` if there is no plate (or network) on the left or no plate (or network) on the right (when following the vertices of the "
				":class:`shared sub-segment <ResolvedTopologicalSharedSubSegment>` that the :attr:`boundary point <boundary_point>` is located on).\n"
				"\n"
				"  This is the equivalent of:\n"
				"  ::\n"
				"\n"
				"    if plate_boundary_stat.convergence_velocity:\n"
				"        convergence_velocity_orthogonal = (plate_boundary_stat.convergence_velocity_magnitude *\n"
				"                                           math.cos(plate_boundary_stat.convergence_velocity_obliquity))\n"
				"    else:\n"
				"        convergence_velocity_orthogonal = float('nan')\n"
				"\n"
				"  .. note:: The velocity units are determined by the call to :meth:`TopologicalSnapshot.calculate_plate_boundary_statistics`.\n"
				"\n"
				"  .. seealso:: :attr:`convergence_velocity`\n")
		.add_property("convergence_velocity_parallel",
				&GPlatesAppLogic::PlateBoundaryStat::get_convergence_velocity_parallel,
				"Parallel component (in direction along boundary line) of convergence velocity vector (at the :attr:`boundary point <boundary_point>`).\n"
				"\n"
				"  :type: float\n"
				"\n"
				"  .. note:: Returns ``float('nan')`` if there is no plate (or network) on the left or no plate (or network) on the right (when following the vertices of the "
				":class:`shared sub-segment <ResolvedTopologicalSharedSubSegment>` that the :attr:`boundary point <boundary_point>` is located on).\n"
				"\n"
				"  This is the equivalent of:\n"
				"  ::\n"
				"\n"
				"    if plate_boundary_stat.convergence_velocity:\n"
				"        convergence_velocity_parallel = (plate_boundary_stat.convergence_velocity_magnitude *\n"
				"                                         math.sin(plate_boundary_stat.convergence_velocity_obliquity))\n"
				"    else:\n"
				"        convergence_velocity_parallel = float('nan')\n"
				"\n"
				"  .. note:: The velocity units are determined by the call to :meth:`TopologicalSnapshot.calculate_plate_boundary_statistics`.\n"
				"\n"
				"  .. seealso:: :attr:`convergence_velocity`\n")
		.add_property("distance_from_start_of_shared_sub_segment",
				&GPlatesAppLogic::PlateBoundaryStat::get_distance_from_start_of_shared_sub_segment,
				"Distance (in radians) from the *start* of the shared sub-segment geometry.\n"
				"\n"
				"  :type: float\n"
				"\n"
				"  A :class:`shared sub-segment <ResolvedTopologicalSharedSubSegment>` represents a part of a resolved topological section that *uniquely* contributes "
				"to the boundaries of one or more resolved topologies. So this is the distance from the *start* of that shared part.\n"
				"\n"
				"  .. note:: The shared sub-segment geometry *includes* any rubber banding. So if the shared sub-segment (containing the :attr:`boundary point <boundary_point>`) "
				"is the first shared sub-segment of the topological section, and the start of the topological section has rubber banding, then the *start* of the "
				"shared sub-segment will be halfway along the rubber band (the line segment joining start of topological section with adjacent topological section in a plate boundary).\n"
				"\n"
				"  To find the distance from the :attr:`boundary point <boundary_point>` to the nearest edge (start or end) of the shared sub-segment "
				"(containing the boundary point):\n"
				"  ::\n"
				"\n"
				"    distance_to_nearest_shared_edge_kms = (pygplates.Earth.mean_radius_in_kms *\n"
				"                                           min(plate_boundary_stat.distance_from_start_of_shared_sub_segment,\n"
				"                                               plate_boundary_stat.distance_to_end_of_shared_sub_segment))\n"
				"\n"
				"  .. seealso:: :attr:`distance_from_start_of_topological_section`\n")
		.add_property("distance_to_end_of_shared_sub_segment",
				&GPlatesAppLogic::PlateBoundaryStat::get_distance_to_end_of_shared_sub_segment,
				"Distance (in radians) to the *end* of the shared sub-segment geometry.\n"
				"\n"
				"  :type: float\n"
				"\n"
				"  A :class:`shared sub-segment <ResolvedTopologicalSharedSubSegment>` represents a part of a resolved topological section that *uniquely* contributes "
				"to the boundaries of one or more resolved topologies. So this is the distance to the *end* of that shared part.\n"
				"\n"
				"  .. note:: The shared sub-segment geometry *includes* any rubber banding. So if the shared sub-segment (containing the :attr:`boundary point <boundary_point>`) "
				"is the last shared sub-segment of the topological section, and the end of the topological section has rubber banding, then the *end* of the "
				"shared sub-segment will be halfway along the rubber band (the line segment joining end of topological section with adjacent topological section in a plate boundary).\n"
				"\n"
				"  To find the distance from the :attr:`boundary point <boundary_point>` to the nearest edge (start or end) of the shared sub-segment "
				"(containing the boundary point):\n"
				"  ::\n"
				"\n"
				"    distance_to_nearest_shared_edge_kms = (pygplates.Earth.mean_radius_in_kms *\n"
				"                                           min(plate_boundary_stat.distance_from_start_of_shared_sub_segment,\n"
				"                                               plate_boundary_stat.distance_to_end_of_shared_sub_segment))\n"
				"\n"
				"  .. seealso:: :attr:`distance_to_end_of_topological_section`\n")
		.add_property("signed_distance_from_start_of_topological_section",
				&GPlatesAppLogic::PlateBoundaryStat::get_signed_distance_from_start_of_topological_section,
				"Signed distance (in radians) from the *start* of the resolved topological section geometry.\n"
				"\n"
				"  :type: float\n"
				"\n"
				"  This distance is *signed* because it is negative if the :attr:`boundary point <boundary_point>` is on a rubber-band part of a plate boundary. "
				"That is, it's not on the actual :meth:`resolved topological section geometry <ResolvedTopologicalSection.get_topological_section_geometry>` itself but on the "
				"rubber band that joins the *start* of the resolved topological section geometry  with the start or end of an adjacent resolved topological section (of that plate boundary). "
				"Rubber banding happens when two adjacent topological sections fail to intersect each other. It's usually the result of an error in the creation of the topological model, "
				"but can happen if points (instead of lines) are directly added to  plate boundaries (typically points are first added to :meth:`topological lines <ResolvedTopologicalLine>` "
				"which are then, in turn, added to plate boundaries).\n"
				"\n"
				"  To see if the :attr:`boundary point <boundary_point>` is on a rubber band:\n"
				"  ::\n"
				"\n"
				"    is_on_rubber_band = (plate_boundary_stat.signed_distance_from_start_of_topological_section < 0 ||\n"
				"                         plate_boundary_stat.signed_distance_to_end_of_topological_section < 0)\n"
				"\n"
				"  .. seealso:: :attr:`distance_from_start_of_topological_section`\n")
		.add_property("distance_from_start_of_topological_section",
				&GPlatesAppLogic::PlateBoundaryStat::get_distance_from_start_of_topological_section,
				"Distance (in radians) from the *start* of the resolved topological section geometry.\n"
				"\n"
				"  :type: float\n"
				"\n"
				"  For example, if the topological section is a subduction zone then this could be considered the distance from the start of the trench "
				"(depending on how the topological model is built).\n"
				"\n"
				"  This is **not** necessarily from the start of the *entire* :meth:`topological section geometry <ResolvedTopologicalSection.get_topological_section_geometry>`. "
				"A resolved topological section represents a distinct feature that is used by one or more plates as part of their boundaries. "
				"But typically only the interior part of the topological section polyline actually contributes to plate boundaries "
				"(due to the intersecting adjacent topological sections when resolving the boundary of a plate at a particular reconstruction time). "
				"And there can be more than one of these interior intersected segments for each topological section geometry "
				"(these are its :meth:`shared sub-segments <ResolvedTopologicalSection.get_shared_sub_segments>`). "
				"So, this distance is the distance from the *start* of the first vertex (eg, intersection) of all these shared sub-segments along the topological section geometry.\n"
				"\n"
				"  To find the distance from the :attr:`boundary point <boundary_point>` to the nearest edge (start or end) of the topological section "
				"(containing the boundary point):\n"
				"  ::\n"
				"\n"
				"    distance_to_nearest_edge_kms = (pygplates.Earth.mean_radius_in_kms *\n"
				"                                    min(plate_boundary_stat.distance_from_start_of_topological_section,\n"
				"                                        plate_boundary_stat.distance_to_end_of_topological_section))\n"
				"\n"
				"  .. note:: This is the absolute value of :attr:`signed_distance_from_start_of_topological_section` and hence only differs from it when the "
				":attr:`boundary point <boundary_point>` is on a rubber-band part of a plate boundary (where it'll be positive here and negative there).\n"
				"\n"
				"  .. seealso:: :attr:`distance_from_start_of_shared_sub_segment`\n")
		.add_property("signed_distance_to_end_of_topological_section",
				&GPlatesAppLogic::PlateBoundaryStat::get_signed_distance_to_end_of_topological_section,
				"Signed distance (in radians) to the *end* of the resolved topological section geometry.\n"
				"\n"
				"  :type: float\n"
				"\n"
				"  This distance is *signed* because it is negative if the :attr:`boundary point <boundary_point>` is on a rubber-band part of a plate boundary. "
				"That is, it's not on the actual :meth:`resolved topological section geometry <ResolvedTopologicalSection.get_topological_section_geometry>` itself but on the "
				"rubber band that joins the *end* of the resolved topological section geometry  with the start or end of an adjacent resolved topological section (of that plate boundary). "
				"Rubber banding happens when two adjacent topological sections fail to intersect each other. It's usually the result of an error in the creation of the topological model, "
				"but can happen if points (instead of lines) are directly added to  plate boundaries (typically points are first added to :meth:`topological lines <ResolvedTopologicalLine>` "
				"which are then, in turn, added to plate boundaries).\n"
				"\n"
				"  To see if the :attr:`boundary point <boundary_point>` is on a rubber band:\n"
				"  ::\n"
				"\n"
				"    is_on_rubber_band = (plate_boundary_stat.signed_distance_from_start_of_topological_section < 0 ||\n"
				"                         plate_boundary_stat.signed_distance_to_end_of_topological_section < 0)\n"
				"\n"
				"  .. seealso:: :attr:`distance_to_end_of_topological_section`\n")
		.add_property("distance_to_end_of_topological_section",
				&GPlatesAppLogic::PlateBoundaryStat::get_distance_to_end_of_topological_section,
				"Distance (in radians) to the *end* of the resolved topological section geometry.\n"
				"\n"
				"  :type: float\n"
				"\n"
				"  For example, if the topological section is a subduction zone then this could be considered the distance to the end of the trench "
				"(depending on how the topological model is built).\n"
				"\n"
				"  This is **not** necessarily to the end of the *entire* :meth:`topological section geometry <ResolvedTopologicalSection.get_topological_section_geometry>`. "
				"A resolved topological section represents a distinct feature that is used by one or more plates as part of their boundaries. "
				"But typically only the interior part of the topological section polyline actually contributes to plate boundaries "
				"(due to the intersecting adjacent topological sections when resolving the boundary of a plate at a particular reconstruction time). "
				"And there can be more than one of these interior intersected segments for each topological section geometry "
				"(these are its :meth:`shared sub-segments <ResolvedTopologicalSection.get_shared_sub_segments>`). "
				"So, this distance is the distance to the *end* of the last vertex (eg, intersection) of all these shared sub-segments along the topological section geometry.\n"
				"\n"
				"  To find the distance from the :attr:`boundary point <boundary_point>` to the nearest edge (start or end) of the topological section "
				"(containing the boundary point):\n"
				"  ::\n"
				"\n"
				"    distance_to_nearest_edge_kms = (pygplates.Earth.mean_radius_in_kms *\n"
				"                                    min(plate_boundary_stat.distance_from_start_of_topological_section,\n"
				"                                        plate_boundary_stat.distance_to_end_of_topological_section))\n"
				"\n"
				"  .. note:: This is the absolute value of :attr:`signed_distance_to_end_of_topological_section` and hence only differs from it when the "
				":attr:`boundary point <boundary_point>` is on a rubber-band part of a plate boundary (where it'll be positive here and negative there).\n"
				"\n"
				"  .. seealso:: :attr:`distance_to_end_of_shared_sub_segment`\n")
		// Due to the numerical tolerance in comparisons we cannot make hashable.
		// Make unhashable, with no *equality* comparison operators (we explicitly define them)...
		.def(GPlatesApi::NoHashDefVisitor(false, true))
		.def(bp::self == bp::self)
		.def(bp::self != bp::self)
	;

	// Enable boost::optional<PlateBoundaryStat> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesAppLogic::PlateBoundaryStat>();


	//
	// TopologicalSnapshot - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesApi::TopologicalSnapshot,
			GPlatesApi::TopologicalSnapshot::non_null_ptr_type,
			boost::noncopyable>(
					"TopologicalSnapshot",
					"A snapshot of resolved topological features (lines, boundaries and networks) at a specific geological time.\n"
					"\n"
					".. seealso:: :ref:`pygplates_primer_topological_snapshot` in the *Primer* documentation.\n"
					"\n"
					"A *TopologicalSnapshot* can also be `pickled <https://docs.python.org/3/library/pickle.html>`_.\n"
					"\n"
					".. versionadded:: 0.30\n"
					"\n"
					".. versionchanged:: 0.42\n"
					"   Added pickle support.\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::topological_snapshot_create,
						bp::default_call_policies(),
						(bp::arg("topological_features"),
							bp::arg("rotation_model"),
							bp::arg("reconstruction_time"),
							bp::arg("anchor_plate_id") = boost::optional<GPlatesModel::integer_plate_id_type>(),
							bp::arg("default_resolve_topology_parameters") =
								boost::optional<GPlatesApi::ResolveTopologyParameters::non_null_ptr_to_const_type>())),
			"__init__(topological_features, rotation_model, reconstruction_time, [anchor_plate_id], [default_resolve_topology_parameters])\n"
			"  Create from topological features and a rotation model at a specific reconstruction time.\n"
			"\n"
			"  :param topological_features: The topological boundary and/or network features and the "
			"topological section features they reference (regular and topological lines) as a feature collection, "
			"or filename, or feature, or sequence of features, or a sequence (eg, ``list`` or ``tuple``) "
			"of any combination of those four types. **Note**: Each entry can optionally be a 2-tuple "
			"(entry, :class:`ResolveTopologyParameters`) to override *default_resolve_topology_parameters* for that entry.\n"
			"  :type topological_features: FeatureCollection, or str, or os.PathLike, or Feature, "
			"or sequence of Feature, or sequence of any combination of those four types\n"
			"  :param rotation_model: A rotation model. Or a rotation feature collection, or a rotation filename, "
			"or a rotation feature, or a sequence of rotation features, or a sequence of any combination of those four types.\n"
			"  :type rotation_model: RotationModel. Or FeatureCollection, or str, or os.PathLike, "
			"or Feature, or sequence of Feature, or sequence of any combination of those four types\n"
			"  :param reconstruction_time: the specific geological time to resolve to\n"
			"  :type reconstruction_time: float or GeoTimeInstant\n"
			"  :param anchor_plate_id: The anchored plate id used for all reconstructions "
			"(resolving topologies, and reconstructing regular features). "
			"Defaults to the default anchor plate of *rotation_model* (or zero if *rotation_model* is not a :class:`RotationModel`).\n"
			"  :type anchor_plate_id: int\n"
			"  :param default_resolve_topology_parameters: Default parameters used to resolve topologies. "
			"Note that these can optionally be overridden in *topological_features*. "
			"Defaults to :meth:`default-constructed ResolveTopologyParameters<ResolveTopologyParameters.__init__>`).\n"
			"  :type default_resolve_topology_parameters: ResolveTopologyParameters\n"
			"\n"
			"  .. seealso:: :ref:`pygplates_primer_topological_snapshot` in the *Primer* documentation.\n"
			"\n"
			"  .. versionchanged:: 0.31\n"
			"     Added *default_resolve_topology_parameters* argument.\n"
			"\n"
			"  .. versionchanged:: 0.44\n"
			"     Filenames can be `os.PathLike <https://docs.python.org/3/library/os.html#os.PathLike>`_ "
			"(such as `pathlib.Path <https://docs.python.org/3/library/pathlib.html>`_) in addition to strings.\n")
		// Pickle support...
		//
		// Note: This adds an __init__ method accepting a single argument (of type 'bytes') that supports pickling.
		//       So we define this *after* (higher priority) the other __init__ methods in case one of them accepts a single argument
		//       of type bp::object (which, being more general, would otherwise obscure the __init__ that supports pickling).
		.def(GPlatesApi::PythonPickle::PickleDefVisitor<GPlatesApi::TopologicalSnapshot::non_null_ptr_type>())
		.def("get_resolved_topologies",
				&GPlatesApi::topological_snapshot_get_resolved_topologies,
				(bp::arg("resolve_topology_types") = GPlatesApi::ResolveTopologyType::DEFAULT_RESOLVE_TOPOLOGY_TYPES,
					bp::arg("same_order_as_topological_features") = false),
				"get_resolved_topologies([resolve_topology_types=(pygplates.ResolveTopologyType.boundary|pygplates.ResolveTopologyType.network)], "
				"[same_order_as_topological_features=False])\n"
				"  Returns the resolved topologies of the requested type(s).\n"
				"\n"
				"  :param resolve_topology_types: specifies the resolved topology types to return - defaults "
				"to :class:`resolved topological boundaries<ResolvedTopologicalBoundary>` and "
				":class:`resolved topological networks<ResolvedTopologicalNetwork>`\n"
				"  :type resolve_topology_types: a bitwise combination of any of pygplates.ResolveTopologyType.line, "
				"pygplates.ResolveTopologyType.boundary or pygplates.ResolveTopologyType.network\n"
				"  :param same_order_as_topological_features: whether the returned resolved topologies are sorted in "
				"the order of the topological features (including order across topological files, if there were any) - "
				"defaults to ``False``\n"
				"  :type same_order_as_topological_features: bool\n"
				"  :returns: the :class:`resolved topological lines<ResolvedTopologicalLine>`, "
				":class:`resolved topological boundaries<ResolvedTopologicalBoundary>` and "
				":class:`resolved topological networks<ResolvedTopologicalNetwork>` (depending on the "
				"optional argument *resolve_topology_types*) - by default "
				":class:`resolved topological lines<ResolvedTopologicalLine>` are excluded\n"
				"  :rtype: list[ResolvedTopologicalLine | ResolvedTopologicalBoundary | ResolvedTopologicalNetwork]\n"
				"  :raises: ValueError if *resolve_topology_types* (if specified) contains a flag that "
				"is not one of ``pygplates.ResolveTopologyType.line``, ``pygplates.ResolveTopologyType.boundary`` or "
				"``pygplates.ResolveTopologyType.network``\n"
				"\n"
				"  .. note:: If *same_order_as_topological_features* is ``True`` then the returned resolved topologies are sorted in the order of their "
				"respective topological features (see :meth:`constructor<__init__>`). This includes the order across any topological feature collections/files.\n")
		.def("export_resolved_topologies",
				&GPlatesApi::topological_snapshot_export_resolved_topologies,
				(bp::arg("export_filename"),
					bp::arg("resolve_topology_types") = GPlatesApi::ResolveTopologyType::DEFAULT_RESOLVE_TOPOLOGY_TYPES,
					bp::arg("wrap_to_dateline") = true,
					bp::arg("force_boundary_orientation") = boost::optional<GPlatesMaths::PolygonOrientation::Orientation>()),
				"export_resolved_topologies(export_filename, [resolve_topology_types=(pygplates.ResolveTopologyType.boundary|pygplates.ResolveTopologyType.network)], "
				"[wrap_to_dateline=True], [force_boundary_orientation])\n"
				"  Exports the resolved topologies of the requested type(s) to a file.\n"
				"\n"
				"  :param export_filename: the name of the export file\n"
				"  :type export_filename: str, or os.PathLike\n"
				"  :param resolve_topology_types: specifies the resolved topology types to export - defaults "
				"to :class:`resolved topological boundaries<ResolvedTopologicalBoundary>` and "
				":class:`resolved topological networks<ResolvedTopologicalNetwork>` "
				"(excludes :class:`resolved topological lines<ResolvedTopologicalLine>`)\n"
				"  :type resolve_topology_types: a bitwise combination of any of pygplates.ResolveTopologyType.line, "
				"pygplates.ResolveTopologyType.boundary or pygplates.ResolveTopologyType.network\n"
				"  :param wrap_to_dateline: Whether to wrap/clip resolved topologies to the dateline "
				"(currently ignored unless exporting to an ESRI Shapefile format *file*). Defaults to ``True``.\n"
				"  :type wrap_to_dateline: bool\n"
				"  :param force_boundary_orientation: Optionally force boundary orientation to "
				"clockwise (``PolygonOnSphere.Orientation.clockwise``) or "
				"counter-clockwise (``PolygonOnSphere.Orientation.counter_clockwise``). "
				"Only applies to resolved topological *boundaries* and *networks* (excludes *lines*). "
				"Note that ESRI Shapefiles always use *clockwise* orientation (and so ignore this parameter).\n"
				"  :type force_boundary_orientation: int\n"
				"  :raises: ValueError if *resolve_topology_types* (if specified) contains a flag that "
				"is not one of ``pygplates.ResolveTopologyType.line``, ``pygplates.ResolveTopologyType.boundary`` or "
				"``pygplates.ResolveTopologyType.network``\n"
				"\n"
				"  The following *export* file formats are currently supported:\n"
				"\n"
				"  =============================== =======================\n"
				"  Export File Format              Filename Extension     \n"
				"  =============================== =======================\n"
				"  ESRI Shapefile                  '.shp'                 \n"
				"  GeoJSON                         '.geojson' or '.json'  \n"
				"  OGR GMT                         '.gmt'                 \n"
				"  GMT xy                          '.xy'                  \n"
				"  =============================== =======================\n"
				"\n"
				"  .. note:: Resolved topologies are exported in the same order as that of their "
				"respective topological features (see :meth:`constructor<__init__>`) and the order across "
				"topological feature collections (if any) is also retained.\n"
				"\n"
				"  .. versionchanged:: 0.44\n"
				"     Filenames can be `os.PathLike <https://docs.python.org/3/library/os.html#os.PathLike>`_ "
				"(such as `pathlib.Path <https://docs.python.org/3/library/pathlib.html>`_) in addition to strings.\n")
		.def("get_resolved_topological_sections",
				&GPlatesApi::topological_snapshot_get_resolved_topological_sections,
				(bp::arg("resolve_topological_section_types") = GPlatesApi::ResolveTopologyType::DEFAULT_RESOLVE_TOPOLOGICAL_SECTION_TYPES,
					bp::arg("same_order_as_topological_features") = false),
				"get_resolved_topological_sections([resolve_topological_section_types=(pygplates.ResolveTopologyType.boundary|pygplates.ResolveTopologyType.network)], "
				"[same_order_as_topological_features=False])\n"
				"  Returns the resolved topological sections of the requested type(s).\n"
				"\n"
				"  :param resolve_topological_section_types: Determines whether :class:`ResolvedTopologicalBoundary` or "
				":class:`ResolvedTopologicalNetwork` (or both types) are listed in the returned resolved topological sections. "
				"Note that ``pygplates.ResolveTopologyType.line`` cannot be specified since only topologies with boundaries are considered. "
				"Defaults to :class:`resolved topological boundaries<ResolvedTopologicalBoundary>` and "
				":class:`resolved topological networks<ResolvedTopologicalNetwork>`.\n"
				"  :type resolve_topological_section_types: a bitwise combination of any of "
				"pygplates.ResolveTopologyType.boundary or pygplates.ResolveTopologyType.network\n"
				"  :param same_order_as_topological_features: whether the returned resolved topological sections are sorted in "
				"the order of the topological features (including order across topological files, if there were any) - "
				"defaults to ``False``\n"
				"  :type same_order_as_topological_features: bool\n"
				"  :rtype: list of ResolvedTopologicalSection\n"
				"  :raises: ValueError if *resolve_topological_section_types* (if specified) contains a flag that "
				"is not one of ``pygplates.ResolveTopologyType.boundary`` or ``pygplates.ResolveTopologyType.network``\n"
				"\n"
				"  .. note:: If *same_order_as_topological_features* is ``True`` then the returned resolved topological sections are sorted in the order of their "
				"respective topological features (see :meth:`constructor<__init__>`). This includes the order across any topological feature collections/files.\n")
		.def("export_resolved_topological_sections",
				&GPlatesApi::topological_snapshot_export_resolved_topological_sections,
				(bp::arg("export_filename"),
					bp::arg("resolve_topological_section_types") = GPlatesApi::ResolveTopologyType::DEFAULT_RESOLVE_TOPOLOGY_TYPES,
					bp::arg("export_topological_line_sub_segments") = true,
					bp::arg("wrap_to_dateline") = true),
				"export_resolved_topological_sections(export_filename, "
				"[resolve_topological_section_types=(pygplates.ResolveTopologyType.boundary|pygplates.ResolveTopologyType.network)], "
				"[export_topological_line_sub_segments=True], [wrap_to_dateline=True])\n"
				"  Exports the resolved topological sections of the requested type(s) to a file.\n"
				"\n"
				"  :param export_filename: the name of the export file\n"
				"  :type export_filename: str, or os.PathLike\n"
				"  :param resolve_topological_section_types: Determines whether :class:`ResolvedTopologicalBoundary` or "
				":class:`ResolvedTopologicalNetwork` (or both types) are listed in the exported resolved topological sections. "
				"Note that ``pygplates.ResolveTopologyType.line`` cannot be specified since only topologies with boundaries are considered. "
				"Defaults to :class:`resolved topological boundaries<ResolvedTopologicalBoundary>` and "
				":class:`resolved topological networks<ResolvedTopologicalNetwork>`.\n"
				"  :type resolve_topological_section_types: a bitwise combination of any of "
				"pygplates.ResolveTopologyType.boundary or pygplates.ResolveTopologyType.network\n"
				"  :param export_topological_line_sub_segments: Whether to export the individual sub-segments of each "
				"boundary segment that came from a resolved topological line (``True``) or export a single geometry "
				"per boundary segment (``False``). Defaults to ``True``.\n"
				"  :type export_topological_line_sub_segments: bool\n"
				"  :param wrap_to_dateline: Whether to wrap/clip resolved topological sections to the dateline "
				"(currently ignored unless exporting to an ESRI Shapefile format *file*). Defaults to ``True``.\n"
				"  :type wrap_to_dateline: bool\n"
				"  :raises: ValueError if *resolve_topological_section_types* (if specified) contains a flag that "
				"is not one of ``pygplates.ResolveTopologyType.boundary`` or ``pygplates.ResolveTopologyType.network``\n"
				"\n"
				"  The following *export* file formats are currently supported:\n"
				"\n"
				"  =============================== =======================\n"
				"  Export File Format              Filename Extension     \n"
				"  =============================== =======================\n"
				"  ESRI Shapefile                  '.shp'                 \n"
				"  GeoJSON                         '.geojson' or '.json'  \n"
				"  OGR GMT                         '.gmt'                 \n"
				"  GMT xy                          '.xy'                  \n"
				"  =============================== =======================\n"
				"\n"
				"  The argument *export_topological_line_sub_segments* only applies to topological lines. "
				"It determines whether to export the section of the resolved topological line (contributing to boundaries) "
				"or its :meth:`sub-segments<ResolvedTopologicalSharedSubSegment.get_sub_segments>`. "
				"Note that this also determines whether the feature properties (such as plate ID and feature type) "
				"of the topological line feature or its individual sub-segment features are exported.\n"
				"\n"
				"  .. note:: Resolved topological sections are exported in the same order as that of their "
				"respective topological features (see :meth:`constructor<__init__>`) and the order across "
				"topological feature collections (if any) is also retained.\n"
				"\n"
				"  .. versionchanged:: 0.33\n"
				"     Added *export_topological_line_sub_segments* argument.\n"
				"\n"
				"  .. versionchanged:: 0.44\n"
				"     Filenames can be `os.PathLike <https://docs.python.org/3/library/os.html#os.PathLike>`_ "
				"(such as `pathlib.Path <https://docs.python.org/3/library/pathlib.html>`_) in addition to strings.\n")
		.def("calculate_plate_boundary_statistics",
				&GPlatesApi::topological_snapshot_calculate_plate_boundary_statistics,
				(bp::arg("uniform_point_spacing_radians"),
					bp::arg("first_uniform_point_spacing_radians") = boost::optional<double>(),
					bp::arg("velocity_delta_time") = 1.0,
					bp::arg("velocity_delta_time_type") = GPlatesAppLogic::VelocityDeltaTime::T_PLUS_DELTA_T_TO_T,
					bp::arg("velocity_units") = GPlatesAppLogic::VelocityUnits::KMS_PER_MY,
					bp::arg("earth_radius_in_kms") = GPlatesUtils::Earth::MEAN_RADIUS_KMS,
					bp::arg("include_network_boundaries") = false,
					bp::arg("boundary_section_filter") = bp::object()/*Py_None*/,
					bp::arg("return_shared_sub_segment_dict") = false),
				"calculate_plate_boundary_statistics(uniform_point_spacing_radians, [first_uniform_point_spacing_radians], "
				"[velocity_delta_time=1.0], [velocity_delta_time_type=pygplates.VelocityDeltaTimeType.t_plus_delta_t_to_t], "
				"[velocity_units=pygplates.VelocityUnits.kms_per_my], [earth_radius_in_kms=pygplates.Earth.mean_radius_in_kms], "
				"[include_network_boundaries=False], [boundary_section_filter], [return_shared_sub_segment_dict=False])\n"
				"Calculate statistics at uniformly spaced points along plate boundaries.\n"
				"\n"
				"  :param uniform_point_spacing_radians: Spacing between uniform points along plate boundaries (in radians). "
				"See :meth:`PolylineOnSphere.to_uniform_points`.\n"
				"  :type uniform_point_spacing_radians: float\n"
				"  :param first_uniform_point_spacing_radians: Spacing of first uniform point in each "
				":class:`resolved topological section <ResolvedTopologicalSection>` (in radians). "
				"Each resolved topological section is associated with a specific boundary :class:`Feature` and represents the "
				":meth:`shared sub-segments <ResolvedTopologicalSection.get_shared_sub_segments>` that are the parts of it that actually "
				"contribute to plate boundaries. So, this parameter is the distance from the *first* vertex of the *first* shared sub-segment "
				"(*along* the sub-segment). And note that the uniform spacing is continuous across adjacent shared sub-segments, unless there's a "
				"gap between them (eg, that no plate uses as part of its boundary), in which case the spacing is reset to *first_uniform_point_spacing_radians* "
				"for the next shared sub-segment (after the gap). "
				"See :meth:`PolylineOnSphere.to_uniform_points`. Defaults to half of *uniform_point_spacing_radians*.\n"
				"  :type first_uniform_point_spacing_radians: float\n"
				"  :param velocity_delta_time: The time delta used to calculate velocities (defaults to 1 Myr).\n"
				"  :type velocity_delta_time: float\n"
				"  :param velocity_delta_time_type: How the two velocity times are calculated relative to the reconstruction time. "
				"This includes [t+dt, t], [t, t-dt] and [t+dt/2, t-dt/2]. Defaults to [t+dt, t].\n"
				"  :type velocity_delta_time_type: VelocityDeltaTimeType.t_plus_delta_t_to_t, "
				"VelocityDeltaTimeType.t_to_t_minus_delta_t or VelocityDeltaTimeType.t_plus_minus_half_delta_t\n"
				"  :param velocity_units: whether to return velocities as *kilometres per million years* or "
				"*centimetres per year* (defaults to *kilometres per million years*)\n"
				"  :type velocity_units: VelocityUnits.kms_per_my or VelocityUnits.cms_per_yr\n"
				"  :param earth_radius_in_kms: The radius of the Earth in *kilometres* (defaults to ``pygplates.Earth.mean_radius_in_kms``). "
				"This is only used to calculate velocities (strain rates always use ``pygplates.Earth.equatorial_radius_in_kms``).\n"
				"  :type earth_radius_in_kms: float\n"
				"  :param include_network_boundaries: Whether to calculate statistics along *network* boundaries "
				"that are **not** also plate boundaries (defaults to ``False``). If a deforming network shares a "
				"boundary with a plate then it'll get included regardless of this option.\n"
				"  :type include_network_boundaries: bool\n"
				"  :param boundary_section_filter: Optionally restrict boundary sections to those that match a feature type, "
				"or match one of several feature types, or match a filter function. Defaults to ``None`` (meaning accept all boundary sections).\n"
				"  :type boundary_section_filter: FeatureType, or list of FeatureType, or callable "
				"(accepting a single ResolvedTopologicalSection)\n"
				"  :param return_shared_sub_segment_dict: Whether to return a ``dict`` mapping each :class:`shared sub-segment <ResolvedTopologicalSharedSubSegment>` "
				"(ie, a boundary section shared by one or more plates) to a ``list`` of :class:`PlateBoundaryStatistic` associated with it. "
				"If ``False`` then just returns one large ``list`` of :class:`PlateBoundaryStatistic` for all plate boundaries. Defaults to ``False``.\n"
				"  :type return_shared_sub_segment_dict: bool\n"
				"  :returns: list of :class:`PlateBoundaryStatistic` for all uniform points, or (if *return_shared_sub_segment_dict* is ``True``) a "
				"``dict`` mapping each :class:`ResolvedTopologicalSharedSubSegment` to a list of :class:`PlateBoundaryStatistic`\n"
				"  :rtype: list[PlateBoundaryStatistic], or dict[ResolvedTopologicalSharedSubSegment, list[PlateBoundaryStatistic]]\n"
				"  :raises: ValueError if *uniform_point_spacing_radians* is negative or zero\n"
				"  :raises: ValueError if *velocity_delta_time* is negative or zero.\n"
				"\n"
				"  .. seealso:: :ref:`pygplates_primer_plate_boundary_statistics` in the *Primer* documentation.\n"
				"\n"
				"  .. note:: If *return_shared_sub_segment_dict* is ``True`` then any shared sub-segments that are not long enough to contain any uniform points "
				"will be missing from the returned ``dict``.\n"
				"\n"
				"  Each :class:`resolved topological section <ResolvedTopologicalSection>` is associated with a specific boundary :class:`Feature` and represents the "
				":meth:`shared sub-segments <ResolvedTopologicalSection.get_shared_sub_segments>` that are the parts of it that actually contribute to plate boundaries. "
				"Along each shared sub-segment, uniform points are ordered from its start to its end.\n"
				"\n"
				"  Uniform points are **not** generated along *network* boundaries by default (unless they happen to also be a plate boundary) since "
				"not all parts of a network's boundary are necessarily along plate boundaries. But you can optionally generate points along them by setting "
				"*include_network_boundaries* to ``True``. Note that, regardless of this option, networks are always used when *calculating* plate statistics. "
				"This is because networks typically *overlay* rigid plates, and so need to be queried (at uniform points along plate boundaries) with a "
				"higher priority than the *underlying* rigid plate.\n"
				"\n"
				"  .. note:: The plate boundaries, *along* which uniform points are generated, can be further restricted using *boundary_section_filter*.\n"
				"\n"
				"  .. versionadded:: 0.47\n")
		.def("get_point_locations",
				&GPlatesApi::topological_snapshot_get_point_locations,
				(bp::arg("points"),
					bp::arg("resolve_topology_types") = GPlatesApi::ResolveTopologyType::BOUNDARY_AND_NETWORK_RESOLVE_TOPOLOGY_TYPES),
				"get_point_locations(points, [resolve_topology_types=(pygplates.ResolveTopologyType.boundary|pygplates.ResolveTopologyType.network)])\n"
				"  Returns the resolved topological boundaries/networks that contain the specified points.\n"
				"\n"
				"  :param points: sequence of points at which to find containing topologies\n"
				"  :type points: any sequence of PointOnSphere or LatLonPoint or tuple (latitude,longitude), in degrees, or tuple (x,y,z)\n"
				"  :param resolve_topology_types: specifies the resolved topology types to search - defaults "
				"to :class:`resolved topological boundaries<ResolvedTopologicalBoundary>` and "
				":class:`resolved topological networks<ResolvedTopologicalNetwork>` "
				"(excludes :class:`resolved topological lines<ResolvedTopologicalLine>` since lines cannot contain points)\n"
				"  :type resolve_topology_types: a bitwise combination of any of pygplates.ResolveTopologyType.boundary or "
				"pygplates.ResolveTopologyType.network\n"
				"  :rtype: list of TopologyPointLocation\n"
				"  :raises: ValueError if *resolve_topology_types* (if specified) contains a flag that "
				"is not one of ``pygplates.ResolveTopologyType.boundary`` or ``pygplates.ResolveTopologyType.network``\n"
				"\n"
				"  :class:`Resolved topological networks<ResolvedTopologicalNetwork>` have a higher priority than "
				":class:`resolved topological boundaries<ResolvedTopologicalBoundary>` since networks typically *overlay* rigid plates. "
				"So if a point is inside both a boundary and a network then the network location is returned.\n"
				"\n"
				"  .. note:: Each point that is *outside* all resolved topologies searched will have "
				":meth:`TopologyPointLocation.not_located_in_resolved_topology` returning ``True``.\n"
				"\n"
				"  To associate each point with the resolved topological boundary/network containing it:\n"
				"  ::\n"
				"\n"
				"    topology_point_locations = topological_snapshot.get_point_locations(points)\n"
				"\n"
				"    for point_index in range(len(points)):\n"
				"        point = points[point_index]\n"
				"        topology_point_location = topology_point_locations[point_index]\n"
				"\n"
				"        if topology_point_location.located_in_resolved_boundary():  # if point is inside a resolved boundary\n"
				"            resolved_topological_boundary = topology_point_location.located_in_resolved_boundary()\n"
				"        elif topology_point_location.located_in_resolved_network():  # if point is inside a resolved network\n"
				"            resolved_topological_network = topology_point_location.located_in_resolved_network()\n"
				"        else:  # point is not in any resolved topologies\n"
				"            ...\n"
				"\n"
				"  .. versionadded:: 0.50\n")
		.def("get_point_velocities",
				&GPlatesApi::topological_snapshot_get_point_velocities,
				(bp::arg("points"),
					bp::arg("resolve_topology_types") = GPlatesApi::ResolveTopologyType::BOUNDARY_AND_NETWORK_RESOLVE_TOPOLOGY_TYPES,
					bp::arg("velocity_delta_time") = 1.0,
					bp::arg("velocity_delta_time_type") = GPlatesAppLogic::VelocityDeltaTime::T_PLUS_DELTA_T_TO_T,
					bp::arg("velocity_units") = GPlatesAppLogic::VelocityUnits::KMS_PER_MY,
					bp::arg("earth_radius_in_kms") = GPlatesUtils::Earth::MEAN_RADIUS_KMS,
					bp::arg("return_point_locations") = false),
				"get_point_velocities(points, [resolve_topology_types=(pygplates.ResolveTopologyType.boundary|pygplates.ResolveTopologyType.network)], "
				"[velocity_delta_time=1.0], [velocity_delta_time_type=pygplates.VelocityDeltaTimeType.t_plus_delta_t_to_t], "
				"[velocity_units=pygplates.VelocityUnits.kms_per_my], [earth_radius_in_kms=pygplates.Earth.mean_radius_in_kms], "
				"[return_point_locations=False])\n"
				"  Returns the velocities of the specified points (as determined by the resolved topological boundaries/networks that contain them).\n"
				"\n"
				"  :param points: sequence of points at which to calculate velocities\n"
				"  :type points: any sequence of PointOnSphere or LatLonPoint or tuple (latitude,longitude), in degrees, or tuple (x,y,z)\n"
				"  :param resolve_topology_types: specifies the resolved topology types to use for calculating velocities - defaults "
				"to :class:`resolved topological boundaries<ResolvedTopologicalBoundary>` and "
				":class:`resolved topological networks<ResolvedTopologicalNetwork>` "
				"(excludes :class:`resolved topological lines<ResolvedTopologicalLine>` since lines cannot contain points)\n"
				"  :type resolve_topology_types: a bitwise combination of any of pygplates.ResolveTopologyType.boundary or "
				"pygplates.ResolveTopologyType.network\n"
				"  :param velocity_delta_time: The time delta used to calculate velocities (defaults to 1 Myr).\n"
				"  :type velocity_delta_time: float\n"
				"  :param velocity_delta_time_type: How the two velocity times are calculated relative to the reconstruction time. "
				"This includes [t+dt, t], [t, t-dt] and [t+dt/2, t-dt/2]. Defaults to [t+dt, t].\n"
				"  :type velocity_delta_time_type: VelocityDeltaTimeType.t_plus_delta_t_to_t, "
				"VelocityDeltaTimeType.t_to_t_minus_delta_t or VelocityDeltaTimeType.t_plus_minus_half_delta_t\n"
				"  :param velocity_units: whether to return velocities as *kilometres per million years* or "
				"*centimetres per year* (defaults to *kilometres per million years*)\n"
				"  :type velocity_units: VelocityUnits.kms_per_my or VelocityUnits.cms_per_yr\n"
				"  :param earth_radius_in_kms: the radius of the Earth in *kilometres* (defaults to ``pygplates.Earth.mean_radius_in_kms``)\n"
				"  :type earth_radius_in_kms: float\n"
				"  :param return_point_locations: whether to also return the resolved topological boundary/network that contains each point - defaults to ``False``\n"
				"  :type return_point_locations: bool\n"
				"  :returns: the velocity of each point (``None`` for each point *outside* all resolved topologies searched), and "
				"(if *return_point_locations* is ``True``) also the :class:`location <TopologyPointLocation>` of each point\n"
				"  :rtype: list[Vector3D | None], or tuple[list[Vector3D | None], list[TopologyPointLocation]]\n"
				"  :raises: ValueError if *resolve_topology_types* (if specified) contains a flag that "
				"is not one of ``pygplates.ResolveTopologyType.boundary`` or ``pygplates.ResolveTopologyType.network``\n"
				"\n"
				"  :class:`Resolved topological networks<ResolvedTopologicalNetwork>` have a higher priority than "
				":class:`resolved topological boundaries<ResolvedTopologicalBoundary>` since networks typically *overlay* rigid plates. "
				"So if a point is inside both a boundary and a network then the velocity of the network is returned.\n"
				"\n"
				"  .. note:: Each point that is *outside* all resolved topologies searched will have a velocity of ``None``, and optionally "
				"(if *return_point_locations* is ``True``) have a :meth:`TopologyPointLocation.not_located_in_resolved_topology` returning ``True``.\n"
				"\n"
				"  To associate each point with its velocity and the resolved topological boundary/network containing it:\n"
				"  ::\n"
				"\n"
				"    velocities, topology_point_locations = topological_snapshot.get_point_velocities(\n"
				"            points,\n"
				"            return_point_locations=True)\n"
				"\n"
				"    for point_index in range(len(points)):\n"
				"        point = points[point_index]\n"
				"        velocity = velocities[point_index]\n"
				"        topology_point_location = topology_point_locations[point_index]\n"
				"\n"
				"        if velocity:  # if point is inside a resolved boundary or network\n"
				"            ...\n"
				"\n"
				"  .. note:: It is more efficient to call ``topological_snapshot.get_point_velocities(points, return_point_locations=True)`` to get both velocities and "
				"point locations than it is to call both ``topological_snapshot.get_point_velocities(points)`` and ``topological_snapshot.get_point_locations(points)``.\n"
				"\n"
				"  .. versionadded:: 0.50\n")
		.def("get_point_strain_rates",
				&GPlatesApi::topological_snapshot_get_point_strain_rates,
				(bp::arg("points"),
					bp::arg("resolve_topology_types") = GPlatesApi::ResolveTopologyType::BOUNDARY_AND_NETWORK_RESOLVE_TOPOLOGY_TYPES,
					bp::arg("return_point_locations") = false),
				"get_point_strain_rates(points, "
				"[resolve_topology_types=(pygplates.ResolveTopologyType.boundary|pygplates.ResolveTopologyType.network)], [return_point_locations=False])\n"
				"  Returns the strain rates of the specified points (as determined by the resolved topological boundaries/networks that contain them).\n"
				"\n"
				"  :param points: sequence of points at which to calculate strain rates\n"
				"  :type points: any sequence of PointOnSphere or LatLonPoint or tuple (latitude,longitude), in degrees, or tuple (x,y,z)\n"
				"  :param resolve_topology_types: specifies the resolved topology types to use for strain rates - defaults "
				"to :class:`resolved topological boundaries<ResolvedTopologicalBoundary>` and "
				":class:`resolved topological networks<ResolvedTopologicalNetwork>` "
				"(excludes :class:`resolved topological lines<ResolvedTopologicalLine>` since lines cannot contain points)\n"
				"  :type resolve_topology_types: a bitwise combination of any of pygplates.ResolveTopologyType.boundary or "
				"pygplates.ResolveTopologyType.network\n"
				"  :param return_point_locations: whether to also return the resolved topological boundary/network that contains each point - defaults to ``False``\n"
				"  :type return_point_locations: bool\n"
				"  :returns: the strain rate of each point (``None`` for each point *outside* all resolved topologies searched), and "
				"(if *return_point_locations* is ``True``) also the :class:`location <TopologyPointLocation>` of each point\n"
				"  :rtype: list[StrainRate | None], or tuple[list[StrainRate | None], list[TopologyPointLocation]]\n"
				"  :raises: ValueError if *resolve_topology_types* (if specified) contains a flag that "
				"is not one of ``pygplates.ResolveTopologyType.boundary`` or ``pygplates.ResolveTopologyType.network``\n"
				"\n"
				"  :class:`Resolved topological networks<ResolvedTopologicalNetwork>` have a higher priority than "
				":class:`resolved topological boundaries<ResolvedTopologicalBoundary>` since networks typically *overlay* rigid plates. "
				"So a point that is inside a resolved topological network can generate a *non-zero* :class:`strain rate <StrainRate>`. "
				"However, a point that is inside a resolved topological boundary (but is outside all resolved topological networks searched) "
				"will generate a *zero* strain rate (``pygplates.StrainRate.zero``) since it is inside a *non-deforming* plate.\n"
				"\n"
				"  .. note:: Each point that is *outside* all resolved topologies searched (boundaries and networks) will have a strain rate of ``None``, and "
				"optionally (if *return_point_locations* is ``True``) have a :meth:`TopologyPointLocation.not_located_in_resolved_topology` returning ``True``.\n"
				"\n"
				"  To associate each point with its strain rate and the resolved topological boundary/network containing it:\n"
				"  ::\n"
				"\n"
				"    strain_rates, topology_point_locations = topological_snapshot.get_point_strain_rates(\n"
				"            points,\n"
				"            return_point_locations=True)\n"
				"\n"
				"    for point_index in range(len(points)):\n"
				"        point = points[point_index]\n"
				"        strain_rate = strain_rates[point_index]\n"
				"        topology_point_location = topology_point_locations[point_index]\n"
				"\n"
				"        if strain_rate:  # if point is inside a resolved boundary or network\n"
				"            ...\n"
				"\n"
				"  .. note:: It is more efficient to call ``topological_snapshot.get_point_strain_rates(points, return_point_locations=True)`` to get both strain rates and "
				"point locations than it is to call both ``topological_snapshot.get_point_strain_rates(points)`` and ``topological_snapshot.get_point_locations(points)``.\n"
				"\n"
				"  .. note:: Strain rates in deforming networks are calculated from the spatial gradients of velocity where the velocities are calculated over "
				"a 1 Myr time interval and using the *equatorial* Earth radius :class:`pygplates.Earth.equatorial_radius_in_kms <Earth>`.\n"
				"\n"
				"  .. versionadded:: 0.50\n")
		.def("reconstruct_points",
				&GPlatesApi::topological_snapshot_reconstruct_points,
				(bp::arg("points"),
					bp::arg("reconstruction_time"),
					bp::arg("resolve_topology_types") = GPlatesApi::ResolveTopologyType::BOUNDARY_AND_NETWORK_RESOLVE_TOPOLOGY_TYPES,
					bp::arg("use_natural_neighbour_interpolation") = true,
					bp::arg("return_input_if_not_intersect") = false),
				"reconstruct_points(points, reconstruction_time, "
				"[resolve_topology_types=(pygplates.ResolveTopologyType.boundary|pygplates.ResolveTopologyType.network)], "
				"[use_natural_neighbour_interpolation=True], [return_input_if_not_intersect=False])\n"
				"  Incrementally reconstruct the specified points (that lie within resolved topological boundaries/networks searched in this snapshot) to the specified reconstruction time.\n"
				"\n"
				"  :param points: sequence of points to reconstruct\n"
				"  :type points: any sequence of PointOnSphere or LatLonPoint or tuple (latitude,longitude), in degrees, or tuple (x,y,z)\n"
				"  :param reconstruction_time: The time to reconstruct *to*. This can be older or younger than the "
				":meth:`reconstruction time of this snapshot <get_reconstruction_time>`.\n"
				"  :type reconstruction_time: float or GeoTimeInstant\n"
				"  :param resolve_topology_types: specifies the resolved topology types to search - defaults "
				"to :class:`resolved topological boundaries<ResolvedTopologicalBoundary>` and "
				":class:`resolved topological networks<ResolvedTopologicalNetwork>` "
				"(excludes :class:`resolved topological lines<ResolvedTopologicalLine>` since lines cannot contain points)\n"
				"  :type resolve_topology_types: a bitwise combination of any of pygplates.ResolveTopologyType.boundary or "
				"pygplates.ResolveTopologyType.network\n"
				"  :param use_natural_neighbour_interpolation: If ``True`` and a point lies within the deforming region of a resolved network, "
				"then the reconstructed point is the interpolation of the natural neighbour deformed triangulation vertex positions "
				"(otherwise barycentric interpolation is used). Only applies if resolved networks are specified in *resolve_topology_types*. "
				"Defaults to ``True``.\n"
				"  :type use_natural_neighbour_interpolation: bool\n"
				"  :param return_input_if_not_intersect: Whether to return the *input* point for each point that does *not* intersect any "
				"resolved topological boundaries/networks searched in this snapshot - if ``False`` then ``None`` is returned. Defaults to ``False``.\n"
				"  :type return_input_if_not_intersect: bool\n"
				"  :returns: the reconstructed points (each is ``None`` if the point does not intersect any resolved topological "
				"boundaries/networks searched and *return_input_if_not_intersect* is ``False``)\n"
				"  :rtype: list[PointOnSphere | None]\n"
				"  :raises: ValueError if *resolve_topology_types* (if specified) contains a flag that "
				"is not one of ``pygplates.ResolveTopologyType.boundary`` or ``pygplates.ResolveTopologyType.network``\n"
				"\n"
				"  :class:`Resolved topological networks<ResolvedTopologicalNetwork>` have a higher priority than "
				":class:`resolved topological boundaries<ResolvedTopologicalBoundary>` since networks typically *overlay* rigid plates. "
				"So if a point is inside both a boundary and a network then the network will reconstruct the point.\n"
				"\n"
				"  .. note:: Each point that is *outside* all resolved topologies searched will return ``None``, unless "
				"*return_input_if_not_intersect* is ``True`` in which case the input point is returned unchanged.\n"
				"\n"
				"  The specified reconstruction time can be older or younger than the :meth:`reconstruction time of this snapshot <get_reconstruction_time>`. "
				"If it's *older* then each point is reconstructed *backward* in time, and if it's *younger* then each point is reconstructed *forward* in time.\n"
				"\n"
				"  .. note:: Each point reconstruction involves calculating a stage rotation (for resolved networks this is for each nearby vertex in the deforming triangulation) "
				"from the :meth:`reconstruction time of this snapshot <get_reconstruction_time>` to *reconstruction_time*. "
				"So ideally a small time increment (such as 1 Myr) should be used since the plate boundaries and rotations typically change over small time intervals.\n"
				"\n"
				"  To reconstruct each point using the resolved topological boundary/network containing it to a new position "
				"at a time 1 Myr older than the snapshot time (and return each original point unreconstructed if it's not contained by any):\n"
				"  ::\n"
				"\n"
				"    reconstructed_points = topological_snapshot.reconstruct_points(\n"
				"            points,\n"
				"            topological_snapshot.get_reconstruction_time() + 1.0,\n"
				"            return_input_if_not_intersect=True)\n"
				"\n"
				"  .. seealso:: :meth:`ResolvedTopologicalBoundary.reconstruct_point` and :meth:`ResolvedTopologicalNetwork.reconstruct_point`\n"
				"\n"
				"  .. note:: This method is noticeably faster than reconstructing each point individually (using :meth:`get_point_locations`, followed by "
				":meth:`ResolvedTopologicalBoundary.reconstruct_point` or :meth:`ResolvedTopologicalNetwork.reconstruct_point` for each point). "
				"This is because extracting a ``ResolvedTopologicalBoundary`` or ``ResolvedTopologicalNetwork`` from the :class:`TopologyPointLocation` "
				"associated with each point is time consuming (for reasons related to the internal implementation). Especially when the number of points "
				"is much larger than the number of resolved topologies in the snapshot.\n"
				"\n"
				"  .. versionadded:: 1.1\n")
		.def("get_rotation_model",
				&GPlatesApi::TopologicalSnapshot::get_rotation_model,
				"get_rotation_model()\n"
				"  Return the rotation model used internally.\n"
				"\n"
				"  :rtype: RotationModel\n"
				"\n"
				"  .. note:: The :meth:`default anchor plate ID<RotationModel.get_default_anchor_plate_id>` of the returned rotation model "
				"may be different to that of the rotation model passed into the :meth:`constructor<__init__>` if an anchor plate ID was specified "
				"in the :meth:`constructor<__init__>`.\n")
		.def("get_reconstruction_time",
				&GPlatesApi::TopologicalSnapshot::get_reconstruction_time,
				"get_reconstruction_time()\n"
				"  Return the reconstruction time of this snapshot.\n"
				"\n"
				"  :rtype: float\n"
				"\n"
				"  .. versionadded:: 0.43\n")
		.def("get_anchor_plate_id",
				&GPlatesApi::TopologicalSnapshot::get_anchor_plate_id,
				"get_anchor_plate_id()\n"
				"  Return the anchor plate ID (see :meth:`constructor<__init__>`).\n"
				"\n"
				"  :rtype: int\n"
				"\n"
				"  .. note:: This is the same as the :meth:`default anchor plate ID<RotationModel.get_default_anchor_plate_id>` "
				"of :meth:`get_rotation_model`.\n")
		// Make hash and comparisons based on C++ object identity (not python object identity)...
		.def(GPlatesApi::ObjectIdentityHashDefVisitor())
	;

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesApi::TopologicalSnapshot>();
}
