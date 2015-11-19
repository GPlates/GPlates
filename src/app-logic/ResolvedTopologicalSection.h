/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_RESOLVEDTOPOLOGICALSECTION_H
#define GPLATES_APP_LOGIC_RESOLVEDTOPOLOGICALSECTION_H

#include <vector>

#include "ReconstructionGeometry.h"
#include "ResolvedTopologicalSharedSubSegment.h"

#include "model/FeatureHandle.h"

#include "utils/ReferenceCount.h"


namespace GPlatesAppLogic
{
	/**
	 * A sequence of all sub-segments of a topological section feature used as part of the *boundary*
	 * of resolved topologies (@a ResolvedTopologicalBoundary and @a ResolvedTopologicalNetwork).
	 */
	class ResolvedTopologicalSection :
			public GPlatesUtils::ReferenceCount<ResolvedTopologicalSection>
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<ResolvedTopologicalSection> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const ResolvedTopologicalSection> non_null_ptr_to_const_type;


		template <typename ResolvedTopologicalSharedSubSegmentIter>
		static
		non_null_ptr_type
		create(
				ResolvedTopologicalSharedSubSegmentIter shared_sub_segments_begin,
				ResolvedTopologicalSharedSubSegmentIter shared_sub_segments_end,
				const ReconstructionGeometry::non_null_ptr_to_const_type &topological_section_reconstruction_geometry,
				const GPlatesModel::FeatureHandle::const_weak_ref &topological_section_feature_ref)
		{
			return non_null_ptr_type(
					new ResolvedTopologicalSection(
							shared_sub_segments_begin,
							shared_sub_segments_end,
							topological_section_reconstruction_geometry,
							topological_section_feature_ref));
		}

		/**
		 * The sequence of sub-segments of the topological section feature used as part of the *boundary*
		 * of resolved topologies (@a ResolvedTopologicalBoundary and @a ResolvedTopologicalNetwork).
		 */
		const std::vector<ResolvedTopologicalSharedSubSegment> &
		get_shared_sub_segments() const
		{
			return d_shared_sub_segments;
		}

		/**
		 * The reconstruction geometry of the topological section feature.
		 *
		 * This can be either a reconstructed feature geometry or a resolved topological *line*.
		 */
		const ReconstructionGeometry::non_null_ptr_to_const_type &
		get_reconstruction_geometry() const
		{
			return d_topological_section_reconstruction_geometry;
		}

		/**
		 * Reference to the topological section feature.
		 */
		const GPlatesModel::FeatureHandle::const_weak_ref &
		get_feature_ref() const
		{
			return d_topological_section_feature_ref;
		}

	private:

		/**
		 * The shared sub-segments that reference the @a ReconstructionGeometry of this topological section.
		 */
		std::vector<ResolvedTopologicalSharedSubSegment> d_shared_sub_segments;

		/**
		 * The reconstruction geometry of the topological section feature.
		 *
		 * This is either a reconstructed feature geometry or a resolved topological *line*.
		 */
		ReconstructionGeometry::non_null_ptr_to_const_type d_topological_section_reconstruction_geometry;

		//! Reference to the source feature handle of the topological section.
		GPlatesModel::FeatureHandle::const_weak_ref d_topological_section_feature_ref;


		template <typename ResolvedTopologicalSharedSubSegmentIter>
		ResolvedTopologicalSection(
				ResolvedTopologicalSharedSubSegmentIter shared_sub_segments_begin,
				ResolvedTopologicalSharedSubSegmentIter shared_sub_segments_end,
				const ReconstructionGeometry::non_null_ptr_to_const_type &topological_section_reconstruction_geometry,
				const GPlatesModel::FeatureHandle::const_weak_ref &topological_section_feature_ref) :
			d_shared_sub_segments(shared_sub_segments_begin, shared_sub_segments_end),
			d_topological_section_reconstruction_geometry(topological_section_reconstruction_geometry),
			d_topological_section_feature_ref(topological_section_feature_ref)
		{  }

	};
}

#endif // GPLATES_APP_LOGIC_RESOLVEDTOPOLOGICALSECTION_H
