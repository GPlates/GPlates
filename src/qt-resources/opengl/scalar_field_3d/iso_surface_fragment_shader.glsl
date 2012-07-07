//
// Fragment shader for rendering a single iso-surface.
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
uniform float scalar_iso_surface_value;

uniform bool lighting_enabled;
uniform vec3 world_space_light_direction;
uniform float light_ambient_contribution;

// If using an orthographic, instead of perspective, projection.
uniform bool using_ortho_projection;
// Eye position (only applies to perspective projection).
// There's not really any eye position with an orthographic projection.
// Instead it gets emulated in the shader.
uniform vec3 perspective_projection_eye_position;

// If using the surface occlusion texture for early ray termination.
uniform bool read_from_surface_occlusion_texture;
// The surface occlusion texture used for early ray termination.
// This contains the RGBA contents of rendering the front surface of the globe.
uniform sampler2D surface_occlusion_texture_sampler;

// If using the depth texture for early ray termination.
uniform bool read_from_depth_texture;
// The depth texture used for early ray termination.
uniform sampler2D depth_texture_sampler;

// If using a surface mask to limit regions of scalar field to render.
uniform bool using_surface_fill_mask;
uniform sampler2DArray surface_fill_mask_sampler;
uniform int surface_fill_mask_resolution;


// Screen-space coordinates are post-projection coordinates in the range [-1,1].
varying vec2 screen_coord;


