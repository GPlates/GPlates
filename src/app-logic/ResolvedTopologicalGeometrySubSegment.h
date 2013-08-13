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

#ifndef GPLATES_APP_LOGIC_RESOLVEDTOPOLOGICALGEOMETRYSUBSEGMENT_H
#define GPLATES_APP_LOGIC_RESOLVEDTOPOLOGICALGEOMETRYSUBSEGMENT_H

#include <vector>

#include "ReconstructionGeometry.h"

#include "maths/GeometryOnSphere.h"

#include "model/FeatureHandle.h"


namespace GPlatesAppLogic
{
	/**
	 * Records the reconstructed geometry, and any other relevant information, of a subsegment.
	 *
	 * A subsegment can come from a reconstructed feature geometry or a resolved topological *line*.
	 *
	 * A subsegment is the subset of a reconstructed topological section's
	 * vertices that are used to form part of the geometry of a resolved topological polygon/polyline
	 * or boundary of a topological network.
	 */
	class ResolvedTopologicalGeometrySubSegment
	{
	public:
		ResolvedTopologicalGeometrySubSegment(
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &sub_segment_geometry,
				const ReconstructionGeometry::non_null_ptr_to_const_type &reconstruction_geometry,
				const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref,
				bool use_reverse) :
			d_sub_segment_geometry(sub_segment_geometry),
			d_reconstruction_geometry(reconstruction_geometry),
			d_feature_ref(feature_ref),
			d_use_reverse(use_reverse)
		{  }

		/**
		 * The subset of vertices of topological section used in resolved topology geometry.
		 *
		 * NOTE: These are the un-reversed vertices of the original geometry that contributed this
		 * sub-segment - the actual order of vertices (as contributed to the final resolved
		 * topological geometry along with other sub-segments) depends on this un-reversed geometry
		 * and the reversal flag returned by @a get_use_reverse.
		 */
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
		get_geometry() const
		{
			return d_sub_segment_geometry;
		}

		/**
		 * The reconstruction geometry that the sub-segment was obtained from.
		 *
		 * This can be either a reconstructed feature geometry or a resolved topological *line*.
		 */
		const ReconstructionGeometry::non_null_ptr_to_const_type &
		get_reconstruction_geometry() const
		{
			return d_reconstruction_geometry;
		}

		//! Reference to the feature referenced by the topological section.
		const GPlatesModel::FeatureHandle::const_weak_ref &
		get_feature_ref() const
		{
			return d_feature_ref;
		}

		/**
		 * If true then the geometry returned by @a get_geometry had its points reversed in order
		 * before contributing to the final resolved topological geometry.
		 */
		bool
		get_use_reverse() const
		{
			return d_use_reverse;
		}

	private:
		//! The subsegment geometry.
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type d_sub_segment_geometry;

		/**
		 * The subsegment reconstruction geometry.
		 *
		 * This is either a reconstructed feature geometry or a resolved topological *line*.
		 */
		ReconstructionGeometry::non_null_ptr_to_const_type d_reconstruction_geometry;

		//! Reference to the source feature handle of the topological section.
		GPlatesModel::FeatureHandle::const_weak_ref d_feature_ref;

		//! Indicates if geometry direction was reversed when assembling topology.
		bool d_use_reverse;
	};

	//! Typedef for a sequence of @a ResolvedTopologicalGeometrySubSegment objects.
	typedef std::vector<ResolvedTopologicalGeometrySubSegment> sub_segment_seq_type;
}

#endif // GPLATES_APP_LOGIC_RESOLVEDTOPOLOGICALGEOMETRYSUBSEGMENT_H
