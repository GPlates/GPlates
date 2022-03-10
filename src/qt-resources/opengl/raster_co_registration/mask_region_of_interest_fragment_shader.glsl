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
 * Fragment shader source to extract target raster in region-of-interest in preparation
 * for reduction operations.
 */

uniform sampler2D target_raster_texture_sampler;
uniform sampler2D region_of_interest_mask_texture_sampler;

#ifdef FILTER_MOMENTS
	uniform vec3 cube_face_centre;
#endif

in VertexData
{
	vec4 target_raster_texture_coordinate;
	vec4 region_of_interest_mask_texture_coordinate;
	#ifdef FILTER_MOMENTS
		vec4 view_position;
	#endif
} fs_in;

layout(location = 0) out vec4 mask_data;

void main (void)
{
	float region_of_interest_mask = texture(
			region_of_interest_mask_texture_sampler,
			region_of_interest_mask_texture_coordinate.st).a;
	if (region_of_interest_mask == 0)
		discard;

	// NOTE: There's no need to bilinear filter since the projection frustums should be
	// such that we're sampling at texel centres.
	vec4 target_raster = texture(target_raster_texture_sampler, target_raster_texture_coordinate.st);
	// The red channel contains the raster data value and the green channel contains the coverage.
	float data = target_raster.r;
	float coverage = target_raster.g;

	// The target raster data value is in the red channel and is premultiplied with coverage
	// (so that bilinear/anisotropic GPU hardware filtering is correctly weighted).
	// So now we need to un-premultiply coverage by dividing by coverage.
	//
	// The end result essentially amounts to:
	//
	//   data = sum(weight(i) * coverage(i) * age(i))
	//          -------------------------------------
	//               sum(weight(i) * coverage(i))
	//
	// ...where 'weight(i)' is the texture filtering weight (eg, bilinear/anisotropic).
	//
	// This is important for MIN/MAX operations and also ensures MEAN correlates with MIN/MAX
	// - ie, a single pixel ROI should give same value for MIN/MAX and MEAN.
	// This typically occurs near a boundary between opaque and transparent regions.
	if (coverage > 0)
		data /= coverage;

	// The coverage is modulated by the region-of-interest mask.
	// Currently the ROI mask is either zero or one so this doesn't do anything
	// (because of the above discard) but will if smoothing near ROI boundary is added.
	coverage *= region_of_interest_mask;

#ifdef FILTER_MOMENTS
	// Adjust the coverage based on the area of the current pixel.
	// The adjustment will be 1.0 at the cube face centre less than 1.0 elsewhere.
	// NOTE: 'fs_in.view_position' only needs to be a vec3 and not a vec4 because we do not
	// need to do the projective divide by w because we are normalising anyway.
	// We normalize to project the view position onto the surface of the globe.
	// NOTE: We only need to do this adjustment for area-weighted operations.
	coverage *= dot(cube_face_centre, normalize(fs_in.view_position.xyz));

	// Output (r, g, a) channels as (C*D, C*D*D, C).
	// Where C is coverage and D is data value.
	// This is enough to cover both mean and standard deviation.
	mask_data = vec4(coverage * data, coverage * data * data, 0, coverage);
#endif

#ifdef FILTER_MIN_MAX
	// Output (r, a) channels as (D, C).
	// Where C is coverage and D is data value.
	// This is enough to cover both minimum and maximum.
	mask_data = vec4(data, 0, 0, coverage);
#endif
}
