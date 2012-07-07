/*
 * Vertex shader source code to render a tile to the scene.
 */

#ifdef SURFACE_LIGHTING
// The world-space coordinates are interpolated across the geometry.
varying vec3 world_space_position;
#endif

void main (void)
{
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

	// Transform present-day position by cube map projection and
	// any texture coordinate adjustments before accessing textures.
	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_Vertex;
	gl_TexCoord[1] = gl_TextureMatrix[1] * gl_Vertex;

#ifdef SURFACE_LIGHTING
    // This assumes the geometry does not need a model transform (eg, reconstruction rotation).
    world_space_position = gl_Vertex.xyz / gl_Vertex.w;
#endif
}
