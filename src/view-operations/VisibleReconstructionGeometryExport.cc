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

#include <vector>

#include "VisibleReconstructionGeometryExport.h"

#include "RenderedGeometryUtils.h"
#include "RenderedGeometryCollection.h"

#include "app-logic/ReconstructionGeometryUtils.h"
#include "file-io/ReconstructedFeatureGeometryExport.h"
#include "file-io/ReconstructedFlowlineExport.h"
#include "file-io/ReconstructedMotionTrackExport.h"

namespace
{
	//! Convenience typedef for sequence of RFGs.
	typedef GPlatesFileIO::ReconstructedFeatureGeometryExport::reconstructed_feature_geom_seq_type
			reconstructed_feature_geom_seq_type;

	//! Convenience typedef for sequence of reconstructed flowline geometries.
	typedef GPlatesFileIO::ReconstructedFlowlineExport::reconstructed_flowline_seq_type
			reconstructed_flowline_seq_type;

	//! Convenience typedef for sequence of reconstructed motion track geometries.
	typedef GPlatesFileIO::ReconstructedMotionTrackExport::reconstructed_motion_track_seq_type
			reconstructed_motion_track_seq_type;
}


void
GPlatesViewOperations::VisibleReconstructionGeometryExport::export_visible_reconstructed_feature_geometries(
		const QString &filename,
		const GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
		const files_collection_type &active_files,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const double &reconstruction_time)
{
	// Get any ReconstructionGeometry objects that are visible in any active layers
	// of the RenderedGeometryCollection.
	RenderedGeometryUtils::reconstruction_geom_seq_type reconstruction_geom_seq;
	RenderedGeometryUtils::get_unique_reconstruction_geometries(
			reconstruction_geom_seq,
			rendered_geom_collection);

	// Get any ReconstructionGeometry objects that are of type ReconstructedFeatureGeometry.
	reconstructed_feature_geom_seq_type reconstruct_feature_geom_seq;
	GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type_sequence(
			reconstruction_geom_seq.begin(),
			reconstruction_geom_seq.end(),
			reconstruct_feature_geom_seq);

	// Export the RFGs to a file format based on the filename extension.
	GPlatesFileIO::ReconstructedFeatureGeometryExport::export_reconstructed_feature_geometries(
			filename,
			reconstruct_feature_geom_seq,
			active_files,
			reconstruction_anchor_plate_id,
			reconstruction_time);
}


void
GPlatesViewOperations::VisibleReconstructionGeometryExport::export_visible_reconstruced_flowlines(
	const QString &filename,
	const GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
	const files_collection_type &active_files,
	const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
	const double &reconstruction_time)
{
	// Get any ReconstructionGeometry objects that are visible in any active layers
	// of the RenderedGeometryCollection.
	RenderedGeometryUtils::reconstruction_geom_seq_type reconstruction_geom_seq;
	RenderedGeometryUtils::get_unique_reconstruction_geometries(
		reconstruction_geom_seq,
		rendered_geom_collection);

	// Get any ReconstructionGeometry objects that are of type ReconstructedFlowline.
	reconstructed_flowline_seq_type reconstructed_flowline_seq;
	GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type_sequence(
		reconstruction_geom_seq.begin(),
		reconstruction_geom_seq.end(),
		reconstructed_flowline_seq);

	// Export the flowlines to a file format based on the filename extension.
	GPlatesFileIO::ReconstructedFlowlineExport::export_reconstructed_flowlines(
		filename,
		reconstructed_flowline_seq,
		active_files,
		reconstruction_anchor_plate_id,
		reconstruction_time);
}

void
GPlatesViewOperations::VisibleReconstructionGeometryExport::export_visible_reconstruced_motion_tracks(
	const QString &filename,
	const GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
	const files_collection_type &active_files,
	const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
	const double &reconstruction_time)
{
	// Get any ReconstructionGeometry objects that are visible in any active layers
	// of the RenderedGeometryCollection.
	RenderedGeometryUtils::reconstruction_geom_seq_type reconstruction_geom_seq;
	RenderedGeometryUtils::get_unique_reconstruction_geometries(
		reconstruction_geom_seq,
		rendered_geom_collection);

	// Get any ReconstructionGeometry objects that are of type ReconstructedFlowline.
	reconstructed_motion_track_seq_type reconstructed_motion_track_seq;
	GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type_sequence(
		reconstruction_geom_seq.begin(),
		reconstruction_geom_seq.end(),
		reconstructed_motion_track_seq);

	// Export the flowlines to a file format based on the filename extension.
	GPlatesFileIO::ReconstructedMotionTrackExport::export_reconstructed_motion_tracks(
		filename,
		reconstructed_motion_track_seq,
		active_files,
		reconstruction_anchor_plate_id,
		reconstruction_time);
}
