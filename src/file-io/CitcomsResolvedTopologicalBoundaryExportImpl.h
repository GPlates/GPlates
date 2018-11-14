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

#ifndef GPLATES_FILE_IO_CITCOMSRESOLVEDTOPOLOGICALBOUNDARYEXPORTIMPL_H
#define GPLATES_FILE_IO_CITCOMSRESOLVEDTOPOLOGICALBOUNDARYEXPORTIMPL_H

#include <vector>

#include "app-logic/ReconstructionGeometry.h"
#include "app-logic/ResolvedTopologicalGeometrySubSegment.h"

#include "model/FeatureHandle.h"


namespace GPlatesFileIO
{
	/**
	 * CitcomS-specific resolved topology export implementation.
	 */
	namespace CitcomsResolvedTopologicalBoundaryExportImpl
	{
		//! Typedef for a sequence of subsegments of resolved topological boundaries.
		typedef std::vector<const GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment *> sub_segment_ptr_seq_type;

		/**
		 * Groups a resolved topology with a subset of its boundary subsegments.
		 *
		 * The reason for the subset, and not the full set, is only a specific subset
		 * (eg, trench) of subsegments is being exported to a particular export file.
		 */
		struct SubSegmentGroup
		{
			explicit
			SubSegmentGroup(
					const GPlatesAppLogic::ReconstructionGeometry *resolved_topology_) :
				resolved_topology(resolved_topology_)
			{  }


			const GPlatesAppLogic::ReconstructionGeometry *resolved_topology;
			sub_segment_ptr_seq_type sub_segments;
		};

		//! Typedef for a sequence of resolved topologies.
		typedef std::vector<const GPlatesAppLogic::ReconstructionGeometry *> resolved_topologies_seq_type;

		//! Typedef for a sequence of @a SubSegmentGroup objects.
		typedef std::vector<SubSegmentGroup> sub_segment_group_seq_type;


		/**
		 * The export type of subsegments.
		 */
		enum SubSegmentExportType
		{
			ALL_SUB_SEGMENTS_EXPORT_TYPE,
			PLATE_POLYGON_SUB_SEGMENTS_EXPORT_TYPE,
			SLAB_POLYGON_SUB_SEGMENTS_EXPORT_TYPE,
			NETWORK_POLYGON_SUB_SEGMENTS_EXPORT_TYPE
		};

		/**
		 * The export type of resolved topological boundaries.
		 */
		enum ResolvedTopologicalBoundaryExportType
		{
			ALL_POLYGON_EXPORT_TYPE,
			PLATE_POLYGON_EXPORT_TYPE,
			SLAB_POLYGON_EXPORT_TYPE,
			NETWORK_POLYGON_EXPORT_TYPE
		};


		/**
		 *  Sub segment feature type.
		 */
		enum SubSegmentType
		{
			SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_LEFT,
			SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_RIGHT,
			SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_UNKNOWN,
			SUB_SEGMENT_TYPE_SLAB_EDGE_LEADING_UNKNOWN,
			SUB_SEGMENT_TYPE_SLAB_EDGE_LEADING_LEFT,
			SUB_SEGMENT_TYPE_SLAB_EDGE_LEADING_RIGHT,
			SUB_SEGMENT_TYPE_SLAB_EDGE_TRENCH,
			SUB_SEGMENT_TYPE_SLAB_EDGE_SIDE,
			SUB_SEGMENT_TYPE_OTHER
		};


		/**
		 * Determines feature type of subsegment source feature referenced by a plate polygon.
		 */
		SubSegmentType
		get_sub_segment_type(
				const GPlatesModel::FeatureHandle::const_weak_ref &sub_segment_feature_ref,
				const double &recon_time);


		/**
		 * Determines feature type of subsegment source feature referenced by a slab polygon.
		*/
		SubSegmentType
		get_slab_sub_segment_type(
				const GPlatesModel::FeatureHandle::const_weak_ref &sub_segment_feature_ref,
				const double &recon_time);
	}
}

#endif // GPLATES_FILE_IO_CITCOMSRESOLVEDTOPOLOGICALBOUNDARYEXPORTIMPL_H
