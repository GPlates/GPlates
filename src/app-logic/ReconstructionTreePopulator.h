/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_RECONSTRUCTIONTREEPOPULATOR_H
#define GPLATES_APP_LOGIC_RECONSTRUCTIONTREEPOPULATOR_H

#include <boost/optional.hpp>

#include "maths/FiniteRotation.h"
#include "model/FeatureVisitor.h"
#include "model/PropertyName.h"
#include "model/types.h"
#include "property-values/GeoTimeInstant.h"


namespace GPlatesAppLogic
{
	class ReconstructionGraph;

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
			public GPlatesModel::FeatureVisitorThatGuaranteesNotToModify
	{
	public:
		/**
		 * Returns true if @a feature_ref can be processed by @a ReconstructionTreePopulator.
		 */
		static
		bool
		can_process(
				const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref);



		struct ReconstructionSequenceAccumulator
		{
			boost::optional<GPlatesModel::PropertyName> d_most_recent_propname_read;
			boost::optional<GPlatesModel::integer_plate_id_type> d_fixed_ref_frame;
			boost::optional<GPlatesModel::integer_plate_id_type> d_moving_ref_frame;
			boost::optional<GPlatesMaths::FiniteRotation> d_finite_rotation;
			bool d_is_expecting_a_finite_rotation;

			ReconstructionSequenceAccumulator():
				d_is_expecting_a_finite_rotation(false)
			{  }

		};

		ReconstructionTreePopulator(
				const double &recon_time,
				ReconstructionGraph &graph);

		virtual
		~ReconstructionTreePopulator()
		{  }

	protected:

		virtual
		bool
		initialise_pre_feature_properties(
				GPlatesModel::FeatureHandle &feature_handle);

		virtual
		void
		finalise_post_feature_properties(
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

		virtual
		void
		visit_gpml_plate_id(
				GPlatesPropertyValues::GpmlPlateId &gpml_plate_id);

	private:

		const GPlatesPropertyValues::GeoTimeInstant d_recon_time;
		ReconstructionGraph *d_graph_ptr;
		boost::optional<ReconstructionSequenceAccumulator> d_accumulator;

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

#endif  // GPLATES_APP_LOGIC_RECONSTRUCTIONTREEPOPULATOR_H
