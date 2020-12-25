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

#ifndef GPLATES_APP_LOGIC_SCALARCOVERAGEEVOLUTION_H
#define GPLATES_APP_LOGIC_SCALARCOVERAGEEVOLUTION_H

#include <map>
#include <utility>
#include <vector>
#include <boost/optional.hpp>

#include "DeformationStrainRate.h"
#include "TimeSpanUtils.h"
#include "TopologyReconstruct.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "property-values/ValueObjectType.h"

#include "utils/ReferenceCount.h"


namespace GPlatesAppLogic
{
	/**
	 * Evolve the scalar values (in a scalar coverage) as a result of deformation.
	 *
	 * For example a scalar coverage containing crustal thickness scalars.
	 */
	class ScalarCoverageEvolution :
			public GPlatesUtils::ReferenceCount<ScalarCoverageEvolution>
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<ScalarCoverageEvolution> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const ScalarCoverageEvolution> non_null_ptr_to_const_type;


		//! Type of evolved scalar (note: not all scalar types are evolved scalar types affected by deformation).
		enum EvolvedScalarType
		{
			// This can be absolute thickness (eg, in kms), or
			// a thickness ratio such as T/Ti (where T/Ti is current/initial thickness)...
			CRUSTAL_THICKNESS,
			// Stretching (beta) factor where 'beta = Ti/T' ...
			CRUSTAL_STRETCHING_FACTOR,
			// Thinning (gamma) factor where 'gamma = (1 - T/Ti)' ...
			CRUSTAL_THINNING_FACTOR,

			// Subsidence of crustal lithosphere due to extension and thermal cooling (in kms).
			TECTONIC_SUBSIDENCE,

			NUM_EVOLVED_SCALAR_TYPES
		};


		//! Typedef for scalar type.
		typedef GPlatesPropertyValues::ValueObjectType scalar_type_type;

		/**
		 * Initial scalar values (to be evolved).
		 */
		class InitialEvolvedScalarCoverage
		{
		public:

			//! Typedef for a map of evolved scalar types to initial scalar values (to be evolved).
			typedef std::map<EvolvedScalarType, std::vector<double>> initial_scalar_values_map_type;

			void
			add_scalar_values(
					EvolvedScalarType scalar_type,
					const std::vector<double> &initial_scalar_values);

			bool
			empty() const
			{
				return d_initial_scalar_values.empty();
			}

			const initial_scalar_values_map_type &
			get_scalar_values() const
			{
				return d_initial_scalar_values;
			}

			/**
			 * Returns number of scalar values (per scalar type).
			 *
			 * Throws exception if @a add_scalar_values has not yet been called at least once.
			 */
			unsigned int
			get_num_scalar_values() const
			{
				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						d_num_scalar_values,
						GPLATES_ASSERTION_SOURCE);

				return d_num_scalar_values.get();
			}

		private:
			//! The initial scalar values (to be evolved) associated with evolved scalar types.
			std::map<EvolvedScalarType, std::vector<double>> d_initial_scalar_values;

