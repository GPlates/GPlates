/*
 * Fragment shader source code to render light direction into cube texture for a 2D map view.
 */

varying vec3 world_space_light_direction;

void main (void)
{
	// Just return the light direction - it doesn't vary across the cube texture face_target.
	// But first convert from range [-1,1] to range [0,1] to store in unsigned 8-bit render face_target.
   gl_FragColor = vec4(0.5 * world_space_light_direction + 0.5, 1);
}
