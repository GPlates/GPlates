/**
 * Copyright (C) 2022 The University of Sydney, Australia
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 *
 * GPlates is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

//
// Renders arrows.
//

layout (push_constant) uniform PushConstants
{
    mat4 view_projection;
    vec3 world_space_light_direction;
    bool lighting_enabled;
    float light_ambient_contribution;
    bool use_map_projection;  // true/false if rendering in map/globe view
};

// Attributes at per-vertex rate.
layout (location = 0, component = 0) in vec2 model_space_normalised_radial_position;      // radial (x, y) position is either (0, 0) or on the unit circle.
layout (location = 0, component = 2) in vec2 model_space_radial_and_axial_normal_weights; // radial (x,y) and axial (z) components of the model-space surface normal
layout (location = 1, component = 0) in float arrow_body_width_weight;  // 1.0 if vertex is on arrow body (otherwise 0.0)
layout (location = 1, component = 1) in float arrowhead_width_weight;   // 1.0 if vertex is on circular part of arrowhead cone (otherwise 0.0)
layout (location = 1, component = 2) in float arrow_body_length_weight; // 1.0 if vertex is at end of arrow body or anywhere on arrowhead (otherwise 0.0)
layout (location = 1, component = 3) in float arrowhead_length_weight;  // 1.0 if vertex is at pointy apex of arrowhead (otherwise 0.0)

// Attributes at per-instance rate (ie, per-arrow rate).
layout (location = 2) in vec3 world_space_start_position;  // (x, y, z) position in world space of base of arrow
layout (location = 2, component = 3) in float arrow_body_width;
layout (location = 3) in vec3 world_space_x_axis;          // arbitrary x-axis orthogonal to arrow direction (z-axis)
layout (location = 3, component = 3) in float arrowhead_width;
layout (location = 4) in vec3 world_space_y_axis;          // arbitrary y-axis orthogonal to arrow direction (z-axis)
layout (location = 4, component = 3) in float arrow_body_length;
layout (location = 5) in vec3 world_space_z_axis;          // arrow direction
layout (location = 5, component = 3) in float arrowhead_length;
layout (location = 6) in vec4 colour;

layout (location = 0) out VertexData
{
	vec4 colour;
	vec3 world_space_x_axis;
	vec3 world_space_y_axis;
	vec3 world_space_z_axis;
	// The model-space coordinates are interpolated across the geometry.
	vec2 model_space_radial_position;
	vec2 model_space_radial_and_axial_normal_weights;
	// Only used in the 3D globe views (not the 2D map views).
	vec3 world_space_sphere_normal;
} vs_out;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main()
{
    // Generate the model-space radial (x,y) position from the model-space radial position (which is either (0,0) or on unit circle).
    //
    // This gives a radial position on:
    // (1) the arrow body if 'arrow_body_width_weight' is 1.0, or
    // (2) the circular part of the arrowhead cone if 'arrowhead_width_weight' is 1.0, or
    // (3) the arrow axis if 'model_space_normalised_radial_position' is (0, 0) or
    //     if both 'arrow_body_width_weight' and 'arrowhead_width_weight' are 0.0.
    vec2 model_space_radial_position = model_space_normalised_radial_position *
            // Note: The 0.5 converts diameter to radius...
            (arrow_body_width_weight * 0.5 * arrow_body_width +
             arrowhead_width_weight * 0.5 * arrowhead_width);
    
    // Generate the model-space axial (z) position.
    //
    // This gives an axial position of:
    // (1) the full arrow length ('arrow_body_length + arrowhead_length') if both 'arrow_body_width_weight' and 'arrowhead_width_weight' are 1.0, or
    // (2) the *body* length if only 'arrow_body_width_weight' is 1.0 (this is where the body connects to the head), or
    // (3) zero if both 'arrow_body_width_weight' and 'arrowhead_width_weight' are 0.0.
    float model_space_axial_position =
            arrow_body_length_weight * arrow_body_length +
            arrowhead_length_weight * arrowhead_length;

    // Generate model-space position from model-space radial (x,y) normalised position and per-instance model-space weights.
    vec3 model_space_position = vec3(model_space_radial_position, model_space_axial_position);

    // Convert model-space position to world-space position using per-instance world start position and reference frame (where arrow points along world-space z axis).
    vec3 world_space_position = world_space_start_position + mat3(world_space_x_axis, world_space_y_axis, world_space_z_axis) * model_space_position;

	gl_Position = view_projection * vec4(world_space_position, 1);

	// Output the vertex colour.
    vs_out.colour = colour;

	if (lighting_enabled)
	{
		// Pass to fragment shader.
		vs_out.world_space_x_axis = world_space_x_axis;
		vs_out.world_space_y_axis = world_space_y_axis;
		vs_out.world_space_z_axis = world_space_z_axis;
		vs_out.model_space_radial_position = model_space_radial_position;
		vs_out.model_space_radial_and_axial_normal_weights = model_space_radial_and_axial_normal_weights;
        if (!use_map_projection)
        {
            // Only used in the 3D globe views (not the 2D map views).
            vs_out.world_space_sphere_normal = normalize(world_space_position);
        }
	}
}
