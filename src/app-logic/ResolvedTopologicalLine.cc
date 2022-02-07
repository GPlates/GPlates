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

#include "ResolvedTopologicalLine.h"


const GPlatesAppLogic::resolved_vertex_source_info_seq_type &
GPlatesAppLogic::ResolvedTopologicalLine::get_vertex_source_infos() const
{
	// Cache all vertex source infos on first call.
	if (!d_vertex_source_infos)
	{
		calc_vertex_source_infos();
	}

	return d_vertex_source_infos.get();
}


void
GPlatesAppLogic::ResolvedTopologicalLine::calc_vertex_source_infos() const
{
	d_vertex_source_infos = resolved_vertex_source_info_seq_type();
	resolved_vertex_source_info_seq_type &vertex_source_infos = d_vertex_source_infos.get();

	// Copy source infos from points in each subsegment.
	sub_segment_seq_type::const_iterator sub_segments_iter = d_sub_segment_seq.begin();
	sub_segment_seq_type::const_iterator sub_segments_end = d_sub_segment_seq.end();
	for ( ; sub_segments_iter != sub_segments_end; ++sub_segments_iter)
	{
		const ResolvedTopologicalGeometrySubSegment::non_null_ptr_type &sub_segment = *sub_segments_iter;
		// Subsegment should be reversed if that's how it contributed to this resolved topological line...
		sub_segment->get_reversed_sub_segment_point_source_infos(
				vertex_source_infos,
				INCLUDE_SUB_SEGMENT_RUBBER_BAND_POINTS_IN_RESOLVED_LINE/*include_rubber_band_points*/);
	}
}
