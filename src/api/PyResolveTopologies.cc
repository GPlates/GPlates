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

// Workaround for compile error in <pyport.h> for Python versions less than 2.7.13 and 3.5.3.
// See https://bugs.python.org/issue10910
// Workaround involves including "global/python.h" at the top of some source files
// to ensure <Python.h> is included before <ctype.h>.
#include "global/python.h"

#include <utility>
#include <vector>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include <QString>

#include "PyResolveTopologies.h"

#include "PyFeatureCollection.h"
#include "PyRotationModel.h"
#include "PythonConverterUtils.h"
#include "PythonHashDefVisitor.h"
#include "PythonUtils.h"
#include "PythonVariableFunctionArguments.h"

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

#include "file-io/FeatureCollectionFileFormatRegistry.h"
#include "file-io/File.h"
#include "file-io/ReadErrorAccumulation.h"
#include "file-io/ReconstructionGeometryExportImpl.h"
#include "file-io/ResolvedTopologicalGeometryExport.h"

// This is not included by <boost/python.hpp>.
// Also we must include this after <boost/python.hpp> which means after "global/python.h".
#include <boost/python/raw_function.hpp>

#include "maths/PolygonOrientation.h"

#include "model/FeatureCollectionHandle.h"
#include "model/types.h"

#include "property-values/GeoTimeInstant.h"


namespace bp = boost::python;


