/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#ifndef GPLATES_FILE_IO_OGRFORMATRESOLVEDTOPOLOGICALGEOMETRYXPORT_H
#define GPLATES_FILE_IO_OGRFORMATRESOLVEDTOPOLOGICALGEOMETRYXPORT_H

#include <boost/optional.hpp>
#include <QFileInfo>

#include "ReconstructionGeometryExportImpl.h"
#include "CitcomsResolvedTopologicalBoundaryExportImpl.h"

#include "maths/PolygonOrientation.h"

#include "model/types.h"


namespace GPlatesAppLogic
{
	class ReconstructionGeometry;
	class ResolvedTopologicalSection;
}

namespace GPlatesFileIO
{
	namespace OgrFormatResolvedTopologicalGeometryExport
	{
		/**
		 * Typedef for a feature geometry group of reconstruction geometries.
		 */
		typedef ReconstructionGeometryExportImpl::FeatureGeometryGroup<GPlatesAppLogic::ReconstructionGeometry>
				feature_geometry_group_type;

		/**
		 * Typedef for a sequence of referenced files.
		 */
		typedef ReconstructionGeometryExportImpl::referenced_files_collection_type
				referenced_files_collection_type;


		/**
		 * Exports resolved topology objects to OGR format.
		 *
		 * This includes @a ResolvedTopologicalLine, @a ResolvedTopologicalBoundary and @a ResolvedTopologicalNetwork.
		 *
		 * If @a export_per_collection is true then the shapefile attributes from the original features are retained.
		 * Otherwise the shapefile attributes from the original features are ignored, which is necessary if the
		 * features came from multiple input files (which might have different attribute field names making it
		 * difficult to merge into a single output).
		 *
		 * If @a force_polygon_orientation is not none then polygon are exported to the specified
		 * orientation (clockwise or counter-clockwise).
		 * NOTE: This option is essentially ignored for the *Shapefile* OGR format because the
		 * OGR Shapefile driver will overwrite our orientation (if counter-clockwise) and just
		 * store exterior rings as clockwise and interior rings as counter-clockwise.
		 *
		 * If @a wrap_to_dateline is true then exported polyline/polygon geometries are wrapped/clipped to the dateline.
		 */
		void
		export_resolved_topological_geometries(
				bool export_per_collection,
				const std::list<feature_geometry_group_type> &feature_geometry_group_seq,
				const QFileInfo& file_info,
				const referenced_files_collection_type &referenced_files,
				const referenced_files_collection_type &active_reconstruction_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time,
				boost::optional<GPlatesMaths::PolygonOrientation::Orientation>
						force_polygon_orientation = boost::none,
				bool wrap_to_dateline = true);


		/**
		 * Exports resolved topological sections to OGR format.
		 *
		 * This includes @a ResolvedTopologicalSection and its @a ResolvedTopologicalSharedSubSegment instances.
		 *
		 * If @a export_per_collection is true then the shapefile attributes from the original features are retained.
		 * Otherwise the shapefile attributes from the original features are ignored, which is necessary if the
		 * features came from multiple input files (which might have different attribute field names making it
		 * difficult to merge into a single output).
		 *
		 * If @a wrap_to_dateline is true then exported polyline geometries are wrapped/clipped to the dateline.
		 */
		void
		export_resolved_topological_sections(
				bool export_per_collection,
				const std::vector<const GPlatesAppLogic::ResolvedTopologicalSection *> &resolved_topological_sections,
				const QFileInfo& file_info,
				const referenced_files_collection_type &referenced_files,
				const referenced_files_collection_type &active_reconstruction_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time,
				bool wrap_to_dateline = true);


		/**
		* Exports @a ResolvedTopologicalGeometry objects to OGR format for use by CitcomS software.
		*
		* If @a wrap_to_dateline is true then exported polygon boundaries are wrapped/clipped to the dateline.
		*/
		void
		export_citcoms_resolved_topological_boundaries(
				const CitcomsResolvedTopologicalBoundaryExportImpl::resolved_topologies_seq_type &resolved_topological_geometries,
				const QFileInfo& file_info,
				const referenced_files_collection_type &referenced_files,
				const referenced_files_collection_type &active_reconstruction_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time,
				bool wrap_to_dateline = true);


		/**
		 * Exports subsegments of resolved topological boundaries to OGR format for use by CitcomS software.
		*
		* If @a wrap_to_dateline is true then exported geometries are wrapped/clipped to the dateline.
		 */
		void
		export_citcoms_sub_segments(
				const CitcomsResolvedTopologicalBoundaryExportImpl::sub_segment_group_seq_type &sub_segments,
				const QFileInfo& file_info,
				const referenced_files_collection_type &referenced_files,
				const referenced_files_collection_type &active_reconstruction_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time,
				bool wrap_to_dateline = true);
	}
}

#endif // GPLATES_FILE_IO_OGRFORMATRESOLVEDTOPOLOGICALGEOMETRYXPORT_H
