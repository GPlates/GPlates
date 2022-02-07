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
// Fragment shader source code to render a sphere either as:
//  (1) coloured (white) with lighting, or
//  (2) screen-space depth.
//

#ifdef WHITE_WITH_LIGHTING
	uniform bool lighting_enabled;
	uniform float light_ambient_contribution;
	uniform vec3 world_space_light_direction;

	// The world-space coordinates are interpolated across the geometry.
	varying vec3 world_space_position;
#endif

void main (void)
{
#ifdef WHITE_WITH_LIGHTING
	// The interpolated fragment colour.
	vec4 colour = gl_Color;

	if (lighting_enabled)
	{
		// Apply the Lambert diffuse lighting using the world-space position as the globe surface normal.
		// Note that neither the light direction nor the surface normal need be normalised.
		float lambert = lambert_diffuse_lighting(world_space_light_direction, world_space_position);

		// Blend between ambient and diffuse lighting.
		float lighting = mix_ambient_with_diffuse_lighting(lambert, light_ambient_contribution);

		colour.rgb *= lighting;
	}

	// The final fragment colour.
	gl_FragColor = colour;
#endif

#ifdef DEPTH_RANGE
	// Write *screen-space* depth (ie, depth range [-1,1] and not [0,1]).
	// This is what's used in the ray-tracing shader since it uses inverse model-view-proj matrix on the depth
	// to get world space position and that requires normalised device coordinates not window coordinates).
	// Ideally this should also consider the effects of glDepthRange but we'll assume it's set to [0,1].
	gl_FragColor = vec4(2 * gl_FragCoord.z - 1);
#endif
}
