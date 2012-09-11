//
// Fragment shader source code to render coloured (white) sphere with lighting.
//

uniform bool lighting_enabled;
uniform float light_ambient_contribution;
uniform vec3 world_space_light_direction;

// The world-space coordinates are interpolated across the geometry.
varying vec3 world_space_position;

void main (void)
{
	// The interpolated fragment colour.
	vec4 colour = gl_Color;

	if (lighting_enabled)
	{
		// Apply the Lambert diffuse lighting using the world-space position as the globe surface normal.
		// Note that neither the light direction nor the surface normal need be normalised.
		float lambert = lambert_diffuse_lighting(world_space_light_direction, world_space_position);

		// Blend between ambient and diffuse lighting.
		float lighting = mix_ambient_with_diffuse_lighting(lambert, light_ambient_contribution);

		colour.rgb *= lighting;
	}

	// The final fragment colour.
	gl_FragColor = colour;
}
