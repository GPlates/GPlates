//
// Vertex shader for rendering a single iso-surface.
//

// Screen-space coordinates are post-projection coordinates in the range [-1,1].
varying vec2 screen_coord;

void main()
{
  gl_Position = gl_Vertex;
  
  screen_coord = gl_Vertex.xy;
}
