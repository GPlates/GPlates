//
// Fragment shader for rendering a single iso-surface.
//

// "#extension" needs to be specified in the shader source *string* where it is used (this is not
// documented in the GLSL spec but is mentioned at http://www.opengl.org/wiki/GLSL_Core_Language).
#extension GL_EXT_texture_array : enable
// Shouldn't need this since '#version 120' (in separate source string) supports 'gl_FragData', but just in case...
#extension GL_ARB_draw_buffers : enable

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
uniform float test_variable_10;
uniform float test_variable_11;
uniform float test_variable_12;
uniform float test_variable_13;
uniform float test_variable_14;
uniform float test_variable_15;

uniform sampler2DArray tile_meta_data_sampler;
uniform sampler2DArray field_data_sampler;
uniform sampler2DArray mask_data_sampler;
uniform sampler1D depth_radius_to_layer_sampler;
uniform sampler1D colour_palette_sampler;

uniform int tile_meta_data_resolution;
uniform int tile_resolution;
uniform int depth_radius_to_layer_resolution;
uniform int num_depth_layers;
uniform int colour_palette_resolution;

uniform vec2 min_max_scalar_value;
uniform vec2 min_max_gradient_magnitude;


//
// Lighting options.
//
uniform bool lighting_enabled;
uniform vec3 world_space_light_direction;
uniform float light_ambient_contribution;

// For testing purposes
// float light_ambient_contribution = test_variable_0;


//
// View options.
//

// If using an orthographic, instead of perspective, projection.
uniform bool using_ortho_projection;
// Eye position (only applies to perspective projection).
// There's not really any eye position with an orthographic projection.
// Instead it gets emulated in the shader.
uniform vec3 perspective_projection_eye_position;


//
// Depth occlusion options.
//

// If using the surface occlusion texture for early ray termination.
uniform bool read_from_surface_occlusion_texture;
// The surface occlusion texture used for early ray termination.
// This contains the RGBA contents of rendering the front surface of the globe.
uniform sampler2D surface_occlusion_texture_sampler;

// If using the depth texture for early ray termination.
uniform bool read_from_depth_texture;
// The depth texture used for early ray termination.
uniform sampler2D depth_texture_sampler;


//
// "Colour mode" options.
//
// NOTE: Only one of these is true.
//
uniform bool colour_mode_depth;
uniform bool colour_mode_isovalue;
uniform bool colour_mode_gradient;


//
// Isovalue options.
//
// First isovalue (used for 'render_mode_isosurface' and 'render_mode_single_deviation_window').
// (x,y,z) components are (isovalue, lower deviation, upper deviation).
// Deviation parameters only available for 'render_mode_single_deviation_window'.
// If deviation parameters are symmetric then lower and upper will have same values.
uniform vec3 isovalue1;
// Second isovalue (used for 'render_mode_double_deviation_window' only).
// (x,y,z) components are (isovalue, lower deviation, upper deviation).
// If deviation parameters are symmetric then lower and upper will have same values.
uniform vec3 isovalue2;


//
// Render options.
//

// Opacity of deviation surfaces (including deviation window at globe surface).
// Only valid for 'render_mode_single_deviation_window' and 'render_mode_double_deviation_window'.
uniform float opacity_deviation_surfaces;
// Is true if volume rendering in deviation window (between deviation surfaces).
// Only valid for 'render_mode_single_deviation_window' and 'render_mode_double_deviation_window'.
uniform bool deviation_window_volume_rendering;
// Opacity of deviation window volume rendering.
// Only valid for 'render_mode_single_deviation_window' and 'render_mode_double_deviation_window'.
uniform float opacity_deviation_window_volume_rendering;
// Is true if rendering deviation window at globe surface.
// Only valid for 'render_mode_single_deviation_window' and 'render_mode_double_deviation_window'.
uniform bool surface_deviation_window;
// The isoline frequency when rendering deviation window at globe surface.
// Only valid for 'render_mode_single_deviation_window' and 'render_mode_double_deviation_window'.
uniform float surface_deviation_isoline_frequency;


//
// Surface fill mask options.
//

// If using a surface mask to limit regions of scalar field to render.
uniform bool using_surface_fill_mask;
uniform sampler2DArray surface_fill_mask_sampler;
uniform int surface_fill_mask_resolution;

// If rendering the walls of the extruded fill volume.
uniform bool show_volume_fill_walls;
uniform sampler2D volume_fill_walls_sampler;


//
// Depth range options.
//

// The actual minimum and maximum depth radii of the scalar field.
uniform vec2 min_max_depth_radius;
// The depth range rendering is restricted to.
// If depth range is not restricted then this is the same as 'min_max_depth_radius'.
// Also the following conditions hold:
//	render_min_max_depth_radius_restriction.x >= min_max_depth_radius.x
//	render_min_max_depth_radius_restriction.y <= min_max_depth_radius.y
// ...in other words the depth range for rendering is always within the actual depth range.
uniform vec2 render_min_max_depth_radius_restriction;


// Quality/performance options.
//
// Sampling rate along ray.
// (x,y) components are (distance between samples, inverse distance between samples).
uniform vec2 sampling_rate;
// Number of convergence iterations for accurate isosurface location.
uniform int bisection_iterations;


// Screen-space coordinates are post-projection coordinates in the range [-1,1].
varying vec2 screen_coord;


// Function for isosurface/deviation window bisection position correction
void
bisection_correction(
		inout vec3 ray_sample_position,
		inout float lambda,
		float lambda_increment,
		Ray ray,
		inout int crossing_level_back,
		inout int crossing_level_front,
		inout bool active_back,
		inout bool active_front,
		inout float field_scalar,
		inout vec3 field_gradient)
{
	int cube_face_index;
	vec3 projected_ray_sample_position;
	vec2 cube_face_coordinate_xy;
	vec2 cube_face_coordinate_uv;
	vec4 field_data;
	float ray_sample_depth_radius;
	float ray_depth_layer;
	vec2 tile_offset_uv;
	vec2 tile_meta_data_coordinate_uv;
	vec2 tile_coordinate_uv;
	vec3 tile_meta_data;
	float tile_ID;
	int crossing_level_center;
	bool active_center = true;
	float mask_data;

	// Current sampling interval [a,b] is recursively devided by 2 and isosurface crossing is checked for both new sub-intervals [a,0.5*(a+b)] and [0.5*(a+b),b]
	for (int i = 1; i <= bisection_iterations; i++)
	{
		lambda_increment *= 0.5;

		ray_sample_position -= lambda_increment*ray.direction;
		lambda -= lambda_increment;

		projected_ray_sample_position = project_world_position_onto_cube_face(ray_sample_position, cube_face_index); 

		cube_face_coordinate_xy = convert_projected_world_position_to_local_cube_face_coordinate_frame(
			projected_ray_sample_position, cube_face_index);

		cube_face_coordinate_uv = 0.5 * cube_face_coordinate_xy + 0.5;

		if (using_surface_fill_mask)
		{
			active_center = projects_into_surface_fill_mask(surface_fill_mask_sampler, surface_fill_mask_resolution, cube_face_index, cube_face_coordinate_uv);
		}

		get_tile_uv_coordinates(
			cube_face_coordinate_uv,
			tile_meta_data_resolution,
			tile_resolution,
			tile_offset_uv,
			tile_meta_data_coordinate_uv,
			tile_coordinate_uv);

		tile_meta_data = texture2DArray(tile_meta_data_sampler, vec3(tile_meta_data_coordinate_uv, cube_face_index)).rgb;
		tile_ID = tile_meta_data.r;

		active_center = active_center && (tile_ID >= 0);

		ray_sample_depth_radius = length(ray_sample_position);

		mask_data = texture2DArray(mask_data_sampler, vec3(tile_coordinate_uv, tile_ID)).r;

		active_center = active_center && (mask_data != 0);

		ray_depth_layer = look_up_table_1D(
			depth_radius_to_layer_sampler,
			depth_radius_to_layer_resolution,
			min_max_depth_radius.x,
			min_max_depth_radius.y,
			ray_sample_depth_radius).r;

		field_data = sample_field_data_texture_array(
			field_data_sampler, tile_coordinate_uv, num_depth_layers, tile_ID, ray_depth_layer);

		field_scalar = field_data.r;

		field_gradient = field_data.gba;

		crossing_level_center = int(field_scalar >= isovalue1.x) + int(field_scalar >= (isovalue1.x-isovalue1.y)) + int(field_scalar >= (isovalue1.x+isovalue1.z));
		crossing_level_center += int(field_scalar >= isovalue2.x) + int(field_scalar >= (isovalue2.x-isovalue2.y)) + int(field_scalar >= (isovalue2.x+isovalue2.z));

		if (crossing_level_back != crossing_level_center)
		{
			// Proceed with left sub-interval
			crossing_level_front = crossing_level_center;
			active_front = active_center;
		}
		else
		{
			// Proceed with right sub-interval
			crossing_level_back = crossing_level_center;
			active_back = active_center;
			ray_sample_position += lambda_increment*ray.direction;
			lambda += lambda_increment;
		}
	}
}

