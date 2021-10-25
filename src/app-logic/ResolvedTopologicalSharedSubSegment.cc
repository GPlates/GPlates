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

#include <algorithm>
#include <iterator>

#include "ResolvedTopologicalSharedSubSegment.h"

#include "ResolvedTopologicalSubSegmentImpl.h"


void
GPlatesAppLogic::ResolvedTopologicalSharedSubSegment::get_shared_sub_segment_point_source_infos(
		resolved_vertex_source_info_seq_type &point_source_infos,
		bool include_rubber_band_points) const
{
	if (!d_point_source_infos)
	{
		d_point_source_infos = resolved_vertex_source_info_seq_type();

		// Get the point source infos (including at the optional rubber band points).
		ResolvedTopologicalSubSegmentImpl::get_sub_segment_vertex_source_infos(
				d_point_source_infos.get(),
				d_shared_sub_segment,
				d_shared_segment_reconstruction_geometry,
				true/*include_rubber_band_points*/);
	}

	// Copy to caller's sequence.
	//
	// If the caller does not want rubber band points then avoid copying them (if they exist).
	resolved_vertex_source_info_seq_type::const_iterator src_point_source_infos_begin = d_point_source_infos->begin();
	resolved_vertex_source_info_seq_type::const_iterator src_point_source_infos_end = d_point_source_infos->end();
	if (!include_rubber_band_points)
	{
		if (d_shared_sub_segment.get_start_rubber_band())
		{
			++src_point_source_infos_begin;
		}
		if (d_shared_sub_segment.get_end_rubber_band())
		{
			--src_point_source_infos_end;
		}
	}

	std::copy(
			src_point_source_infos_begin,
			src_point_source_infos_end,
			std::back_inserter(point_source_infos));
}


void
GPlatesAppLogic::ResolvedTopologicalSharedSubSegment::get_reversed_shared_sub_segment_point_source_infos(
		resolved_vertex_source_info_seq_type &point_source_infos,
		bool use_reverse,
		bool include_rubber_band_points) const
{
	if (!d_point_source_infos)
	{
		d_point_source_infos = resolved_vertex_source_info_seq_type();

		// Get the point source infos (including at the optional rubber band points).
		ResolvedTopologicalSubSegmentImpl::get_sub_segment_vertex_source_infos(
				d_point_source_infos.get(),
				d_shared_sub_segment,
				d_shared_segment_reconstruction_geometry,
				true/*include_rubber_band_points*/);
	}

	// Copy to caller's sequence.
	//
	// If the caller does not want rubber band points then avoid copying them (if they exist).
	resolved_vertex_source_info_seq_type::const_iterator src_point_source_infos_begin = d_point_source_infos->begin();
	resolved_vertex_source_info_seq_type::const_iterator src_point_source_infos_end = d_point_source_infos->end();
	if (!include_rubber_band_points)
	{
		if (d_shared_sub_segment.get_start_rubber_band())
		{
			++src_point_source_infos_begin;
		}
		if (d_shared_sub_segment.get_end_rubber_band())
		{
			--src_point_source_infos_end;
		}
	}

	if (use_reverse)
	{
		std::reverse_copy(
				src_point_source_infos_begin,
				src_point_source_infos_end,
				std::back_inserter(point_source_infos));
	}
	else
	{
		std::copy(
				src_point_source_infos_begin,
				src_point_source_infos_end,
				std::back_inserter(point_source_infos));
	}
}


const boost::optional<GPlatesAppLogic::sub_segment_seq_type> &
GPlatesAppLogic::ResolvedTopologicalSharedSubSegment::get_sub_sub_segments() const
{
	if (!d_calculated_sub_sub_segments)
	{
		ResolvedTopologicalSubSegmentImpl::get_sub_sub_segments(
			d_sub_sub_segments,
			d_shared_sub_segment,
			d_shared_segment_reconstruction_geometry);

		d_calculated_sub_sub_segments = true;
	}

	return d_sub_sub_segments;
}
