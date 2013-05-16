//
// Vertex shader source code to render points, lines and polygons with lighting.
//

#ifndef MAP_VIEW
    // The 3D globe view needs to calculate lighting across the geometry but the map view does not
    // because the surface normal is constant across the map (ie, it's flat unlike the 3D globe).

    // The world-space coordinates are interpolated across the geometry.
    varying vec3 world_space_position;
#endif

void main (void)
{
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

	// Output the vertex colour.
	// We render both sides (front and back) of triangles (ie, there's no back-face culling).
	gl_FrontColor = gl_Color;
	gl_BackColor = gl_Color;

#ifndef MAP_VIEW
    // This assumes the geometry does not need a model transform (eg, reconstruction rotation).
    world_space_position = gl_Vertex.xyz / gl_Vertex.w;
#endif
}