namespace GPlatesApi
{
	/**
	 * This is called directly from Python via 'TopologicalSnapshot.__init__()'.
	 */
	TopologicalSnapshot::non_null_ptr_type
	topological_snapshot_create(
			const FeatureCollectionSequenceFunctionArgument &topological_features,
			const RotationModelFunctionArgument &rotation_model_argument,
			const GPlatesPropertyValues::GeoTimeInstant &reconstruction_time,
			boost::optional<GPlatesModel::integer_plate_id_type> anchor_plate_id)
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
				anchor_plate_id);
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
			const QString &export_file_name,
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
			const QString &export_file_name,
			ResolveTopologyType::flags_type resolve_topological_section_types,
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
				wrap_to_dateline);
	}


	TopologicalSnapshot::non_null_ptr_type
	TopologicalSnapshot::create(
			const FeatureCollectionSequenceFunctionArgument &topological_features_argument,
			const RotationModelFunctionArgument &rotation_model_argument,
			const double &reconstruction_time,
			boost::optional<GPlatesModel::integer_plate_id_type> anchor_plate_id)
	{
		// Extract the rotation model from the function argument and adapt it to a new one that has 'anchor_plate_id'
		// as its default (which if none, then uses default anchor plate of extracted rotation model instead).
		// This ensures we will reconstruct topological sections using the correct anchor plate.
		RotationModel::non_null_ptr_type rotation_model = RotationModel::create(
				rotation_model_argument.get_rotation_model(),
				1/*reconstruction_tree_cache_size*/,
				anchor_plate_id);

		return non_null_ptr_type(
				new TopologicalSnapshot(topological_features_argument, rotation_model, reconstruction_time));
	}

	TopologicalSnapshot::TopologicalSnapshot(
			const FeatureCollectionSequenceFunctionArgument &topological_features_argument,
			const RotationModel::non_null_ptr_type &rotation_model,
			const double &reconstruction_time) :
		d_rotation_model(rotation_model),
		d_reconstruction_time(reconstruction_time)
	{
		// Extract the topological files from the function argument.
		topological_features_argument.get_files(d_topological_files);

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
					reconstruction_time);
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
										rotation_model->get_reconstruction_tree_creator())),
						reconstruction_time);

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
						rotation_model->get_reconstruction_tree_creator(), 
						reconstruction_time,
						// Resolved topo lines use the reconstructed non-topo geometries...
						topological_sections_reconstruct_handles,
						// Only those topo lines references by resolved boundaries/networks...
						topological_sections_referenced);

		topological_sections_reconstruct_handles.push_back(resolved_topological_lines_handle);

		// Resolve topological boundaries.
		GPlatesAppLogic::TopologyUtils::resolve_topological_boundaries(
				d_resolved_topological_boundaries,
				topological_feature_collections,
				rotation_model->get_reconstruction_tree_creator(), 
				reconstruction_time,
				// Resolved topo boundaries use the resolved topo lines *and* the reconstructed non-topo geometries...
				topological_sections_reconstruct_handles);

		// Resolve topological networks.
		GPlatesAppLogic::TopologyUtils::resolve_topological_networks(
				d_resolved_topological_networks,
				reconstruction_time,
				topological_feature_collections,
				// Resolved topo networks use the resolved topo lines *and* the reconstructed non-topo geometries...
				topological_sections_reconstruct_handles);
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
			const QString &export_file_name,
			ResolveTopologyType::flags_type resolve_topology_types,
			bool wrap_to_dateline,
			boost::optional<GPlatesMaths::PolygonOrientation::Orientation> force_boundary_orientation) const
	{
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
						export_file_name,
						file_format_registry);

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

		if ((resolve_topological_section_types & (ResolveTopologyType::BOUNDARY | ResolveTopologyType::NETWORK)) ==
			(ResolveTopologyType::BOUNDARY | ResolveTopologyType::NETWORK))
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
			const QString &export_file_name,
			ResolveTopologyType::flags_type resolve_topological_section_types,
			bool wrap_to_dateline) const
	{
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
						export_file_name,
						file_format_registry);

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


	namespace
	{
		/**
		 * The argument types for 'resolved topologies'.
		 */
		typedef boost::variant<
				// Export filename...
				QString,
				// List of 'ResolvedTopologicalLine's, 'ResolvedTopologicalBoundary's and 'ResolvedTopologicalNetwork's...
				bp::list>
						resolved_topologies_argument_type;

		/**
		 * The argument types for 'resolved topological sections'.
		 */
		typedef boost::variant<
				// Export filename...
				QString,
				// List of 'ResolvedTopologicalSection's...
				bp::list>
						resolved_topological_sections_argument_type;


		/**
		 * Retrieve the function arguments from the python 'resolve_topologies()' function.
		 */
		void
		get_resolve_topologies_args(
				bp::tuple positional_args,
				bp::dict keyword_args,
				boost::optional<FeatureCollectionSequenceFunctionArgument> &topological_features,
				boost::optional<RotationModelFunctionArgument> &rotation_model,
				resolved_topologies_argument_type &resolved_topologies,
				GPlatesPropertyValues::GeoTimeInstant &reconstruction_time,
				boost::optional<resolved_topological_sections_argument_type> &resolved_topological_sections,
				boost::optional<GPlatesModel::integer_plate_id_type> &anchor_plate_id,
				ResolveTopologyType::flags_type &resolve_topology_types,
				ResolveTopologyType::flags_type &resolve_topological_section_types,
				bool &export_wrap_to_dateline,
				boost::optional<GPlatesMaths::PolygonOrientation::Orientation> &export_force_boundary_orientation)
		{
			//
			// Get arguments for 'resolve_topologies().
			// If this fails then a python exception will be generated.
			//

			// The non-explicit function arguments.
			// These are our variable number of export parameters.
			VariableArguments::keyword_arguments_type unused_keyword_args;

			// Define the explicit function argument types...
			typedef boost::tuple<
					FeatureCollectionSequenceFunctionArgument,
					RotationModelFunctionArgument,
					resolved_topologies_argument_type,
					GPlatesPropertyValues::GeoTimeInstant,
					boost::optional<resolved_topological_sections_argument_type>,
					boost::optional<GPlatesModel::integer_plate_id_type>>
							resolve_topologies_args_type;

			// Define the explicit function argument names...
			const boost::tuple<const char *, const char *, const char *, const char *, const char *, const char *>
					explicit_arg_names(
							"topological_features",
							"rotation_model",
							"resolved_topologies",
							"reconstruction_time",
							"resolved_topological_sections",
							"anchor_plate_id");

			// Define the default function arguments...
			typedef boost::tuple<
					boost::optional<resolved_topological_sections_argument_type>,
					boost::optional<GPlatesModel::integer_plate_id_type>>
							default_args_type;
			default_args_type defaults_args(boost::none, boost::none/*anchor_plate_id*/);

			const resolve_topologies_args_type resolve_topologies_args =
					VariableArguments::get_explicit_args<resolve_topologies_args_type>(
							positional_args,
							keyword_args,
							explicit_arg_names,
							defaults_args,
							boost::none/*unused_positional_args*/,
							unused_keyword_args);

			topological_features = boost::get<0>(resolve_topologies_args);
			rotation_model = boost::get<1>(resolve_topologies_args);
			resolved_topologies = boost::get<2>(resolve_topologies_args);
			reconstruction_time = boost::get<3>(resolve_topologies_args);
			resolved_topological_sections = boost::get<4>(resolve_topologies_args);
			anchor_plate_id = boost::get<5>(resolve_topologies_args);

			//
			// Get the optional non-explicit output parameters from the variable argument list.
			//

			resolve_topology_types =
					VariableArguments::extract_and_remove_or_default<ResolveTopologyType::flags_type>(
							unused_keyword_args,
							"resolve_topology_types",
							ResolveTopologyType::BOUNDARY | ResolveTopologyType::NETWORK);

			resolve_topological_section_types =
					VariableArguments::extract_and_remove_or_default<ResolveTopologyType::flags_type>(
							unused_keyword_args,
							"resolve_topological_section_types",
							// Defaults to the value of 'resolve_topology_types'...
							resolve_topology_types);

			export_wrap_to_dateline =
					VariableArguments::extract_and_remove_or_default<bool>(
							unused_keyword_args,
							"export_wrap_to_dateline",
							true);

			export_force_boundary_orientation =
					VariableArguments::extract_and_remove_or_default<
									boost::optional<GPlatesMaths::PolygonOrientation::Orientation> >(
							unused_keyword_args,
							"export_force_boundary_orientation",
							boost::none);

			// Raise a python error if there are any unused keyword arguments remaining.
			// These will be keywords that we didn't recognise.
			VariableArguments::raise_python_error_if_unused(unused_keyword_args);
		}
	}


	/**
	 * Resolve topological feature collections, optionally loaded from files, to a specific geological time
	 * and optionally export to file(s).
	 *
	 * The function signature enables us to use bp::raw_function to get variable keyword arguments and
	 * also more flexibility in function overloading.
	 *
	 * We must return a value (required by 'bp::raw_function') so we just return Py_None.
	 */
	bp::object
	resolve_topologies(
			bp::tuple positional_args,
			bp::dict keyword_args)
	{
		//
		// Get the explicit function arguments from the variable argument list.
		//

		boost::optional<FeatureCollectionSequenceFunctionArgument> topological_features_argument;
		boost::optional<RotationModelFunctionArgument> rotation_model_argument;
		resolved_topologies_argument_type resolved_topologies_argument;
		GPlatesPropertyValues::GeoTimeInstant reconstruction_time(0);
		boost::optional<resolved_topological_sections_argument_type> resolved_topological_sections_argument;
		boost::optional<GPlatesModel::integer_plate_id_type> anchor_plate_id;
		ResolveTopologyType::flags_type resolve_topology_types;
		ResolveTopologyType::flags_type resolve_topological_section_types;
		bool export_wrap_to_dateline;
		boost::optional<GPlatesMaths::PolygonOrientation::Orientation> export_force_boundary_orientation;

		get_resolve_topologies_args(
				positional_args,
				keyword_args,
				topological_features_argument,
				rotation_model_argument,
				resolved_topologies_argument,
				reconstruction_time,
				resolved_topological_sections_argument,
				anchor_plate_id,
				resolve_topology_types,
				resolve_topological_section_types,
				export_wrap_to_dateline,
				export_force_boundary_orientation);

		// Time must not be distant past/future.
		if (!reconstruction_time.is_real())
		{
			PyErr_SetString(PyExc_ValueError,
				"Time values cannot be distant-past (float('inf')) or distant-future (float('-inf')).");
			bp::throw_error_already_set();
		}

		// Resolved topology type flags must correspond to existing flags.
		if ((resolve_topology_types & ~ResolveTopologyType::ALL_RESOLVE_TOPOLOGY_TYPES) != 0)
		{
			PyErr_SetString(PyExc_ValueError, "Unknown bit flag specified in resolve topology types.");
			bp::throw_error_already_set();
		}
		// Resolved topological section type flags must correspond to BOUNDARY and/or NETWORK.
		if ((resolve_topological_section_types & ~ResolveTopologyType::BOUNDARY_AND_NETWORK_RESOLVE_TOPOLOGY_TYPES) != 0)
		{
			PyErr_SetString(PyExc_ValueError, "Bit flags specified in resolve topological section types must be "
					"ResolveTopologyType.BOUNDARY and/or ResolveTopologyType.NETWORK.");
			bp::throw_error_already_set();
		}

		//
		// Resolve the topologies (as a topological snapshot).
		//

		TopologicalSnapshot::non_null_ptr_type topological_snapshot = TopologicalSnapshot::create(
				topological_features_argument.get(),
				rotation_model_argument.get(),
				reconstruction_time.value(),
				anchor_plate_id);

		//
		// Either export the resolved topologies to a file or append them to a python list.
		//

		if (const QString *resolved_topologies_export_file_name =
			boost::get<QString>(&resolved_topologies_argument))
		{
			// Export resolved topologies.
			topological_snapshot->export_resolved_topologies(
					*resolved_topologies_export_file_name,
					resolve_topology_types,
					export_wrap_to_dateline,
					export_force_boundary_orientation);
		}
		else // list of resolved topologies...
		{
			// Gather all the resolved topologies to output (limited to the resolve types requested).
			std::vector<GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_type> resolved_topologies =
					topological_snapshot->get_resolved_topologies(
							resolve_topology_types,
							// Sort the resolved topologies in the order of the features in the
							// topological files (and the order across files) since we promise this in the docs...
							true/*same_order_as_topological_features*/);

			// The caller's Python list.
			bp::list output_resolved_topologies_list = boost::get<bp::list>(resolved_topologies_argument);

			// Add to the list.
			for (auto resolved_topology : resolved_topologies)
			{
				output_resolved_topologies_list.append(resolved_topology);
			}
		}

		if (resolved_topological_sections_argument)
		{
			//
			// Either export the resolved topological sections to a file or append them to a python list.
			//

			if (const QString *resolved_topological_sections_export_file_name =
				boost::get<QString>(&resolved_topological_sections_argument.get()))
			{
				// Export resolved topological sections.
				topological_snapshot->export_resolved_topological_sections(
						*resolved_topological_sections_export_file_name,
						resolve_topological_section_types,
						export_wrap_to_dateline);
			}
			else // list of resolved topologies...
			{
				// Gather all the resolved topological sections to output (limited to the resolve types requested).
				std::vector<GPlatesAppLogic::ResolvedTopologicalSection::non_null_ptr_type> resolved_topological_sections =
						topological_snapshot->get_resolved_topological_sections(
								resolve_topological_section_types,
								// Sort the resolved topological sections in the order of the features in the
								// topological files (and the order across files) since we promise this in the docs...
								true/*same_order_as_topological_features*/);

				// The caller's Python list.
				bp::list output_resolved_topological_sections_list = boost::get<bp::list>(resolved_topological_sections_argument.get());

				// Add to the list.
				for (auto resolved_topological_section : resolved_topological_sections)
				{
					output_resolved_topological_sections_list.append(resolved_topological_section);
				}
			}
		}

		// We must return a value (required by 'bp::raw_function') so just return Py_None.
		return bp::object();
	}
}

	
void
export_resolve_topologies()
{
	// An enumeration nested within 'pygplates' (ie, current) module.
	bp::enum_<GPlatesApi::ResolveTopologyType::Value>("ResolveTopologyType")
			.value("line", GPlatesApi::ResolveTopologyType::LINE)
			.value("boundary", GPlatesApi::ResolveTopologyType::BOUNDARY)
			.value("network", GPlatesApi::ResolveTopologyType::NETWORK);


	//
	// TopologicalSnapshot - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesApi::TopologicalSnapshot,
			GPlatesApi::TopologicalSnapshot::non_null_ptr_type,
			boost::noncopyable>(
					"TopologicalSnapshot",
					"A snapshot of topologies at a specific geological time.\n"
					"\n"
					"  .. versionadded:: 30\n",
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
							bp::arg("anchor_plate_id") = boost::optional<GPlatesModel::integer_plate_id_type>())),
			"__init__(topological_features, rotation_model, reconstruction_time, [anchor_plate_id])\n"
			"  Create from topological features and a rotation model at a specific reconstruction time.\n"
			"\n"
			"  :param topological_features: the topological boundary and/or network features and the "
			"topological section features they reference (regular and topological lines) as a feature collection, "
			"or filename, or feature, or sequence of features, or a sequence (eg, ``list`` or ``tuple``) "
			"of any combination of those four types\n"
			"  :type topological_features: :class:`FeatureCollection`, or string, or :class:`Feature`, "
			"or sequence of :class:`Feature`, or sequence of any combination of those four types\n"
			"  :param rotation_model: A rotation model or a rotation feature collection or a rotation "
			"filename or a sequence of rotation feature collections and/or rotation filenames\n"
			"  :type rotation_model: :class:`RotationModel` or :class:`FeatureCollection` or string "
			"or sequence of :class:`FeatureCollection` instances and/or strings\n"
			"  :param reconstruction_time: the specific geological time to resolve to\n"
			"  :type reconstruction_time: float or :class:`GeoTimeInstant`\n"
			"  :param anchor_plate_id: The anchored plate id used for all reconstructions "
			"(resolving topologies, and reconstructing regular features). "
			"Defaults to the default anchor plate of *rotation_model*.\n"
			"  :type anchor_plate_id: int\n"
			"\n"
			"  Create a topological snapshot by resolving topologies at a specific reconstruction time:\n"
			"  ::\n"
			"\n"
			"    reconstruction_time = 100\n"
			"    topology_features = pygplates.FeatureCollection('topologies.gpml')\n"
			"    rotation_model = pygplates.RotationModel('rotations.rot')\n"
			"    topological_snapshot = pygplates.TopologicalSnapshot(topology_features, rotation_model, reconstruction_time)\n")
		.def("get_resolved_topologies",
				&GPlatesApi::topological_snapshot_get_resolved_topologies,
				(bp::arg("resolve_topology_types") = GPlatesApi::ResolveTopologyType::DEFAULT_RESOLVE_TOPOLOGY_TYPES,
					bp::arg("same_order_as_topological_features") = false),
				"get_resolved_topologies([resolve_topology_types], [same_order_as_topological_features=False])\n"
				"  Returns the resolved topologies of the requested type(s).\n"
				"\n"
				"  :param resolve_topology_types: specifies the resolved topology types to return - defaults "
				"to :class:`resolved topological boundaries<ResolvedTopologicalBoundary>` and "
				":class:`resolved topological networks<ResolvedTopologicalNetwork>`\n"
				"  :type resolve_topology_types: a bitwise combination of any of ``pygplates.ResolveTopologyType.line``, "
				"``pygplates.ResolveTopologyType.boundary`` or ``pygplates.ResolveTopologyType.network``\n"
				"  :param same_order_as_topological_features: whether the returned resolved topologies are sorted in "
				"the order of the topological features (including order across topological files, if there were any) - "
				"defaults to ``False``\n"
				"  :type same_order_as_topological_features: bool\n"
				"  :returns: the :class:`resolved topological lines<ResolvedTopologicalLine>`, "
				":class:`resolved topological boundaries<ResolvedTopologicalBoundary>` and "
				":class:`resolved topological networks<ResolvedTopologicalNetwork>` (depending on the "
				"optional argument *resolve_topology_types*) - by default "
				":class:`resolved topological lines<ResolvedTopologicalLine>` are excluded\n"
				"  :rtype: ``list``\n"
				"  :raises: ValueError if *resolve_topology_types* (if specified) contains a flag that "
				"is not one of ``pygplates.ResolveTopologyType.line``, ``pygplates.ResolveTopologyType.boundary`` or "
				"``pygplates.ResolveTopologyType.network``\n")
		.def("export_resolved_topologies",
				&GPlatesApi::topological_snapshot_export_resolved_topologies,
				(bp::arg("export_filename"),
					bp::arg("resolve_topology_types") = GPlatesApi::ResolveTopologyType::DEFAULT_RESOLVE_TOPOLOGY_TYPES,
					bp::arg("wrap_to_dateline") = true,
					bp::arg("force_boundary_orientation") = boost::optional<GPlatesMaths::PolygonOrientation::Orientation>()),
				"export_resolved_topologies(export_filename, [resolve_topology_types], [wrap_to_dateline=True], [force_boundary_orientation])\n"
				"  Exports the resolved topologies to a file.\n"
				"\n"
				"  :param export_filename: the name of the export file\n"
				"  :type export_filename: string\n"
				"  :param resolve_topology_types: specifies the resolved topology types to export - defaults "
				"to :class:`resolved topological boundaries<ResolvedTopologicalBoundary>` and "
				":class:`resolved topological networks<ResolvedTopologicalNetwork>` "
				"(excludes :class:`resolved topological lines<ResolvedTopologicalLine>`)\n"
				"  :type resolve_topology_types: a bitwise combination of any of ``pygplates.ResolveTopologyType.line``, "
				"``pygplates.ResolveTopologyType.boundary`` or ``pygplates.ResolveTopologyType.network``\n"
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
				"  .. note:: Resolved topologies are exported in the same order as that of their "
				"respective topological features (see :meth:`constructor<__init__>`) and the order across "
				"topological feature collections (if any) is also retained.\n")
		.def("get_resolved_topological_sections",
				&GPlatesApi::topological_snapshot_get_resolved_topological_sections,
				(bp::arg("resolve_topological_section_types") = GPlatesApi::ResolveTopologyType::DEFAULT_RESOLVE_TOPOLOGICAL_SECTION_TYPES,
					bp::arg("same_order_as_topological_features") = false),
				"get_resolved_topological_sections([resolve_topological_section_types], [same_order_as_topological_features=False])\n"
				"  Returns the resolved topological sections of the requested type(s).\n"
				"\n"
				"  :param resolve_topological_section_types: Determines whether :class:`ResolvedTopologicalBoundary` or "
				":class:`ResolvedTopologicalNetwork` (or both types) are listed in the returned resolved topological sections. "
				"Note that ``ResolveTopologyType.line`` cannot be specified since only topologies with boundaries are considered. "
				"Defaults to :class:`resolved topological boundaries<ResolvedTopologicalBoundary>` and "
				":class:`resolved topological networks<ResolvedTopologicalNetwork>`.\n"
				"  :type resolve_topological_section_types: a bitwise combination of any of "
				"``pygplates.ResolveTopologyType.boundary`` or ``pygplates.ResolveTopologyType.network``\n"
				"  :param same_order_as_topological_features: whether the returned resolved topological sections are sorted in "
				"the order of the topological features (including order across topological files, if there were any) - "
				"defaults to ``False``\n"
				"  :type same_order_as_topological_features: bool\n"
				"  :rtype: ``list`` of :class:`ResolvedTopologicalSection`\n"
				"  :raises: ValueError if *resolve_topological_section_types* (if specified) contains a flag that "
				"is not one of ``pygplates.ResolveTopologyType.boundary`` or ``pygplates.ResolveTopologyType.network``\n")
		.def("export_resolved_topological_sections",
				&GPlatesApi::topological_snapshot_export_resolved_topological_sections,
				(bp::arg("export_filename"),
					bp::arg("resolve_topological_section_types") = GPlatesApi::ResolveTopologyType::DEFAULT_RESOLVE_TOPOLOGY_TYPES,
					bp::arg("wrap_to_dateline") = true),
				"export_resolved_topological_sections(export_filename, [resolve_topological_section_types], [wrap_to_dateline=True])\n"
				"  Exports the resolved topological sections to a file.\n"
				"\n"
				"  :param export_filename: the name of the export file\n"
				"  :type export_filename: string\n"
				"  :param resolve_topological_section_types: Determines whether :class:`ResolvedTopologicalBoundary` or "
				":class:`ResolvedTopologicalNetwork` (or both types) are listed in the exported resolved topological sections. "
				"Note that ``ResolveTopologyType.line`` cannot be specified since only topologies with boundaries are considered. "
				"Defaults to :class:`resolved topological boundaries<ResolvedTopologicalBoundary>` and "
				":class:`resolved topological networks<ResolvedTopologicalNetwork>`.\n"
				"  :type resolve_topological_section_types: a bitwise combination of any of "
				"``pygplates.ResolveTopologyType.boundary`` or ``pygplates.ResolveTopologyType.network``\n"
				"  :param wrap_to_dateline: Whether to wrap/clip resolved topological sections to the dateline "
				"(currently ignored unless exporting to an ESRI Shapefile format *file*). Defaults to ``True``.\n"
				"  :type wrap_to_dateline: bool\n"
				"  :raises: ValueError if *resolve_topological_section_types* (if specified) contains a flag that "
				"is not one of ``pygplates.ResolveTopologyType.boundary`` or ``pygplates.ResolveTopologyType.network``\n"
				"\n"
				"  .. note:: Resolved topological sections are exported in the same order as that of their "
				"respective topological features (see :meth:`constructor<__init__>`) and the order across "
				"topological feature collections (if any) is also retained.\n")
		.def("get_rotation_model",
				&GPlatesApi::TopologicalSnapshot::get_rotation_model,
				"get_rotation_model()\n"
				"  Return the rotation model used internally.\n"
				"\n"
				"  :rtype: :class:`RotationModel`\n"
				"\n"
				"  .. note:: The :meth:`default anchor plate ID<RotationModel.get_default_anchor_plate_id>` of the returned rotation model "
				"may be different to the rotation model passed into the :meth:`constructor<__init__>` if an anchor plate ID was specified "
				"in the :meth:`constructor<__init__>`.\n")
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

	const char *resolve_topologies_function_name = "resolve_topologies";
	bp::def(resolve_topologies_function_name, bp::raw_function(&GPlatesApi::resolve_topologies));

	// Seems we cannot set a docstring using non-class bp::def combined with bp::raw_function,
	// or any case where the second argument of bp::def is a bp::object,
	// so we set the docstring the old-fashioned way.
	bp::scope().attr(resolve_topologies_function_name).attr("__doc__") =
			"resolve_topologies(topological_features, rotation_model, resolved_topologies, "
			"reconstruction_time, [resolved_topological_sections], [anchor_plate_id], [\\*\\*output_parameters])\n"
			"  Resolve topological features (lines, boundaries and networks) to a specific geological time.\n"
			"\n"
			"  :param topological_features: the topological boundary and network features and the "
			"topological section features they reference (regular and topological lines) as a feature collection, "
			"or filename, or feature, or sequence of features, or a sequence (eg, ``list`` or ``tuple``) "
			"of any combination of those four types\n"
			"  :type topological_features: :class:`FeatureCollection`, or string, or :class:`Feature`, "
			"or sequence of :class:`Feature`, or sequence of any combination of those four types\n"
			"  :param rotation_model: A rotation model or a rotation feature collection or a rotation "
			"filename or a sequence of rotation feature collections and/or rotation filenames\n"
			"  :type rotation_model: :class:`RotationModel` or :class:`FeatureCollection` or string "
			"or sequence of :class:`FeatureCollection` instances and/or strings\n"
			"  :param resolved_topologies: the "
			":class:`resolved topological lines<ResolvedTopologicalLine>`, "
			":class:`resolved topological boundaries<ResolvedTopologicalBoundary>` and "
			":class:`resolved topological networks<ResolvedTopologicalNetwork>` (depending on the optional "
			"keyword argument *resolve_topology_types* - see *output_parameters* table) are either exported "
			"to a file (with specified filename) or *appended* to a python ``list`` (note that the list is "
			"*not* cleared first)\n"
			"  :type resolved_topologies: string or ``list``\n"
			"  :param reconstruction_time: the specific geological time to resolve to\n"
			"  :type reconstruction_time: float or :class:`GeoTimeInstant`\n"
			"  :param resolved_topological_sections: The :class:`resolved topological sections<ResolvedTopologicalSection>` "
			" are either exported to a file (with specified filename) or *appended* to a python ``list`` "
			"(note that the list is *not* cleared first). Default is to do neither.\n"
			"  :type resolved_topological_sections: string or ``list``\n"
			"  :param anchor_plate_id: The anchored plate id used during reconstruction. "
			"Defaults to the default anchor plate of *rotation_model*.\n"
			"  :type anchor_plate_id: int\n"
			"  :param output_parameters: Variable number of keyword arguments specifying output "
			"parameters (see table below). Default is no keyword arguments.\n"
			"  :raises: OpenFileForReadingError if any input file is not readable (when filenames specified)\n"
			"  :raises: OpenFileForWritingError if *resolved_topologies* is a filename and it is not writeable\n"
			"  :raises: FileFormatNotSupportedError if any input file format (identified by any "
			"topological and rotation filename extensions) does not support reading "
			"(when filenames specified)\n"
			"  :raises: ValueError if *reconstruction_time* is "
			":meth:`distant past<GeoTimeInstant.is_distant_past>` or "
			":meth:`distant future<GeoTimeInstant.is_distant_future>`\n"
			"  :raises: ValueError if *resolve_topology_types* (if specified) contains a flag that "
			"is not one of ``pygplates.ResolveTopologyType.line``, ``pygplates.ResolveTopologyType.boundary`` or "
			"``pygplates.ResolveTopologyType.network``\n"
			"  :raises: ValueError if *resolve_topological_section_types* (if specified) contains a flag that "
			"is not one of ``pygplates.ResolveTopologyType.boundary`` or ``pygplates.ResolveTopologyType.network``\n"
			"\n"
			"  The following optional keyword arguments are supported by *output_parameters*:\n"
			"\n"
			"  +-----------------------------------+------+-----------------------------------------------------------------+----------------------------------------------------------------------------------+\n"
			"  | Name                              | Type | Default                                                         | Description                                                                      |\n"
			"  +===================================+======+=================================================================+==================================================================================+\n"
			"  | resolve_topology_types            | int  | ``ResolveTopologyType.boundary | ResolveTopologyType.network``  | A bitwise combination of any of the following:                                   |\n"
			"  |                                   |      |                                                                 |                                                                                  |\n"
			"  |                                   |      |                                                                 | - ``ResolveTopologyType.line``:                                                  |\n"
			"  |                                   |      |                                                                 | - ``ResolveTopologyType.boundary``:                                              |\n"
			"  |                                   |      |                                                                 | - ``ResolveTopologyType.network``:                                               |\n"
			"  |                                   |      |                                                                 |                                                                                  |\n"
			"  |                                   |      |                                                                 | Determines whether to generate :class:`ResolvedTopologicalLine`,                 |\n"
			"  |                                   |      |                                                                 | :class:`ResolvedTopologicalBoundary` and :class:`ResolvedTopologicalNetwork`.    |\n"
			"  +-----------------------------------+------+-----------------------------------------------------------------+----------------------------------------------------------------------------------+\n"
			"  | resolve_topological_section_types | int  | ``ResolveTopologyType.boundary | ResolveTopologyType.network``  | A bitwise combination of any of the following:                                   |\n"
			"  |                                   |      |                                                                 |                                                                                  |\n"
			"  |                                   |      |                                                                 | - ``ResolveTopologyType.boundary``:                                              |\n"
			"  |                                   |      |                                                                 | - ``ResolveTopologyType.network``:                                               |\n"
			"  |                                   |      |                                                                 |                                                                                  |\n"
			"  |                                   |      |                                                                 | .. note:: ``ResolveTopologyType.line`` is excluded since only                    |\n"
			"  |                                   |      |                                                                 |    topologies with boundaries are considered.                                    |\n"
			"  |                                   |      |                                                                 |                                                                                  |\n"
			"  |                                   |      |                                                                 | Determines whether :class:`ResolvedTopologicalBoundary` or                       |\n"
			"  |                                   |      |                                                                 | :class:`ResolvedTopologicalNetwork` (or both types) are listed in the            |\n"
			"  |                                   |      |                                                                 | :class:`resolved topological sections<pygplates.ResolvedTopologicalSection>`     |\n"
			"  |                                   |      |                                                                 | of *resolved_topological_sections*.                                              |\n"
			"  +-----------------------------------+------+-----------------------------------------------------------------+----------------------------------------------------------------------------------+\n"
			"  | export_wrap_to_dateline           | bool | True                                                            | | Wrap/clip resolved topologies to the dateline (currently                       |\n"
			"  |                                   |      |                                                                 |   ignored unless exporting to an ESRI Shapefile format *file*).                  |\n"
			"  |                                   |      |                                                                 |                                                                                  |\n"
			"  |                                   |      |                                                                 | .. note:: Only applies when exporting to a file (ESRI Shapefile).                |\n"
			"  +-----------------------------------+------+-----------------------------------------------------------------+----------------------------------------------------------------------------------+\n"
			"  | export_force_boundary_orientation | int  | ``None`` (don't force)                                          | Optionally force boundary orientation (clockwise or counter-clockwise):          |\n"
			"  |                                   |      |                                                                 |                                                                                  |\n"
			"  |                                   |      |                                                                 | - ``PolygonOnSphere.Orientation.clockwise``                                      |\n"
			"  |                                   |      |                                                                 | - ``PolygonOnSphere.Orientation.counter_clockwise``                              |\n"
			"  |                                   |      |                                                                 |                                                                                  |\n"
			"  |                                   |      |                                                                 | .. note:: Only applies to resolved topological *boundaries* and *networks*.      |\n"
			"  |                                   |      |                                                                 |                                                                                  |\n"
			"  |                                   |      |                                                                 | .. note:: ESRI Shapefiles always use *clockwise* orientation.                    |\n"
			"  |                                   |      |                                                                 |                                                                                  |\n"
			"  |                                   |      |                                                                 | .. warning:: Only applies when exporting to a **file** (except ESRI Shapefile).  |\n"
			"  +-----------------------------------+------+-----------------------------------------------------------------+----------------------------------------------------------------------------------+\n"
			"\n"
			"  | The argument *topological_features* consists of the *topological* :class:`features<Feature>` "
			"as well as the topological sections (also :class:`features<Feature>`) that are referenced by the "
			"*topological* features.\n"
			"  | They can all be mixed in a single :class:`feature collection<FeatureCollection>` or file, "
			"or they can be distributed across multiple :class:`feature collections<FeatureCollection>` or files.\n"
			"  | For example the dynamic polygons in the `GPlates sample data <http://www.gplates.org/download.html#download_data>`_ "
			"have everything in a single file.\n"
			"\n"
			"  .. note:: Topological *sections* can be regular features or topological *line* features. "
			"The latter are typically used for sections of a plate polygon (or network) boundary that are deforming.\n"
			"\n"
			"  | The argument *resolved_topologies* can be either an export filename or a python ``list``.\n"
			"  | In the latter case the resolved topologies generated by the reconstruction are appended "
			"to the python ``list`` (instead of exported to a file).\n"
			"\n"
			"  | A similar argument *resolved_topological_sections* can also be either an export filename or a python ``list``.\n"
			"  | In the latter case the :class:`resolved topological sections<pygplates.ResolvedTopologicalSection>` "
			"generated by the reconstruction are appended to the python ``list`` (instead of exported to a file).\n"
			"\n"
			"  | Both *resolved_topologies* and *resolved_topological_sections* are output in the same order as that of their "
			"respective features in *topological_features* (the order across feature collections is also retained). "
			"This happens regardless of whether *topological_features*, and *resolved_topologies* and *resolved_topological_sections*, "
			"include files or not.\n"
			"\n"
			"  .. note:: | :class:`Resolved topological sections<pygplates.ResolvedTopologicalSection>` can be used "
			"to find the unique (non-overlapping) set of boundary sub-segments that are shared by the resolved topologies.\n"
			"            | Each resolved topology also has a list of its boundary sub-segments but they overlap with the "
			"boundary sub-segments of neighbouring topologies.\n"
			"\n"
			"  | The optional keyword argument *resolve_topology_types* (see *output_parameters* table) determines "
			"the type of resolved topologies output to *resolved_topologies*.\n"
			"  | This can consist of "
			":class:`resolved topological lines<ResolvedTopologicalLine>`, "
			":class:`resolved topological boundaries<ResolvedTopologicalBoundary>` and "
			":class:`resolved topological networks<ResolvedTopologicalNetwork>` (and any combination of them).\n"
			"  | By default only "
			":class:`resolved topological boundaries<ResolvedTopologicalBoundary>` and "
			":class:`resolved topological networks<ResolvedTopologicalNetwork>` are output since "
			":class:`resolved topological lines<ResolvedTopologicalLine>` are typically only used as "
			"topological sections for resolved topological boundaries and networks.\n"
			"\n"
			"  | A similar optional keyword argument is *resolve_topological_section_types* (see *output_parameters* table).\n"
			"  | This determines which resolved topology types are listed in the "
			":meth:`shared sub-segments<ResolvedTopologicalSharedSubSegment.get_sharing_resolved_topologies>` "
			"of the *resolved_topological_sections*.\n"
			"\n"
			"  The following *export* file formats are currently supported by GPlates:\n"
			"\n"
			"  =============================== =======================\n"
			"  Export File Format              Filename Extension     \n"
			"  =============================== =======================\n"
			"  ESRI Shapefile                  '.shp'                 \n"
			"  OGR GMT                         '.gmt'                 \n"
			"  GMT xy                          '.xy'                  \n"
			"  =============================== =======================\n"
			"\n"
			"  .. warning:: | Currently, resolved topological **networks** exported to *OGR GMT* or "
			"*ESRI Shapefile* will not be loaded if the exported file is subsequently loaded into "
			"`GPlates <http://www.gplates.org>`_.\n"
			"               | The resolved topological networks will still be in the exported file though.\n"
			"\n"
			"  .. note:: When exporting to a file, the filename extension of *resolved_topologies* "
			"determines the export file format.\n"
			"\n"
			"  .. note:: *topological_features* can be a :class:`FeatureCollection` or a filename "
			"or a :class:`Feature` or a sequence of :class:`features<Feature>`, or a sequence (eg, ``list`` "
			"or ``tuple``) of any combination of those four types.\n"
			"\n"
			"  .. note:: *rotation_model* can be either a :class:`RotationModel` or a "
			"rotation :class:`FeatureCollection` or a rotation filename or a sequence "
			"(eg, ``list`` or ``tuple``) containing rotation :class:`FeatureCollection` instances "
			"or filenames (or a mixture of both). When a :class:`RotationModel` is not specified "
			"then a temporary one is created internally (and hence is less efficient if this "
			"function is called multiple times with the same rotation data).\n"
			"\n"
			"  If any filenames are specified then :class:`FeatureCollectionFileFormatRegistry` is "
			"used internally to read feature collections from those files.\n"
			"\n"
			"  Resolving a file containing dynamic plate polygons to a shapefile at 10Ma:\n"
			"  ::\n"
			"\n"
            "    pygplates.resolve_topologies(\n"
			"        'dynamic_plate_polygons.gpml', 'rotations.rot', 'resolved_plate_polygons_10Ma.shp', 10)\n"
			"\n"
			"  | Resolving the same file but also exporting resolved topological sections.\n"
			"  | These are the unique (non-duplicated) segments (shared by neighbouring topology boundaries).\n"
			"\n"
			"  ::\n"
			"\n"
            "    pygplates.resolve_topologies(\n"
			"       'dynamic_plate_polygons.gpml', 'rotations.rot', 'resolved_plate_polygons_10Ma.shp', 10,\n"
			"       'resolved_plate_segments_10Ma.shp')\n"
			"\n"
			"  Resolving only topological networks in a file containing both dynamic plate polygons and deforming networks:\n"
			"  ::\n"
			"\n"
            "    pygplates.resolve_topologies(\n"
			"        'plate_polygons_and_networks.gpml', 'rotations.rot', 'resolved_networks_10Ma.shp', 10,\n"
			"        resolve_topology_types=pygplates.ResolveTopologyType.network)\n"
			"\n"
			"  Writing only resolved networks to ``resolved_networks_10Ma.shp`` but writing shared boundary segments "
			"between resolved plate polygons *and* networks to ``resolved_boundary_segments_10Ma.shp``:\n"
			"  ::\n"
			"\n"
            "    pygplates.resolve_topologies(\n"
			"        'plate_polygons_and_networks.gpml', 'rotations.rot', 'resolved_networks_10Ma.shp', 10,\n"
			"        'resolved_boundary_segments_10Ma.shp',\n"
			"        resolve_topology_types=pygplates.ResolveTopologyType.network,\n"
			"        resolve_topological_section_types=pygplates.ResolveTopologyType.boundary | pygplates.ResolveTopologyType.network)\n"
			"\n"
			"  Resolving to a list of topologies and a list of topological sections:\n"
			"  ::\n"
			"\n"
			"    resolved_topologies = []\n"
			"    resolved_topological_sections = []\n"
            "    pygplates.resolve_topologies(\n"
			"        'plate_polygons_and_networks.gpml', 'rotations.rot', resolved_topologies, 10,\n"
			"         resolved_topological_sections)\n"
			"\n"
			"  .. versionchanged:: 29\n"
			"     The output order of *resolved_topological_sections* is now same as that of their "
			"respective features in *topological_features* (the order across feature collections is also retained). "
			"Previously the order was only retained for *resolved_topologies*.\n";

	// Register 'resolved topologies' variant.
	GPlatesApi::PythonConverterUtils::register_variant_conversion<GPlatesApi::resolved_topologies_argument_type>();

	// Register 'resolved topological sections' variant.
	GPlatesApi::PythonConverterUtils::register_variant_conversion<GPlatesApi::resolved_topological_sections_argument_type>();
	// Enable boost::optional<resolved_topological_sections_argument_type> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesApi::resolved_topological_sections_argument_type>();
}