			//! Number of scalar values (per scalar type) - only initialised after first call to @a add_scalar_values.
			boost::optional<unsigned int> d_num_scalar_values;
		};


		/**
		 * The default initial crustal thickness to use when no initial scalar values are provided for it.
		 */
		static constexpr double DEFAULT_INITIAL_CRUSTAL_THICKNESS_KMS = 40.0;

		/**
		 * Returns the scalar type associated with the specified evolved scalar type enumeration.
		 */
		static
		scalar_type_type
		get_scalar_type(
				EvolvedScalarType evolved_scalar_type);

		/**
		 * Returns the evolved scalar type enumeration associated with the specified scalar type,
		 * otherwise returns none.
		 */
		static
		boost::optional<EvolvedScalarType>
		is_evolved_scalar_type(
				const scalar_type_type &scalar_type);


		/**
		 * Evolve scalar values over time (starting with the initial scalar values) and
		 * store them in the returned scalar coverage time span.
		 */
		static
		non_null_ptr_type
		create(
				const InitialEvolvedScalarCoverage &initial_scalar_coverage,
				const double &initial_time,
				TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type geometry_time_span)
		{
			return non_null_ptr_type(
					new ScalarCoverageEvolution(
							initial_scalar_coverage,
							initial_time,
							geometry_time_span));
		}


		/**
		 * Returns the scalar values at *all* points at the specified time (including inactive points).
		 *
		 * If scalar values for the specified scalar type have not yet been generated then they are first
		 * generated/evolved for all active times and stored @a time_span (which should be the time span
		 * created with @a create_evolved_time_span and containing this scalar coverage).
		 *
		 * Note: Inactive points will store 'false' at the equivalent index in @a scalar_values_are_active
		 * (such that the size of @a scalar_values and @a scalar_values_are_active will always be @a get_num_scalar_values).
		 * And the order of scalar values matches the order of associated points
		 * returned by 'TopologyReconstruct::GeometryTimeSpan::get_all_geometry_data()'.
		 */
		void
		get_scalar_values(
				EvolvedScalarType evolved_scalar_type,
				const double &reconstruction_time,
				std::vector<double> &scalar_values,
				std::vector<bool> &scalar_values_are_active) const;

		/**
		 * Returns number of scalar values (per scalar type).
		 */
		unsigned int
		get_num_scalar_values() const
		{
			return d_num_scalar_values;
		}

	private:

		/**
		 * A snapshot in time of the evolved scalar values (associated with points in a geometry).
		 *
		 * Note that this is only for those scalar types that *evolve* over time (due to deformation).
		 */
		class EvolvedScalarCoverage :
				public GPlatesUtils::ReferenceCount<EvolvedScalarCoverage>
		{
		public:
			typedef GPlatesUtils::non_null_intrusive_ptr<EvolvedScalarCoverage> non_null_ptr_type;
			typedef GPlatesUtils::non_null_intrusive_ptr<const EvolvedScalarCoverage> non_null_ptr_to_const_type;


			struct State
			{
				State(
						const InitialEvolvedScalarCoverage &initial_scalar_coverage);

				/**
				 * The scalar values for the evolved scalar types.
				 *
				 * This is none for those scalar types that did not provide initial values.
				 * However, these scalar types can still be queried (with @a get_scalar_values), in
				 * which case their values will be derived from @a d_crustal_thickness_factor (and the
				 * default initial crustal thickness in the case of the CRUSTAL_THICKNESS scalar type).
				 */
				boost::optional<std::vector<double>> scalar_values[NUM_EVOLVED_SCALAR_TYPES];

				/**
				 * Whether each scalar value (in each scalar type) is active or not.
				 * All scalar values are initially active and can become deactivated due to the
				 * associated geometry time span.
				 */
				std::vector<bool> scalar_values_are_active;

				/**
				 * T(n)/T(0)
				 *
				 * This is the only crustal thickness-related quantity that is actually evolved (solved).
				 * Because all the crustal thickness-related scalar types are derived from this.
				 */
				std::vector<double> crustal_thickness_factor;
			};


			/**
			 * Create from initial scalar values.
			 *
			 * All scalar values are initially active.
			 */
			static
			non_null_ptr_type
			create(
					const InitialEvolvedScalarCoverage &initial_scalar_coverage)
			{
				return non_null_ptr_type(new EvolvedScalarCoverage(initial_scalar_coverage));
			}

			//! Create a new scalar coverage containing the specified internal stated.
			static
			non_null_ptr_type
			create(
					const State &state_)
			{
				return non_null_ptr_type(new EvolvedScalarCoverage(state_));
			}


			State state;

		private:
			//! Constructor that copies the specified state to our internal storage.
			explicit
			EvolvedScalarCoverage(
					const State &state_) :
				state(state_)
			{  }

			explicit
			EvolvedScalarCoverage(
					const InitialEvolvedScalarCoverage &initial_scalar_coverage) :
				state(initial_scalar_coverage)
			{  }
		};

		//! Typedef for a time span of evolved scalar coverages.
		typedef TimeSpanUtils::TimeWindowSpan<EvolvedScalarCoverage::non_null_ptr_type> time_span_type;


		TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type d_geometry_time_span;
		time_span_type::non_null_ptr_type d_scalar_coverage_time_span;
		unsigned int d_num_scalar_values;
		double d_initial_time;


		ScalarCoverageEvolution(
				const InitialEvolvedScalarCoverage &initial_scalar_coverage,
				const double &initial_time,
				TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type geometry_time_span);

		void
		evolve_time_steps(
				unsigned int start_time_slot,
				unsigned int end_time_slot,
				EvolvedScalarCoverage::non_null_ptr_type import_scalar_coverage);

		/**
		 * Evolves the current scalar values from the current time to the next time.
		 *
		 * Note that 'boost::optional' is used for each point's scalar values and strain rate.
		 * This represents whether the associated point is active. Points can become inactive over time (active->inactive) but
		 * do not get re-activated (inactive->active). So if the current strain rate is inactive then so should the next strain rate.
		 * Also the active state of the current scalar value should match that of the current deformation strain rate
		 * (because they represent the same point). If the current scalar value is inactive then it just remains inactive.
		 * And if the current scalar value is active then it becomes inactive if the next strain rate is inactive, otherwise
		 * both (current and next) strain rates are active and the scalar value is evolved from its current value to its next value.
		 * This ensures the active state of the next scalar values match that of the next deformation strain rate
		 * (which in turn comes from the active state of the associated domain geometry point).
		 *
		 * Throws exception if the sizes of the strain rate arrays and the internal scalar values arrays do not match.
		 */
		void
		evolve_time_step(
				EvolvedScalarCoverage::State &current_scalar_coverage_state,
				const std::vector< boost::optional<DeformationStrainRate> > &current_deformation_strain_rates,
				const std::vector< boost::optional<DeformationStrainRate> > &next_deformation_strain_rates,
				const double &current_time,
				const double &next_time);

		/**
		 * The sample *creator* function for TimeSpanUtils::TimeWindowSpan<EvolvedScalarCoverage>.
		 */
		EvolvedScalarCoverage::non_null_ptr_type
		create_time_span_rigid_sample(
				const double &reconstruction_time,
				const double &closest_younger_sample_time,
				const EvolvedScalarCoverage::non_null_ptr_type &closest_younger_sample);

		/**
		 * The sample *interpolator* function for TimeSpanUtils::TimeWindowSpan<EvolvedScalarCoverage>.
		 */
		EvolvedScalarCoverage::non_null_ptr_type
		interpolate_time_span_samples(
				const double &interpolate_position,
				const double &first_geometry_time,
				const double &second_geometry_time,
				const EvolvedScalarCoverage::non_null_ptr_type &first_sample,
				const EvolvedScalarCoverage::non_null_ptr_type &second_sample);
	};
}

#endif // GPLATES_APP_LOGIC_SCALARCOVERAGEEVOLUTION_H
