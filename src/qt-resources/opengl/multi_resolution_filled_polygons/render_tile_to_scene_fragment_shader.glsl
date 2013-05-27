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

/*
 * Fragment shader source code to render a tile to the scene.
 *
 * If we're near the edge of a polygon (and there's no adjacent polygon)
 * then the fragment alpha will not be 1.0 (also happens if clipped).
 * This reduces the anti-aliasing affect of the bilinear filtering since the bilinearly
 * filtered alpha will soften the edge (during the alpha-blend stage) but also the RGB colour
 * has been bilinearly filtered with black (RGB of zero) which is a double-reduction that
 * reduces the softness of the anti-aliasing.
 * To get around this we revert the effect of blending with black leaving only the alpha-blending.
 */

uniform sampler2D tile_texture_sampler;

#ifdef SURFACE_LIGHTING
uniform float light_ambient_contribution;
uniform vec3 world_space_light_direction;
// The world-space coordinates are interpolated across the geometry.
varying vec3 world_space_position;
#endif

#ifdef ENABLE_CLIPPING
uniform sampler2D clip_texture_sampler;
#endif // ENABLE_CLIPPING

void main (void)
{
	// Projective texturing to handle cube map projection.
	gl_FragColor = texture2DProj(tile_texture_sampler, gl_TexCoord[0]);

#ifdef ENABLE_CLIPPING
	gl_FragColor *= texture2DProj(clip_texture_sampler, gl_TexCoord[1]);
#endif // ENABLE_CLIPPING

	// As a small optimisation discard the pixel if the alpha is zero.
	if (gl_FragColor.a == 0)
		discard;

	// Revert effect of blending with black texels near polygon edge.
	gl_FragColor.rgb /= gl_FragColor.a;

#ifdef SURFACE_LIGHTING
	// Apply the Lambert diffuse lighting using the world-space position as the globe surface normal.
	// Note that neither the light direction nor the surface normal need be normalised.
	float lambert = lambert_diffuse_lighting(world_space_light_direction, world_space_position);

	// Blend between ambient and diffuse lighting.
	float lighting = mix_ambient_with_diffuse_lighting(lambert, light_ambient_contribution);

	gl_FragColor.rgb *= lighting;
#endif
}