// Function for returning the colour when sampling intervals contain transparent window enclosing surface, isosurface or several surfaces combined in one interval
vec4
get_blended_crossing_colour(
		vec3 ray_sample_position,
		Ray ray,
		vec3 normal,
		int crossing_level_back,
		int crossing_level_front,
		bool positive_crossing,
		mat4 colour_matrix_init,
		vec3 field_gradient)
{
	vec4 colour_crossing = vec4(0,0,0,0);
	
	float ray_sample_depth_radius = length(ray_sample_position);

	// Special view dependent opacity correction for surfaces resulting in improved 3D effects
	float alpha = 1-exp(log(1-opacity_deviation_surfaces)/abs(dot(normal,ray.direction)));

	// Relative depth: 0 -> core , 1 -> surface (for restricted max/min depth range)
	float depth_relative = (ray_sample_depth_radius-render_min_max_depth_radius_restriction.x)/(render_min_max_depth_radius_restriction.y-render_min_max_depth_radius_restriction.x);

	float diffuse = 1;

	if (lighting_enabled)
	{
		// The Lambert term in diffuse lighting.
		float lambert = lambert_diffuse_lighting(world_space_light_direction, normal);

		// Blend between ambient and diffuse lighting - when ambient is 1.0 there is no diffuse.
		diffuse = mix(lambert, 1.0, light_ambient_contribution);
	}

	// Switch between different colour modes			
	
	// Gradient strength to isosurface colour-mapping
	if (colour_mode_gradient)
	{
		// MacOS (Snow Leopard) GLSL compiler crashes if this is mat3x4 so
		// just adding an unused column to keep compiler happy.
		mat4 colour_matrix;

		if (!positive_crossing)
		{
			// Ray crosses surfaces in opposite gradient direction: dot(ray.direction,gradient) < 0
			
			// Colour of first window surface crossed by ray, whose gradient is pointing "away" from isosurface
			// Mapping of -||gradient||: [-max||gradient||,0] -> [col_map(0),col_map(0.5)]
			vec4 colour_tmp1 = look_up_table_1D(
					colour_palette_sampler,
					colour_palette_resolution,
					-min_max_gradient_magnitude.y,
					min_max_gradient_magnitude.y,
					-length(field_gradient));

			// Colour of second window surface crossed by ray, whose gradient is pointing "to" the isosurface
			// Mapping of ||gradient||: [0,max||gradient||] -> [col_map(0.5),col_map(1)]
			vec4 colour_tmp2 = look_up_table_1D(
					colour_palette_sampler,
					colour_palette_resolution,
					-min_max_gradient_magnitude.y,
					min_max_gradient_magnitude.y,
					length(field_gradient));

			// MacOS (Snow Leopard) GLSL compiler crashes if this is mat3x4 so
			// just adding an unused column to keep compiler happy.
			colour_matrix = mat4(vec4(colour_tmp1.rgb,alpha),vec4(1,1,1,1),vec4(colour_tmp2.rgb,alpha),vec4(1)); 
			
			crossing_level_back = 6-crossing_level_back;
			crossing_level_front = 6-crossing_level_front;
		}
		else
		{
			// Ray crosses surfaces in gradient direction: dot(ray.direction,gradient) > 0

			// Colour of first window surface crossed by ray, whose gradient is pointing "to" the isosurface
			// Mapping of ||gradient||: [0,max||gradient||] -> [col_map(0.5),col_map(1)]
			vec4 colour_tmp1 = look_up_table_1D(
					colour_palette_sampler,
					colour_palette_resolution,
					-min_max_gradient_magnitude.y,
					min_max_gradient_magnitude.y,
					length(field_gradient));

			// Colour of first window surface crossed by ray, whose gradient is pointing "away" from isosurface
			// Mapping of -||gradient||: [-max||gradient||,0] -> [col_map(0),col_map(0.5)]
			vec4 colour_tmp2 = look_up_table_1D(
					colour_palette_sampler,
					colour_palette_resolution,
					-min_max_gradient_magnitude.y,
					min_max_gradient_magnitude.y,
					-length(field_gradient));

			// MacOS (Snow Leopard) GLSL compiler crashes if this is mat3x4 so
			// just adding an unused column to keep compiler happy.
			colour_matrix = mat4(vec4(colour_tmp1.rgb,alpha),vec4(1,1,1,1),vec4(colour_tmp2.rgb,alpha),vec4(1));
		}

		// Blending of colours of surfaces in the deviation window zones for isovalue1 and isovalue2
		for (int i = crossing_level_back; i < crossing_level_front; i++)
		{
			int i_mod = int(mod(i,3));
			colour_crossing.rgb += (1-colour_crossing.a)*colour_matrix[i_mod].rgb*colour_matrix[i_mod].a*diffuse;
			colour_crossing.a += (1-colour_crossing.a)*colour_matrix[i_mod].a;
		}

		return colour_crossing;
	}

	// Depth to colour mapping
	// Mapping: [radius=0,radius=1] -> [red,yellow] for window surface isovalue + deviation
	//			[radius=0,radius=1] -> [blue,cyan] for window surface isovalue - deviation
	//			Isosurface is white.
	if (colour_mode_depth)
	{
		// MacOS (Snow Leopard) GLSL compiler crashes if this is mat3x4 so
		// just adding an unused column to keep compiler happy.
		mat4 colour_matrix;

		if (!positive_crossing)
		{
			// Ray crosses surfaces in opposite gradient direction: dot(ray.direction,gradient) < 0

			// Colour order of surfaces in one deviation window zone in ray direction
			// MacOS (Snow Leopard) GLSL compiler crashes if this is mat3x4 so
			// just adding an unused column to keep compiler happy.
			colour_matrix = mat4(vec4(1,depth_relative,0,alpha),vec4(1,1,1,1),vec4(0,depth_relative,1,alpha),vec4(1)); 
			
			// Flip order of crossing level IDs
			crossing_level_back = 6-crossing_level_back;
			crossing_level_front = 6-crossing_level_front;
		}
		else
		{
			// Ray crosses surfaces in gradient direction: dot(ray.direction,gradient) > 0

			// Colour order of surfaces in one deviation window zone in ray direction
			// MacOS (Snow Leopard) GLSL compiler crashes if this is mat3x4 so
			// just adding an unused column to keep compiler happy.
			colour_matrix = mat4(vec4(0,depth_relative,1,alpha),vec4(1,1,1,1),vec4(1,depth_relative,0,alpha),vec4(1)); 
		}

		// Blending of colours of surfaces in the deviation window zones for isovalue1 and isovalue2
		for (int i = crossing_level_back; i < crossing_level_front; i++)
		{
			int i_mod = int(mod(i,3));
			colour_crossing.rgb += (1-colour_crossing.a)*colour_matrix[i_mod].rgb*colour_matrix[i_mod].a*diffuse;
			colour_crossing.a += (1-colour_crossing.a)*colour_matrix[i_mod].a;
		}

		return colour_crossing;
	}

	// Isovalue colour mapping
	// Mapping according to colourmap. Colour for window surfaces is stored in colour_matrix_init.
	if (colour_mode_isovalue)
	{
		// MacOS (Snow Leopard) GLSL compiler crashes if this is mat3x4 so
		// just adding an unused column to keep compiler happy.
		mat4 colour_matrix_back, colour_matrix_front;

		if (!positive_crossing)
		{
			// Ray crosses surfaces in opposite gradient direction: dot(ray.direction,gradient) < 0

			// Colour order of surfaces in deviation window zone for isovalue2
			// MacOS (Snow Leopard) GLSL compiler crashes if this is mat3x4 so
			// just adding an unused column to keep compiler happy.
			colour_matrix_back = mat4(vec4(colour_matrix_init[3].rgb,alpha),vec4(1,1,1,1),vec4(colour_matrix_init[2].rgb,alpha),vec4(1));

			// Colour order of surfaces in deviation window zone for isovalue1
			// MacOS (Snow Leopard) GLSL compiler crashes if this is mat3x4 so
			// just adding an unused column to keep compiler happy.
			colour_matrix_front = mat4(vec4(colour_matrix_init[1].rgb,alpha),vec4(1,1,1,1),vec4(colour_matrix_init[0].rgb,alpha),vec4(1));

			// Flip order of crossing level IDs
			crossing_level_back = 6-crossing_level_back;
			crossing_level_front = 6-crossing_level_front;

		}
		else
		{
			// Ray crosses surfaces in gradient direction: dot(ray.direction,gradient) > 0

			// Colour order of surfaces in deviation window zone for isovalue1
			// MacOS (Snow Leopard) GLSL compiler crashes if this is mat3x4 so
			// just adding an unused column to keep compiler happy.
			colour_matrix_back = mat4(vec4(colour_matrix_init[0].rgb,alpha),vec4(1,1,1,1),vec4(colour_matrix_init[1].rgb,alpha),vec4(1));

			// Colour order of surfaces in deviation window zone for isovalue2
			// MacOS (Snow Leopard) GLSL compiler crashes if this is mat3x4 so
			// just adding an unused column to keep compiler happy.
			colour_matrix_front = mat4(vec4(colour_matrix_init[2].rgb,alpha),vec4(1,1,1,1),vec4(colour_matrix_init[3].rgb,alpha),vec4(1));
		}

		// Blending of colours of surfaces in the first deviation window (in ray direction)
		for (int i = crossing_level_back; i < int(min(float(crossing_level_front),3)); i++)
		{
			colour_crossing.rgb += (1-colour_crossing.a)*colour_matrix_back[i].rgb*colour_matrix_back[i].a*diffuse;
			colour_crossing.a += (1-colour_crossing.a)*colour_matrix_back[i].a;
		}

		// Blending of colours of surfaces in the second deviation window (in ray direction)
		for (int i = int(max(float(crossing_level_back),3)); i < crossing_level_front; i++)
		{
			colour_crossing.rgb += (1-colour_crossing.a)*colour_matrix_front[i-3].rgb*colour_matrix_front[i-3].a*diffuse;
			colour_crossing.a += (1-colour_crossing.a)*colour_matrix_front[i-3].a;
		}

		return colour_crossing;
	}
}


