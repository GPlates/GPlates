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

#ifndef GPLATES_FILE_IO_GMTFORMATRESOLVEDTOPOLOGICALBOUNDARYEXPORT_H
#define GPLATES_FILE_IO_GMTFORMATRESOLVEDTOPOLOGICALBOUNDARYEXPORT_H

#include <QFileInfo>

#include "ReconstructionGeometryExportImpl.h"
#include "ResolvedTopologicalBoundaryExportImpl.h"

#include "model/types.h"


namespace GPlatesAppLogic
{
	class ResolvedTopologicalBoundary;
}

namespace GPlatesFileIO
{
	namespace GMTFormatResolvedTopologicalBoundaryExport
	{
		/**
		 * Typedef for a sequence of referenced files.
		 */
		typedef ReconstructionGeometryExportImpl::referenced_files_collection_type
				referenced_files_collection_type;

		/**
		 * Typedef for a feature geometry group of resolved topological geometries.
		 */
		typedef ResolvedTopologicalBoundaryExportImpl::resolved_geom_seq_type resolved_geom_seq_type;

		/**
		 * Typedef for a sequence of @a SubSegmentGroup objects.
		 */
		typedef ResolvedTopologicalBoundaryExportImpl::sub_segment_group_seq_type sub_segment_group_seq_type;


		/**
		 * Exports @a ResolvedTopologicalBoundary objects to GMT format.
		 */
		void
		export_resolved_topological_boundaries(
				const resolved_geom_seq_type &resolved_topological_geometries,
				ResolvedTopologicalBoundaryExportImpl::ResolvedTopologicalBoundaryExportType export_type,
				const QFileInfo& file_info,
				const referenced_files_collection_type &referenced_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time);


		/**
		 * Exports subsegments of resolved topological boundaries to GMT format.
		 */
		void
		export_sub_segments(
				const sub_segment_group_seq_type &sub_segments,
				ResolvedTopologicalBoundaryExportImpl::SubSegmentExportType export_type,
				const QFileInfo& file_info,
				const referenced_files_collection_type &referenced_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time);
	}
}

#endif // GPLATES_FILE_IO_GMTFORMATRESOLVEDTOPOLOGICALBOUNDARYEXPORT_H
