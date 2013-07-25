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

#ifndef GPLATES_GUI_SCENELIGHTINGPARAMETERS_H
#define GPLATES_GUI_SCENELIGHTINGPARAMETERS_H

#include <bitset>
#include <boost/operators.hpp>
#include <boost/optional.hpp>

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "maths/types.h"
#include "maths/MathsUtils.h"
#include "maths/UnitVector3D.h"

namespace GPlatesGui
{
	/**
	 * Parameters to control scene lighting such as light direction, ambient light level, etc.
	 */
	class SceneLightingParameters :
			public boost::equality_comparable<SceneLightingParameters>
	{
	public:

		/**
		 * The types of primitives that lighting can be individually enabled/disabled for.
		 */
		enum LightingPrimitiveType
		{
			LIGHTING_POINT_POLYLINE_POLYGON,
			LIGHTING_FILLED_POLYGON,
			LIGHTING_RASTER,
			LIGHTING_SCALAR_FIELD,

			NUM_LIGHTING_PRIMITIVE_TYPES // Must be the last enum.
		};


		/**
		 * The initial light direction in the 3D globe views is along x-axis which is
		 * latitude/longitude (0,0) which is initially facing the user when GPlates starts.
		 *
		 * The initial light direction in the 2D map views is perpendicular to the map plane
		 * which is towards the viewer and hence along the z-axis.
		 *
		 * Default to 40% ambient (non-lit) and 60% diffuse lighting since
		 * it gives good visual contrast/results for the user to start off with.
		 */
		SceneLightingParameters() :
			d_light_direction_attached_to_view_frame(true),
			d_ambient_light_contribution(0.4),
			d_globe_view_light_direction(1, 0, 0),
			d_map_view_light_direction(0, 0, 1)
		{
			// Disable all lighting (except scalar fields) for now by default - until the lighting
			// canvas tool is in place - since it will have controls to individually enable/disable
			// lighting for vector geometries, filled geometries, rasters, scalar fields, etc...
			d_lighting_primitives_enable_state.set(LIGHTING_SCALAR_FIELD);
		}

		//! Enables (or disables) scene lighting for the specified lighting primitive.
		void
		enable_lighting(
				LightingPrimitiveType lighting_primitive_type,
				bool enable = true)
		{
			d_lighting_primitives_enable_state.set(lighting_primitive_type, enable);
		}

		//! Returns true if scene lighting is enabled for the specified lighting primitive.
		bool
		is_lighting_enabled(
				LightingPrimitiveType lighting_primitive_type) const
		{
			return d_lighting_primitives_enable_state.test(lighting_primitive_type);
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

		//! Sets the globe view light direction.
		void
		set_globe_view_light_direction(
				const GPlatesMaths::UnitVector3D &light_direction)
		{
			d_globe_view_light_direction = light_direction;
		}

		//! Gets the globe view light direction.
		const GPlatesMaths::UnitVector3D &
		get_globe_view_light_direction() const
		{
			return d_globe_view_light_direction;
		}

		//! Sets the map view light direction.
		void
		set_map_view_light_direction(
				const GPlatesMaths::UnitVector3D &map_view_light_direction)
		{
			d_map_view_light_direction = map_view_light_direction;
		}

		//! Gets the map view light direction.
		const GPlatesMaths::UnitVector3D &
		get_map_view_light_direction() const
		{
			return d_map_view_light_direction;
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
				const SceneLightingParameters &rhs) const
		{
			return
				d_lighting_primitives_enable_state == rhs.d_lighting_primitives_enable_state &&
				d_light_direction_attached_to_view_frame == rhs.d_light_direction_attached_to_view_frame &&
				GPlatesMaths::are_almost_exactly_equal(d_ambient_light_contribution, rhs.d_ambient_light_contribution) &&
				d_globe_view_light_direction == rhs.d_globe_view_light_direction &&
				d_map_view_light_direction == rhs.d_map_view_light_direction;
		}

	private:

		/**
		 * Type contains lighting enabled state.
		 */
		typedef std::bitset<NUM_LIGHTING_PRIMITIVE_TYPES> lighting_primitives_enable_state_type;


		/**
		 * Determines what lighting is enabled for.
		 */
		lighting_primitives_enable_state_type d_lighting_primitives_enable_state;

		bool d_light_direction_attached_to_view_frame;

		double d_ambient_light_contribution;

		/**
		 * The light direction for the 3D globe views.
		 */
		GPlatesMaths::UnitVector3D d_globe_view_light_direction;

		/**
		 * The light direction for the 2D map views.
		 */
		GPlatesMaths::UnitVector3D d_map_view_light_direction;
	};
}

#endif // GPLATES_GUI_SCENELIGHTINGPARAMETERS_H