void main()
{
    // This branches on a uniform variable and hence is efficient since all pixels follow same path.
    if (read_from_surface_occlusion_texture)
    {
        // If a surface object on the front of the globe has an alpha value of 1.0 (and hence occludes
        // the current ray) then discard the current pixel.
        // Convert [-1,1] range to [0,1] for texture coordinates.
        float surface_occlusion = texture2D(surface_occlusion_texture_sampler, 0.5 * screen_coord + 0.5).a;
        if (surface_occlusion == 1.0)
        {
            discard;
        }
    }

    vec3 eye_position;
    // This branches on a uniform variable and hence is efficient since all pixels follow same path.
    if (using_ortho_projection)
    {
        // Calculate a pseudo eye position when using an orthographic view projection.
        // The real eye position is at infinity and generates parallel rays for all pixels.
        // This is such that the ray will always be parallel to the view direction.
        // Use an arbitrary post-projection screen-space depth of -2.
        // Any value will do as long as it's outside the view frustum [-1,1] and not *on* the near plane (z = -1).
        eye_position = screen_to_world(vec3(screen_coord, -2.0), gl_ModelViewProjectionMatrixInverse);
    }
    else
    {
        eye_position = perspective_projection_eye_position;
    }

    // Squared outer depth sphere radius is set.
    Sphere outer_depth_sphere = Sphere(min_max_depth_radius.y * min_max_depth_radius.y);

    // Ray is initialized.
    Ray ray = get_ray(screen_coord, gl_ModelViewProjectionMatrixInverse, eye_position);

    // Test if ray intersects with outer depth sphere.
    Interval outer_depth_sphere_interval;
    if (!intersect(outer_depth_sphere, ray, outer_depth_sphere_interval))
    {
        // Discard the current pixel.
        discard;
    }

    // Maximum ray length.
    float max_lambda = outer_depth_sphere_interval.to;

    // This branches on a uniform variable and hence is efficient since all pixels follow same path.
    if (read_from_depth_texture)
    {
        // The depth texture might reduce the maximum ray length enabling early ray termination.
        max_lambda = min(max_lambda, get_max_lambda(screen_coord, depth_texture_sampler, gl_ModelViewProjectionMatrixInverse, eye_position));
    }

    // Squared radius of the inner depth sphere is set.
    Sphere inner_depth_sphere = Sphere(min_max_depth_radius.x * min_max_depth_radius.x);
    Interval inner_depth_sphere_interval;

    // Transparent colour unless ray hits isosurface or inner depth sphere (white).
    vec4 colour = vec4(0,0,0,0);
    vec3 normal = vec3(0,0,0);

    // Test if ray intersects with inner depth sphere
    if (intersect(inner_depth_sphere, ray, inner_depth_sphere_interval))
    {
        max_lambda = min(inner_depth_sphere_interval.from, max_lambda);

        // Inner depth sphere is white.
        // Or maybe it's not ?
        // Perhaps in presence of two scalar fields (with different min-depth-radii) there's
        // still only one white inner occlusion sphere ?
        colour = vec4(1,1,1,1);
        normal = normalize(at(ray, max_lambda));
    }

    // Start distance between sampling point and ray origin.
    float min_lambda = outer_depth_sphere_interval.from;

    float lambda_increment = 1.0 / 100;
    int num_ray_samples = 1 + int(ceil((max_lambda - min_lambda) / lambda_increment));
    // Adjust increment to match integer number of ray samples.
    lambda_increment = (max_lambda - min_lambda) / (num_ray_samples - 1);

    // Current distance between sampling point and ray origin => lambda
    float lambda = min_lambda;
    
    // The actual intersection of ray with iso-surface.
    // Set to furthest distance from eye by default in case ray iteration fails to intersect the scalar field.
    vec3 iso_surface_position = at(ray, max_lambda);

    bool is_first_sample_from_field = true;
    bool entry_field_scalar_is_larger_than_iso_value = false;
    for (int ray_iteration = 0; ray_iteration < num_ray_samples; ++ray_iteration, lambda += lambda_increment)
    {
        // Position in world coordinates for the current ray sample.
        vec3 ray_sample_position = at(ray, lambda);

        // Project the ray sample position onto the cube face and determine the cube face that it projects onto.
        int cube_face_index;
        vec3 projected_ray_sample_position = project_world_position_onto_cube_face(ray_sample_position, cube_face_index);

        // Transform the ray sample position (projected onto a cube face) into that cube face's local coordinate frame.
        vec2 cube_face_coordinate_xy = convert_projected_world_position_to_local_cube_face_coordinate_frame(
                projected_ray_sample_position, cube_face_index);

        // Convert range [-1,1] to [0,1].
        vec2 cube_face_coordinate_uv = 0.5 * cube_face_coordinate_xy + 0.5;
        
        // If we're limiting rendering to regions extruded from filled polygons on the globe surface towards globe origin...
        if (using_surface_fill_mask)
        {
            // If the current ray sample does not project into the surface mask then
            // continue to the next ray sample (it may enter a fill region).
            if (!projects_into_surface_fill_mask(
                surface_fill_mask_sampler, surface_fill_mask_resolution, cube_face_index, cube_face_coordinate_uv))
            {
                continue;
            }
        }

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

        // If the current tile has no data then continue to the next ray sample (it may enter a new tile).
        if (tile_ID < 0)
        {
            continue;
        }

        // Sample the mask data texture array.
        float mask_data = texture2DArray(mask_data_sampler, vec3(tile_coordinate_uv, tile_ID)).r;

        // If the current ray sample location (at all vertical depth locations) contains no field data
        // then continue to the next ray sample.
        //
        // TODO: What happens for (bilinearly filtered) values *between* 0 and 1 ? Do they mean accept or reject ?
        // If accept, then does that mean one or more samples used in bilinear filter of field data is invalid and
        // hence corrupts scalar/gradient result ?
        // Or is the field data dilated one texel (towards mask boundary texels) during pre-processing ?
        if (mask_data == 0)
        {
            continue;
        }

        // Determine texture layer index to lookup our field data texture array with.
        // We look up a 1D texture to convert depth radius to layer index.
        // This is in case the depth layers are not uniformly spaced.
        float ray_sample_depth_radius = length(ray_sample_position);
        // There will be small mapping errors within a texel distance to each depth radius (for non-equidistant depth mappings)
        // but that can only be reduced by increasing the resolution of the 1D texture.
        // Note that 'ray_depth_layer' is not an integer layer index.
        float ray_depth_layer = look_up_table_1D(
                depth_radius_to_layer_sampler,
                depth_radius_to_layer_resolution,
                min_max_depth_radius.x, // input_lower_bound
                min_max_depth_radius.y, // input_upper_bound
                ray_sample_depth_radius).r;

        // Sample the field data texture array.
        vec4 field_data = sample_field_data_texture_array(
                field_data_sampler, tile_coordinate_uv, num_depth_layers, tile_ID, ray_depth_layer);
        float field_scalar = field_data.r;
        vec3 field_gradient = field_data.gba;

        // See if ray crossed isosurface.
        // If first field sample along sample then determine if approaching
        // isosurface from higher or lower scalar field values.
        if (is_first_sample_from_field)
        {
            if (field_scalar > scalar_iso_surface_value)
            {
                entry_field_scalar_is_larger_than_iso_value = true;
            }
            is_first_sample_from_field = false;
        }
        else
        {
            if (entry_field_scalar_is_larger_than_iso_value)
            {
                // If hit front-side of isosurface...
                if (field_scalar < scalar_iso_surface_value)
                {
                    normal = normalize(field_gradient);

                    // Red for front-side of isosurface.
                    //colour = vec4(1,0,0,1);

                    // Use the field gradient magnitude to look up the colour palette.
                    colour = look_up_table_1D(
                            colour_palette_sampler,
                            colour_palette_resolution,
                            min_max_gradient_magnitude.x, // input_lower_bound
                            min_max_gradient_magnitude.y, // input_upper_bound
                            length(field_gradient));

                    // The actual intersection of ray with iso-surface.
                    // TODO: Needs refinement to put it exactly on the iso-surface.
                    // Currently it's just the last ray sample position.
                    iso_surface_position = ray_sample_position;

                    break;
                }
            }
            else
            {
                // If hit back-side of isosurface...
                if (field_scalar > scalar_iso_surface_value)
                {
                    // Reverse isosurface normal.
                    normal = -normalize(field_gradient);

                    // Green for back-side of isosurface.
                    //colour = vec4(0,1,0,1);

                    // Use the field gradient magnitude to look up the colour palette.
                    colour = look_up_table_1D(
                            colour_palette_sampler,
                            colour_palette_resolution,
                            min_max_gradient_magnitude.x, // input_lower_bound
                            min_max_gradient_magnitude.y, // input_upper_bound
                            length(field_gradient));

                    // The actual intersection of ray with iso-surface.
                    // TODO: Needs refinement to put it exactly on the iso-surface.
                    // Currently it's just the last ray sample position.
                    iso_surface_position = ray_sample_position;

                    break;
                }
            }
        }
    }

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
    // Not that we have transparent iso-surfaces (yet).
    colour.rgb *= colour.a;

    // This branches on a uniform variable and hence is efficient since all pixels follow same path.
    if (lighting_enabled)
    {
        // The Lambert term in diffuse lighting.
        float lambert = lambert_diffuse_lighting(world_space_light_direction, normal);

        // Blend between ambient and diffuse lighting - when ambient is 1.0 there is no diffuse.
        float diffuse = mix(lambert, 1.0, light_ambient_contribution);

        colour.rgb *= diffuse;
    }

    // Note that we need to write to 'gl_FragDepth' because if we don't then the fixed-function
    // depth will get written (if depth writes enabled) using the fixed-function depth which is that
    // of our full-screen quad (not the actual ray-traced depth).
    //
    // NOTE: Currently depth writes are turned off for scalar fields but will be turned on soon.
    vec4 screen_space_iso_surface_position = gl_ModelViewProjectionMatrix * vec4(iso_surface_position, 1.0);
    float screen_space_iso_surface_depth = screen_space_iso_surface_position.z / screen_space_iso_surface_position.w;

    // Convert normalised device coordinates (screen-space) to window coordinates which, for depth, is [-1,1] -> [0,1].
    // Ideally this should also consider the effects of glDepthRange but we'll assume it's set to [0,1].
    gl_FragDepth = 0.5 * screen_space_iso_surface_depth + 0.5;

    //
    // Using multiple render targets here.
    //
    // This is in case another scalar field is rendered as an iso-surface (or cross-section) in which case
    // it cannot use the hardware depth buffer because we're not rendering traditional geometry
    // (and so instead it must query a depth texture).
    // This enables correct depth sorting of the cross-sections and iso-surfaces (although it doesn't
    // work with semi-transparency but that's a much harder problem to solve - depth peeling).
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
    // Note that this does not need to consider the effects of glDepthRange.
    //
    // NOTE: If this output is connected to a depth texture and 'read_from_depth_texture' is true then
    // they cannot both be the same texture because a shader cannot write to the same texture it reads from.
    gl_FragData[1] = vec4(screen_space_iso_surface_depth);
}
