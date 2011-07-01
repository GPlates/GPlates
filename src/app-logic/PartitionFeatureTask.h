/* $Id$ */


/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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
 
#ifndef GPLATES_APP_LOGIC_PARTITIONFEATURETASK_H
#define GPLATES_APP_LOGIC_PARTITIONFEATURETASK_H

#include <vector>
#include <boost/shared_ptr.hpp>

#include "AssignPlateIds.h"
#include "ReconstructionTree.h"

#include "model/FeatureCollectionHandle.h"
#include "model/FeatureHandle.h"


namespace GPlatesAppLogic
{
	class GeometryCookieCutter;
	class PartitionFeatureTask;

	//! Typedef for a shared pointer to a task.
	typedef boost::shared_ptr<PartitionFeatureTask>
			partition_feature_task_ptr_type;

	//! Typedef for a sequence of shared pointers to tasks.
	typedef std::vector<partition_feature_task_ptr_type>
			partition_feature_task_ptr_seq_type;

	/**
	 * Creates and returns all @a PartitionFeatureTask tasks
	 * in the order in which they should be processed.
	 */
	partition_feature_task_ptr_seq_type
	get_partition_feature_tasks(
			const ReconstructionTree &reconstruction_tree,
			GPlatesAppLogic::AssignPlateIds::AssignPlateIdMethodType assign_plate_id_method,
			const GPlatesAppLogic::AssignPlateIds::feature_property_flags_type &feature_property_types_to_assign);


	/**
	 * Interface for a task that can be queried to see if it can assign a plate id
	 * to a specific feature and asked to assign the plate id.
	 */
	class PartitionFeatureTask
	{
	public:
		virtual
		~PartitionFeatureTask()
		{  }


		/**
		 * Return true if can partition @a feature_ref.
		 */
		virtual
		bool
		can_partition_feature(
				const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref) const = 0;


		/**
		 * Assigns properties of the partitioning polygons to @a feature_ref and
		 * any clones of it that hold partitioned geometry.
		 *
		 * If @a respect_feature_time_period is true (the default) then the feature is only
		 * partitioned if the reconstruction time (stored in derived class instance) is within
		 * the time period over which the feature is defined.
		 * Note that some this can be ignored by some derived classes (eg, @a VgpPartitionFeatureTask).
		 *
		 * NOTE: Currently @a feature_ref can be modified to hold one of geometries
		 * resulting from partitioning while clones of it can hold the other
		 * partitioned geometries.
		 */
		virtual
		void
		partition_feature(
				const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection_ref,
				const GeometryCookieCutter &geometry_cookie_cutter,
				bool respect_feature_time_period = true) = 0;
	};
}

#endif // GPLATES_APP_LOGIC_PARTITIONFEATURETASK_H
