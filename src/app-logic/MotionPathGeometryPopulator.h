/* $Id: ReconstructedFeatureGeometryPopulator.h 9200 2010-08-11 03:48:32Z mchin $ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2010-08-11 05:48:32 +0200 (on, 11 aug 2010) $
 * 
 * Copyright (C) 2010, 2011 Geological Survey of Norway
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

#ifndef GPLATES_APP_LOGIC_MOTIONPATHGEOMETRYPOPULATOR_H
#define GPLATES_APP_LOGIC_MOTIONPATHGEOMETRYPOPULATOR_H

#include <vector>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include "MotionPathUtils.h"
#include "ReconstructedMotionPath.h"
#include "ReconstructedFeatureGeometry.h"
#include "ReconstructionFeatureProperties.h"
#include "ReconstructionTree.h"
#include "ReconstructionTreeCreator.h"

#include "maths/FiniteRotation.h"
#include "maths/PointOnSphere.h"

#include "model/FeatureVisitor.h"
#include "model/types.h"

#include "property-values/GeoTimeInstant.h"

namespace GPlatesAppLogic
{
	/**
	 * Reconstructs motion path features
	 */
	class MotionPathGeometryPopulator:
			public GPlatesModel::FeatureVisitor,
			private boost::noncopyable
	{
	public:
		MotionPathGeometryPopulator(
				std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &reconstructed_feature_geometries,
				const ReconstructionTreeCreator &reconstruction_tree_creator,
				const double &reconstruction_time);


		virtual
		~MotionPathGeometryPopulator()
		{  }

	protected:

		virtual
		bool
		initialise_pre_feature_properties(
				GPlatesModel::FeatureHandle &feature_handle);


		virtual
		void
		visit_gml_multi_point(
				GPlatesPropertyValues::GmlMultiPoint &gml_multi_point);


		virtual
		void
		visit_gml_point(
				GPlatesPropertyValues::GmlPoint &gml_point);

		virtual
		void
		visit_gpml_constant_value(
				GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value);

	private:

		void
		create_motion_path_geometry(
			const GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type &present_day_seed_point,
			const GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type &present_day_seed_geometry);


		/**
		 * The @a ReconstructedFeatureGeometry objects generated during reconstruction.
		 */
		std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &d_reconstructed_feature_geometries;

		/**
		 * The function to call (with a time/anchor argument) to get a @a ReconstructionTree.
		 */
		ReconstructionTreeCreator d_reconstruction_tree_creator;

		const GPlatesPropertyValues::GeoTimeInstant d_recon_time;

		MotionPathUtils::MotionPathPropertyFinder d_motion_track_property_finder;

		std::vector<GPlatesMaths::FiniteRotation> d_rotations;

	};
}

#endif  // GPLATES_APP_LOGIC_MOTIONPATHGEOMETRYPOPULATOR_H
