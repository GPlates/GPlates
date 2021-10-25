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
 
#ifndef GPLATES_APP_LOGIC_VGPPARTITIONFEATURETASK_H
#define GPLATES_APP_LOGIC_VGPPARTITIONFEATURETASK_H

#include "PartitionFeatureTask.h"


namespace GPlatesAppLogic
{
	/**
	 * Task for assigning properties to VirtualGeomagneticPole features.
	 */
	class VgpPartitionFeatureTask :
			public PartitionFeatureTask
	{
	public:

		/**
		 * If 'verify_information_model' is true then feature property types are only added if they don't not violate the GPGIM.
		 */
		explicit
		VgpPartitionFeatureTask(
				bool verify_information_model);

		virtual
		bool
		can_partition_feature(
				const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref) const;


		virtual
		void
		partition_feature(
				const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection_ref,
				const GeometryCookieCutter &geometry_cookie_cutter,
				const ReconstructMethodInterface::Context &reconstruct_method_context,
				const double &reconstruction_time,
				bool respect_feature_time_period);

	private:
		bool d_verify_information_model;
	};
}

#endif // GPLATES_APP_LOGIC_VGPPARTITIONFEATURETASK_H
