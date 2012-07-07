/*
 * Fragment shader source to render polygons to the polygon stencil texture.
 */

varying vec4 clip_position_params;
varying vec4 fragment_fill_colour;

void main (void)
{
	// Discard current pixel if outside the frustum side planes.
	// Inside clip frustum means -1 < x/w < 1 and -1 < y/w < 1 which is same as
	// -w < x < w and -w < y < w.
	// 'clip_position_params' is (x, y, w, -w).
	if (!all(lessThan(clip_position_params.wxwy, clip_position_params.xzyz)))
		discard;

	// Output the polygon fill colour.
	gl_FragColor = fragment_fill_colour;
}