// Function for returning the colour of transparant volume between window enclosing surfaces and isosurface if volume rendering is turned on
vec4
get_volume_colour(
		float ray_sample_depth_radius,
		int crossing_level_front_mod,
		float field_scalar,
		vec3 field_gradient)
{
	vec4 colour_volume;

	// Switch between different colour modes			
	
	// Gradient strength colour-mapping
	if (colour_mode_gradient)
	{
		if (crossing_level_front_mod==1)
		{
			// Sampling point with gradient pointing towards isosurface
			// Mapping of ||gradient||: [0,max||gradient||] -> [col_map(0.5),col_map(1)]
			colour_volume = look_up_table_1D(
								colour_palette_sampler,
								colour_palette_resolution,
                        -min_max_gradient_magnitude.y,
                        min_max_gradient_magnitude.y,
                        length(field_gradient));
		}
		else
		{
			// Sampling point with gradient pointing away from isosurface
			// Mapping of -||gradient||: [-max||gradient||,0] -> [col_map(0),col_map(0.5)]
			colour_volume = look_up_table_1D(
								colour_palette_sampler,
								colour_palette_resolution,
                        -min_max_gradient_magnitude.y,
                        min_max_gradient_magnitude.y,
                        -length(field_gradient));
		}

		return colour_volume;
	}

	// Depth to colour mapping
	// Mapping: [radius=0,radius=1] -> [red,yellow] for zone [isovalue,isovalue + deviation]
	//			[radius=0,radius=1] -> [blue,cyan] for zone [isovalue - deviation,isovalue]
	if (colour_mode_depth)
	{
		colour_volume.rgb = vec3(int(crossing_level_front_mod==2),(ray_sample_depth_radius-render_min_max_depth_radius_restriction.x)/(render_min_max_depth_radius_restriction.y-render_min_max_depth_radius_restriction.x),int(crossing_level_front_mod==1));
	
		return colour_volume;
	}
	
	// Isovalue colour mapping
	// Mapping according to colourmap.
	if (colour_mode_isovalue)
	{
		colour_volume = look_up_table_1D(colour_palette_sampler,
						colour_palette_resolution,
						min_max_scalar_value.x,
						min_max_scalar_value.y,
						field_scalar);
	
		return colour_volume;
	}
}

