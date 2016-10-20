/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_SCALARCOVERAGEDEFORMATION_H
#define GPLATES_APP_LOGIC_SCALARCOVERAGEDEFORMATION_H

#include <vector>
#include <boost/optional.hpp>

#include "DeformationStrain.h"
#include "ReconstructedFeatureGeometry.h"
#include "ScalarCoverageEvolution.h"
#include "TimeSpanUtils.h"

#include "utils/ReferenceCount.h"


namespace GPlatesAppLogic
{
	namespace ScalarCoverageDeformation
	{
		/**
		 * Builds and keeps track of scalar values (associated with points in a geometry) over a time span.
		 */
		class ScalarCoverageTimeSpan :
				public GPlatesUtils::ReferenceCount<ScalarCoverageTimeSpan>
		{
		public:

			//! A convenience typedef for a shared pointer to a non-const @a ScalarCoverageTimeSpan.
			typedef GPlatesUtils::non_null_intrusive_ptr<ScalarCoverageTimeSpan> non_null_ptr_type;

			//! A convenience typedef for a shared pointer to a const @a ScalarCoverageTimeSpan.
			typedef GPlatesUtils::non_null_intrusive_ptr<const ScalarCoverageTimeSpan> non_null_ptr_to_const_type;


			/**
			 * A time span of domain nRFGs over a time range.
			 */
			typedef TimeSpanUtils::TimeSampleSpan<ReconstructedFeatureGeometry::non_null_ptr_type>
					domain_reconstruction_time_span_type;


			/**
			 * Creates an *empty* scalar coverage time span containing only present day scalars.
			 *
			 * Subsequently calling @a get_scalar_values will always return the present day scalars
			 * regardless of reconstruction time specified.
			 */
			static
			non_null_ptr_type
			create(
					const std::vector<double> &present_day_scalar_values)
			{
				return non_null_ptr_type(new ScalarCoverageTimeSpan(present_day_scalar_values));
			}


			/**
			 * Creates a scalar coverage time span containing scalars evolved (backwards) from present day.
			 *
			 * The time span of reconstructed/deformed feature geometries supply the deformation strains
			 * that are needed to evolve the scalar values over time (eg, crustal thickness).
			 */
			static
			non_null_ptr_type
			create(
					const scalar_evolution_function_type &scalar_evolution_function,
					const domain_reconstruction_time_span_type &domain_reconstruction_time_span,
					const std::vector<double> &present_day_scalar_values)
			{
				return non_null_ptr_type(
						new ScalarCoverageTimeSpan(
								scalar_evolution_function,
								domain_reconstruction_time_span,
								present_day_scalar_values));
			}


			/**
			 * Returns the (per-point) scalar values at the specified time (which can be any time within the
			 * valid time period of the geometry's feature - it's up to the caller to check that).
			 */
			std::vector<double>
			get_scalar_values(
					const double &reconstruction_time);

		private:

			//! Typedef for a sequence of (per-point) scalar values.
			typedef std::vector<double> scalar_value_seq_type;

			//! Typedef for a time span of scalar value sequences.
			typedef TimeSpanUtils::TimeWindowSpan<scalar_value_seq_type> scalar_values_time_span_type;


			scalar_values_time_span_type::non_null_ptr_type d_scalar_values_time_span;


			static
			scalar_value_seq_type
			create_rigid_scalar_values_sample(
					const double &reconstruction_time,
					const double &closest_younger_sample_time,
					const scalar_value_seq_type &closest_younger_sample);

			static
			scalar_value_seq_type
			interpolate_scalar_values_sample(
					const double &interpolate_position,
					const scalar_value_seq_type &first_sample,
					const scalar_value_seq_type &second_sample);

			static
			boost::optional<const std::vector<DeformationStrain> &>
			get_deformation_strain_rates(
					const domain_reconstruction_time_span_type &domain_reconstruction_time_span,
					unsigned int time_slot);

			explicit
			ScalarCoverageTimeSpan(
					const std::vector<double> &present_day_scalar_values);

			explicit
			ScalarCoverageTimeSpan(
					const scalar_evolution_function_type &scalar_evolution_function,
					const domain_reconstruction_time_span_type &domain_reconstruction_time_span,
					const std::vector<double> &present_day_scalar_values);

			void
			initialise_time_span(
					const scalar_evolution_function_type &scalar_evolution_function,
					const domain_reconstruction_time_span_type &domain_reconstruction_time_span);
		};
	}
}

#endif // GPLATES_APP_LOGIC_SCALARCOVERAGEDEFORMATION_H
