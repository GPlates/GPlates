// Vertex shader source code to render a tile to the scene.

void main (void)
{
	// Position gets transformed exactly same as fixed-function pipeline.
	gl_Position = ftransform(); //gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

	// Transform 3D texture coords (present day position) by cube map projection and
	// any texture coordinate adjustments before accessing textures.
	// We have two texture transforms but only one texture coordinate.
	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
	gl_TexCoord[1] = gl_TextureMatrix[1] * gl_MultiTexCoord0;
}
