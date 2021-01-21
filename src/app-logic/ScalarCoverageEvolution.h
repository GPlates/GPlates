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
			// Crustal thickness (in km)...
			CRUSTAL_THICKNESS_KMS,
			// Stretching (beta) factor where 'beta = Ti/T' ...
			CRUSTAL_STRETCHING_FACTOR,
			// Thinning (gamma) factor where 'gamma = (1 - T/Ti)' ...
			CRUSTAL_THINNING_FACTOR,

			// Subsidence (in km) of crustal lithosphere due to extension and thermal cooling...
			TECTONIC_SUBSIDENCE_KMS,

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

			explicit
			InitialEvolvedScalarCoverage(
					unsigned int num_scalar_values) :
				d_num_scalar_values(num_scalar_values)
			{  }

			/**
			 * Add initial scalar values for the specified scalar type (if any).
			 *
			 * Note that you are not required to add any initial scalar values for any scalar type
			 * since evolved scalar values can be generated from default initial scalar values.
			 */
			void
			add_initial_scalar_values(
					EvolvedScalarType scalar_type,
					const std::vector<double> &initial_scalar_values);

			/**
			 * Returns the initial scalar values for the specified scalar type (if any).
			 */
			const boost::optional<std::vector<double>> &
			get_initial_scalar_values(
					EvolvedScalarType scalar_type) const
			{
				return d_initial_scalar_values[scalar_type];
			}

			/**
			 * Returns number of scalar values (per scalar type).
			 */
			unsigned int
			get_num_scalar_values() const
			{
				return d_num_scalar_values;
			}

		private:
			//! Number of scalar values (per scalar type).
			unsigned int d_num_scalar_values;

			/**
			 * The initial scalar values for the evolved scalar types (if any).
			 *
			 * This is none for those scalar types that did not provide initial values.
			 * However, these scalar types can still later be queried, in which case their values
			 * will be derived from the crustal thickness *factor* (and the default initial
			 * crustal thickness in the case of the CRUSTAL_THICKNESS_KMS scalar type).
			 */
			boost::optional<std::vector<double>> d_initial_scalar_values[NUM_EVOLVED_SCALAR_TYPES];
		};


		/**
		 * The default initial crustal thickness (in km) to use when no initial scalar values are provided for it.
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


			/**
			 * The evolved state of the scalar values that needs to be stored for each time step.
			 */
			struct State
			{
				//! Default initial state.
				explicit
				State(
						unsigned int num_scalar_values);

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

				/**
				 * Subsidence (in km) due to crustal stretching and lithospheric thermal cooling.
				 *
				 * This is calculated on demand (hence the boost::optional).
				 */
				boost::optional<std::vector<double>> tectonic_subsidence_kms;
			};


			//! Create a new scalar coverage containing the default (initial) internal state.
			static
			non_null_ptr_type
			create(
					unsigned int num_scalar_values)
			{
				return non_null_ptr_type(new EvolvedScalarCoverage(num_scalar_values));
			}

			//! Create a new scalar coverage containing the specified internal state.
			static
			non_null_ptr_type
			create(
					const State &state_)
			{
				return non_null_ptr_type(new EvolvedScalarCoverage(state_));
			}


			State state;

		private:
			//! Default constructor that initialises to default (initial) state.
			explicit
			EvolvedScalarCoverage(
					unsigned int num_scalar_values) :
				state(num_scalar_values)
			{  }

			//! Constructor that copies the specified state to our internal storage.
			explicit
			EvolvedScalarCoverage(
					const State &state_) :
				state(state_)
			{  }
		};

		//! Typedef for a time span of evolved scalar coverages.
		typedef TimeSpanUtils::TimeWindowSpan<EvolvedScalarCoverage::non_null_ptr_type> time_span_type;


		TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type d_geometry_time_span;
		InitialEvolvedScalarCoverage d_initial_scalar_coverage;
		double d_initial_time;
		unsigned int d_num_scalar_values;
		time_span_type::non_null_ptr_type d_scalar_coverage_time_span;

		//! Tectonic subsidence is initialised only when/if it is first requested since it's relatively expensive.
		mutable bool d_have_initialised_tectonic_subsidence;


		ScalarCoverageEvolution(
				const InitialEvolvedScalarCoverage &initial_scalar_coverage,
				const double &initial_time,
				TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type geometry_time_span);

		void
		initialise();

		void
		evolve_time_steps(
				unsigned int start_time_slot,
				unsigned int end_time_slot);

		/**
		 * Evolves the current scalar values from the current time to the next time.
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
		 * Initialise tectonic subsidence from crustal stretching and lithospheric thermal cooling.
		 */
		void
		initialise_tectonic_subsidence() const;

		void
		evolve_lithospheric_temperature(
				std::vector<bool> &have_started_evolving_lithospheric_temperature,
				double *const lithospheric_temperature_integrated_over_depth_kms,
				double *current_temperature_depth,
				double *next_temperature_depth,
				unsigned int scalar_values_start_index,
				unsigned int scalar_values_end_index) const;

		void
		evolve_lithospheric_temperature_time_step(
				const double &time_increment,
				const std::vector< boost::optional<DeformationStrainRate> > &current_deformation_strain_rates,
				const std::vector< boost::optional<DeformationStrainRate> > &next_deformation_strain_rates,
				const EvolvedScalarCoverage::State &current_scalar_coverage_state,
				EvolvedScalarCoverage::State &next_scalar_coverage_state,
				// Each of the following three arrays contains
				// 'scalar_values_end_index - scalar_values_start_index' values...
				std::vector<bool> &have_started_evolving_lithospheric_temperature,
				double *const current_lithospheric_temperature_integrated_over_depth_kms,
				double *const next_lithospheric_temperature_integrated_over_depth_kms,
				double *const current_temperature_depth,
				double *const next_temperature_depth,
				unsigned int scalar_values_start_index,
				unsigned int scalar_values_end_index) const;

		void
		evolve_tectonic_subsidence(
				const double *const lithospheric_temperature_integrated_over_depth_kms,
				unsigned int scalar_values_start_index,
				unsigned int scalar_values_end_index) const;

		void
		evolve_tectonic_subsidence_time_steps(
				const double *const lithospheric_temperature_integrated_over_depth_kms,
				unsigned int start_time_slot,
				unsigned int end_time_slot,
				unsigned int scalar_values_start_index,
				unsigned int scalar_values_end_index) const;

		/**
		 * Each lithospheric-temperature-integrated-over-depth array contains
		 * 'scalar_values_end_index - scalar_values_start_index' values...
		 */
		void
		evolve_tectonic_subsidence_time_step(
				const EvolvedScalarCoverage::State &current_scalar_coverage_state,
				EvolvedScalarCoverage::State &next_scalar_coverage_state,
				const double *const current_lithospheric_temperature_integrated_over_depth_kms,
				const double *const next_lithospheric_temperature_integrated_over_depth_kms,
				unsigned int scalar_values_start_index,
				unsigned int scalar_values_end_index) const;

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
