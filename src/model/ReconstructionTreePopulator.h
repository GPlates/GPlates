/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_RECONSTRUCTIONTREEPOPULATOR_H
#define GPLATES_MODEL_RECONSTRUCTIONTREEPOPULATOR_H

#include <memory>  /* std::auto_ptr */
#include <boost/intrusive_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include "FeatureVisitor.h"
#include "GeoTimeInstant.h"


namespace GPlatesMaths
{
	class FiniteRotation;
}

namespace GPlatesModel
{
	class PropertyName;
	class ReconstructionTree;

	/**
	 * Populate a ReconstructionTree instance with total reconstruction poles for a particular
	 * reconstruction time.
	 *
	 * This operation may involve finite rotation interpolation.
	 *
	 * This class is effectively a re-distribution of the functionality of the function
	 * 'GPlatesMaths::RotationSequence::finiteRotationAtTime' over a FeatureVisitor, to enable
	 * the operation to be performed upon a Total Reconstruction Sequence feature in the GPGIM
	 * implementation.
	 */
	class ReconstructionTreePopulator:
			public FeatureVisitor
	{
	public:

		struct ReconstructionSequenceAccumulator
		{
			boost::scoped_ptr<PropertyName> d_most_recent_propname_read;
			boost::intrusive_ptr<GpmlPlateId> d_fixed_ref_frame;
			boost::intrusive_ptr<GpmlPlateId> d_moving_ref_frame;
			std::auto_ptr<GPlatesMaths::FiniteRotation> d_finite_rotation;
			bool d_is_expecting_a_finite_rotation;

			ReconstructionSequenceAccumulator():
				d_most_recent_propname_read(NULL),
				d_fixed_ref_frame(NULL),
				d_moving_ref_frame(NULL),
				d_finite_rotation(NULL),
				d_is_expecting_a_finite_rotation(false)
			{  }

		};

		explicit
		ReconstructionTreePopulator(
				const double &recon_time,
				ReconstructionTree &recon_tree);

		virtual
		~ReconstructionTreePopulator()
		{  }

		virtual
		void
		visit_feature_handle(
				FeatureHandle &feature_handle);

		virtual
		void
		visit_gpml_finite_rotation(
				GpmlFiniteRotation &gpml_finite_rotation);

		virtual
		void
		visit_gpml_finite_rotation_slerp(
				GpmlFiniteRotationSlerp &gpml_finite_rotation_slerp);

		virtual
		void
		visit_gpml_irregular_sampling(
				GpmlIrregularSampling &gpml_irregular_sampling);

		virtual
		void
		visit_gpml_plate_id(
				GpmlPlateId &gpml_plate_id);

		virtual
		void
		visit_gpml_time_sample(
				GpmlTimeSample &gpml_time_sample);

		virtual
		void
		visit_single_valued_property_container(
				SingleValuedPropertyContainer &single_valued_property_container);

	private:

		const GeoTimeInstant d_recon_time;
		ReconstructionTree *d_recon_tree_ptr;
		boost::scoped_ptr<ReconstructionSequenceAccumulator> d_accumulator;

		// This constructor should never be defined, because we don't want to allow
		// copy-construction.
		ReconstructionTreePopulator(
				const ReconstructionTreePopulator &);

		// This operator should never be defined, because we don't want to allow
		// copy-assignment.
		ReconstructionTreePopulator &
		operator=(
				const ReconstructionTreePopulator &);
	};
}

#endif  // GPLATES_MODEL_RECONSTRUCTIONTREEPOPULATOR_H
