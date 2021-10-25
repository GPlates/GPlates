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
		 * Creates a @a ReconstructMethodHalfStageRotation object associated with the specified feature.
		 */
		static
		ReconstructMethodHalfStageRotation::non_null_ptr_type
		create(
				const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
				const Context &context)
		{
			return non_null_ptr_type(new ReconstructMethodHalfStageRotation(feature_ref, context));
		}


		/**
		 * Returns the present day geometries of the feature associated with this reconstruct method.
		 */
		virtual
		void
		get_present_day_feature_geometries(
				std::vector<Geometry> &present_day_geometries) const;


		/**
		 * Reconstructs the feature associated with this reconstruct method to the specified
		 * reconstruction time and returns one or more reconstructed feature geometries.
		 */
		virtual
		void
		reconstruct_feature_geometries(
				std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &reconstructed_feature_geometries,
				const ReconstructHandle::type &reconstruct_handle,
				const Context &context,
				const double &reconstruction_time);


		/**
		 * Calculates velocities at the positions of the reconstructed feature geometries, of the feature
		 * associated with this reconstruct method, at the specified reconstruction time and returns
		 * one or more reconstructed feature *velocities*.
		 */
		virtual
		void
		reconstruct_feature_velocities(
				std::vector<MultiPointVectorField::non_null_ptr_type> &reconstructed_feature_velocities,
				const ReconstructHandle::type &reconstruct_handle,
				const Context &context,
				const double &reconstruction_time,
				const double &velocity_delta_time,
				VelocityDeltaTime::Type velocity_delta_time_type);


		/**
		 * Reconstructs the specified geometry from present day to the specified reconstruction time -
		 * unless @a reverse_reconstruct is true in which case the geometry is assumed to be
		 * the reconstructed geometry (at the reconstruction time) and the returned geometry will
		 * then be the present day geometry.
		 */
		virtual
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
		reconstruct_geometry(
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry,
				const Context &context,
				const double &reconstruction_time,
				bool reverse_reconstruct);

	private:

		explicit
		ReconstructMethodHalfStageRotation(
				const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
				const Context &context) :
			ReconstructMethodInterface(ReconstructMethod::HALF_STAGE_ROTATION, feature_ref)
		{  }
	};
}

#endif // GPLATES_APP_LOGIC_RECONSTRUCTMETHODHALFSTAGEROTATION_H
