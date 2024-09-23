/**
 * Copyright (C) 2024 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_PLATE_BOUNDARY_STATS_H
#define GPLATES_APP_LOGIC_PLATE_BOUNDARY_STATS_H

#include <boost/optional.hpp>
#include <cmath>
#include <map>
#include <vector>

#include "ResolvedTopologicalSection.h"
#include "ResolvedTopologicalSharedSubSegment.h"

#include "VelocityDeltaTime.h"
#include "VelocityUnits.h"

#include "maths/MathsUtils.h"
#include "maths/Real.h"
#include "maths/UnitVector3D.h"
#include "maths/Vector3D.h"

// Try to only include the heavyweight "Scribe.h" in '.cc' files where possible.
#include "scribe/Transcribe.h"

#include "utils/Earth.h"


namespace GPlatesAppLogic
{
	/**
	 * Statistics at a point location on a plate boundary.
	 */
	class PlateBoundaryStat
	{
	public:
		PlateBoundaryStat(
				const GPlatesMaths::PointOnSphere &point_,
				const double &length_,
				const GPlatesMaths::UnitVector3D &boundary_normal_,
				const GPlatesMaths::Vector3D &boundary_velocity_,
				const boost::optional<GPlatesMaths::Vector3D> &left_plate_velocity_,
				const boost::optional<GPlatesMaths::Vector3D> &right_plate_velocity_,
				const double &signed_distance_from_start_of_topological_section_,
				const double &signed_distance_to_end_of_topological_section_) :
			d_point(point_),
			d_length(length_),
			d_boundary_normal(boundary_normal_),
			d_boundary_velocity(boundary_velocity_),
			d_left_plate_velocity(left_plate_velocity_),
			d_right_plate_velocity(right_plate_velocity_),
			d_signed_distance_from_start_of_topological_section(signed_distance_from_start_of_topological_section_),
			d_signed_distance_to_end_of_topological_section(signed_distance_to_end_of_topological_section_)
		{  }

		//! Get the point location on a plate boundary.
		const GPlatesMaths::PointOnSphere &
		get_point_location() const
		{
			return d_point;
		}

		//! Get the length (in radians) of the plate boundary represented by the point location.
		double
		get_length() const
		{
			return d_length.dval();
		}

		/**
		 * Get the normal to the plate boundary (at the point location).
		 *
		 * This is the normal of the great circle arc segment the point is located on
		 * (which is to the left of the segment).
		 */
		const GPlatesMaths::UnitVector3D &
		get_boundary_normal() const
		{
			return d_boundary_normal;
		}

		/**
		 * Get the clockwise (East-wise) angle from North to the plate boundary normal (at the point location).
		 *
		 * The angle is in the range [0, 2*pi].
		 */
		double
		get_boundary_normal_azimuth() const;

		//! Get the velocity of the plate boundary itself (at the point location).
		const GPlatesMaths::Vector3D &
		get_boundary_velocity() const
		{
			return d_boundary_velocity;
		}

		//! Get the magnitude of velocity of the plate boundary (at the point location).
		double
		get_boundary_velocity_magnitude() const
		{
			return get_velocity_magnitude(d_boundary_velocity);
		}

		/**
		 * Get the angle of the plate boundary velocity relative to the boundary normal (at the point location).
		 *
		 * Clockwise angles are positive (anti-clockwise angles are negative).
		 *
		 * Returns zero if the plate boundary velocity vector has zero magnitude.
		 */
		double
		get_boundary_velocity_obliquity() const
		{
			return get_velocity_obliquity(d_boundary_velocity);
		}

		/**
		 * Get the orthogonal component (in direction of boundary normal) of the plate boundary velocity (at the point location).
		 */
		double
		get_boundary_velocity_orthogonal() const
		{
			return get_velocity_magnitude(d_boundary_velocity) * std::cos(get_velocity_obliquity(d_boundary_velocity));
		}

		/**
		 * Get the parallel component (in direction along boundary line) of the plate boundary velocity (at the point location).
		 */
		double
		get_boundary_velocity_parallel() const
		{
			return get_velocity_magnitude(d_boundary_velocity) * std::sin(get_velocity_obliquity(d_boundary_velocity));
		}

		/**
		 * Get the plate velocity of the left plate (at the point location).
		 *
		 * The left plate is with respect to the direction of the shared sub-segment (that this point is on).
		 *
		 * Returns none if there is no plate on the left.
		 */
		const boost::optional<GPlatesMaths::Vector3D> &
		get_left_plate_velocity() const
		{
			return d_left_plate_velocity;
		}

		/**
		 * Get the magnitude of plate velocity of the left plate (at the point location).
		 *
		 * Returns NaN if no left plate velocity.
		 */
		double
		get_left_plate_velocity_magnitude() const
		{
			return d_left_plate_velocity ? get_velocity_magnitude(d_left_plate_velocity.get()) : GPlatesMaths::quiet_nan<double>();
		}

		/**
		 * Get the angle of the left plate velocity relative to the boundary normal (at the point location).
		 *
		 * Clockwise angles are positive (anti-clockwise angles are negative).
		 *
		 * Returns NaN if no left plate velocity.
		 *
		 * Returns zero if the left plate velocity vector has zero magnitude.
		 */
		double
		get_left_plate_velocity_obliquity() const
		{
			return d_left_plate_velocity ? get_velocity_obliquity(d_left_plate_velocity.get()) : GPlatesMaths::quiet_nan<double>();
		}

		/**
		 * Get the orthogonal component (in direction of boundary normal) of the left plate velocity (at the point location).
		 *
		 * Returns NaN if no left plate velocity.
		 */
		double
		get_left_plate_velocity_orthogonal() const
		{
			return d_left_plate_velocity
					? get_velocity_magnitude(d_left_plate_velocity.get()) * std::cos(get_velocity_obliquity(d_left_plate_velocity.get()))
					: GPlatesMaths::quiet_nan<double>();
		}

		/**
		 * Get the parallel component (in direction along boundary line) of the left plate velocity (at the point location).
		 *
		 * Returns NaN if no left plate velocity.
		 */
		double
		get_left_plate_velocity_parallel() const
		{
			return d_left_plate_velocity
					? get_velocity_magnitude(d_left_plate_velocity.get()) * std::sin(get_velocity_obliquity(d_left_plate_velocity.get()))
					: GPlatesMaths::quiet_nan<double>();
		}

		/**
		 * Get the plate velocity of the right plate (at the point location).
		 *
		 * The right plate is with respect to the direction of the shared sub-segment (that this point is on).
		 *
		 * Returns none if there is no plate on the right.
		 */
		const boost::optional<GPlatesMaths::Vector3D> &
		get_right_plate_velocity() const
		{
			return d_right_plate_velocity;
		}

		/**
		 * Get the magnitude of plate velocity of the right plate (at the point location).
		 *
		 * Returns NaN if no right plate velocity.
		 */
		double
		get_right_plate_velocity_magnitude() const
		{
			return d_right_plate_velocity ? get_velocity_magnitude(d_right_plate_velocity.get()) : GPlatesMaths::quiet_nan<double>();
		}

		/**
		 * Get the angle of the right plate velocity relative to the boundary normal (at the point location).
		 *
		 * Clockwise angles are positive (anti-clockwise angles are negative).
		 *
		 * Returns NaN if no right plate velocity.
		 *
		 * Returns zero if the right plate velocity vector has zero magnitude.
		 */
		double
		get_right_plate_velocity_obliquity() const
		{
			return d_right_plate_velocity ? get_velocity_obliquity(d_right_plate_velocity.get()) : GPlatesMaths::quiet_nan<double>();
		}

		/**
		 * Get the orthogonal component (in direction of boundary normal) of the right plate velocity (at the point location).
		 *
		 * Returns NaN if no right plate velocity.
		 */
		double
		get_right_plate_velocity_orthogonal() const
		{
			return d_right_plate_velocity
					? get_velocity_magnitude(d_right_plate_velocity.get()) * std::cos(get_velocity_obliquity(d_right_plate_velocity.get()))
					: GPlatesMaths::quiet_nan<double>();
		}

		/**
		 * Get the parallel component (in direction along boundary line) of the right plate velocity (at the point location).
		 *
		 * Returns NaN if no right plate velocity.
		 */
		double
		get_right_plate_velocity_parallel() const
		{
			return d_right_plate_velocity
					? get_velocity_magnitude(d_right_plate_velocity.get()) * std::sin(get_velocity_obliquity(d_right_plate_velocity.get()))
					: GPlatesMaths::quiet_nan<double>();
		}

		/**
		 * Get the velocity of the right plate relative to the left plate (at the point location).
		 *
		 * Returns none if there is no plate on the left or no plate on the right.
		 */
		boost::optional<GPlatesMaths::Vector3D>
		get_convergence_velocity() const;

		/**
		 * Get the magnitude of convergence velocity (at the point location).
		 *
		 * If @a return_signed_magnitude is true then negate magnitude if plates are *diverging*.
		 *
		 * Returns NaN if there is no plate on the left or no plate on the right (ie, convergence velocity is none).
		 *
		 * Returns zero if the convergence velocity vector has zero magnitude.
		 */
		double
		get_convergence_velocity_magnitude(
				bool return_signed_magnitude = false) const;

		/**
		 * Get the angle of the convergence velocity relative to the boundary normal (at the point location).
		 *
		 * Clockwise angles are positive (anti-clockwise angles are negative).
		 *
		 * Since the boundary normal is to the left, an angle in the range [-pi/2, pi/2] represents convergence and
		 * an angle in the range [-pi, -pi/2] or [pi/2, pi] represents divergence.
		 *
		 * Returns NaN if there is no plate on the left or no plate on the right (ie, convergence velocity is none).
		 *
		 * Returns zero if the convergence velocity vector has zero magnitude.
		 */
		double
		get_convergence_velocity_obliquity() const;

		/**
		 * Get the orthogonal component (in direction of boundary normal) of the convergence velocity (at the point location).
		 *
		 * Returns NaN if there is no plate on the left or no plate on the right (ie, convergence velocity is none).
		 */
		double
		get_convergence_velocity_orthogonal() const
		{
			const boost::optional<GPlatesMaths::Vector3D> convergence_velocity = get_convergence_velocity();

			return convergence_velocity
					? get_velocity_magnitude(convergence_velocity.get()) * std::cos(get_velocity_obliquity(convergence_velocity.get()))
					: GPlatesMaths::quiet_nan<double>();
		}

		/**
		 * Get the parallel component (in direction along boundary line) of the convergence velocity (at the point location).
		 *
		 * Returns NaN if there is no plate on the left or no plate on the right (ie, convergence velocity is none).
		 */
		double
		get_convergence_velocity_parallel() const
		{
			const boost::optional<GPlatesMaths::Vector3D> convergence_velocity = get_convergence_velocity();

			return convergence_velocity
					? get_velocity_magnitude(convergence_velocity.get()) * std::sin(get_velocity_obliquity(convergence_velocity.get()))
					: GPlatesMaths::quiet_nan<double>();
		}

		/**
		 * Get the signed distance (in radians) from the *start* of the resolved topological section geometry
		 * (the part spanned by its shared sub-segments) to the current location
		 * (along the geometry of the resolved topological section).
		 *
		 * It is negative if point is on a rubber-band part of a plate boundary. That is, it's not on
		 * the actual resolved topological section geometry but on the part that rubber-bands (joins)
		 * the *start* of the resolved topological section geometry with an adjacent resolved topological section
		 * (that's also part of a plate boundary).
		 *
		 * A resolved topological section represents a distinct feature used as part of the boundary of a plate.
		 * So, depending on how the topological model is built, this could be considered the distance to the start
		 * of a trench if the topological section is a subduction zone, for example.
		 *
		 * This distance can include gaps (between consecutive shared sub-segments) that no plate uses as part of its boundary
		 * (these don't typically exist for a *global* topological model where plates cover the entire globe).
		 */
		double
		get_signed_distance_from_start_of_topological_section() const
		{
			return d_signed_distance_from_start_of_topological_section.dval();
		}

		/**
		 * Same as @a get_signed_distance_from_start_of_topological_section but returns its absolute value.
		 *
		 * This is the *absolute* distance (in radians) from the *start* of the resolved topological section geometry.
		 * So for points on a rubber-band part of the shared sub-segment the distance will be positive (instead of negative).
		 */
		double
		get_distance_from_start_of_topological_section() const
		{
			return abs(d_signed_distance_from_start_of_topological_section).dval();
		}


		/**
		 * Similar to @a get_signed_distance_from_start_of_topological_section, but it's the distance to the *end*
		 * of the resolved topological section geometry (the part spanned by its shared sub-segments).
		 *
		 * And, similarly, it is negative if point is on a rubber-band part of a plate boundary connecting
		 * to the *end* of the resolved topological section geometry.
		 */
		double
		get_signed_distance_to_end_of_topological_section() const
		{
			return d_signed_distance_to_end_of_topological_section.dval();
		}

		/**
		 * Same as @a get_signed_distance_to_end_of_topological_section but returns its absolute value.
		 *
		 * This is the *absolute* distance (in radians) to the *end* of the resolved topological section geometry.
		 * So for points on a rubber-band part of the shared sub-segment the distance will be positive (instead of negative).
		 */
		double
		get_distance_to_end_of_topological_section() const
		{
			return abs(d_signed_distance_to_end_of_topological_section).dval();
		}


		bool
		operator==(
				const PlateBoundaryStat &other) const
		{
			return d_point == other.d_point &&
					d_length == other.d_length &&
					d_boundary_normal == other.d_boundary_normal &&
					d_boundary_velocity == other.d_boundary_velocity &&
					d_left_plate_velocity == other.d_left_plate_velocity &&
					d_right_plate_velocity == other.d_right_plate_velocity &&
					d_signed_distance_from_start_of_topological_section == other.d_signed_distance_from_start_of_topological_section &&
					d_signed_distance_to_end_of_topological_section == other.d_signed_distance_to_end_of_topological_section;
		}

		bool
		operator!=(
				const PlateBoundaryStat &other) const
		{
			return !operator==(other);
		}

	private:

		double
		get_velocity_magnitude(
				const GPlatesMaths::Vector3D &velocity) const;

		double
		get_velocity_obliquity(
				const GPlatesMaths::Vector3D &velocity) const;


		//! Point location on a plate boundary.
		GPlatesMaths::PointOnSphere d_point;

		//! Length (in radians) of the plate boundary represented by the point location.
		GPlatesMaths::Real d_length;

		//! Normal to the plate boundary (at the point location).
		GPlatesMaths::UnitVector3D d_boundary_normal;

		//! Velocity of the plate boundary itself (at the point location).
		GPlatesMaths::Vector3D d_boundary_velocity;

		//! Plate velocity of the left plate (at point location), or none if no left plate.
		boost::optional<GPlatesMaths::Vector3D> d_left_plate_velocity;

		//! Plate velocity of the right plate (at point location), or none if no right plate.
		boost::optional<GPlatesMaths::Vector3D> d_right_plate_velocity;

		/**
		 * Signed distance (in radians) from the *start* of the resolved topological section geometry
		 * (the part spanned by its shared sub-segments) to the current location
		 * (along the geometry of the resolved topological section).
		 */
		GPlatesMaths::Real d_signed_distance_from_start_of_topological_section;

		/**
		 * Similar to @a distance_from_start_of_topological_section, but it's the distance to the *end*
		 * of the resolved topological section geometry (the part spanned by its shared sub-segments).
		 */
		GPlatesMaths::Real d_signed_distance_to_end_of_topological_section;

	private: // Transcribe...

		friend class GPlatesScribe::Access;

		static
		GPlatesScribe::TranscribeResult
		transcribe_construct_data(
				GPlatesScribe::Scribe &scribe,
				GPlatesScribe::ConstructObject<PlateBoundaryStat> &plate_boundary_stat);

		GPlatesScribe::TranscribeResult
		transcribe(
				GPlatesScribe::Scribe &scribe,
				bool transcribed_construct_data);
	};


	/**
	 * Calculates statistics along the plate boundaries specified by the resolved topological sections
	 * (each is a list of sub-segments shared by resolved topology boundaries, optionally including networks).
	 *
	 * Generates a sequence of uniformly-spaced points along each resolved topological section and calculates
	 * plate boundary statistics at each point. If a shared sub-segment of a resolved topological section
	 * is not long enough to contain any uniform points then it will be missing from the returned mapping.
	 *
	 * For each resolved topological section, its first point is located @a first_uniform_point_spacing radians
	 * from its first vertex and each subsequent point is separated by @a uniform_point_spacing radians.
	 * Since a resolved topological section contains one or more shared sub-segments, the uniform spacing
	 * is continuous across boundaries between shared sub-segments (unless there's a gap between them,
	 * in which case the spacing is reset to @a first_uniform_point_spacing for the next shared sub-segment).
	 *
	 * Returns a mapping of shared sub-segments to their plate boundary statistics (at uniform points along them).
	 */
	void
	calculate_plate_boundary_stats(
			std::map<ResolvedTopologicalSharedSubSegment::non_null_ptr_type, std::vector<PlateBoundaryStat>> &plate_boundary_stats,
			const std::vector<ResolvedTopologicalSection::non_null_ptr_type> &resolved_topological_sections,
			const double &reconstruction_time,
			const double &uniform_point_spacing,
			const double &first_uniform_point_spacing = 0.0,
			const double &velocity_delta_time = 1.0,
			VelocityDeltaTime::Type velocity_delta_time_type = VelocityDeltaTime::T_PLUS_DELTA_T_TO_T,
			VelocityUnits::Value velocity_units = VelocityUnits::CMS_PER_YR,
			const double &earth_radius_in_kms = GPlatesUtils::Earth::EQUATORIAL_RADIUS_KMS);
}

#endif // GPLATES_APP_LOGIC_PLATE_BOUNDARY_STATS_H
