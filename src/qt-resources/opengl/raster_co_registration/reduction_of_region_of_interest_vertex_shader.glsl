/*
 * Vertex shader source to reduce region-of-interest filter results.
 */

// (x,y,z) is (translate_x, translate_y, scale).
uniform vec3 target_quadrant_translate_scale;

void main (void)
{
	gl_TexCoord[0] = gl_MultiTexCoord0;

	// Scale and translate the pixel coordinates to the appropriate quadrant
	// of the destination render target.
	gl_Position.x = dot(target_quadrant_translate_scale.zx, gl_Vertex.xw);
	gl_Position.y = dot(target_quadrant_translate_scale.zy, gl_Vertex.yw);
	gl_Position.zw = gl_Vertex.zw;
}
