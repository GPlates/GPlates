/*
 * Vertex shader source code to render light direction into cube texture for a 2D map view.
 */

uniform vec3 view_space_light_direction;

varying vec3 world_space_light_direction;

void main (void)
{
	gl_Position = gl_Vertex;

	// If the light direction is attached to view space then reverse rotate to world space.
	// We're hijacking the GL_MODELVIEW matrix to store the view transform so we can get
	// OpenGL to calculate the matrix inverse for us.
	world_space_light_direction = mat3(gl_ModelViewMatrixInverse) * view_space_light_direction;
}
