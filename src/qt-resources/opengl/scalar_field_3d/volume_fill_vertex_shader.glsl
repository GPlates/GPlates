//
// Vertex shader source code to render volume fill boundary.
//
// Used for both depth range and wall normals.
// Also used for both walls and spherical caps (for depth range).
//

attribute vec4 surface_point;
attribute vec4 centroid_point;

varying vec4 polygon_centroid_point;

void main (void)
{
    // Currently there is no model transform (Euler rotation).
    // Just pass the vertex position on to the geometry shader.
    // Later, if there's a model transform, then we'll do it here to take the load
    // off the geometry shader (only vertex shader has a post-transform hardware cache).
    gl_Position = surface_point;

    // This is not used for wall normals (only used for spherical caps).
    polygon_centroid_point = centroid_point;
}
