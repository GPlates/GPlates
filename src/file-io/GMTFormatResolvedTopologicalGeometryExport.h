/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#ifndef GPLATES_FILE_IO_GMTFORMATRESOLVEDTOPOLOGICALGEOMETRYEXPORT_H
#define GPLATES_FILE_IO_GMTFORMATRESOLVEDTOPOLOGICALGEOMETRYEXPORT_H

#include <boost/optional.hpp>
#include <QFileInfo>

#include "ReconstructionGeometryExportImpl.h"

#include "maths/PolygonOrientation.h"

#include "model/types.h"


namespace GPlatesAppLogic
{
	class ReconstructionGeometry;
	class ResolvedTopologicalSection;
}

namespace GPlatesFileIO
{
	namespace GMTFormatResolvedTopologicalGeometryExport
	{
		/**
		 * Typedef for a feature geometry group of resolved topologies.
		 */
		typedef ReconstructionGeometryExportImpl::FeatureGeometryGroup<GPlatesAppLogic::ReconstructionGeometry>
				feature_geometry_group_type;

		/**
		 * Typedef for a sequence of referenced files.
		 */
		typedef ReconstructionGeometryExportImpl::referenced_files_collection_type
				referenced_files_collection_type;


		/**
		 * Exports resolved topology objects to GMT format.
		 *
		 * This includes @a ResolvedTopologicalLine, @a ResolvedTopologicalBoundary and @a ResolvedTopologicalNetwork.
		 *
		 * @param force_polygon_orientation optionally force polygon orientation (clockwise or counter-clockwise).
		 */
		void
		export_resolved_topological_geometries(
				const std::list<feature_geometry_group_type> &feature_geometry_group_seq,
				const QFileInfo& file_info,
				const referenced_files_collection_type &referenced_files,
				const referenced_files_collection_type &active_reconstruction_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time,
				boost::optional<GPlatesMaths::PolygonOrientation::Orientation>
						force_polygon_orientation = boost::none);


		/**
		 * Exports resolved topological sections to GMT format.
		 *
		 * This includes @a ResolvedTopologicalSection and its @a ResolvedTopologicalSharedSubSegment instances.
		 */
		void
		export_resolved_topological_sections(
				const std::vector<const GPlatesAppLogic::ResolvedTopologicalSection *> &resolved_topological_sections,
				const QFileInfo& file_info,
				const referenced_files_collection_type &referenced_files,
				const referenced_files_collection_type &active_reconstruction_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time,
				bool export_topological_line_sub_segments);
	}
}

#endif // GPLATES_FILE_IO_GMTFORMATRESOLVEDTOPOLOGICALGEOMETRYEXPORT_H
