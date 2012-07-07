//
// Fragment shader for rendering a vertical cross-section through a 3D scalar field.
//
// The cross-section is formed by extruding a surface polyline (or polygon) from the
// field's outer depth radius to its inner depth radius.
//

// "#extension" needs to be specified in the shader source *string* where it is used (this is not
// documented in the GLSL spec but is mentioned at http://www.opengl.org/wiki/GLSL_Core_Language).
#extension GL_EXT_texture_array : enable

//
// Uniform variables default to zero according to the GLSL spec:
//
// "The link time initial value is either the value of the
// variable's initializer, if present, or 0 if no initializer is present."
//

// Test variables (in range [0,1]) set via the scalar field visual layer GUI.
// Initial values are zero.
uniform float test_variable_0;
uniform float test_variable_1;
uniform float test_variable_2;
uniform float test_variable_3;
uniform float test_variable_4;
uniform float test_variable_5;
uniform float test_variable_6;
uniform float test_variable_7;
uniform float test_variable_8;
uniform float test_variable_9;

uniform sampler2DArray tile_meta_data_sampler;
uniform sampler2DArray field_data_sampler;
uniform sampler2DArray mask_data_sampler;
uniform sampler1D depth_radius_to_layer_sampler;
uniform sampler1D colour_palette_sampler;

uniform int tile_meta_data_resolution;
uniform int tile_resolution;
uniform int depth_radius_to_layer_resolution;
uniform int colour_palette_resolution;
uniform vec2 min_max_depth_radius;
uniform int num_depth_layers;
uniform vec2 min_max_scalar_value;
uniform vec2 min_max_gradient_magnitude;

uniform bool lighting_enabled;
uniform vec3 world_space_light_direction;
uniform float light_ambient_contribution;

// The world-space coordinates are interpolated across the cross-section geometry.
varying vec3 world_position;

