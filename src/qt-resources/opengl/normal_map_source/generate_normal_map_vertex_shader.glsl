/*
 * Vertex shader source to generate normals from a height field.
 */

// (x,y,z.w) is (texture u scale, texture v scale, texture translate, height field scale).
uniform vec4 height_field_parameters;

void main (void)
{
	// Scale and translate the texture coordinates from normal map to height map
	// which has extra texels around the border. This also takes into account partial
	// normal map tiles (such as near bottom or right edges of source raster).
	gl_TexCoord[0].x = dot(height_field_parameters.xz, gl_MultiTexCoord0.xw);
	gl_TexCoord[0].y = dot(height_field_parameters.yz, gl_MultiTexCoord0.yw);
	gl_TexCoord[0].zw = gl_MultiTexCoord0.zw;

	gl_Position = gl_Vertex;
}
