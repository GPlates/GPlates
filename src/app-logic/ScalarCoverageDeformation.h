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

#include <map>
#include <vector>
#include <boost/optional.hpp>

#include "DeformationStrainRate.h"
#include "ScalarCoverageEvolution.h"
#include "TimeSpanUtils.h"
#include "TopologyReconstruct.h"

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


			//! Typedef for scalar type.
			typedef GPlatesPropertyValues::ValueObjectType scalar_type_type;

			/**
			 * Typedef for the initial scalar values associated with scalar types.
			 */
			typedef std::map<scalar_type_type, std::vector<double>> initial_scalar_coverage_type;

			/**
			 * Typedef for a sequence of (per-point) scalar values.
			 *
			 * Inactive points/scalars are none.
			 */
			typedef std::vector< boost::optional<double> > scalar_value_seq_type;


			/**
			 * Scalar values (associated with points in a geometry) for all scalar types in the
			 * range associated with the domain geometry.
			 *
			 * Some scalar types evolve over time (due to deformation) and while other scalar types do not.
			 * Furthermore, the scalar types that do *not* evolve can be deactivated over time if the geometry
			 * is reconstructed using topologies (otherwise the scalar values do not change/deactivate over time).
			 */
			class ScalarCoverage
			{
			public:

				/**
				 * Returns the number of scalar values returned by @a get_all_scalar_values.
				 *
				 * Note that this can be different from the number of original scalar values passed into
				 * @a ScalarCoverageTimeSpan::create if the associated topologically reconstructed geometry
				 * was tessellated (thus introducing new points and hence new interpolated scalar values).
				 */
				unsigned int
				get_num_all_scalar_values() const
				{
					return d_num_all_scalar_values;
				}

				/**
				 * Returns all scalar types contained in this scalar coverage.
				 */
				std::vector<scalar_type_type>
				get_scalar_types() const
				{
					std::vector<scalar_type_type> scalar_types;

					for (const auto &item : d_scalar_values_map)
					{
						scalar_types.push_back(item.first);
					}

					return scalar_types;
				}

				/**
				 * Returns the scalar values at the specified time.
				 *
				 * Note: Only scalar values at *active* points are returned (which means the size of
				 * the returned scalar value sequence could be less than @a get_num_all_scalar_values).
				 * And the order of scalar values matches the order of associated points
				 * returned by 'TopologyReconstruct::GeometryTimeSpan::get_geometry_data()'.
				 *
				 * Returns none if @a scalar_type is not contained in this scalar coverage.
				 */
				boost::optional<std::vector<double>>
				get_scalar_values(
						const scalar_type_type &scalar_type) const;

				boost::optional<std::vector<double>>
				get_evolved_scalar_values(
						ScalarCoverageEvolution::EvolvedScalarType evolved_scalar_type) const
				{
					return get_scalar_values(ScalarCoverageEvolution::get_scalar_type(evolved_scalar_type));
				}

				/**
				 * Returns the scalar values at *all* points at the specified time (including inactive points).
				 *
				 * Note: Inactive points will store 'none' (such that the size of the returned
				 * scalar value sequence will always be @a get_num_all_scalar_values).
				 * And the order of scalar values matches the order of associated points
				 * returned by 'TopologyReconstruct::GeometryTimeSpan::get_all_geometry_data()'.
				 *
				 * Returns none if @a scalar_type is not contained in this scalar coverage.
				 */
				boost::optional<const scalar_value_seq_type &>
				get_all_scalar_values(
						const scalar_type_type &scalar_type) const;

				boost::optional<const scalar_value_seq_type &>
				get_all_evolved_scalar_values(
						ScalarCoverageEvolution::EvolvedScalarType evolved_scalar_type) const
				{
					return get_all_scalar_values(ScalarCoverageEvolution::get_scalar_type(evolved_scalar_type));
				}

			private:

				typedef std::map<scalar_type_type, scalar_value_seq_type> scalar_values_map_type;

				/**
				 * Subsequently calling @a get_scalar_values will always return the specified scalar values
				 * regardless of reconstruction time specified.
				 */
				scalar_values_map_type d_scalar_values_map;

				unsigned int d_num_all_scalar_values;


				// Only class ScalarCoverageTimeSpan can instantiate us.
				explicit
				ScalarCoverage(
						unsigned int num_all_scalar_values) :
					d_num_all_scalar_values(num_all_scalar_values)
				{  }
				friend class ScalarCoverageTimeSpan;
			};


			/**
			 * Creates an *empty* scalar coverage time span containing only the specified initial scalar coverage.
			 *
			 * Subsequently calling @a get_scalar_coverage will always return the specified scalar coverage
			 * regardless of reconstruction time specified.
			 */
			static
			non_null_ptr_type
			create(
					const initial_scalar_coverage_type &initial_scalar_coverage)
			{
				return non_null_ptr_type(new ScalarCoverageTimeSpan(initial_scalar_coverage));
			}


			/**
			 * Creates a scalar coverage time span containing the time progression of a scalar coverage.
			 *
			 * The time span of reconstructed/deformed feature geometries, @a geometry_time_span,
			 * supplies the domain points associated with the scalar values. It contains deformation info
			 * such as strain rates that evolve our scalar values (eg, crustal thickness) and also
			 * deactivation info (associated with subducted/consumed points).
			 *
			 * If the scalar coverage contains scalar types that evolve (due to deformation) those
			 * scalar values are evolved over time (provided the geometry time span contains non-zero
			 * strain rates - ie, passed through a deforming network). For scalar types that do not
			 * evolve (due to deformation) the geometry time span is only used to deactivate points
			 * (and their associated scalar values) over time.
			 *
			 * If @a scalar_values represent the scalar values at the geometry import time of the
			 * geometry time span - see 'TopologyReconstruct::create_geometry_time_span()' for more details.
			 * Those scalar values are then evolved forward and/or backward in time as necessary to fill the
			 * time range of the specified geometry time span.
			 * Note that the number of scalar values generated by @a get_all_scalar_values can be different
			 * from the size of @a scalar_values here if the associated topologically reconstructed geometry
			 * captured in @a geometry_time_span was tessellated (thus introducing new points and hence new
			 * interpolated scalar values).
			 */
			static
			non_null_ptr_type
			create(
					const initial_scalar_coverage_type &initial_scalar_coverage,
					TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type geometry_time_span)
			{
				return non_null_ptr_type(new ScalarCoverageTimeSpan(initial_scalar_coverage, geometry_time_span));
			}


			/**
			 * Returns true if the scalar values are active at the specified reconstruction time.
			 *
			 * If all geometry points (associated with the scalar values) subduct (going forward in time)
			 * or are consumed by mid-ocean ridges (going backward in time) or both, then the time range
			 * in which the geometry is valid will be reduced from the normal unlimited range (ie, [-inf, inf]).
			 * Note that this is different than the *feature* time of appearance/disappearance which
			 * is outside the scope of this class (and must be handled by the caller).
			 */
			bool
			is_valid(
					const double &reconstruction_time) const;

			/**
			 * Returns the scalar coverage at the specified time.
			 *
			 * Returns none if @a is_valid returns false.
			 */
			boost::optional<ScalarCoverage>
			get_scalar_coverage(
					const double &reconstruction_time) const;

			/**
			 * Returns the scalar values at the specified time.
			 *
			 * Note: Only scalar values at *active* points are returned (which means the size of
			 * @a scalar_values could be less than @a get_num_all_scalar_values).
			 * And the order of scalar values matches the order of associated points
			 * returned by 'TopologyReconstruct::GeometryTimeSpan::get_geometry_data()'.
			 *
			 * Returns false if @a is_valid returns false (in which case @a scalar_values is unmodified).
			 */
			bool
			get_scalar_values(
					const scalar_type_type &scalar_type,
					const double &reconstruction_time,
					std::vector<double> &scalar_values) const;

			/**
			 * Returns the scalar values at *all* points at the specified time (including inactive points).
			 *
			 * Note: Inactive points will store 'none' (such that the size of @a scalar_values
			 * will always be @a get_num_all_scalar_values).
			 * And the order of scalar values matches the order of associated points
			 * returned by 'TopologyReconstruct::GeometryTimeSpan::get_all_geometry_data()'.
			 *
			 * Returns false if @a is_valid returns false or @a scalar_type is not in the scalar coverage
			 * (in which case @a scalar_values is unmodified).
			 */
			bool
			get_all_scalar_values(
					const scalar_type_type &scalar_type,
					const double &reconstruction_time,
					std::vector< boost::optional<double> > &scalar_values) const;

			/**
			 * Returns the number of scalar values returned by @a get_all_scalar_values.
			 *
			 * Note that this can be different from the number of original scalar values passed
			 * into @a create if the associated topologically reconstructed geometry was tessellated
			 * (thus introducing new points and hence new interpolated scalar values).
			 */
			unsigned int
			get_num_all_scalar_values() const
			{
				return d_num_all_scalar_values;
			}

			/**
			 * The time that we started topology reconstruction of the initial scalar values from.
			 *
			 * Returns 0.0 if there was no topology reconstruction (see @a create without a geometry time span).
			 */
			const double &
			get_scalar_import_time() const
			{
				return d_scalar_import_time;
			}

			/**
			 * Returns optional geometry time span if one was used (to obtain deformation info to
			 * evolve scalar values, or to deactivate points/scalars, or both).
			 *
			 * Returns none if a geometry time span was not used
			 * (ie, if associated domain geometry was not topologically reconstructed).
			 *
			 * If none is returned then the scalar values do not change over time,
			 * and are valid for all time (ie, @a is_valid always returns true).
			 */
			boost::optional<TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type>
			get_geometry_time_span() const
			{
				return d_geometry_time_span;
			}

		private:

			/**
			 * Typedef for the non-envolved scalar values associated with scalar types.
			 */
			typedef std::map<scalar_type_type, std::vector<double>> non_evolved_scalar_coverage_type;

			//! Typedef for a time span of evolved scalar coverages.
			typedef TimeSpanUtils::TimeWindowSpan<ScalarCoverageEvolution::EvolvedScalarCoverage> evolved_scalar_coverage_time_span_type;


			//! Optional geometry time span if one was used to obtain deformation info to evolve scalar values.
			boost::optional<TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type> d_geometry_time_span;

			/**
			 * Optional evolved scalar coverage time span.
			 *
			 * Only scalar types that evolve (due to deformation) are handled here.
			 *
			 * This is none if there's no deformed geometry time span or if none of the scalar types
			 * correspond to evolved scalar types (affected by deformation).
			 */
			boost::optional<evolved_scalar_coverage_time_span_type::non_null_ptr_type> d_evolved_scalar_coverage_time_span;

			/**
			 * All scalar values corresponding to scalar types that do *not* evolve over time (due to deformation).
			 *
			 * These scalar values do not change over time and hence are not stored in the scalar coverage time span.
			 */
			non_evolved_scalar_coverage_type d_non_evolved_scalar_coverage;

			//! The number of scalar values (active and inactive) per scalar type.
			unsigned int d_num_all_scalar_values;

			double d_scalar_import_time;

			//! The first time slot that the geometry becomes active (if was even de-activated going backward in time).
			boost::optional<unsigned int> d_time_slot_of_appearance;
			//! The last time slot that the geometry remains active (if was even de-activated going forward in time).
			boost::optional<unsigned int> d_time_slot_of_disappearance;


			explicit
			ScalarCoverageTimeSpan(
					const initial_scalar_coverage_type &initial_scalar_coverage);

			ScalarCoverageTimeSpan(
					const initial_scalar_coverage_type &initial_scalar_coverage,
					TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type geometry_time_span);

			void
			initialise_evolved_time_span(
					const TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type &geometry_time_span,
					const evolved_scalar_coverage_time_span_type::non_null_ptr_type &evolved_scalar_coverage_time_span,
					const ScalarCoverageEvolution::EvolvedScalarCoverage &import_evolved_scalar_coverage);

			bool
			evolve_time_steps(
					unsigned int start_time_slot,
					unsigned int end_time_slot,
					const TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type &geometry_time_span,
					const evolved_scalar_coverage_time_span_type::non_null_ptr_type &evolved_scalar_coverage_time_span,
					const ScalarCoverageEvolution::EvolvedScalarCoverage &import_evolved_scalar_coverage);

			static
			std::vector<double>
			create_import_scalar_values(
					const std::vector<double> &scalar_values,
					TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type geometry_time_span);

			ScalarCoverageEvolution::EvolvedScalarCoverage
			create_evolved_rigid_sample(
					const double &reconstruction_time,
					const double &closest_younger_sample_time,
					const ScalarCoverageEvolution::EvolvedScalarCoverage &closest_younger_sample);

			ScalarCoverageEvolution::EvolvedScalarCoverage
			interpolate_envolved_samples(
					const double &interpolate_position,
					const double &first_geometry_time,
					const double &second_geometry_time,
					const ScalarCoverageEvolution::EvolvedScalarCoverage &first_sample,
					const ScalarCoverageEvolution::EvolvedScalarCoverage &second_sample);
		};
	}
}

#endif // GPLATES_APP_LOGIC_SCALARCOVERAGEDEFORMATION_H
