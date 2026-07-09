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

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include "ReconstructionGraphBuilder.h"

#include "maths/FiniteRotation.h"

#include "model/FeatureHandle.h"
#include "model/FeatureVisitor.h"
#include "model/PropertyName.h"
#include "model/types.h"

#include "property-values/GeoTimeInstant.h"


namespace GPlatesAppLogic
{
	/**
	 * Populate a ReconstructionGraph instance (via a ReconstructionGraphBuilder) with total reconstruction sequences.
	 */
	class ReconstructionGraphPopulator :
			public GPlatesModel::ConstFeatureVisitor,
			private boost::noncopyable
	{
	public:
		/**
		 * Returns true if @a feature_ref can be processed by @a ReconstructionGraphPopulator.
		 */
		static
		bool
		can_process(
				const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref);


		/**
		 * When reconstruction features are visited, total reconstruction sequences will get inserted
		 * into ReconstructionGraphBuilder.
		 */
		explicit
		ReconstructionGraphPopulator(
				ReconstructionGraphBuilder &graph_builder);

		virtual
		~ReconstructionGraphPopulator()
		{  }

	protected:

		virtual
		bool
		initialise_pre_feature_properties(
				feature_handle_type &feature_handle);

		virtual
		void
		finalise_post_feature_properties(
				feature_handle_type &feature_handle);

		virtual
		void
		visit_gpml_finite_rotation(
				gpml_finite_rotation_type &gpml_finite_rotation);

		virtual
		void
		visit_gpml_irregular_sampling(
				gpml_irregular_sampling_type &gpml_irregular_sampling);

		virtual
		void
		visit_gpml_plate_id(
				gpml_plate_id_type &gpml_plate_id);

	private:

		struct ReconstructionSequenceAccumulator
		{
			boost::optional<GPlatesModel::integer_plate_id_type> d_fixed_ref_frame;
			boost::optional<GPlatesModel::integer_plate_id_type> d_moving_ref_frame;
			ReconstructionGraphBuilder::total_reconstruction_pole_type d_total_reconstruction_pole;

			// Used when collecting a finite rotation...
			boost::optional<GPlatesPropertyValues::GeoTimeInstant> d_finite_rotation_time_instant;
			bool d_is_expecting_a_finite_rotation;

			ReconstructionSequenceAccumulator() :
				d_is_expecting_a_finite_rotation(false)
			{  }

			void
			reset()
			{
				d_fixed_ref_frame = boost::none;
				d_moving_ref_frame = boost::none;
				d_total_reconstruction_pole.clear();

				d_finite_rotation_time_instant = boost::none;
				d_is_expecting_a_finite_rotation = false;
			}
		};

		ReconstructionGraphBuilder &d_graph_builder;
		ReconstructionSequenceAccumulator d_accumulator;
	};
}

#endif  // GPLATES_APP_LOGIC_RECONSTRUCTIONTREEPOPULATOR_H
