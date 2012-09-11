//
// Vertex shader source code to coloured (white) sphere with lighting.
//

// The radius of the sphere.
uniform float depth_radius;

// The world-space coordinates are interpolated across the geometry.
varying vec3 world_space_position;

void main (void)
{
	world_space_position = depth_radius * gl_Vertex.xyz;

	gl_Position = gl_ModelViewProjectionMatrix * vec4(world_space_position, 1);

	// Output the vertex colour.
	// We render both sides (front and back) of triangles (ie, there's no back-face culling).
	gl_FrontColor = gl_Color;
	gl_BackColor = gl_Color;
}
