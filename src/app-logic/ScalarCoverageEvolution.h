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
#include <vector>
#include <boost/optional.hpp>

#include "DeformationStrainRate.h"
#include "TimeSpanUtils.h"
#include "TopologyReconstruct.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "property-values/ValueObjectType.h"


namespace GPlatesAppLogic
{
	/**
	 * This namespace contains functions that evolve the scalar values (in a scalar coverage).
	 *
	 * For example a scalar coverage containing crustal thickness scalars.
	 */
	namespace ScalarCoverageEvolution
	{
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

			NUM_EVOLVED_SCALAR_TYPES
		};


		//! Typedef for scalar type.
		typedef GPlatesPropertyValues::ValueObjectType scalar_type_type;

		/**
		 * Typedef for a sequence of (per-point) scalar values.
		 *
		 * Inactive scalars are none.
		 */
		typedef std::vector< boost::optional<double> > scalar_value_seq_type;

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
		 * Scalar values (associated with points in a geometry) for only those scalar types that
		 * evolve over time (due to deformation).
		 */
		class EvolvedScalarCoverage
		{
		public:

			explicit
			EvolvedScalarCoverage(
					const InitialEvolvedScalarCoverage &initial_scalar_coverage);

			/**
			 * Returns the scalar values at *all* points at the specified time (including inactive points).
			 *
			 * Note: Inactive points will store 'none' (such that the size of the returned
			 * scalar value sequence will always be @a get_num_all_scalar_values).
			 * And the order of scalar values matches the order of associated points
			 * returned by 'TopologyReconstruct::GeometryTimeSpan::get_all_geometry_data()'.
			 */
			const scalar_value_seq_type &
			get_scalar_values(
					EvolvedScalarType evolved_scalar_type) const
			{
				return d_scalar_values[evolved_scalar_type];
			}

			/**
			 * Returns number of scalar values (per scalar type).
			 */
			unsigned int
			get_num_scalar_values() const
			{
				return d_scalar_values[0].size();
			}

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
					const std::vector< boost::optional<DeformationStrainRate> > &current_deformation_strain_rates,
					const std::vector< boost::optional<DeformationStrainRate> > &next_deformation_strain_rates,
					const double &current_time,
					const double &next_time);

		private:
			scalar_value_seq_type d_scalar_values[NUM_EVOLVED_SCALAR_TYPES];

			//! Scalar values are initialised with default values (except crustal thickness).
			EvolvedScalarCoverage(
					unsigned int num_scalar_values);
		};

		//! Typedef for a time span of evolved scalar coverages.
		typedef TimeSpanUtils::TimeWindowSpan<EvolvedScalarCoverage> time_span_type;


		/**
		 * The default initial crustal thickness to use when no initial scalar values are provided for it.
		 */
		static constexpr double DEFAULT_INITIAL_CRUSTAL_THICKNESS_KMS = 40.0;

		/**
		 * Returns the scalar type associated with the specified evolved scalar type enumeration.
		 */
		scalar_type_type
		get_scalar_type(
				EvolvedScalarType evolved_scalar_type);

		/**
		 * Returns the evolved scalar type enumeration associated with the specified scalar type,
		 * otherwise returns none.
		 */
		boost::optional<EvolvedScalarType>
		is_evolved_scalar_type(
				const scalar_type_type &scalar_type);


		time_span_type::non_null_ptr_type
		create_evolved_time_span(
				const InitialEvolvedScalarCoverage &initial_scalar_coverage,
				const double &initial_time,
				TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type geometry_time_span);
	};
}

#endif // GPLATES_APP_LOGIC_SCALARCOVERAGEEVOLUTION_H
