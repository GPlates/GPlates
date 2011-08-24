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

#ifndef GPLATES_APP_LOGIC_RESOLVEDTOPOLOGICALBOUNDARYSUBSEGMENT_H
#define GPLATES_APP_LOGIC_RESOLVEDTOPOLOGICALBOUNDARYSUBSEGMENT_H

#include <vector>

#include "maths/GeometryOnSphere.h"

#include "model/FeatureHandle.h"


namespace GPlatesAppLogic
{
	/**
	 * Records the reconstructed geometry, and any other relevant information, of a subsegment.
	 *
	 * A subsegment is the subset of a reconstructed topological section's
	 * vertices that are used to form part of the boundary of a resolved topological closed
	 * plate polygon or a topological network.
	 */
	class ResolvedTopologicalBoundarySubSegment
	{
	public:
		ResolvedTopologicalBoundarySubSegment(
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &sub_segment_geometry,
				const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref,
				bool use_reverse) :
			d_sub_segment_geometry(sub_segment_geometry),
			d_feature_ref(feature_ref),
			d_use_reverse(use_reverse)
		{  }

		/**
		 * The subset of vertices of topological section used in resolved topology geometry.
		 *
		 * NOTE: The vertices have already been reversed if this subsegment is reversed
		 * (as determined by @a get_use_reverse).
		 */
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
		get_geometry() const
		{
			return d_sub_segment_geometry;
		}

		//! Reference to the feature referenced by the topological section.
		const GPlatesModel::FeatureHandle::const_weak_ref &
		get_feature_ref() const
		{
			return d_feature_ref;
		}

		bool
		get_use_reverse() const
		{
			return d_use_reverse;
		}

	private:
		//! The subsegment geometry.
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type d_sub_segment_geometry;

		//! Reference to the source feature handle of the topological section.
		GPlatesModel::FeatureHandle::const_weak_ref d_feature_ref;

		//! Indicates if geometry direction was reversed when assembling topology.
		bool d_use_reverse;
	};

	//! Typedef for a sequence of @a ResolvedTopologicalBoundarySubSegment objects.
	typedef std::vector<ResolvedTopologicalBoundarySubSegment> sub_segment_seq_type;
}

#endif // GPLATES_APP_LOGIC_RESOLVEDTOPOLOGICALBOUNDARYSUBSEGMENT_H
