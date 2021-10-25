/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2009, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_FEATUREVISITORS_TOTALRECONSTRUCTIONSEQUENCEROTATIONINTERPOLATER_H
#define GPLATES_FEATUREVISITORS_TOTALRECONSTRUCTIONSEQUENCEROTATIONINTERPOLATER_H

#include <boost/optional.hpp>

#include "maths/FiniteRotation.h"

#include "model/FeatureVisitor.h"
#include "model/PropertyName.h"
#include "model/types.h"

#include "property-values/GeoTimeInstant.h"


namespace GPlatesFeatureVisitors
{
	/**
	 * Interpolate a finite rotation from a total reconstruction sequence for a particular
	 * reconstruction time.
	 *
	 * This class is based very strongly on 'GPlatesAppLogic::ReconstructionTreePopulator'.
	 */
	class TotalReconstructionSequenceRotationInterpolater:
			public GPlatesModel::ConstFeatureVisitor
	{
	public:
		TotalReconstructionSequenceRotationInterpolater(
				const double &recon_time);

		virtual
		~TotalReconstructionSequenceRotationInterpolater()
		{  }

		const boost::optional<GPlatesMaths::FiniteRotation> &
		result() const
		{
			return d_finite_rotation_result;
		}

	protected:

		virtual
		bool
		initialise_pre_feature_properties(
				const GPlatesModel::FeatureHandle &feature_handle);

		virtual
		void
		visit_gpml_finite_rotation(
				const GPlatesPropertyValues::GpmlFiniteRotation &gpml_finite_rotation);

		virtual
		void
		visit_gpml_finite_rotation_slerp(
				const GPlatesPropertyValues::GpmlFiniteRotationSlerp &gpml_finite_rotation_slerp);

		virtual
		void
		visit_gpml_irregular_sampling(
				const GPlatesPropertyValues::GpmlIrregularSampling &gpml_irregular_sampling);

	private:

		const GPlatesPropertyValues::GeoTimeInstant d_recon_time;
		bool d_is_expecting_a_finite_rotation;
		bool d_trp_time_matches_exactly;

		boost::optional<GPlatesMaths::FiniteRotation> d_finite_rotation_result;
		boost::optional<GPlatesMaths::FiniteRotation> d_finite_rotation_for_interp;

		// This constructor should never be defined, because we don't want to allow
		// copy-construction.
		TotalReconstructionSequenceRotationInterpolater(
				const TotalReconstructionSequenceRotationInterpolater &);

		// This operator should never be defined, because we don't want to allow
		// copy-assignment.
		TotalReconstructionSequenceRotationInterpolater &
		operator=(
				const TotalReconstructionSequenceRotationInterpolater &);
	};
}

#endif  // GPLATES_FEATUREVISITORS_TOTALRECONSTRUCTIONSEQUENCEROTATIONINTERPOLATER_H
