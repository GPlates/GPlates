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

#ifndef GPLATES_APP_LOGIC_RECONSTRUCTMETHODHALFSTAGEROTATION_H
#define GPLATES_APP_LOGIC_RECONSTRUCTMETHODHALFSTAGEROTATION_H

#include <boost/optional.hpp>

#include "ReconstructedFeatureGeometry.h"
#include "ReconstructMethodInterface.h"

#include "utils/non_null_intrusive_ptr.h"


namespace GPlatesAppLogic
{
	/**
	 * Reconstructs a feature using its present day geometry and a half-stage rotation defined
	 * by its left and right plate id.
	 */
	class ReconstructMethodHalfStageRotation :
			public ReconstructMethodInterface
	{
	public:
		// Convenience typedefs for a shared pointer to a @a ReconstructMethodHalfStageRotation.
		typedef GPlatesUtils::non_null_intrusive_ptr<ReconstructMethodHalfStageRotation> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const ReconstructMethodHalfStageRotation> non_null_ptr_to_const_type;


		/**
		 * Returns true if can reconstruct the specified feature.
		 *
		 * Feature must have a geometry and a "gpml:reconstructionMethod" property of 'HalfStageRotation' and
		 * "gpml:leftPlate" and "gpml:rightPlate" properties.
		 */
		static
		bool
		can_reconstruct_feature(
				const GPlatesModel::FeatureHandle::const_weak_ref &feature_weak_ref);


		/**
		 * Creates a @a ReconstructMethodHalfStageRotation object.
		 */
		static
		ReconstructMethodHalfStageRotation::non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new ReconstructMethodHalfStageRotation());
		}


		/**
		 * Returns the present day geometries of the specified feature.
		 */
		virtual
		void
		get_present_day_geometries(
				std::vector<Geometry> &present_day_geometries,
				const GPlatesModel::FeatureHandle::weak_ref &feature_weak_ref) const;


		/**
		 * Reconstructs the specified feature at the specified reconstruction time and returns
		 * one more more reconstructed feature geometries.
		 */
		virtual
		void
		reconstruct_feature(
				std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &reconstructed_feature_geometries,
				const GPlatesModel::FeatureHandle::weak_ref &feature_weak_ref,
				const ReconstructParams &reconstruct_params,
				const ReconstructionTreeCreator &reconstruction_tree_creator,
				const double &reconstruction_time);

	private:
		ReconstructMethodHalfStageRotation()
		{  }
	};
}

#endif // GPLATES_APP_LOGIC_RECONSTRUCTMETHODHALFSTAGEROTATION_H
