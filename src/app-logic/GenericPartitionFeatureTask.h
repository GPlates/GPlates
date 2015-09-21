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
 
#ifndef GPLATES_APP_LOGIC_GENERICPARTITIONFEATURETASK_H
#define GPLATES_APP_LOGIC_GENERICPARTITIONFEATURETASK_H

#include <boost/optional.hpp>

#include "PartitionFeatureTask.h"

#include "PartitionFeatureUtils.h"
#include "ReconstructionTree.h"

#include "model/FeatureCollectionHandle.h"


namespace GPlatesModel
{
	class Gpgim;
}

namespace GPlatesAppLogic
{
	/**
	 * Generic task for assigning properties to a feature.
	 *
	 * This is the last resort after all special case tasks have been tried first.
	 */
	class GenericPartitionFeatureTask :
			public PartitionFeatureTask
	{
	public:

		/**
		 * If 'verify_information_model' is true then feature property types are only added if they don't not violate the GPGIM.
		 */
		GenericPartitionFeatureTask(
				const GPlatesModel::Gpgim &gpgim,
				const ReconstructionTree &reconstruction_tree,
				GPlatesAppLogic::AssignPlateIds::AssignPlateIdMethodType assign_plate_id_method,
				const GPlatesAppLogic::AssignPlateIds::feature_property_flags_type &feature_property_types_to_assign,
				bool verify_information_model);


		virtual
		bool
		can_partition_feature(
				const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref) const
		{
			return feature_ref.is_valid();
		}


		virtual
		void
		partition_feature(
				const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection_ref,
				const GeometryCookieCutter &geometry_cookie_cutter,
				bool respect_feature_time_period);

	private:
		const GPlatesModel::Gpgim &d_gpgim;
		bool d_verify_information_model;

		const ReconstructionTree &d_reconstruction_tree;
		GPlatesAppLogic::AssignPlateIds::AssignPlateIdMethodType d_assign_plate_id_method;
		GPlatesAppLogic::AssignPlateIds::feature_property_flags_type d_feature_property_types_to_assign;


		void
		partition_feature(
				const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
				const boost::shared_ptr<const PartitionFeatureUtils::PartitionedFeature> &partitioned_feature,
				PartitionFeatureUtils::PartitionedFeatureManager &partitioned_feature_manager);


		void
		assign_feature_to_plate_it_overlaps_the_most(
				const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
				const boost::shared_ptr<const PartitionFeatureUtils::PartitionedFeature> &partitioned_feature,
				PartitionFeatureUtils::PartitionedFeatureManager &partitioned_feature_manager);

		void
		assign_feature_sub_geometry_to_plate_it_overlaps_the_most(
				const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
				const boost::shared_ptr<const PartitionFeatureUtils::PartitionedFeature> &partitioned_feature,
				PartitionFeatureUtils::PartitionedFeatureManager &partitioned_feature_manager);

		void
		partition_feature_into_plates(
				const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
				const boost::shared_ptr<const PartitionFeatureUtils::PartitionedFeature> &partitioned_feature,
				PartitionFeatureUtils::PartitionedFeatureManager &partitioned_feature_manager);
	};
}

#endif // GPLATES_APP_LOGIC_GENERICPARTITIONFEATURETASK_H
