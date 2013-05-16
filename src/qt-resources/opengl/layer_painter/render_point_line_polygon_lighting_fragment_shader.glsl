//
// Fragment shader source code to render points, lines and polygons with lighting.
//

#ifdef MAP_VIEW
    // In the map view the lighting is constant across the map because
    // the surface normal is constant across the map (ie, it's flat unlike the 3D globe).
    uniform float ambient_and_diffuse_lighting;
#else
    uniform float light_ambient_contribution;
    uniform vec3 world_space_light_direction;
    // The world-space coordinates are interpolated across the geometry.
    varying vec3 world_space_position;
#endif

void main (void)
{
	// The interpolated fragment colour.
	vec4 colour = gl_Color;

#ifndef MAP_VIEW
	// Apply the Lambert diffuse lighting using the world-space position as the globe surface normal.
	// Note that neither the light direction nor the surface normal need be normalised.
	float lambert = lambert_diffuse_lighting(world_space_light_direction, world_space_position);

	// Blend between ambient and diffuse lighting.
    float ambient_and_diffuse_lighting = mix_ambient_with_diffuse_lighting(lambert, light_ambient_contribution);
#endif

	colour.rgb *= ambient_and_diffuse_lighting;

	// The final fragment colour.
	gl_FragColor = colour;
}
