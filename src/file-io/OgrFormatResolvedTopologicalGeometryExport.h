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

#include <QFileInfo>

#include "ReconstructionGeometryExportImpl.h"
#include "CitcomsResolvedTopologicalBoundaryExportImpl.h"

#include "model/types.h"


namespace GPlatesAppLogic
{
	class ResolvedTopologicalGeometry;
}

namespace GPlatesFileIO
{
	namespace OgrFormatResolvedTopologicalGeometryExport
	{
		/**
		 * Typedef for a feature geometry group of @a ResolvedTopologicalGeometry objects.
		 */
		typedef ReconstructionGeometryExportImpl::FeatureGeometryGroup<GPlatesAppLogic::ResolvedTopologicalGeometry>
				feature_geometry_group_type;

		/**
		 * Typedef for a sequence of referenced files.
		 */
		typedef ReconstructionGeometryExportImpl::referenced_files_collection_type
				referenced_files_collection_type;


		/**
		* Exports @a ResolvedTopologicalGeometry objects to OGR format.
		*
		* If @a wrap_to_dateline is true then exported polyline/polygon geometries are wrapped/clipped to the dateline.
		*/
		void
		export_geometries(
				const std::list<feature_geometry_group_type> &feature_geometry_group_seq,
				const QFileInfo& file_info,
				const referenced_files_collection_type &referenced_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time,
				bool wrap_to_dateline = false);

		/**
		* Exports @a ResolvedTopologicalGeometry objects to OGR format.
		*
		* If @a wrap_to_dateline is true then exported polyline/polygon geometries are wrapped/clipped to the dateline.
		*/
		void
		export_geometries_per_collection(
				const std::list<feature_geometry_group_type> &feature_geometry_group_seq,
				const QFileInfo& file_info,
				const std::vector<const File::Reference *> &referenced_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time,
				bool wrap_to_dateline = false);


		/**
		* Exports @a ResolvedTopologicalGeometry objects to OGR format for use by CitcomS software.
		*
		* If @a wrap_to_dateline is true then exported polygon boundaries are wrapped/clipped to the dateline.
		*/
		void
		export_citcoms_resolved_topological_boundaries(
				const CitcomsResolvedTopologicalBoundaryExportImpl::resolved_geom_seq_type &resolved_topological_geometries,
				const QFileInfo& file_info,
				const referenced_files_collection_type &referenced_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time,
				bool wrap_to_dateline = false);


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
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time,
				bool wrap_to_dateline = false);
	}
}

#endif // GPLATES_FILE_IO_OGRFORMATRESOLVEDTOPOLOGICALGEOMETRYXPORT_H
