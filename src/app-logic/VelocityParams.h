/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_VELOCITYPARAMS_H
#define GPLATES_APP_LOGIC_VELOCITYPARAMS_H

#include <boost/operators.hpp>
#include <boost/optional.hpp>

#include "VelocityDeltaTime.h"

#include "maths/MathsUtils.h"
#include "maths/Real.h"

// Try to only include the heavyweight "Scribe.h" in '.cc' files where possible.
#include "scribe/Transcribe.h"


namespace GPlatesAppLogic
{
	/**
	 * VelocityParams is used to store additional parameters for calculating velocities in
	 * @a VelocityFieldCalculatorLayerTask layers.
	 */
	class VelocityParams :
			public boost::less_than_comparable<VelocityParams>,
			public boost::equality_comparable<VelocityParams>
	{
	public:

		//! How to calculate velocities.
		enum SolveVelocitiesMethodType
		{
			// Intersects reconstructed domain geometry with polygon/network surface and
			// calculates velocity of latter at the position of the former.
			SOLVE_VELOCITIES_OF_SURFACES_AT_DOMAIN_POINTS,

			// Calculates velocity of reconstructed domain geometry itself.
			SOLVE_VELOCITIES_OF_DOMAIN_POINTS,

			// NOTE: Any new values should also be added to @a transcribe.

			NUM_SOLVE_VELOCITY_METHODS    // This must be last.
		};


		VelocityParams();


		SolveVelocitiesMethodType
		get_solve_velocities_method() const
		{
			return d_solve_velocities_method;
		}

		void
		set_solve_velocities_method(
				SolveVelocitiesMethodType solve_velocities_method)
		{
			d_solve_velocities_method = solve_velocities_method;
		}


		VelocityDeltaTime::Type
		get_delta_time_type() const
		{
			return d_delta_time_type;
		}

		void
		set_delta_time_type(
				VelocityDeltaTime::Type delta_time_calculation)
		{
			d_delta_time_type = delta_time_calculation;
		}


		double
		get_delta_time() const
		{
			return d_delta_time.dval();
		}

		void
		set_delta_time(
				const double &delta_time)
		{
			d_delta_time = delta_time;
		}


		bool
		get_is_boundary_smoothing_enabled() const
		{
			return d_is_boundary_smoothing_enabled;
		}

		void
		set_is_boundary_smoothing_enabled(
				bool is_boundary_smoothing_enabled = true)
		{
			d_is_boundary_smoothing_enabled = is_boundary_smoothing_enabled;
		}


		/**
		 * Specifies the angular distance (radians) over which velocities are smoothed across
		 * a plate/network boundary.
		 *
		 * If any points of the reconstructed velocity domain lie within this distance from a
		 * boundary then their velocity is interpolated between the domain point's calculated velocity
		 * and the average velocity (at the nearest boundary point) using the distance-to-boundary
		 * for interpolation. The average velocity at the boundary point is the average of the
		 * velocities a very small (epsilon) distance on either side of the boundary.
		 *
		 * The smoothing occurs over boundaries of topological boundaries/networks and static polygons.
		 */
		double
		get_boundary_smoothing_angular_half_extent_degrees() const
		{
			return d_boundary_smoothing_angular_half_extent_degrees.dval();
		}

		void
		set_boundary_smoothing_angular_half_extent_degrees(
				const double &boundary_smoothing_angular_half_extent_degrees)
		{
			d_boundary_smoothing_angular_half_extent_degrees = boundary_smoothing_angular_half_extent_degrees;
		}


		bool
		get_exclude_deforming_regions_from_smoothing() const
		{
			return d_exclude_deforming_regions_from_smoothing;
		}

		void
		set_exclude_deforming_regions_from_smoothing(
				bool exclude_deforming_regions_from_smoothing = true)
		{
			d_exclude_deforming_regions_from_smoothing = exclude_deforming_regions_from_smoothing;
		}


		//! Equality comparison operator.
		bool
		operator==(
				const VelocityParams &rhs) const;

		//! Less than comparison operator.
		bool
		operator<(
				const VelocityParams &rhs) const;

	private:

		SolveVelocitiesMethodType d_solve_velocities_method;

		VelocityDeltaTime::Type d_delta_time_type;
		GPlatesMaths::Real d_delta_time;

		bool d_is_boundary_smoothing_enabled;
		GPlatesMaths::Real d_boundary_smoothing_angular_half_extent_degrees;
		bool d_exclude_deforming_regions_from_smoothing;

	private: // Transcribe for sessions/projects...

		friend class GPlatesScribe::Access;

		GPlatesScribe::TranscribeResult
		transcribe(
				GPlatesScribe::Scribe &scribe,
				bool transcribed_construct_data);
	};


	/**
	 * Transcribe for sessions/projects.
	 */
	GPlatesScribe::TranscribeResult
	transcribe(
			GPlatesScribe::Scribe &scribe,
			VelocityParams::SolveVelocitiesMethodType &solve_velocities_method_type,
			bool transcribed_construct_data);
}

#endif // GPLATES_APP_LOGIC_VELOCITYPARAMS_H
