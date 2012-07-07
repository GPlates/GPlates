//
// Vertex shader for rendering a vertical cross-section through a 3D scalar field.
//
// The cross-section is formed by extruding a surface polyline (or polygon) from the
// field's outer depth radius to its inner depth radius.
//

// The world-space coordinates are interpolated across the cross-section geometry.
varying vec3 world_position;

void main()
{
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

    // This assumes the cross-section geometry does not need a model transform (eg, reconstruction rotation).
    world_position = gl_Vertex.xyz / gl_Vertex.w;
}
