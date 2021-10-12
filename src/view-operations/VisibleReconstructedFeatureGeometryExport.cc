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

#include "VisibleReconstructedFeatureGeometryExport.h"

#include "RenderedGeometryUtils.h"
#include "RenderedGeometryCollection.h"
#include "app-logic/ReconstructionGeometryUtils.h"
#include "file-io/ReconstructedFeatureGeometryExport.h"
#include "model/ReconstructedFeatureGeometry.h"


namespace
{
	//! Convenience typedef for sequence of RFGs.
	typedef GPlatesFileIO::ReconstructedFeatureGeometryExport::reconstructed_feature_geom_seq_type
			reconstructed_feature_geom_seq_type;
}


void
GPlatesViewOperations::VisibleReconstructedFeatureGeometryExport::export_visible_geometries(
		const QString &filename,
		const GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
		const files_collection_type &reconstructable_files,
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
	GPlatesFileIO::ReconstructedFeatureGeometryExport::export_geometries(
			filename,
			reconstruct_feature_geom_seq,
			reconstructable_files,
			reconstruction_anchor_plate_id,
			reconstruction_time);
}
