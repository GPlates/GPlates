/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2014 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_RESOLVEDTOPOLOGICALSHAREDSUBSEGMENT_H
#define GPLATES_APP_LOGIC_RESOLVEDTOPOLOGICALSHAREDSUBSEGMENT_H

#include <vector>

#include "ReconstructionGeometry.h"
#include "ResolvedTopologicalGeometrySubSegment.h"


namespace GPlatesAppLogic
{
	/**
	 * Associates a sub-segment (@a ResolvedTopologicalGeometrySubSegment) with those resolved
	 * topological boundaries that share it as part of their boundary.
	 *
	 * This is kept as a separate class from @a ResolvedTopologicalGeometrySubSegment partly in order
	 * to avoid memory islands (cyclic references of shared pointers) - see below for more details and
	 * partly for design reasons.
	 */
	class ResolvedTopologicalSharedSubSegment
	{
	public:
		ResolvedTopologicalSharedSubSegment(
				const ResolvedTopologicalGeometrySubSegment &sub_segment,
				const std::vector<ReconstructionGeometry::non_null_ptr_to_const_type> resolved_topology_boundaries) :
			d_sub_segment(sub_segment),
			d_resolved_topology_boundaries(resolved_topology_boundaries)
		{  }


		/**
		 * Returns the resolved topology boundaries that share this sub-segment.
		 *
		 * Resolved topology boundaries can be @a ResolvedTopologicalGeometry containing a polygon,
		 * and @a ResolvedTopologicalNetwork (the boundary of one). For example the sub-segment
		 * at the boundary of a deforming @a ResolvedTopologicalNetwork is also shared by a
		 * non-deforming @a ResolvedTopologicalGeometry (plate polygon).
		 *
		 * Typically there will be two boundaries but there can be one if a topological
		 * boundary has no adjacent boundary (eg, topologies don't cover the entire globe).
		 * If there are more than two boundaries then there is probably some overlap of
		 * topological boundaries occurring.
		 */
		const std::vector<ReconstructionGeometry::non_null_ptr_to_const_type> &
		get_resolved_topological_boundaries() const
		{
			return d_resolved_topology_boundaries;
		}


		//
		// The following methods simply delegate to @a ResolvedTopologicalGeometrySubSegment.
		//


		/**
		 * The subset of vertices of topological section used in resolved topology geometry.
		 *
		 * NOTE: These are the un-reversed vertices of the original geometry that contributed this
		 * sub-segment - the actual order of vertices (as contributed to the final resolved
		 * topological boundary along with other sub-segments) depends on this un-reversed geometry
		 * and the reversal flag returned by @a get_use_reverse.
		 */
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
		get_geometry() const
		{
			return d_sub_segment.get_geometry();
		}

		/**
		 * The reconstruction geometry that the sub-segment was obtained from.
		 *
		 * This can be either a reconstructed feature geometry or a resolved topological *line*.
		 */
		const ReconstructionGeometry::non_null_ptr_to_const_type &
		get_reconstruction_geometry() const
		{
			return d_sub_segment.get_reconstruction_geometry();
		}

		//! Reference to the feature referenced by the topological section.
		const GPlatesModel::FeatureHandle::const_weak_ref &
		get_feature_ref() const
		{
			return d_sub_segment.get_feature_ref();
		}

		/**
		 * If true then the geometry returned by @a get_geometry had its points reversed in order
		 * before contributing to the final resolved topological geometry.
		 */
		bool
		get_use_reverse() const
		{
			return d_sub_segment.get_use_reverse();
		}

	private:

		/**
		 * The sub-segment being shared.
		 */
		ResolvedTopologicalGeometrySubSegment d_sub_segment;

		/**
		 * The resolved topology boundaries that share this sub-segment.
		 *
		 * NOTE: We won't get a memory island (cyclic reference of shared pointers) because
		 * @a ResolvedTopologicalSharedSubSegment are not owned by @a ReconstructionGeometry
		 * (only @a ResolvedTopologicalGeometrySubSegment are owned by @a ReconstructionGeometry).
		 */
		std::vector<ReconstructionGeometry::non_null_ptr_to_const_type> d_resolved_topology_boundaries;
	};

	//! Typedef for a sequence of @a ResolvedTopologicalSharedSubSegment objects.
	typedef std::vector<ResolvedTopologicalSharedSubSegment> shared_sub_segment_seq_type;
}

#endif // GPLATES_APP_LOGIC_RESOLVEDTOPOLOGICALSHAREDSUBSEGMENT_H
