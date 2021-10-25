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
 * Vertex shader source to extract target raster in region-of-interest in preparation
 * for reduction operations.
 */

// The 'xy' values are (translate, scale)
const vec2 clip_space_to_texture_space_transform = vec2(0.5, 0.5);

attribute vec4 screen_space_position;
// The 'xyz' values are (translate_x, translate_y, scale)
attribute vec3 raster_frustum_to_seed_frustum_clip_space_transform;
// The 'xyz' values are (translate_x, translate_y, scale)
attribute vec3 seed_frustum_to_render_target_clip_space_transform;

#ifdef FILTER_MOMENTS
	varying vec4 view_position;
#endif

void main (void)
{
	// Post-projection translate/scale to position NDC space around raster frustum.
	// NOTE: We actually need to take the 'inverse' transform since we want to go from
	// seed frustum to raster frustum rather than the opposite direction.
	// See 'GPlatesUtils::QuadTreeClipSpaceTransform::get_inverse_translate_x()'
	// for details of the inverse transform.
	vec3 loose_seed_frustum_to_raster_frustum_clip_space_transform = vec3(
			-raster_frustum_to_seed_frustum_clip_space_transform.x / 
				raster_frustum_to_seed_frustum_clip_space_transform.z,
			-raster_frustum_to_seed_frustum_clip_space_transform.y / 
				raster_frustum_to_seed_frustum_clip_space_transform.z,
			1.0 / raster_frustum_to_seed_frustum_clip_space_transform.z);
	// This takes the 'screen_space_position' range [-1,1] and makes it cover the 
	// raster frustum (so [-1,1] covers the raster frustum).
	vec4 raster_frustum_position = vec4(
		// Scale and translate x component...
		dot(loose_seed_frustum_to_raster_frustum_clip_space_transform.zx,
				screen_space_position.xw),
		// Scale and translate y component...
		dot(loose_seed_frustum_to_raster_frustum_clip_space_transform.zy,
				screen_space_position.yw),
		// z and w components unaffected...
		screen_space_position.zw);

#ifdef FILTER_MOMENTS
	// Convert from the screen-space of the raster frustum to view-space using
	// the inverse view-projection *inverse* matrix.
	// The view position is used in the fragment shader to adjust for cube map distortion.
	//
	// Note: Seems gl_ModelViewProjectionMatrixInverse does not always work on Mac OS X.
	view_position = gl_ModelViewMatrixInverse * gl_ProjectionMatrixInverse * raster_frustum_position;
#endif

	// Post-projection translate/scale to position NDC space around render target frustum.
	// This takes the 'screen_space_position' range [-1,1] and makes it cover the 
	// render target frustum (so [-1,1] covers the render target frustum).
	vec4 render_target_frustum_position = vec4(
		// Scale and translate x component...
		dot(seed_frustum_to_render_target_clip_space_transform.zx,
				screen_space_position.xw),
		// Scale and translate y component...
		dot(seed_frustum_to_render_target_clip_space_transform.zy,
				screen_space_position.yw),
		// z and w components unaffected...
		screen_space_position.zw);

	// The target raster texture coordinates.
	// Convert clip-space range [-1,1] to texture coordinate range [0,1].
	gl_TexCoord[0] = vec4(
		// Scale and translate s component...
		dot(clip_space_to_texture_space_transform.yx,
				raster_frustum_position.xw),
		// Scale and translate t component...
		dot(clip_space_to_texture_space_transform.yx,
				raster_frustum_position.yw),
		// p and q components unaffected...
		raster_frustum_position.zw);

	// The region-of-interest mask texture coordinates.
	// Convert clip-space range [-1,1] to texture coordinate range [0,1].
	gl_TexCoord[1] = vec4(
		// Scale and translate s component...
		dot(clip_space_to_texture_space_transform.yx,
				render_target_frustum_position.xw),
		// Scale and translate t component...
		dot(clip_space_to_texture_space_transform.yx,
				render_target_frustum_position.yw),
		// p and q components unaffected...
		render_target_frustum_position.zw);

	gl_Position = render_target_frustum_position;
}
