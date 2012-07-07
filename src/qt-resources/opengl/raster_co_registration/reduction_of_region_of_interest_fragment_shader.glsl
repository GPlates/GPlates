/*
 * Fragment shader source to reduce region-of-interest filter results.
 */

uniform sampler2D reduce_source_texture_sampler;
// 'x' component is half texel offset and 'y' component is negative of that.
uniform vec2 reduce_source_texture_half_texel_offset;

void main (void)
{
	// Get the texture coordinates of the four source texels.
	// Since it's a 2x2 -> 1x1 reduction the texture coordinate of the current pixel
	// will be equidistant from four source texels (in each texel's corner).
	vec2 st = gl_TexCoord[0].st;
	vec2 st00 = st + reduce_source_texture_half_texel_offset.yy;
	vec2 st01 = st + reduce_source_texture_half_texel_offset.yx;
	vec2 st10 = st + reduce_source_texture_half_texel_offset.xy;
	vec2 st11 = st + reduce_source_texture_half_texel_offset.xx;

	vec4 src[4];
	// Sample the four source texels.
	src[0] = texture2D(reduce_source_texture_sampler, st00);
	src[1] = texture2D(reduce_source_texture_sampler, st01);
	src[2] = texture2D(reduce_source_texture_sampler, st10);
	src[3] = texture2D(reduce_source_texture_sampler, st11);

#ifdef REDUCTION_SUM
	vec4 sum = vec4(0);

	// Apply the reduction operation on the four source texels.
	for (int n = 0; n < 4; ++n)
	{
		sum += src[n];
	}

	gl_FragColor = sum;
#endif

#ifdef REDUCTION_MIN
	// First find the maximum value and coverage.
	vec4 max_value = max(max(src[0], src[1]), max(src[2], src[3]));

	// If the coverage values are all zero then discard this fragment.
	// The framebuffer already has zero values meaning zero coverage.
	float max_coverage = max_value.a;
	if (max_coverage == 0)
		discard;

	// Apply the reduction operation on the four source texels.
	vec3 min_covered_value = max_value.rgb;
	for (int n = 0; n < 4; ++n)
	{
		// If the coverage is non-zero then find new minimum value, otherwise ignore.
		if (src[n].a > 0)
			min_covered_value = min(min_covered_value, src[n].rgb);
	}

	gl_FragColor = vec4(min_covered_value, max_coverage);
#endif

#ifdef REDUCTION_MAX
	// First find the maximum coverage.
	float max_coverage = max(max(src[0].a, src[1].a), max(src[2].a, src[3].a));

	// If the coverage values are all zero then discard this fragment.
	// The framebuffer already has zero values meaning zero coverage.
	if (max_coverage == 0)
		discard;

	// First find the minimum value.
	vec3 min_value = min(min(src[0].rgb, src[1].rgb), min(src[2].rgb, src[3].rgb));

	// Apply the reduction operation on the four source texels.
	vec3 max_covered_value = min_value;
	for (int n = 0; n < 4; ++n)
	{
		// If the coverage is non-zero then find new maximum value, otherwise ignore.
		if (src[n].a > 0)
			max_covered_value = max(max_covered_value, src[n].rgb);
	}

	gl_FragColor = vec4(max_covered_value, max_coverage);
#endif
}
