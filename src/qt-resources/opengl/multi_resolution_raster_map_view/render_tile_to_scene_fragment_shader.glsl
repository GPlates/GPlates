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
 */

uniform sampler2D tile_texture_sampler;

#ifdef ENABLE_CLIPPING
	uniform sampler2D clip_texture_sampler;
#endif // ENABLE_CLIPPING

#ifdef SOURCE_RASTER_IS_FLOATING_POINT
	uniform vec4 source_texture_dimensions;
#endif

void main (void)
{
#ifdef ENABLE_CLIPPING
	// Discard the pixel if it has been clipped by the clip texture.
	//
	// Note: Discarding is necessary for our 'data' floating-point textures,
	//       as opposed to just modulating the alpha channel as you could with
	//       fixed-point colour textures, because there is no alpha blending/testing
	//       for floating-point textures (and also they store data in red channel and
	//       coverage in green channel).
	float clip_mask = texture2DProj(clip_texture_sampler, gl_TexCoord[1]).a;
	if (clip_mask == 0)
		discard;
#endif // ENABLE_CLIPPING

#ifdef SOURCE_RASTER_IS_FLOATING_POINT
	// Do the texture transform projective divide.
	vec2 source_texture_coords = gl_TexCoord[0].st / gl_TexCoord[0].q;
	// Bilinearly filter the tile texture (data/coverage is in red/green channel).
	// The texture access in 'bilinearly_interpolate' starts a new indirection phase.
	gl_FragColor = bilinearly_interpolate_data_coverge_RG(
		 tile_texture_sampler, source_texture_coords, source_texture_dimensions);
#else
	// Projective texturing to handle cube map projection.
	// Use hardware bilinear interpolation of fixed-point texture.
	gl_FragColor = texture2DProj(tile_texture_sampler, gl_TexCoord[0]);
#endif
}
