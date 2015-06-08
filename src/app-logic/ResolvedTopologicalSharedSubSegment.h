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

#include "maths/GeometryOnSphere.h"

#include "model/FeatureHandle.h"


namespace GPlatesAppLogic
{
	/**
	 * Associates a sub-segment (@a ResolvedTopologicalGeometrySubSegment) with those resolved topologies
	 * (@a ResolvedTopologicalBoundary and @a ResolvedTopologicalNetwork) that share it as part of their boundary.
	 *
	 * This is kept as a separate class from @a ResolvedTopologicalGeometrySubSegment partly in order
	 * to avoid memory islands (cyclic references of shared pointers) - see below for more details -
	 * and partly for design reasons.
	 */
	class ResolvedTopologicalSharedSubSegment
	{
	public:

		/**
		 * A resolved topology's relationship to the shared sub-segment.
		 */
		struct ResolvedTopologyInfo
		{
			ResolvedTopologyInfo(
					const ReconstructionGeometry::non_null_ptr_to_const_type &resolved_topology_,
					bool is_sub_segment_geometry_reversed_) :
				resolved_topology(resolved_topology_),
				is_sub_segment_geometry_reversed(is_sub_segment_geometry_reversed_)
			{  }

			/**
			 * A resolved topology can be a @a ResolvedTopologicalBoundary (the boundary of a plate polygon)
			 * or a @a ResolvedTopologicalNetwork (the boundary of a deforming network).
			 */
			ReconstructionGeometry::non_null_ptr_to_const_type resolved_topology;

			// Is sub-segment geometry reversed with respect to the section geometry.
			bool is_sub_segment_geometry_reversed;
		};


		ResolvedTopologicalSharedSubSegment(
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &shared_sub_segment_geometry,
				const ReconstructionGeometry::non_null_ptr_to_const_type &shared_segment_reconstruction_geometry,
				const GPlatesModel::FeatureHandle::const_weak_ref &shared_segment_feature_ref,
				const std::vector<ResolvedTopologyInfo> &sharing_resolved_topologies) :
			d_shared_sub_segment_geometry(shared_sub_segment_geometry),
			d_shared_segment_reconstruction_geometry(shared_segment_reconstruction_geometry),
			d_shared_segment_feature_ref(shared_segment_feature_ref),
			d_sharing_resolved_topologies(sharing_resolved_topologies)
		{  }


		/**
		 * Returns the resolved topologies that share this sub-segment.
		 *
		 * Along with each resolved topology there is also returned a flag to indicated whether the
		 * shared sub-segment geometry (returned by @a get_geometry) had its points reversed
		 * in order before contributing to that particular resolved topology.
		 *
		 * Resolved topologies can be @a ResolvedTopologicalBoundary (the boundary of a plate polygon)
		 * and @a ResolvedTopologicalNetwork (the boundary of a deforming network).
		 * For example the sub-segment at the boundary of a deforming @a ResolvedTopologicalNetwork
		 * is also shared by a non-deforming @a ResolvedTopologicalBoundary (plate polygon).
		 *
		 * Typically there will be two sharing boundaries but there can be one if a topological
		 * boundary has no adjacent boundary (eg, topologies don't cover the entire globe).
		 * If there are more than two sharing boundaries then there is probably some overlap of
		 * topologies occurring.
		 */
		const std::vector<ResolvedTopologyInfo> &
		get_sharing_resolved_topologies() const
		{
			return d_sharing_resolved_topologies;
		}


		/**
		 * The subset of vertices of topological section used in the shared resolved topologies.
		 *
		 * NOTE: These are the un-reversed vertices of the original geometry that contributed this
		 * shared sub-segment - the actual order of vertices (as contributed to the shared resolved
		 * topological geometries along with other sub-segments) depends on the shared resolved
		 * topology (different topologies will have different reverse flags - see @a ResolvedTopologyInfo).
		 */
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
		get_geometry() const
		{
			return d_shared_sub_segment_geometry;
		}

		/**
		 * The reconstruction geometry that this shared sub-segment was obtained from.
		 *
		 * This can be either a reconstructed feature geometry or a resolved topological *line*.
		 */
		const ReconstructionGeometry::non_null_ptr_to_const_type &
		get_reconstruction_geometry() const
		{
			return d_shared_segment_reconstruction_geometry;
		}

		//! Reference to the feature referenced by the topological section.
		const GPlatesModel::FeatureHandle::const_weak_ref &
		get_feature_ref() const
		{
			return d_shared_segment_feature_ref;
		}

	private:

		//! The shared subsegment geometry.
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type d_shared_sub_segment_geometry;

		/**
		 * The shared segment reconstruction geometry.
		 *
		 * This is either a reconstructed feature geometry or a resolved topological *line*.
		 */
		ReconstructionGeometry::non_null_ptr_to_const_type d_shared_segment_reconstruction_geometry;

		//! Reference to the source feature handle of the topological section.
		GPlatesModel::FeatureHandle::const_weak_ref d_shared_segment_feature_ref;

		/**
		 * The resolved topologies that share this sub-segment.
		 *
		 * NOTE: We won't get a memory island (cyclic reference of shared pointers) because
		 * @a ResolvedTopologicalSharedSubSegment are not owned by
		 * @a ResolvedTopologicalBoundary and @a ResolvedTopologicalNetwork
		 * (only @a ResolvedTopologicalGeometrySubSegment are owned by them).
		 */
		std::vector<ResolvedTopologyInfo> d_sharing_resolved_topologies;
	};


	//! Typedef for a sequence of @a ResolvedTopologicalSharedSubSegment objects.
	typedef std::vector<ResolvedTopologicalSharedSubSegment> shared_sub_segment_seq_type;
}

#endif // GPLATES_APP_LOGIC_RESOLVEDTOPOLOGICALSHAREDSUBSEGMENT_H
