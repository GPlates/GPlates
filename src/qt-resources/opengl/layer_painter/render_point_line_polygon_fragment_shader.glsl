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

//
// Fragment shader source code to render points, lines and polygons with lighting.
//

uniform bool map_view_enabled;

uniform bool lighting_enabled;

// In the map view the lighting is constant across the map because
// the surface normal is constant across the map (ie, it's flat unlike the 3D globe).
uniform float map_view_ambient_and_diffuse_lighting;

uniform float globe_view_light_ambient_contribution;
uniform vec3 globe_view_world_space_light_direction;

// The world-space coordinates are interpolated across the geometry.
varying vec3 globe_view_world_space_position;

void main (void)
{
	// The interpolated fragment colour.
	vec4 colour = gl_Color;

	if (lighting_enabled)
	{
		if (map_view_enabled)
		{
			colour.rgb *= map_view_ambient_and_diffuse_lighting;
		}
		else // globe view ...
		{
			// Apply the Lambert diffuse lighting using the world-space position as the globe surface normal.
			// Note that neither the light direction nor the surface normal need be normalised.
			float lambert = lambert_diffuse_lighting(globe_view_world_space_light_direction, globe_view_world_space_position);

			// Blend between ambient and diffuse lighting.
			float ambient_and_diffuse_lighting = mix_ambient_with_diffuse_lighting(lambert, globe_view_light_ambient_contribution);

			colour.rgb *= ambient_and_diffuse_lighting;
		}
	}

	// The final fragment colour.
	gl_FragColor = colour;
}