// Function for returning the colour within window zones at surface of outer sphere
vec4
get_surface_entry_colour(
		float field_scalar,
		vec3 field_gradient,
		float iso_value,
		float deviation)
{
	vec4 colour_surface;
			
	// Gradient strength colour-mapping
	if (colour_mode_gradient)
	{
		if (field_scalar < iso_value)
		{
			// Sampling point with gradient pointing towards isosurface
			// Mapping of ||gradient||: [0,max||gradient||] -> [col_map(0.5),col_map(1)]
			colour_surface = look_up_table_1D(
								colour_palette_sampler,
								colour_palette_resolution,
                        -min_max_gradient_magnitude.y,
                        min_max_gradient_magnitude.y,
                        length(field_gradient));
		}
		else
		{
			// Sampling point with gradient pointing away from isosurface
			// Mapping of -||gradient||: [-max||gradient||,0] -> [col_map(0),col_map(0.5)]
			colour_surface = look_up_table_1D(
								colour_palette_sampler,
								colour_palette_resolution,
                        -min_max_gradient_magnitude.y,
                        min_max_gradient_magnitude.y,
                        -length(field_gradient));
		}

		return colour_surface;
	}
	
	// Depth to colour mapping
	// Mapping: (field_scalar-iso_value)/deviation -> [-1,1] -> [cyan -> green -> yellow]
	if (colour_mode_depth)
	{
		float deviation_relative = (field_scalar-iso_value)/deviation;

		if (deviation_relative < 0)
		{	
			colour_surface = vec4(0,1,-deviation_relative,0);
		}
		else
		{
			colour_surface = vec4(deviation_relative,1,0,0);
		}
		
		return colour_surface;
	}

	// Isovalue colour mapping
	// Mapping according to colourmap.
	if (colour_mode_isovalue)
	{
		colour_surface = look_up_table_1D(colour_palette_sampler,
							colour_palette_resolution,
							min_max_scalar_value.x,
							min_max_scalar_value.y,
							field_scalar);

		return colour_surface;
	}
}

// Function for returning the colour within window zones at extruded polygon walls
vec4
get_wall_entry_colour(
		float field_scalar,
		vec3 field_gradient,
		float iso_value,
		float deviation,
		float depth_relative)
{
	vec4 colour_wall;
			
	// Gradient strength colour-mapping
	if (colour_mode_gradient)
	{
		if (field_scalar < iso_value)
		{
			// Sampling point with gradient pointing towards isosurface
			// Mapping of ||gradient||: [0,max||gradient||] -> [col_map(0.5),col_map(1)]
			colour_wall = look_up_table_1D(
								colour_palette_sampler,
								colour_palette_resolution,
                        -min_max_gradient_magnitude.y,
                        min_max_gradient_magnitude.y,
                        length(field_gradient));
		}
		else
		{
			// Sampling point with gradient pointing away from isosurface
			// Mapping of -||gradient||: [-max||gradient||,0] -> [col_map(0),col_map(0.5)]
			colour_wall = look_up_table_1D(
								colour_palette_sampler,
								colour_palette_resolution,
                        -min_max_gradient_magnitude.y,
                        min_max_gradient_magnitude.y,
                        -length(field_gradient));
		}

		return colour_wall;
	}
	
	// Depth to colour mapping
	// Mapping: (field_scalar-iso_value)/deviation -> [-1,1] -> COLOUR_RANGE
	// Mapping COLOUR_RANGE: depth_relative -> [0,1] -> [[red -> green -> blue],[cyan -> green -> yellow]]
	if (colour_mode_depth)
	{
		float deviation_relative = (field_scalar-iso_value)/deviation;

		float colour_index = 0.5*(deviation_relative+1)*(1-depth_relative*0.5)+depth_relative*0.25;

		if (colour_index <= 0.5)
		{
			if (colour_index <= 0.25)
			{
				colour_wall = vec4(0,colour_index*4,1,0);
			}
			else
			{
				colour_wall = vec4(0,1,2-colour_index*4,0);
			}
		}
		else
		{
			if (colour_index <= 0.75)
			{
				colour_wall = vec4(colour_index*4-2,1,0,0);
			}
			else
			{
				colour_wall = vec4(1,4-colour_index*4,0,0);
			}
		}

		return colour_wall;
	}

	// Isovalue colour mapping
	// Mapping according to colourmap.
	if (colour_mode_isovalue)
	{
		colour_wall = look_up_table_1D(colour_palette_sampler,
							colour_palette_resolution,
							min_max_scalar_value.x, // input_lower_bound
							min_max_scalar_value.y, // input_upper_bound
							field_scalar);
	
		return colour_wall;
	}
}
		
