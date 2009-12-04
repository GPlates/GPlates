/* $Id$ */


/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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
 
#ifndef GPLATES_APP_LOGIC_TOPOLOGYUTILS_H
#define GPLATES_APP_LOGIC_TOPOLOGYUTILS_H

#include <vector>
#include <boost/shared_ptr.hpp>

#include "model/FeatureCollectionHandle.h"


namespace GPlatesMaths
{
	class PointOnSphere;
}

namespace GPlatesModel
{
	class Reconstruction;
	class ResolvedTopologicalGeometry;
}

namespace GPlatesAppLogic
{
	/**
	 * This namespace contains utilities that clients of topology-related functionality use.
	 */
	namespace TopologyUtils
	{
		//! Typedef for a sequence of resolved topological geometries.
		typedef std::vector<const GPlatesModel::ResolvedTopologicalGeometry *>
				resolved_topological_geometry_seq_type;


		class ResolvedGeometriesForPointInclusionQuery;
		/**
		 * Typedef for the results of finding resolved topologies (and creating
		 * a structure optimised for point-in-polygon tests).
		 */
		typedef boost::shared_ptr<ResolvedGeometriesForPointInclusionQuery>
				resolved_geometries_for_point_inclusion_query_type;


		/**
		 * Finds all topological closed plate boundary features in @a topological_features_collection
		 * that exist at time @a reconstruction_time and creates @a ResolvedTopologicalGeometry objects
		 * for each one and stores them in @a reconstruction.
		 *
		 * @pre the features referenced by any of these topological closed plate boundary features
		 *      must have already been reconstructed and exist in @a reconstruction.
		 */
		void
		resolve_topologies(
				const double &reconstruction_time,
				GPlatesModel::Reconstruction &reconstruction,
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
						topological_features_collection);


		/**
		 * Finds all @a ResolvedTopologicalGeometry objects in @a reconstruction
		 * and returns a structure optimised for point-in-polygon tests.
		 *
		 * The returned structure can be passed to @a find_resolved_topologies_containing_point
		 * for point-in-polygon tests.
		 *
		 * The returned structure can tested tested like a bool - it's true
		 * if any @a ResolvedTopologicalGeometry objects are found in @a reconstruction.
		 */
		resolved_geometries_for_point_inclusion_query_type
		query_resolved_topologies_for_testing_point_inclusion(
				GPlatesModel::Reconstruction &reconstruction);


		/**
		 * Searches all @a ResolvedTopologicalGeometry objects in @a resolved_geoms_query
		 * (returned from a call to @a query_resolved_topologies_for_testing_point_inclusion)
		 * to see which ones contain @a point and returns any found in
		 * @a resolved_topological_geom_seq.
		 *
		 * Returns true if any are found.
		 */
		bool
		find_resolved_topologies_containing_point(
				resolved_topological_geometry_seq_type &resolved_topological_geom_seq,
				const GPlatesMaths::PointOnSphere &point,
				const resolved_geometries_for_point_inclusion_query_type &resolved_geoms_query);
	}
}

#endif // GPLATES_APP_LOGIC_TOPOLOGYUTILS_H
