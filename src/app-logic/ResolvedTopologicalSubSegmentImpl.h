/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2018 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_RESOLVEDTOPOLOGICALSUBSEGMENTIMPL_H
#define GPLATES_APP_LOGIC_RESOLVEDTOPOLOGICALSUBSEGMENTIMPL_H

#include <boost/optional.hpp>

#include "ReconstructionGeometry.h"
#include "ResolvedSubSegmentRangeInSection.h"
#include "ResolvedVertexSourceInfo.h"


namespace GPlatesAppLogic
{
	namespace ResolvedTopologicalSubSegmentImpl
	{
		/**
		 * Find the vertex source infos in the specified sub-segment range of the specified resolved topological section geometry.
		 *
		 * A topological section can come from a reconstructed feature geometry or a resolved topological *line*.
		 *
		 * If a reconstructed feature geometry then all points in the subsegment geometry (except the optional
		 * rubber band points at either/both ends) will share that same source reconstructed feature geometry.
		 *
		 * If a resolved topological line then each point in the subsegment geometry (except the optional
		 * rubber band points at either/both ends)will come from a subsegment of that resolved topological line
		 * (where those subsegments, in turn, are reconstructed feature geometries).
		 *
		 * If the specified section did not intersect its previous and/or next sections (not even touching them)
		 * then there will be an extra rubber band point for each adjacent section not intersected that is an equal
		 * blend between the appropriate end vertex of the previous/next section and the appropriate end vertex
		 * of the current section.
		 */
		void
		get_sub_segment_vertex_source_infos(
				resolved_vertex_source_info_seq_type &vertex_source_infos,
				const ResolvedSubSegmentRangeInSection &sub_segment,
				ReconstructionGeometry::non_null_ptr_to_const_type section_reconstruction_geometry,
				boost::optional<ReconstructionGeometry::non_null_ptr_to_const_type> prev_section_reconstruction_geometry,
				boost::optional<ReconstructionGeometry::non_null_ptr_to_const_type> next_section_reconstruction_geometry);
	}
}

#endif // GPLATES_APP_LOGIC_RESOLVEDTOPOLOGICALSUBSEGMENTIMPL_H
