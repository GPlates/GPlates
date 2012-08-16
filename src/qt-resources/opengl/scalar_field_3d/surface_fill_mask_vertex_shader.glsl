//
// Vertex shader source code to render surface fill mask.
//

attribute vec4 surface_point;

void main (void)
{
    // Currently there is no model transform (Euler rotation).
    // Just pass the vertex position on to the geometry shader.
    // Later, if there's a model transform, then we'll do it here to take the load
    // off the geometry shader (only vertex shader has a post-transform hardware cache).
    gl_Position = surface_point;
}
