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
					return d_time_span->get_num_all_scalar_values();
				}

			private:
				ScalarCoverageTimeSpan::non_null_ptr_to_const_type d_time_span;

				// Only class ScalarCoverageTimeSpan can instantiate us.
				explicit
				ScalarCoverage(
						ScalarCoverageTimeSpan::non_null_ptr_to_const_type time_span) :
					d_time_span(time_span)
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
			 * Returns true if this scalar coverage (time span) contains the specified scalar type.
			 */
			bool
			contains_scalar_type(
					const scalar_type_type &scalar_type) const;

			/**
			 * Returns the scalar values at the specified time.
			 *
			 * Note: Only scalar values at *active* points are returned (which means the size of
			 * @a scalar_values could be less than @a get_num_all_scalar_values).
			 * And the order of scalar values matches the order of associated points
			 * returned by 'TopologyReconstruct::GeometryTimeSpan::get_geometry_data()'.
			 *
			 * Returns false if @a is_valid returns false or @a scalar_type is not in the scalar coverage
			 * (in which case @a scalar_values is unmodified).
			 */
			bool
			get_scalar_values(
					const scalar_type_type &scalar_type,
					const double &reconstruction_time,
					std::vector<double> &scalar_values) const;

			/**
			 * Returns the scalar values at *all* points at the specified time (including inactive points).
			 *
			 * Note: Inactive points will store 'false' at the equivalent index in @a scalar_values_are_active
			 * (such that the size of @a scalar_values and @a scalar_values_are_active will always be @a get_num_all_scalar_values).
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
					std::vector<double> &scalar_values,
					std::vector<bool> &scalar_values_are_active) const;

			/**
			 * Returns whether each scalar value, of *all* scalar values (regardless of scalar type)
			 * at the specified time, is active or not. Note that the scalar type has no effect here.
			 *
			 * The same could be achieved by calling
			 * 'get_all_scalar_values(scalar_type, reconstruction_time, scalar_values, scalar_values_are_active)'
			 * and then testing 'scalar_values_are_active', but this method is easier.
			 *
			 * Returns false if @a is_valid returns false (in which case @a scalar_values_are_active is not modified).
			 */
			bool
			get_are_scalar_values_active(
					const double &reconstruction_time,
					std::vector<bool> &scalar_values_are_active) const;

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


			//! Optional geometry time span if one was used to obtain deformation info to evolve scalar values.
			boost::optional<TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type> d_geometry_time_span;

			/**
			 * Optional evolved scalar coverage time span.
			 *
			 * Only scalar types that evolve (due to deformation) are handled here.
			 *
			 * This is none if there's no deformed geometry time span.
			 * If there is a deformed geometry time span then this is not none, even if no initial
			 * scalar values were provided for any of the *evolved* scalar types (affected by deformation)
			 * because evolved scalar types can use *default* initial values.
			 */
			boost::optional<ScalarCoverageEvolution::non_null_ptr_type> d_evolved_scalar_coverage_time_span;

			/**
			 * All scalar values corresponding to scalar types that do *not* evolve over time (due to deformation).
			 *
			 * These scalar values do not change over time and hence are not stored in the scalar coverage time span.
			 */
			non_evolved_scalar_coverage_type d_non_evolved_scalar_coverage;

			double d_scalar_import_time;

			//! The number of scalar values (active and inactive) per scalar type.
			unsigned int d_num_all_scalar_values;


			explicit
			ScalarCoverageTimeSpan(
					const initial_scalar_coverage_type &initial_scalar_coverage);

			ScalarCoverageTimeSpan(
					const initial_scalar_coverage_type &initial_scalar_coverage,
					TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type geometry_time_span);

			static
			std::vector<double>
			create_import_scalar_values(
					const std::vector<double> &scalar_values,
					TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type geometry_time_span);
		};
	}
}

#endif // GPLATES_APP_LOGIC_SCALARCOVERAGEDEFORMATION_H
