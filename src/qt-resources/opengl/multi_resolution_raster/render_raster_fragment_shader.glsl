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
#ifdef SOURCE_RASTER_IS_FLOATING_POINT
	// Just return the source raster.
   gl_FragColor = texture2D(raster_texture_sampler, gl_TexCoord[0].st);
#endif

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
#endif

#ifdef SCALAR_GRADIENT
	// Sample the scalar and gradient in the RGBA channels.
    vec4 scalar_gradient = texture2D(raster_texture_sampler, gl_TexCoord[0].st);

	// NOTE: We don't normalize each interpolated 3D texture coordinate.
	// At least the tangent and binormal vectors have magnitudes corresponding to the
    // length of a texel in the u and v directions. The normal (radial) is unit length.
	vec3 tangent = gl_TexCoord[1].xyz;
	vec3 binormal = gl_TexCoord[2].xyz;
	vec3 normal = gl_TexCoord[3].xyz;
    
    // The scalar value is stored in the red channel.
    gl_FragColor.r = scalar_gradient.r;

    // The input texture gradient is a mixture of finite differences of the scalar field
    // and inverse distances along the tangent frame directions.
    // Complete the gradient generation by providing the tangent frame directions and the
    // missing components of the inverse distances (the inverse distance across a texel
    // along the tangent and binormal directions on the surface of the globe - the input texture
    // has taken care of radial distances for all tangent frame components).
    //
	// The gradient is stored in the (green,blue,alpha) channels.
	gl_FragColor.gba = mat3(tangent, binormal, normal) * scalar_gradient.gba;
#endif
}
