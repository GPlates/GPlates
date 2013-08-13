/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2009 The University of Sydney, Australia
 * Copyright (C) 2010 Geological Survey of Norway
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

#ifndef GPLATES_FEATUREVISITORS_TOTALRECONSTRUCTIONSEQUENCEROTATIONINSERTER_H
#define GPLATES_FEATUREVISITORS_TOTALRECONSTRUCTIONSEQUENCEROTATIONINSERTER_H

#include <boost/optional.hpp>

#include "model/FeatureVisitor.h"
#include "model/PropertyName.h"
#include "model/types.h"
#include "property-values/GeoTimeInstant.h"
#include "property-values/GpmlTotalReconstructionPole.h"
#include "maths/FiniteRotation.h"

namespace GPlatesFileIO
{
	class PlatesRotationFileProxy;
}

namespace GPlatesFeatureVisitors
{
	/**
	 * Insert an updated finite rotation into a total reconstruction sequence for a particular
	 * reconstruction time.
	 *
	 * This is performed by applying (composing) the supplied Rotation to the finite rotation
	 * found/interpolated in the sequence for that time.
	 *
	 * This class is based very strongly on 'GPlatesAppLogic::ReconstructionTreePopulator'.
	 */
	class TotalReconstructionSequenceRotationInserter:
			public GPlatesModel::FeatureVisitor
	{
	public:
		TotalReconstructionSequenceRotationInserter(
				const double &recon_time,
				const GPlatesMaths::Rotation &rotation_to_apply,
				const QString &comment);

		virtual
		~TotalReconstructionSequenceRotationInserter()
		{  }

	protected:

		virtual
		bool
		initialise_pre_feature_properties(
				GPlatesModel::FeatureHandle &feature_handle);

		virtual
		void
		visit_gpml_finite_rotation(
				GPlatesPropertyValues::GpmlFiniteRotation &gpml_finite_rotation);

		virtual
		void
		visit_gpml_finite_rotation_slerp(
				GPlatesPropertyValues::GpmlFiniteRotationSlerp &gpml_finite_rotation_slerp);

		virtual
		void
		visit_gpml_irregular_sampling(
				GPlatesPropertyValues::GpmlIrregularSampling &gpml_irregular_sampling);

		void
		visit_gpml_total_reconstruction_pole(
				GPlatesPropertyValues::GpmlTotalReconstructionPole &pole)
		{
			visit_gpml_finite_rotation(pole);
		}

	private:

		const GPlatesPropertyValues::GeoTimeInstant d_recon_time;
		GPlatesMaths::Rotation d_rotation_to_apply;
		bool d_is_expecting_a_finite_rotation;
		bool d_trp_time_matches_exactly;
		boost::optional<GPlatesMaths::FiniteRotation> d_finite_rotation;
		QString d_comment;
		GPlatesFileIO::PlatesRotationFileProxy* d_grot_proxy;
		int d_moving_plate_id, d_fixed_plate_id;

		// This constructor should never be defined, because we don't want to allow
		// copy-construction.
		TotalReconstructionSequenceRotationInserter(
				const TotalReconstructionSequenceRotationInserter &);

		// This operator should never be defined, because we don't want to allow
		// copy-assignment.
		TotalReconstructionSequenceRotationInserter &
		operator=(
				const TotalReconstructionSequenceRotationInserter &);
	};
}

#endif  // GPLATES_FEATUREVISITORS_TOTALRECONSTRUCTIONSEQUENCEROTATIONINSERTER_H
