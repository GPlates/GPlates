//
// Vertex shader source code to render points, lines and polygons with lighting.
//

// The world-space coordinates are interpolated across the geometry.
varying vec3 world_space_position;

void main (void)
{
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

    // Output the vertex colour.
    // We render both sides (front and back) of triangles (ie, there's no back-face culling).
    gl_FrontColor = gl_Color;
    gl_BackColor = gl_Color;

    // This assumes the geometry does not need a model transform (eg, reconstruction rotation).
    world_space_position = gl_Vertex.xyz / gl_Vertex.w;
}
