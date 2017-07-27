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

#include "PartitionFeatureTask.h"

#include "GenericPartitionFeatureTask.h"
#include "VgpPartitionFeatureTask.h"


GPlatesAppLogic::partition_feature_task_ptr_seq_type
GPlatesAppLogic::get_partition_feature_tasks(
		GPlatesAppLogic::AssignPlateIds::AssignPlateIdMethodType assign_plate_id_method,
		const GPlatesAppLogic::AssignPlateIds::feature_property_flags_type &feature_property_types_to_assign,
		bool verify_information_model)
{
	// Order the tasks from most specific to least specific
	// since they'll get processed from front to back of the list.
	partition_feature_task_ptr_seq_type tasks;

	// VirtualGeomagneticPole task.
	tasks.push_back(
			partition_feature_task_ptr_type(
					new VgpPartitionFeatureTask(
							verify_information_model)));

	// Generic default task.
	// NOTE: Must be last since it can process any feature type.
	tasks.push_back(
			partition_feature_task_ptr_type(
					new GenericPartitionFeatureTask(
							assign_plate_id_method,
							feature_property_types_to_assign,
							verify_information_model)));

	return tasks;
}
