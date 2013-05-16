/*
 * Fragment shader source code to render sphere normals as part of clearing a render target before
 * rendering a normal-map raster.
 *
 * For example, for a *regional* normal map raster the normals outside the region are not rendered by
 * the normal map raster itself and so must be initialised to be normal to the globe's surface.
 */

void main (void)
{
	// Normalize the interpolated 3D texture coordinate (which represents position on a rendered cube).
	// The cube vertices are unit length but not between vertices.
	vec3 sphere_normal = normalize(gl_TexCoord[0].xyz);

	// All components of world-space normal are signed and need to be converted to
	// unsigned ([-1,1] -> [0,1]) before storing in fixed-point 8-bit RGBA render target.
	gl_FragColor.xyz = 0.5 * sphere_normal + 0.5;
	gl_FragColor.w = 1;
}
