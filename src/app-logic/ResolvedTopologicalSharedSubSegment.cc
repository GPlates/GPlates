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
		resolved_vertex_source_info_seq_type &point_source_infos) const
{
	if (!d_point_source_infos)
	{
		d_point_source_infos = resolved_vertex_source_info_seq_type();

		ResolvedTopologicalSubSegmentImpl::get_sub_segment_vertex_source_infos(
				d_point_source_infos.get(),
				d_shared_sub_segment,
				d_shared_segment_reconstruction_geometry);
	}

	std::copy(
			d_point_source_infos->begin(),
			d_point_source_infos->end(),
			std::back_inserter(point_source_infos));
}


void
GPlatesAppLogic::ResolvedTopologicalSharedSubSegment::get_reversed_shared_sub_segment_point_source_infos(
		resolved_vertex_source_info_seq_type &point_source_infos,
		bool use_reverse) const
{
	if (!d_point_source_infos)
	{
		d_point_source_infos = resolved_vertex_source_info_seq_type();

		ResolvedTopologicalSubSegmentImpl::get_sub_segment_vertex_source_infos(
				d_point_source_infos.get(),
				d_shared_sub_segment,
				d_shared_segment_reconstruction_geometry);
	}

	if (use_reverse)
	{
		std::reverse_copy(
				d_point_source_infos->begin(),
				d_point_source_infos->end(),
				std::back_inserter(point_source_infos));
	}
	else
	{
		std::copy(
				d_point_source_infos->begin(),
				d_point_source_infos->end(),
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
