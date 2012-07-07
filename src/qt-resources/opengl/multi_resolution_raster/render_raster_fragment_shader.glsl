/*
 * Fragment shader source code to render a source raster as either a floating-point raster or
 * a normal-map raster.
 *
 * For a normal-map raster this converts the surface normals from tangent-space to world-space so
 * that they can be captured in a cube raster (which is decoupled from the raster geo-referencing,
 * or local tangent-space of raster).
 */
uniform sampler2D raster_texture_sampler;

void main (void)
{

#ifdef SURFACE_NORMALS

	// Sample the tangent-space normal in the source raster.
   vec3 tangent_space_normal = texture2D(raster_texture_sampler, gl_TexCoord[0].st).xyz;
	// Need to convert the x and y components from unsigned to signed ([0,1] -> [-1,1]).
	// The z component is always positive (in range [0,1]) so does not need conversion.
	tangent_space_normal.xy = 2 * tangent_space_normal.xy - 1;

	// Normalize each interpolated 3D texture coordinate.
	// They are unit length at vertices but not necessarily between vertices.
	// TODO: Might need to be careful if any interpolated vectors are zero length.
	vec3 tangent = normalize(gl_TexCoord[1].xyz);
	vec3 binormal = normalize(gl_TexCoord[2].xyz);
	vec3 normal = normalize(gl_TexCoord[3].xyz);

	// Convert tangent-space normal to world-space normal.
	vec3 world_space_normal = normalize(
		mat3(tangent, binormal, normal) * tangent_space_normal);

	// All components of world-space normal are signed and need to be converted to
	// unsigned ([-1,1] -> [0,1]) before storing in fixed-point 8-bit RGBA render target.
	gl_FragColor.xyz = 0.5 * world_space_normal + 0.5;
	gl_FragColor.w = 1;

#else

	// Just return the source raster.
   gl_FragColor = texture2D(raster_texture_sampler, gl_TexCoord[0].st);

#endif

}
