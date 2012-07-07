/*
 * Fragment shader source code to render a tile to the scene.
 */

uniform sampler2D tile_texture_sampler;

#ifdef ENABLE_CLIPPING
uniform sampler2D clip_texture_sampler;
#endif // ENABLE_CLIPPING

void main (void)
{
	// Projective texturing to handle cube map projection.
	gl_FragColor = texture2DProj(tile_texture_sampler, gl_TexCoord[0]);

#ifdef ENABLE_CLIPPING
	gl_FragColor *= texture2DProj(clip_texture_sampler, gl_TexCoord[1]);
#endif // ENABLE_CLIPPING
}