void main()
{
    // Project the world position onto the cube face and determine the cube face that it projects onto.
    int cube_face_index;
    vec3 projected_world_position = project_world_position_onto_cube_face(world_position, cube_face_index);

    // Transform the world position (projected onto a cube face) into that cube face's local coordinate frame.
    vec2 cube_face_coordinate_xy = convert_projected_world_position_to_local_cube_face_coordinate_frame(
            projected_world_position, cube_face_index);

    // Convert range [-1,1] to [0,1].
    vec2 cube_face_coordinate_uv = 0.5 * cube_face_coordinate_xy + 0.5;

    // Convert the local cube face (x,y) coordinates to (u,v) coordinates into metadata and field data textures.
    vec2 tile_offset_uv;
    vec2 tile_meta_data_coordinate_uv;
    vec2 tile_coordinate_uv;
    get_tile_uv_coordinates(
            cube_face_coordinate_uv,
            tile_meta_data_resolution,
            tile_resolution,
            tile_offset_uv,
            tile_meta_data_coordinate_uv,
            tile_coordinate_uv);

    // Sample the tile metadata texture array.
    vec3 tile_meta_data = texture2DArray(tile_meta_data_sampler, vec3(tile_meta_data_coordinate_uv, cube_face_index)).rgb;
    float tile_ID = tile_meta_data.r;
    float max_field_scalar = tile_meta_data.g;
    float min_field_scalar = tile_meta_data.b;

    // If the current tile has no data then there's nothing to render so discard the current pixel.
    if (tile_ID < 0)
    {
        discard;
    }

    // Sample the mask data texture array.
    float mask_data = texture2DArray(mask_data_sampler, vec3(tile_coordinate_uv, tile_ID)).r;
        
    // If the current world position (at all vertical depth locations) contains no field data
    // then there's nothing to render so discard the current pixel.
    //
    // TODO: What happens for (bilinearly filtered) values *between* 0 and 1 ? Do they mean accept or reject ?
    // If accept, then does that mean one or more samples used in bilinear filter of field data is invalid and
    // hence corrupts scalar/gradient result ?
    // Or is the field data dilated one texel (towards mask boundary texels) during pre-processing ?
    if (mask_data == 0)
    {
        discard;
    }

    // Determine texture layer index to lookup our field data texture array with.
    // We look up a 1D texture to convert depth radius to layer index.
    // This is in case the depth layers are not uniformly spaced.
    float world_position_depth_radius = length(world_position);
    // There will be small mapping errors within a texel distance to each depth radius (for non-equidistant depth mappings)
    // but that can only be reduced by increasing the resolution of the 1D texture.
    // Note that 'world_position_depth_layer' is not an integer layer index.
    float world_position_depth_layer = look_up_table_1D(
            depth_radius_to_layer_sampler,
            depth_radius_to_layer_resolution,
            min_max_depth_radius.x, // input_lower_bound
            min_max_depth_radius.y, // input_upper_bound
            world_position_depth_radius).r;

    // Sample the field data texture array.
    vec4 field_data = sample_field_data_texture_array(
            field_data_sampler, tile_coordinate_uv, num_depth_layers, tile_ID, world_position_depth_layer);
    float field_scalar = field_data.r;
    vec3 field_gradient = field_data.gba;

    // Look up the colour palette using scalar value (or gradient magnitude).
    colour = look_up_table_1D(
            colour_palette_sampler,
            colour_palette_resolution,
            min_max_gradient_magnitude.x, // input_lower_bound
            min_max_gradient_magnitude.y, // input_upper_bound
            length(field_gradient));

    // If the current pixel is fully transparent then discard it.
    // Not that we have transparent iso-surfaces (yet).
    if (colour.a == 0)
    {
        discard;
    }

    // Output colour as pre-multiplied alpha.
    // This enables alpha-blending to use (src,dst) blend factors of (1, 1-SrcAlpha) instead of
    // (SrcAlpha, 1-SrcAlpha) which keeps alpha intact (avoids double-blending when rendering to
    // a render-target and then, in turn, blending that into the main framebuffer).
    // Note: We don't have semi-transparent cross-sections (yet).
    colour.rgb *= colour.a;

    // This branches on a uniform variable and hence is efficient since all pixels follow same path.
    if (lighting_enabled)
    {
        // The normal to use for lighting.
        normal = normalize(field_gradient);

        // The Lambert term in diffuse lighting.
        float lambert = lambert_diffuse_lighting(world_space_light_direction, normal);

        // Blend between ambient and diffuse lighting - when ambient is 1.0 there is no diffuse.
        float diffuse = mix(lambert, 1.0, light_ambient_contribution);

        colour.rgb *= diffuse;
    }

    // Note that we're not writing to 'gl_FragDepth'.
    // This means the fixed-function depth will get written to the hardware depth buffer
    // which is what we want for non-ray-traced geometries.

    //
    // Using multiple render targets here.
    //
    // This is in case another scalar field is rendered as an iso-surface in which case
    // it cannot use the hardware depth buffer because it's not rendering traditional geometry.
    // Well it can, but it's not efficient because it cannot early cull rays (it needs to do the full
    // ray trace before it can output fragment depth to make use of the depth buffer).
    // So instead it must query a depth texture in order to early-cull rays.
    // The hardware depth buffer is still used for correct depth sorting of the cross-sections and iso-surfaces.
    // Note that this doesn't work with semi-transparency but that's a much harder problem to solve - depth peeling.
    //

    // According to the GLSL spec...
    //
    // "If a shader statically assigns a value to gl_FragColor, it may not assign a value
    // to any element of gl_FragData. If a shader statically writes a value to any element
    // of gl_FragData, it may not assign a value to gl_FragColor. That is, a shader may
    // assign values to either gl_FragColor or gl_FragData, but not both."
    // "A shader contains a static assignment to a variable x if, after pre-processing,
    // the shader contains a statement that would write to x, whether or not run-time flow
    // of control will cause that statement to be executed."
    //
    // ...so we can't branch on a uniform (boolean) variable here.
    // So we write to both gl_FragData[0] and gl_FragData[1] with the latter output being ignored
    // if glDrawBuffers is NONE for draw buffer index 1 (the default).

    // Write colour to render target 0.
    gl_FragData[0] = colour;

    // Write *screen-space* depth (ie, depth range [-1,1] and not [0,1]) to render target 1.
    // This is what's used in the ray-tracing shader since it uses inverse model-view-proj matrix on the depth
    // to get world space position and that requires normalised device coordinates not window coordinates).
    // Ideally this should also consider the effects of glDepthRange but we'll assume it's set to [0,1].
    gl_FragData[1] = vec4(2 * gl_FragCoord.z - 1);
}
