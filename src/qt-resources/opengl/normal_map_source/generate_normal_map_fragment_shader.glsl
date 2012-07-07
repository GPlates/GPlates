/*
 * Fragment shader source to generate normals from a height field.
 */

uniform sampler2D height_field_texture_sampler;

// (x,y,z.w) is (texture u scale, texture v scale, texture translate, height field scale).
uniform vec4 height_field_parameters;

void main (void)
{
	// The height field coverage is in the green channel.
	float height_coverage = texture2D(height_field_texture_sampler, gl_TexCoord[0].st).g;
	// If there's no height sample at the current texel then discard fragment and
	// rely on the default normal in the framebuffer.
	if (height_coverage == 0)
		discard;

	// Texel offsets in the height map (-1, 0, 1).
	vec3 height_field_texel_offsets = vec3(
		-height_field_parameters.z,
		0,
		height_field_parameters.z);

	// Get the texture coordinates of the eight height map texels
	// surrounding the current normal map texel.
	vec2 st = gl_TexCoord[0].st;
	vec2 st00 = st + height_field_texel_offsets.xx;
	vec2 st10 = st + height_field_texel_offsets.yx;
	vec2 st20 = st + height_field_texel_offsets.zx;
	vec2 st01 = st + height_field_texel_offsets.xy;
	vec2 st21 = st + height_field_texel_offsets.zy;
	vec2 st02 = st + height_field_texel_offsets.xz;
	vec2 st12 = st + height_field_texel_offsets.yz;
	vec2 st22 = st + height_field_texel_offsets.zz;

	vec2 height00 = texture2D(height_field_texture_sampler, st00).rg;
	vec2 height10 = texture2D(height_field_texture_sampler, st10).rg;
	vec2 height20 = texture2D(height_field_texture_sampler, st20).rg;
	vec2 height01 = texture2D(height_field_texture_sampler, st01).rg;
	vec2 height21 = texture2D(height_field_texture_sampler, st21).rg;
	vec2 height02 = texture2D(height_field_texture_sampler, st02).rg;
	vec2 height12 = texture2D(height_field_texture_sampler, st12).rg;
	vec2 height22 = texture2D(height_field_texture_sampler, st22).rg;

	// Coverage is in the green channel.
	bool have_coverage00 = (height00.y != 0);
	bool have_coverage10 = (height10.y != 0);
	bool have_coverage20 = (height20.y != 0);
	bool have_coverage01 = (height01.y != 0);
	bool have_coverage21 = (height21.y != 0);
	bool have_coverage02 = (height02.y != 0);
	bool have_coverage12 = (height12.y != 0);
	bool have_coverage22 = (height22.y != 0);

	float du = 0;
	if (have_coverage00 && have_coverage20)
		du += height20.x - height00.x;
	if (have_coverage01 && have_coverage21)
		du += height21.x - height01.x;
	if (have_coverage02 && have_coverage22)
		du += height22.x - height02.x;

	float dv = 0;
	if (have_coverage00 && have_coverage02)
		dv += height02.x - height00.x;
	if (have_coverage10 && have_coverage12)
		dv += height12.x - height10.x;
	if (have_coverage20 && have_coverage22)
		dv += height22.x - height20.x;

	float height_field_scale = height_field_parameters.w;
	du *= height_field_scale;
	dv *= height_field_scale;

	vec3 normal = normalize(vec3(-du, -dv, 1));

	// The normal will get stored as fixed-point unsigned 8-bit RGB.
	// The final normal gets stored in RGB channels of fragment colour.
	// Convert the x and y components from the range [-1,1] to [0,1].
	gl_FragColor.xy = 0.5 * normal.xy + 0.5;
	// Leave the z component as is since it's always positive.
	gl_FragColor.z = normal.z;
	gl_FragColor.w = 1;
}
