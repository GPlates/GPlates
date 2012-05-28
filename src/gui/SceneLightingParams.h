/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_SCENELIGHTINGPARAMS_H
#define GPLATES_GUI_SCENELIGHTINGPARAMS_H

#include <boost/operators.hpp>
#include <boost/optional.hpp>

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "maths/types.h"
#include "maths/MathsUtils.h"

namespace GPlatesGui
{
	/**
	 * Parameters to control scene lighting such as light direction, ambient light level, etc.
	 */
	class SceneLightingParams :
			public boost::equality_comparable<SceneLightingParams>
	{
	public:

		/**
		 * Initial light direction is along x-axis which is latitude/longitude (0,0) which is initially
		 * facing the user when GPlates starts.
		 *
		 * Default to half ambient (non-lit) and half diffuse lighting since
		 * it gives good visual results for the user to start off with.
		 */
		SceneLightingParams() :
			d_lighting_enabled(false),
			d_light_direction_attached_to_view_frame(true),
			d_ambient_light_contribution(0.5),
			d_light_direction(1, 0, 0)
		{  }

		//! Enables (or disables) scene lighting.
		void
		enable_lighting(
				bool enable = true)
		{
			d_lighting_enabled = enable;
		}

		//! Returns true if scene lighting is enabled.
		bool
		is_lighting_enabled() const
		{
			return d_lighting_enabled;
		}

		/**
		 * Returns the ambient light contribution in the range [0,1].
		 *
		 * The lighting contribution for diffuse light is (1 - ambient).
		 * The diffuse contribution uses the light direction but ambient does not.
		 *
		 * An ambient contribution of 1.0 effectively leaves the input colours unchanged.
		 */
		const double &
		get_ambient_light_contribution() const
		{
			return d_ambient_light_contribution;
		}

		//! Sets the ambient light contribution - must be in the range [0,1].
		void
		set_ambient_light_contribution(
				const double &ambient_light_contribution)
		{
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					ambient_light_contribution >= 0 && ambient_light_contribution <= 1.0,
					GPLATES_ASSERTION_SOURCE);
			d_ambient_light_contribution = ambient_light_contribution;
		}

		//! Sets the light direction.
		void
		set_light_direction(
				const GPlatesMaths::UnitVector3D &light_direction)
		{
			d_light_direction = light_direction;
		}

		//! Gets the light direction.
		const GPlatesMaths::UnitVector3D &
		get_light_direction() const
		{
			return d_light_direction;
		}

		//! Enables (or disables) scene lighting.
		void
		set_light_direction_attached_to_view_frame(
				bool light_direction_attached_to_view_frame = true)
		{
			d_light_direction_attached_to_view_frame = light_direction_attached_to_view_frame;
		}

		/**
		 * Returns true if light direction is attached to the view frame (and hence rotates as the view rotates).
		 *
		 * If false then the light direction is attached to the world frame (and hence remains fixed to the globe).
		 */
		bool
		is_light_direction_attached_to_view_frame() const
		{
			return d_light_direction_attached_to_view_frame;
		}


		//! Equality comparison operator.
		bool
		operator==(
				const SceneLightingParams &rhs) const
		{
			return
				d_lighting_enabled == rhs.d_lighting_enabled &&
				d_light_direction_attached_to_view_frame == rhs.d_light_direction_attached_to_view_frame &&
				GPlatesMaths::are_almost_exactly_equal(d_ambient_light_contribution, rhs.d_ambient_light_contribution) &&
				d_light_direction == rhs.d_light_direction;
		}

	private:

		bool d_lighting_enabled;
		bool d_light_direction_attached_to_view_frame;

		double d_ambient_light_contribution;
		GPlatesMaths::UnitVector3D d_light_direction;
	};
}

#endif // GPLATES_GUI_SCENELIGHTINGPARAMS_H
