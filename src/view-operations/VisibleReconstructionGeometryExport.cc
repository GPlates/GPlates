/* $Id$ */

/**
 * \file Exports visible reconstructed feature geometries to a file.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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
#include <iterator>
#include <vector>
#include <boost/bind.hpp>
#include <boost/mem_fn.hpp>

#include "VisibleReconstructionGeometryExport.h"

#include "RenderedGeometryUtils.h"
#include "RenderedGeometryCollection.h"

#include "app-logic/ReconstructionGeometryUtils.h"

#include "file-io/ReconstructedFeatureGeometryExport.h"
#include "file-io/ReconstructedFlowlineExport.h"
#include "file-io/ReconstructedMotionPathExport.h"
#include "file-io/ResolvedTopologicalGeometryExport.h"


namespace
{
	//! Convenience typedef for sequence of RFGs.
	typedef std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry *>
			reconstructed_feature_geom_seq_type;

	//! Convenience typedef for sequence of reconstructed flowline geometries.
	typedef std::vector<const GPlatesAppLogic::ReconstructedFlowline *>
			reconstructed_flowline_seq_type;

	//! Convenience typedef for sequence of reconstructed motion track geometries.
	typedef std::vector<const GPlatesAppLogic::ReconstructedMotionPath *>
			reconstructed_motion_path_seq_type;

	//! Convenience typedef for sequence of resolved topological geometries.
	typedef std::vector<const GPlatesAppLogic::ResolvedTopologicalGeometry *>
			resolved_topological_geom_seq_type;
}


void
GPlatesViewOperations::VisibleReconstructionGeometryExport::export_visible_reconstructed_feature_geometries(
		const QString &filename,
		const GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
		const GPlatesFileIO::FeatureCollectionFileFormat::Registry &file_format_registry,
		const files_collection_type &active_files,
		const files_collection_type &active_reconstruction_files,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const double &reconstruction_time,
		bool export_single_output_file,
		bool export_per_input_file,
		bool export_separate_output_directory_per_input_file,
		bool wrap_to_dateline)
{
	// Get any ReconstructionGeometry objects that are visible in any active layers
	// of the RenderedGeometryCollection.
	RenderedGeometryUtils::reconstruction_geom_seq_type reconstruction_geom_seq;
	RenderedGeometryUtils::get_unique_reconstruction_geometries(
			reconstruction_geom_seq,
			rendered_geom_collection,
			// Don't want to export a duplicate reconstructed geometry if one is currently in focus...
			GPlatesViewOperations::RenderedGeometryCollection::RECONSTRUCTION_LAYER);

	// Get any ReconstructionGeometry objects that are of type ReconstructedFeatureGeometry.
	reconstructed_feature_geom_seq_type reconstruct_feature_geom_seq;
	GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type_sequence(
			reconstruction_geom_seq.begin(),
			reconstruction_geom_seq.end(),
			reconstruct_feature_geom_seq);

	// Export the RFGs to a file format based on the filename extension.
	GPlatesFileIO::ReconstructedFeatureGeometryExport::export_reconstructed_feature_geometries(
			filename,
			GPlatesFileIO::ReconstructedFeatureGeometryExport::get_export_file_format(filename, file_format_registry),
			reconstruct_feature_geom_seq,
			active_files,
			active_reconstruction_files,
			reconstruction_anchor_plate_id,
			reconstruction_time,
			export_single_output_file,
			export_per_input_file,
			export_separate_output_directory_per_input_file,
			wrap_to_dateline);
}


void
GPlatesViewOperations::VisibleReconstructionGeometryExport::export_visible_reconstructed_flowlines(
	const QString &filename,
	const GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
	const GPlatesFileIO::FeatureCollectionFileFormat::Registry &file_format_registry,
	const files_collection_type &active_files,
	const files_collection_type &active_reconstruction_files,
	const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
	const double &reconstruction_time,
	bool export_single_output_file,
	bool export_per_input_file,
	bool export_separate_output_directory_per_input_file,
	bool wrap_to_dateline)
{
	// Get any ReconstructionGeometry objects that are visible in any active layers
	// of the RenderedGeometryCollection.
	RenderedGeometryUtils::reconstruction_geom_seq_type reconstruction_geom_seq;
	RenderedGeometryUtils::get_unique_reconstruction_geometries(
		reconstruction_geom_seq,
		rendered_geom_collection,
		// Don't want to export a duplicate reconstructed flowline if one is currently in focus...
		GPlatesViewOperations::RenderedGeometryCollection::RECONSTRUCTION_LAYER);

	// Get any ReconstructionGeometry objects that are of type ReconstructedFlowline.
	reconstructed_flowline_seq_type reconstructed_flowline_seq;
	GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type_sequence(
		reconstruction_geom_seq.begin(),
		reconstruction_geom_seq.end(),
		reconstructed_flowline_seq);

	// Export the flowlines to a file format based on the filename extension.
	GPlatesFileIO::ReconstructedFlowlineExport::export_reconstructed_flowlines(
		filename,
		GPlatesFileIO::ReconstructedFlowlineExport::get_export_file_format(filename, file_format_registry),
		reconstructed_flowline_seq,
		active_files,
		active_reconstruction_files,
		reconstruction_anchor_plate_id,
		reconstruction_time,
		export_single_output_file,
		export_per_input_file,
		export_separate_output_directory_per_input_file,
		wrap_to_dateline);
}

void
GPlatesViewOperations::VisibleReconstructionGeometryExport::export_visible_reconstructed_motion_paths(
	const QString &filename,
	const GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
	const GPlatesFileIO::FeatureCollectionFileFormat::Registry &file_format_registry,
	const files_collection_type &active_files,
	const files_collection_type &active_reconstruction_files,
	const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
	const double &reconstruction_time,
	bool export_single_output_file,
	bool export_per_input_file,
	bool export_separate_output_directory_per_input_file,
	bool wrap_to_dateline)
{
	// Get any ReconstructionGeometry objects that are visible in any active layers
	// of the RenderedGeometryCollection.
	RenderedGeometryUtils::reconstruction_geom_seq_type reconstruction_geom_seq;
	RenderedGeometryUtils::get_unique_reconstruction_geometries(
		reconstruction_geom_seq,
		rendered_geom_collection,
		// Don't want to export a duplicate reconstructed motion path if one is currently in focus...
		GPlatesViewOperations::RenderedGeometryCollection::RECONSTRUCTION_LAYER);

	// Get any ReconstructionGeometry objects that are of type ReconstructedMotionPath.
	reconstructed_motion_path_seq_type reconstructed_motion_path_seq;
	GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type_sequence(
		reconstruction_geom_seq.begin(),
		reconstruction_geom_seq.end(),
		reconstructed_motion_path_seq);

	// Export the flowlines to a file format based on the filename extension.
	GPlatesFileIO::ReconstructedMotionPathExport::export_reconstructed_motion_paths(
		filename,
		GPlatesFileIO::ReconstructedMotionPathExport::get_export_file_format(filename, file_format_registry),
		reconstructed_motion_path_seq,
		active_files,
		active_reconstruction_files,
		reconstruction_anchor_plate_id,
		reconstruction_time,
		export_single_output_file,
		export_per_input_file,
		export_separate_output_directory_per_input_file,
		wrap_to_dateline);
}

void
GPlatesViewOperations::VisibleReconstructionGeometryExport::export_visible_resolved_topologies(
		const QString &filename,
		const GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
		const GPlatesFileIO::FeatureCollectionFileFormat::Registry &file_format_registry,
		const files_collection_type &active_files,
		const files_collection_type &active_reconstruction_files,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const double &reconstruction_time,
		bool export_single_output_file,
		bool export_per_input_file,
		bool export_separate_output_directory_per_input_file,
		bool export_topological_lines,
		bool export_topological_polygons,
		boost::optional<GPlatesMaths::PolygonOrientation::Orientation> force_polygon_orientation,
		bool wrap_to_dateline)
{
	// Get any ReconstructionGeometry objects that are visible in any active layers
	// of the RenderedGeometryCollection.
	RenderedGeometryUtils::reconstruction_geom_seq_type reconstruction_geom_seq;
	RenderedGeometryUtils::get_unique_reconstruction_geometries(
			reconstruction_geom_seq,
			rendered_geom_collection,
			// Don't want to export a duplicate reconstructed geometry if one is currently in focus...
			GPlatesViewOperations::RenderedGeometryCollection::RECONSTRUCTION_LAYER);

	// Get any ReconstructionGeometry objects that are of type ResolvedTopologicalGeometry.
	resolved_topological_geom_seq_type resolved_topological_geom_seq;
	GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type_sequence(
			reconstruction_geom_seq.begin(),
			reconstruction_geom_seq.end(),
			resolved_topological_geom_seq);

	// Remove topological lines if they should not be exported...
	if (!export_topological_lines)
	{
		resolved_topological_geom_seq.erase(
				std::remove_if(resolved_topological_geom_seq.begin(), resolved_topological_geom_seq.end(),
						// The explicit template parameter and boost::mem_fn were added to avoid MSVC2005 compile error...
						boost::bind< boost::optional<GPlatesAppLogic::ResolvedTopologicalGeometry::resolved_topology_line_ptr_type> >(
								boost::mem_fn(&GPlatesAppLogic::ResolvedTopologicalGeometry::resolved_topology_line),
								_1)),
				resolved_topological_geom_seq.end());
	}

	// Remove topological polygons if they should not be exported...
	if (!export_topological_polygons)
	{
		resolved_topological_geom_seq.erase(
				std::remove_if(resolved_topological_geom_seq.begin(), resolved_topological_geom_seq.end(),
						// The explicit template parameter and boost::mem_fn were added to avoid MSVC2005 compile error...
						boost::bind< boost::optional<GPlatesAppLogic::ResolvedTopologicalGeometry::resolved_topology_boundary_ptr_type> >(
								boost::mem_fn(&GPlatesAppLogic::ResolvedTopologicalGeometry::resolved_topology_boundary),
								_1)),
				resolved_topological_geom_seq.end());
	}

	// Export the RTGs to a file format based on the filename extension.
	GPlatesFileIO::ResolvedTopologicalGeometryExport::export_resolved_topological_geometries(
			filename,
			GPlatesFileIO::ResolvedTopologicalGeometryExport::get_export_file_format(filename, file_format_registry),
			resolved_topological_geom_seq,
			active_files,
			active_reconstruction_files,
			reconstruction_anchor_plate_id,
			reconstruction_time,
			export_single_output_file,
			export_per_input_file,
			export_separate_output_directory_per_input_file,
			force_polygon_orientation,
			wrap_to_dateline);
}