// Raycasting process
bool
raycasting(
		out vec4 colour,
		out vec3 iso_surface_position)
{
	// Return values after ray casting stopped
	colour = vec4(0,0,0,0);
	iso_surface_position = vec3(0,0,0);

	// Normal at ray- (window/iso)surface crossing
	vec3 normal = vec3(0,0,0);

	// Background color (for either inner sphere or wall) is blended after the ray color 
	vec4 colour_background = vec4(0,0,0,0);

	// Colour matrix contains colour vectors for window surfaces using isovalue colour mapping
	mat4 colour_matrix_init; 

	colour_matrix_init[0] = look_up_table_1D(colour_palette_sampler,
								colour_palette_resolution,
								min_max_scalar_value.x, // input_lower_bound
								min_max_scalar_value.y, // input_upper_bound
								isovalue1.x-isovalue1.y); 

	colour_matrix_init[1] = look_up_table_1D(colour_palette_sampler,
								colour_palette_resolution,
								min_max_scalar_value.x, // input_lower_bound
								min_max_scalar_value.y, // input_upper_bound
								isovalue1.x+isovalue1.z);

	colour_matrix_init[2] = look_up_table_1D(colour_palette_sampler,
								colour_palette_resolution,
								min_max_scalar_value.x, // input_lower_bound
								min_max_scalar_value.y, // input_upper_bound
								isovalue2.x-isovalue2.y); 

	colour_matrix_init[3] = look_up_table_1D(colour_palette_sampler,
								colour_palette_resolution,
								min_max_scalar_value.x, // input_lower_bound
								min_max_scalar_value.y, // input_upper_bound
								isovalue2.x+isovalue2.z);


	// This branches on a uniform variable and hence is efficient since all pixels follow same path.
	if (read_from_surface_occlusion_texture)
	{
		// If a surface object on the front of the globe has an alpha value of 1.0 (and hence occludes
		// the current ray) then discard the current pixel.
		// Convert [-1,1] range to [0,1] for texture coordinates.
		float surface_occlusion = texture2D(surface_occlusion_texture_sampler, 0.5 * screen_coord + 0.5).a;
		
		if (surface_occlusion == 1.0)
		{
			return false;
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
	Sphere outer_depth_sphere = Sphere(render_min_max_depth_radius_restriction.y * render_min_max_depth_radius_restriction.y);

	// Ray is initialized.
	Ray ray = get_ray(screen_coord, gl_ModelViewProjectionMatrixInverse, eye_position);

	// Test if ray intersects with outer depth sphere.
	Interval outer_depth_sphere_interval;
	if (!intersect(outer_depth_sphere, ray, outer_depth_sphere_interval))
	{
		// Discard the current pixel.
		return false;
	}

	// Maximum ray length.
	float max_lambda = outer_depth_sphere_interval.to;

	 // Start distance between sampling point and ray origin.
	float min_lambda = outer_depth_sphere_interval.from;

   
	// This branches on a uniform variable and hence is efficient since all pixels follow same path.
	if (read_from_depth_texture)
	{
		// The depth texture might reduce the maximum ray length enabling early ray termination.
		// Convert [-1,1] range to [0,1] for texture coordinates.
		// Depth texture is a single-channel floating-point texture (GL_R32F).
		float depth_texture_screen_space_depth = texture2D(depth_texture_sampler, 0.5 * screen_coord + 0.5).r;
		float depth_texture_lambda = convert_screen_space_depth_to_ray_lambda(
				depth_texture_screen_space_depth, screen_coord, gl_ModelViewProjectionMatrixInverse, eye_position);
		max_lambda = min(max_lambda, depth_texture_lambda);
	}

	

	// Squared radius of the inner depth sphere is set.
	Sphere inner_depth_sphere = Sphere(render_min_max_depth_radius_restriction.x * render_min_max_depth_radius_restriction.x);
	Interval inner_depth_sphere_interval;

 
	// Test if ray intersects with inner depth sphere
	if (intersect(inner_depth_sphere, ray, inner_depth_sphere_interval))
	{
		max_lambda = min(inner_depth_sphere_interval.from, max_lambda);

		iso_surface_position = at(ray, max_lambda);

		// Inner depth sphere is white.
		colour_background = vec4(1,1,1,1);
		
		if (lighting_enabled)
		{
			normal = normalize(at(ray, max_lambda));

			// The Lambert term in diffuse lighting.
			float lambert = lambert_diffuse_lighting(world_space_light_direction, normal);

			// Blend between ambient and diffuse lighting - when ambient is 1.0 there is no diffuse.
			float diffuse = mix(lambert, 1.0, light_ambient_contribution);

			colour_background.rgb *= diffuse;
		}

	}

	float lambda_increment = sampling_rate.x;
	lambda_increment = (max_lambda - min_lambda)/ceil((max_lambda - min_lambda) / lambda_increment);
	//int num_ray_samples = 1 + int(ceil((max_lambda - min_lambda) / lambda_increment));
	//// Adjust increment to match integer number of ray samples.
	//lambda_increment = (max_lambda - min_lambda) / (num_ray_samples - 1);

	// Current distance between sampling point and ray origin => lambda
	float lambda = min_lambda;

	vec3 ray_sample_position = at(ray, lambda);

	vec3 pos_increment = lambda_increment * ray.direction;
	
	

	// The crossing level ID determines in which interval between the isosurfaces and window surfaces for both isovalues the sampling point is located.
	// The ID is computed as: ID = int(field_scalar >= isovalue1.x) + int(field_scalar >= (isovalue1.x-isovalue1.y)) + int(field_scalar >= (isovalue1.x+isovalue1.z)) +
	// + int(field_scalar >= isovalue2.x) + int(field_scalar >= (isovalue2.x-isovalue2.y)) + int(field_scalar >= (isovalue2.x+isovalue2.z));
	int crossing_level_back = 0;
	int crossing_level_front = 0;


	// For each sampling interval along the ray it is tested if the interval boundaries are below regions with true values in the mask texture. 
	// Only if active_back and active_front are both true, the respective surface crossed by the interval is rendered.
	bool active_back = true;
	bool active_front = true;
	

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
	
		active_front = projects_into_surface_fill_mask(surface_fill_mask_sampler, surface_fill_mask_resolution, cube_face_index, cube_face_coordinate_uv);
	}

	// Extruded polygon walls
	if (show_volume_fill_walls)
	{
		// Access the wall normal/depth (if present for the current screen pixel).
		vec4 wall_sample = texture2D(volume_fill_walls_sampler, 0.5 * screen_coord + 0.5);
		float screen_space_wall_depth = wall_sample.a;

		// The screen-space depth range is [-1, 1] and a value of 1 (far plane) indicates there's no wall.
		if (screen_space_wall_depth < 1.0)
		{
			vec4 colour_wall = vec4(1,1,1,1);

			// Extraction of wall colour at ray intersection point

			float lambda_wall = convert_screen_space_depth_to_ray_lambda(
					wall_sample.a/*min depth*/, screen_coord, gl_ModelViewProjectionMatrixInverse, eye_position);

			vec3 ray_sample_position_wall = at(ray, lambda_wall);

			int cube_face_index_wall;
			vec3 projected_ray_sample_position_wall = project_world_position_onto_cube_face(ray_sample_position_wall, cube_face_index_wall);

			
			vec2 cube_face_coordinate_xy_wall = convert_projected_world_position_to_local_cube_face_coordinate_frame(
					projected_ray_sample_position_wall, cube_face_index_wall);


			vec2 cube_face_coordinate_uv_wall = 0.5 * cube_face_coordinate_xy_wall + 0.5;

			vec2 tile_offset_uv_wall;
			vec2 tile_meta_data_coordinate_uv_wall;
			vec2 tile_coordinate_uv_wall;
			vec3 tile_meta_data_wall;
			float tile_ID_wall;

			get_tile_uv_coordinates(
				cube_face_coordinate_uv_wall,
				tile_meta_data_resolution,
				tile_resolution,
				tile_offset_uv_wall,
				tile_meta_data_coordinate_uv_wall,
				tile_coordinate_uv_wall);

			tile_meta_data_wall = texture2DArray(tile_meta_data_sampler, vec3(tile_meta_data_coordinate_uv_wall, cube_face_index_wall)).rgb;
			tile_ID_wall = tile_meta_data_wall.r;

			float ray_sample_depth_radius_wall = length(ray_sample_position_wall);

			float ray_depth_layer_wall = look_up_table_1D(
					depth_radius_to_layer_sampler,
					depth_radius_to_layer_resolution,
					min_max_depth_radius.x, // input_lower_bound
					min_max_depth_radius.y, // input_upper_bound
					ray_sample_depth_radius_wall).r;

			// Sample the field data texture array.
			vec4 field_data_wall = sample_field_data_texture_array(
					field_data_sampler, tile_coordinate_uv_wall, num_depth_layers, tile_ID_wall, ray_depth_layer_wall);

			// Relative depth: 0 -> core , 1 -> surface (for restricted max/min depth range)
			float depth_relative = (ray_sample_depth_radius_wall-render_min_max_depth_radius_restriction.x)/(render_min_max_depth_radius_restriction.y-render_min_max_depth_radius_restriction.x);

			// Colour for window zone at wall around isovalue1
			if (field_data_wall.r <= isovalue1.x)
			{
				if (field_data_wall.r >= (isovalue1.x-isovalue1.y))
				{
					if (mod(floor((surface_deviation_isoline_frequency*4+1)*0.5*abs(field_data_wall.r-isovalue1.x+isovalue1.y)/isovalue1.y),2) == 0)
					{
						vec4 colour_surface = get_wall_entry_colour(field_data_wall.r, field_data_wall.gba, isovalue1.x, isovalue1.y, depth_relative);

						colour_wall = vec4(colour_surface.rgb,1);
					}
				}
			}
			else
			{
				if (field_data_wall.r <= (isovalue1.x+isovalue1.z))
				{
					if (mod(floor((surface_deviation_isoline_frequency*4+1)*0.5*abs(field_data_wall.r-isovalue1.x+isovalue1.z)/isovalue1.z),2) == 0)
					{
						vec4 colour_surface = get_wall_entry_colour(field_data_wall.r, field_data_wall.gba, isovalue1.x, isovalue1.z, depth_relative);

						colour_wall = vec4(colour_surface.rgb,1);
					}
				}
			}

			// Colour for window zone at wall around isovalue2
			if (field_data_wall.r <= isovalue2.x)
			{
				if (field_data_wall.r >= (isovalue2.x-isovalue2.y))
				{
					if (mod(floor((surface_deviation_isoline_frequency*4+1)*0.5*abs(field_data_wall.r-isovalue2.x+isovalue2.y)/isovalue2.y),2) == 0)
					{
						vec4 colour_surface = get_wall_entry_colour(field_data_wall.r, field_data_wall.gba, isovalue2.x, isovalue2.y, depth_relative);

						colour_wall = vec4(colour_surface.rgb,1);
					}
				}
			}
			else
			{
				if (field_data_wall.r <= (isovalue2.x+isovalue2.z))
				{
					if (mod(floor((surface_deviation_isoline_frequency*4+1)*0.5*abs(field_data_wall.r-isovalue2.x+isovalue2.z)/isovalue2.z),2) == 0)
					{
						vec4 colour_surface = get_wall_entry_colour(field_data_wall.r, field_data_wall.gba, isovalue2.x, isovalue2.z, depth_relative);

						colour_wall = vec4(colour_surface.rgb,1);
					}
				}
			}

			if (lighting_enabled)
			{
				// The Lambert term in diffuse lighting.
				float lambert = lambert_diffuse_lighting(world_space_light_direction, wall_sample.rgb);

				// Blend between ambient and diffuse lighting - when ambient is 1.0 there is no diffuse.
				float diffuse = mix(lambert, 1.0, light_ambient_contribution);

				colour_wall.rgb *= diffuse;
			}

			if (active_front)
			{
				// Wall is crossed by a ray coming from an active masking zone (covered by polygon)
				// Ray casting is performed; background colour is blended after raycasting colour
				
				// Wall crossing happens before inner sphere crossing -> new background colour
				if (lambda_wall <= max_lambda)
				{
					colour_background = colour_wall;
					iso_surface_position = at(ray, lambda_wall);
				}
			}
			else
			{
				// Wall is crossed by a ray coming from outside of an active masking zone (not covered by polygon)
				// Ray casting is NOT performed; background colour is return as final raycasting colour

				if (lambda_wall >= max_lambda)
				{
					// Wall crossing happens after inner sphere crossing -> background colour of inner sphere is returned; 

					colour = colour_background;
				}
				else
				{
					// Wall crossing happens before inner sphere crossing -> wall colour is returned

					colour = colour_wall;
					iso_surface_position = at(ray, lambda_wall);
				}

				return true;
			}

			// Ray will be terminated at wall intersection point.
			max_lambda = min(max_lambda, lambda_wall);
		}
		else
		{
			if (!active_front)
			{
				// No intersection with wall, but ray starts outside an active masking (polygon) zone -> cannot enter an active zone as all walls are fully opaque
				// Ray casting is NOT performed; background colour is return as final raycasting colour
				
				colour = colour_background;
				
				return (colour.a == 1);
			}
		}
	
	}

	 // Make sure min_lambda did not somehow become larger than max_lambda.
	min_lambda = min(min_lambda, max_lambda);
			
	// Convert the local cube face (x,y) coordinates to (u,v) coordinates into metadata and field data textures.
	vec2 tile_offset_uv;
	vec2 tile_meta_data_coordinate_uv;
	vec2 tile_coordinate_uv;
	vec3 tile_meta_data;
	float tile_ID;

	get_tile_uv_coordinates(
		cube_face_coordinate_uv,
		tile_meta_data_resolution,
		tile_resolution,
		tile_offset_uv,
		tile_meta_data_coordinate_uv,
		tile_coordinate_uv);

	// Sample the tile metadata texture array.
	tile_meta_data = texture2DArray(tile_meta_data_sampler, vec3(tile_meta_data_coordinate_uv, cube_face_index)).rgb;
	tile_ID = tile_meta_data.r;

	// Not needed right now ... maybe useful for later acceleration approaches 
	//float max_field_scalar = tile_meta_data.g;
	//float min_field_scalar = tile_meta_data.b;

	// If the current tile has no data then continue to the next ray sample (it may enter a new tile).
	active_front = active_front && (tile_ID >= 0);
	
	// Sample the mask data texture array.
	float mask_data = texture2DArray(mask_data_sampler, vec3(tile_coordinate_uv, tile_ID)).r;

	// If the current ray sample location (at all vertical depth locations) contains no field data
	// then continue to the next ray sample.
	//
	// TODO: What happens for (bilinearly filtered) values *between* 0 and 1 ? Do they mean accept or reject ?
	// If accept, then does that mean one or more samples used in bilinear filter of field data is invalid and
	// hence corrupts scalar/gradient result ?
	// Or is the field data dilated one texel (towards mask boundary texels) during pre-processing ?

	active_front = active_front && (mask_data != 0);
	
	
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

	// Crossing level ID is obtained for front sampling point of sampling interval
	crossing_level_front = int(field_scalar >= isovalue1.x) + int(field_scalar >= (isovalue1.x-isovalue1.y)) + int(field_scalar >= (isovalue1.x+isovalue1.z));
	crossing_level_front += int(field_scalar >= isovalue2.x) + int(field_scalar >= (isovalue2.x-isovalue2.y)) + int(field_scalar >= (isovalue2.x+isovalue2.z));
	crossing_level_back = crossing_level_front;


	// Rendering of contours in +- "deviation"-window around the isovalues on the outer sphere
	if ((surface_deviation_window) && (active_front))
	{
		if (field_scalar <= isovalue1.x)
		{
			if (field_scalar >= (isovalue1.x - isovalue1.y))
			{
				// Special view dependent opacity correction for surfaces resulting in improved 3D effects
				float alpha = 1-exp(log(1-opacity_deviation_surfaces)/abs(dot(normalize(ray_sample_position),ray.direction)));

				// Differentiation between coloured and white stripes (stripe containing isocontour is always coloured!)
				if (mod(floor((surface_deviation_isoline_frequency*4+1)*0.5*abs(field_scalar-isovalue1.x+isovalue1.y)/isovalue1.y),2) == 0)
				{
					vec4 colour_surface = get_surface_entry_colour(field_scalar, field_gradient, isovalue1.x, isovalue1.y);

					colour = vec4(colour_surface.rgb,1)*alpha;
				}
				else
				{
					colour = vec4(1,1,1,1)*alpha;
				}

				if (lighting_enabled)
				{
					// The Lambert term in diffuse lighting.
					float lambert = lambert_diffuse_lighting(world_space_light_direction, normalize(ray_sample_position));

					// Blend between ambient and diffuse lighting - when ambient is 1.0 there is no diffuse.
					float diffuse = mix(lambert, 1.0, light_ambient_contribution);

					colour.rgb *= diffuse;
				}
			}
		}
		else
		{
			if (field_scalar <= (isovalue1.x + isovalue1.z))
			{
				// Special view dependent opacity correction for surfaces resulting in improved 3D effects
				float alpha = 1-exp(log(1-opacity_deviation_surfaces)/abs(dot(normalize(ray_sample_position),ray.direction)));

				// Differentiation between coloured and white stripes (stripe containing isocontour is always coloured!)
				if (mod(floor((surface_deviation_isoline_frequency*4+1)*0.5*abs(field_scalar-isovalue1.x+isovalue1.z)/isovalue1.z),2) == 0)
				{
					vec4 colour_surface = get_surface_entry_colour(field_scalar, field_gradient, isovalue1.x, isovalue1.z);

					colour = vec4(colour_surface.rgb,1)*alpha;
				}
				else
				{
					colour = vec4(1,1,1,1)*alpha;
				}

				if (lighting_enabled)
				{
					// The Lambert term in diffuse lighting.
					float lambert = lambert_diffuse_lighting(world_space_light_direction, normalize(ray_sample_position));

					// Blend between ambient and diffuse lighting - when ambient is 1.0 there is no diffuse.
					float diffuse = mix(lambert, 1.0, light_ambient_contribution);

					colour.rgb *= diffuse;
				}
			}
		}

		if (field_scalar <= isovalue2.x)
		{
			if (field_scalar >= (isovalue2.x - isovalue2.y))
			{
				// Special view dependent opacity correction for surfaces resulting in improved 3D effects
				float alpha = 1-exp(log(1-opacity_deviation_surfaces)/abs(dot(normalize(ray_sample_position),ray.direction)));

				// Differentiation between coloured and white stripes (stripe containing isocontour is always coloured!)
				if (mod(floor((surface_deviation_isoline_frequency*4+1)*0.5*abs(field_scalar-isovalue2.x+isovalue2.y)/isovalue2.y),2) == 0)
				{
					vec4 colour_surface = get_surface_entry_colour(field_scalar, field_gradient, isovalue2.x, isovalue2.y);

					colour = vec4(colour_surface.rgb,1)*alpha;
				}
				else
				{
					colour = vec4(1,1,1,1)*alpha;
				}

				if (lighting_enabled)
				{
					// The Lambert term in diffuse lighting.
					float lambert = lambert_diffuse_lighting(world_space_light_direction, normalize(ray_sample_position));

					// Blend between ambient and diffuse lighting - when ambient is 1.0 there is no diffuse.
					float diffuse = mix(lambert, 1.0, light_ambient_contribution);

					colour.rgb *= diffuse;
				}
			}
		}
		else
		{
			if (field_scalar <= (isovalue2.x + isovalue2.z))
			{
				// Special view dependent opacity correction for surfaces resulting in improved 3D effects
				float alpha = 1-exp(log(1-opacity_deviation_surfaces)/abs(dot(normalize(ray_sample_position),ray.direction)));

				// Differentiation between coloured and white stripes (stripe containing isocontour is always coloured!)
				if (mod(floor((surface_deviation_isoline_frequency*4+1)*0.5*abs(field_scalar-isovalue2.x+isovalue2.z)/isovalue2.z),2) == 0)
				{
					vec4 colour_surface = get_surface_entry_colour(field_scalar, field_gradient, isovalue2.x, isovalue2.z);

					colour = vec4(colour_surface.rgb,1)*alpha;
				}
				else
				{
					colour = vec4(1,1,1,1)*alpha;
				}

				if (lighting_enabled)
				{
					// The Lambert term in diffuse lighting.
					float lambert = lambert_diffuse_lighting(world_space_light_direction, normalize(ray_sample_position));

					// Blend between ambient and diffuse lighting - when ambient is 1.0 there is no diffuse.
					float diffuse = mix(lambert, 1.0, light_ambient_contribution);

					colour.rgb *= diffuse;
				}
			}
		}
	}

	// Blending step during volume rendering for deviation window zone
	if (deviation_window_volume_rendering)
	{
		int crossing_level_front_mod = int(mod(crossing_level_front,3));

		// Sampling point is in window around isovalue1 (ID = 1 or ID = 2) or around isovalue2 (ID = 4 or ID = 5)
		if ((crossing_level_front_mod>= 1) && (crossing_level_front_mod <= 2) && (active_front))
		{
			vec4 colour_volume = get_volume_colour(ray_sample_depth_radius, crossing_level_front_mod, field_scalar, field_gradient);

			colour.rgb += (1-colour.a)*colour_volume.rgb*opacity_deviation_window_volume_rendering;
			colour.a += (1-colour.a)*opacity_deviation_window_volume_rendering;	
		}
	}

	// Update sample position and lambda
	ray_sample_position += pos_increment;
	lambda += lambda_increment;

	// Do ray casting until max_lambda is reached or colour becomes opaque
	while ((lambda < max_lambda) && (colour.a <= 0.95))
	{
		// Update data for previous sampling point
		active_back = active_front;
		crossing_level_back = crossing_level_front;

		projected_ray_sample_position = project_world_position_onto_cube_face(ray_sample_position, cube_face_index); 

		cube_face_coordinate_xy = convert_projected_world_position_to_local_cube_face_coordinate_frame(
			projected_ray_sample_position, cube_face_index);

		cube_face_coordinate_uv = 0.5 * cube_face_coordinate_xy + 0.5;

		if (using_surface_fill_mask)
		{
			// If the current ray sample does not project into the surface mask then
			// continue to the next ray sample (it may enter a fill region).
	
			active_front = projects_into_surface_fill_mask(surface_fill_mask_sampler, surface_fill_mask_resolution, cube_face_index, cube_face_coordinate_uv);
		}

		get_tile_uv_coordinates(
			cube_face_coordinate_uv,
			tile_meta_data_resolution,
			tile_resolution,
			tile_offset_uv,
			tile_meta_data_coordinate_uv,
			tile_coordinate_uv);

		tile_meta_data = texture2DArray(tile_meta_data_sampler, vec3(tile_meta_data_coordinate_uv, cube_face_index)).rgb;
		tile_ID = tile_meta_data.r;

		active_front = active_front && (tile_ID >= 0);
//
		//if (tile_ID < 0)
		//{
			//ray_sample_position += pos_increment;
			//lambda += lambda_increment;
			//continue;
		//}

		float mask_data = texture2DArray(mask_data_sampler, vec3(tile_coordinate_uv, tile_ID)).r;

		active_front = active_front && (mask_data != 0);
//
		//if (mask_data == 0)
		//{
			//ray_sample_position += pos_increment;
			//lambda += lambda_increment;
			//continue;
		//}

		ray_sample_depth_radius = length(ray_sample_position);

		ray_depth_layer = look_up_table_1D(
			depth_radius_to_layer_sampler,
			depth_radius_to_layer_resolution,
			min_max_depth_radius.x,
			min_max_depth_radius.y,
			ray_sample_depth_radius).r;

		field_data = sample_field_data_texture_array(
			field_data_sampler, tile_coordinate_uv, num_depth_layers, tile_ID, ray_depth_layer);

		field_scalar = field_data.r;

		field_gradient = field_data.gba;

		// Determine crossing level ID for current sampling point
		crossing_level_front = int(field_scalar >= isovalue1.x) + int(field_scalar >= (isovalue1.x-isovalue1.y)) + int(field_scalar >= (isovalue1.x+isovalue1.z));
		crossing_level_front += int(field_scalar >= isovalue2.x) + int(field_scalar >= (isovalue2.x-isovalue2.y)) + int(field_scalar >= (isovalue2.x+isovalue2.z));
		
		// Crossing of one or multiple surfaces is detected
		if (crossing_level_front != crossing_level_back)
		{		
			// Bisection correction to optain more accurate values for crossing location and field gradient
			bisection_correction(
				ray_sample_position,
				lambda,
				lambda_increment,
				ray,
				crossing_level_back,
				crossing_level_front,
				active_back,
				active_front,
				field_scalar,
				field_gradient);	

			// Crossing happens in an active masking zone
			if ((active_back) && (active_front))
			{
				// Update position of detected isosurface (if multiple surfaces are detected only ONE position is returned!)
				iso_surface_position = ray_sample_position;

				bool positive_crossing = (crossing_level_front >= crossing_level_back);

				normal = (1-2*int(dot(field_gradient,ray.direction)>=0))*normalize(field_gradient);

				// Get final colour of one surface or final blended colour of multiple surfaces
				vec4 colour_crossing = get_blended_crossing_colour(
											ray_sample_position, 
											ray, 
											normal,
											crossing_level_back,
											crossing_level_front,
											positive_crossing, 
											colour_matrix_init,
											field_gradient);

				colour.rgb += (1-colour.a)*colour_crossing.rgb;
				colour.a += (1-colour.a)*colour_crossing.a;
			}

			
		}

		// Blending step during volume rendering for deviation window zone
		if (deviation_window_volume_rendering)
		{
			int crossing_level_front_mod = int(mod(crossing_level_front,3));

			// Sampling point is in window around isovalue1 (ID = 1 or ID = 2) or around isovalue2 (ID = 4 or ID = 5)
			if ((crossing_level_front_mod>= 1) && (crossing_level_front_mod <= 2) && (active_front))
			{
				vec4 colour_volume = get_volume_colour(ray_sample_depth_radius, crossing_level_front_mod, field_scalar, field_gradient);

				colour.rgb += (1-colour.a)*colour_volume.rgb*opacity_deviation_window_volume_rendering;
				colour.a += (1-colour.a)*opacity_deviation_window_volume_rendering;	
			}
		}

		ray_sample_position += pos_increment;
		lambda += lambda_increment;
	}

	// Artefact correction. Due to numerical inaccuracies (single precision!), adding up the lambda_increments does not result in max_lambda as last interval boundary.
	// This would result in pixel noise artefacts. This correction re-computes the last interval with the exact max_lambda value to overcome this problem.

	bool last_crossing = true;

	ray_sample_position = at(ray,max_lambda);
	lambda_increment = max_lambda - (lambda - lambda_increment);

	// The last interval becomes consecutively smaller; ray casting terminates as soon as no crossing is detected anymore
	while ((last_crossing) && (colour.a <= 0.95))
	{
		active_back = active_front;
		crossing_level_back = crossing_level_front;

		projected_ray_sample_position = project_world_position_onto_cube_face(ray_sample_position, cube_face_index); 

		cube_face_coordinate_xy = convert_projected_world_position_to_local_cube_face_coordinate_frame(
			projected_ray_sample_position, cube_face_index);

		cube_face_coordinate_uv = 0.5 * cube_face_coordinate_xy + 0.5;

		if (using_surface_fill_mask)
		{
			// If the current ray sample does not project into the surface mask then
			// continue to the next ray sample (it may enter a fill region).
		
			active_front = projects_into_surface_fill_mask(surface_fill_mask_sampler, surface_fill_mask_resolution, cube_face_index, cube_face_coordinate_uv);
		}

		get_tile_uv_coordinates(
			cube_face_coordinate_uv,
			tile_meta_data_resolution,
			tile_resolution,
			tile_offset_uv,
			tile_meta_data_coordinate_uv,
			tile_coordinate_uv);

		tile_meta_data = texture2DArray(tile_meta_data_sampler, vec3(tile_meta_data_coordinate_uv, cube_face_index)).rgb;
		tile_ID = tile_meta_data.r;

		active_front = active_front && (tile_ID >= 0);


		float mask_data = texture2DArray(mask_data_sampler, vec3(tile_coordinate_uv, tile_ID)).r;

		active_front = active_front && (mask_data != 0);

		ray_sample_depth_radius = length(ray_sample_position);

		ray_depth_layer = look_up_table_1D(
			depth_radius_to_layer_sampler,
			depth_radius_to_layer_resolution,
			min_max_depth_radius.x, // input_lower_bound
			min_max_depth_radius.y, // input_upper_bound
			ray_sample_depth_radius).r;

		field_data = sample_field_data_texture_array(
			field_data_sampler, tile_coordinate_uv, num_depth_layers, tile_ID, ray_depth_layer);

		field_scalar = field_data.r;

		field_gradient = field_data.gba;

		// Determine crossing level ID for current sampling point
		crossing_level_front = int(field_scalar >= isovalue1.x) + int(field_scalar >= (isovalue1.x-isovalue1.y)) + int(field_scalar >= (isovalue1.x+isovalue1.z));
		crossing_level_front += int(field_scalar >= isovalue2.x) + int(field_scalar >= (isovalue2.x-isovalue2.y)) + int(field_scalar >= (isovalue2.x+isovalue2.z));

		last_crossing = (crossing_level_front != crossing_level_back);

		// Crossing of one or multiple surfaces is detected
		if (last_crossing)
		{
			// Bisection correction to optain more accurate values for crossing location and field gradient
			bisection_correction(
						ray_sample_position,
						lambda,
						lambda_increment,
						ray,
						crossing_level_back,
						crossing_level_front,
						active_back,
						active_front,
						field_scalar,
						field_gradient);	

			// Crossing happens in an active masking zone
			if ((active_back) && (active_front))
			{
				// Update position of detected isosurface (if multiple surfaces are detected only ONE position is returned!)
				iso_surface_position = ray_sample_position;

				bool positive_crossing = (crossing_level_front >= crossing_level_back);

				normal = (1-2*int(dot(field_gradient,ray.direction)>=0))*normalize(field_gradient);

				// Get final colour of one surface or final blended colour of multiple surfaces
				vec4 colour_crossing = get_blended_crossing_colour(
											ray_sample_position, 
											ray, 
											normal,
											crossing_level_back,
											crossing_level_front,
											positive_crossing, 
											colour_matrix_init,
											field_gradient);

				colour.rgb += (1-colour.a)*colour_crossing.rgb;
				colour.a += (1-colour.a)*colour_crossing.a;
			}
		}

		// Blending step during volume rendering for devaition window zone
		if (deviation_window_volume_rendering)
		{
			int crossing_level_front_mod = int(mod(crossing_level_front,3));

			// Sampling point is in window around isovalue1 (ID = 1 or ID = 2) or around isovalue2 (ID = 4 or ID = 5)
			if ((crossing_level_front_mod>= 1) && (crossing_level_front_mod <= 2) && (active_front))
			{
				vec4 colour_volume = get_volume_colour(ray_sample_depth_radius, crossing_level_front_mod, field_scalar, field_gradient);

				colour.rgb += (1-colour.a)*colour_volume.rgb*opacity_deviation_window_volume_rendering;
				colour.a += (1-colour.a)*opacity_deviation_window_volume_rendering;
			}
		}

		ray_sample_position = at(ray,max_lambda);
		lambda_increment = max_lambda - lambda;

	}

	// Blending of earth core sphere or wall colour
	colour += (1-colour.a)*colour_background;

	return (colour.a > 0);

}
	

void
main()
{
	// Return values after ray casting stopped
	vec4 colour;
	vec3 iso_surface_position;

	// Start raycasting process
	//If raycasting does not return a colour or was not sucessful, discard pixel
	if (!raycasting(colour, iso_surface_position))
	{
		discard;
	}

	// Output colour as pre-multiplied alpha.
	// This enables alpha-blending to use (src,dst) blend factors of (1, 1-SrcAlpha) instead of
	// (SrcAlpha, 1-SrcAlpha) which keeps alpha intact (avoids double-blending when rendering to
	// a render-target and then, in turn, blending that into the main framebuffer).
	// Not that we have transparent iso-surfaces (yet).
	// colour.rgb *= colour.a;

	// This branches on a uniform variable and hence is efficient since all pixels follow same path.
	//if (lighting_enabled)
	//{
		//// The Lambert term in diffuse lighting.
		//float lambert = lambert_diffuse_lighting(world_space_light_direction, normal);
//
		//// Blend between ambient and diffuse lighting - when ambient is 1.0 there is no diffuse.
		//float diffuse = mix(lambert, 1.0, light_ambient_contribution);
//
		//colour.rgb *= diffuse;
	//}

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
