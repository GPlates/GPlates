/*
 * Fragment shader source to extract target raster in region-of-interest in preparation
 * for reduction operations.
 */

uniform sampler2D target_raster_texture_sampler;
uniform sampler2D region_of_interest_mask_texture_sampler;

#ifdef FILTER_MOMENTS
	uniform vec3 cube_face_centre;
	varying vec4 view_position;
#endif

void main (void)
{
	float region_of_interest_mask =
			texture2D(region_of_interest_mask_texture_sampler, gl_TexCoord[1].st).a;
	if (region_of_interest_mask == 0)
		discard;

	// NOTE: There's no need to bilinear filter since the projection frustums should be
	// such that we're sampling at texel centres.
	vec4 target_raster = texture2D(target_raster_texture_sampler, gl_TexCoord[0].st);
	// The red channel contains the raster data value and the green channel contains the coverage.
	float data = target_raster.r;
	float coverage = target_raster.g;

	// Due to bilinear filtering of the source raster (data and coverage) the data
	// value can be reduced depending on the bi-linearly filtered coverage value.
	// For example half of the four bilinearly filtered pixels might have zero coverage
	// and hence zero data values so we'd get '0.25 * (P00 + P01) + 0 * (P10 + P11)'
	// which gives us '0.25 * (D00 + D01)' for data and 0.25 * (1 + 1) for coverage
	// but we want '0.5 * (D00 + D01)' for data which is obtained by dividing by coverage.
	// So we need to undo that effect as best we can - this is important for MIN/MAX
	// operations and also ensures MEAN correlates with MIN/MAX - ie, a single pixel
	// ROI should give same value for MIN/MAX and MEAN.
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
	// NOTE: 'view_position' only needs to be a vec3 and not a vec4 because we do not
	// need to do the projective divide by w because we are normalising anyway.
	// We normalize to project the view position onto the surface of the globe.
	// NOTE: We only need to do this adjustment for area-weighted operations.
	coverage *= dot(cube_face_centre, normalize(view_position.xyz));

	// Output (r, g, a) channels as (C*D, C*D*D, C).
	// Where C is coverage and D is data value.
	// This is enough to cover both mean and standard deviation.
	gl_FragColor = vec4(coverage * data, coverage * data * data, 0, coverage);
#endif

#ifdef FILTER_MIN_MAX
	// Output (r, a) channels as (D, C).
	// Where C is coverage and D is data value.
	// This is enough to cover both minimum and maximum.
	gl_FragColor = vec4(data, 0, 0, coverage);
#endif
}
