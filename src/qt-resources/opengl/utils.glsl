/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

/*
 * Shader source code to bilinearly interpolate a *non-mipmapped*,
 * *non-anisotropically filtered* 2D texture.
 *
 * Returns the four (nearest) sampled texels and the interpolation coefficients.
 *
 * 'tex_dimensions' should contain the following (xyzw) components:
 *	x: texture width,
 *	y: texture height,
 *	z: inverse texture width,
 *	w: inverse texture height.
 */
void
bilinearly_interpolate(
		sampler2D tex_sampler,
		vec2 tex_coords,
		vec4 tex_dimensions,
		out vec4 tex11,
		out vec4 tex21,
		out vec4 tex12,
		out vec4 tex22,
		out vec2 interp)
{
	// Multiply tex coords by texture dimensions to convert to unnormalised form.
	vec2 uv = tex_coords * tex_dimensions.xy;

	vec4 st;

	// The lower-left texel centre.
	st.xy = floor(uv - 0.5) + 0.5;
	// The upper-right texel centre.
	st.zw = st.xy + 1;

	// The bilinear interpolation coefficients.
	interp = uv - st.xy;

	// Multiply tex coords by inverse texture dimensions to return to normalised form.
	st *= tex_dimensions.zwzw;

	// Sample texture at 4 nearest texels.
	tex11 = texture(tex_sampler, st.xy);
	tex21 = texture(tex_sampler, st.zy);
	tex12 = texture(tex_sampler, st.xw);
	tex22 = texture(tex_sampler, st.zw);
}

/*
 * Bilinearly interpolate a data raster where the data is in the red channel and
 * the coverage is in the green channel.
 *
 * This function weights the bilinear filter according to the coverage texels:
 *
 *   V = sum(Wi * Ci * Vi)
 *       -----------------
 *         sum(Wi * Ci)
 *
 * ...where Wi is bilinear weight, Ci is coverage (in green channel) and Vi is value (in red channel).
 *
 * This RG format is used for data (numerical) rasters in GPlates.
 */
vec4
bilinearly_interpolate_data_coverage_RG(
		sampler2D tex_sampler,
		vec2 tex_coords,
		vec4 tex_dimensions)
{
	// The 2x2 texture sample to interpolate.
	vec4 tex11;
	vec4 tex21;
	vec4 tex12;
	vec4 tex22;

	// The bilinear interpolation coefficients.
	vec2 interp;

	// Sample texture as 4 nearest neighbour samples and get bilinear interpolation weights.
	bilinearly_interpolate(
		tex_sampler, tex_coords, tex_dimensions, tex11, tex21, tex12, tex22, interp);
	
	// Multiple the data (in red channel) by coverage (in green channel) for each sample.
	tex11.r *= tex11.g;
	tex21.r *= tex21.g;
	tex12.r *= tex12.g;
	tex22.r *= tex22.g;

	// Bilinearly interpolate the four texels.
	vec4 result = mix(mix(tex11, tex21, interp.x), mix(tex12, tex22, interp.x), interp.y);
	
	// Divide the interpolated data-times-coverage by the interpolated coverage.
	return vec4(result.r / result.g, result.gba);
}


/*
 * Shader source code to rotate an (x,y,z) vector by a quaternion.
 *
 * Normally it is faster to convert a quaternion to a matrix and then use that one matrix
 * to transform many vectors. However this usually means storing the rotation matrix
 * as shader constants which reduces batching when the matrix needs to be changed.
 * In some situations batching can be improved by sending the rotation matrix as vertex
 * attribute data (can then send a lot more geometries, each with different matrices,
 * in one batch because not limited by shader constant space limit) - and using
 * quaternions means 4 floats instead of 9 floats (ie, a single 4-component vertex attribute).
 * The only issue is a quaternion needs to be sent with *each* vertex of each geometry and
 * the shader code to do the transform is more expensive but in some situations (involving
 * large numbers of geometries) the much-improved batching is more than worth it.
 * The reason batching is important is each batch has a noticeable CPU overhead
 * (in OpenGL and the driver, etc) and it's easy to become CPU-limited.
 *
 * The following shader code is based on http://code.google.com/p/kri/wiki/Quaternions
 */
vec3
rotate_vector_by_quaternion(
		vec4 q,
		vec3 v)
{
	return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
}


/*
 * The Lambert term in the diffuse lighting equation.
 *
 * NOTE: Neither the light direction nor the surface normal need to be normalised.
 */
float
lambert_diffuse_lighting(
		vec3 light_direction,
		vec3 normal)
{
	// The light direction typically needs to be normalised because it's interpolated between vertices
	// (if using tangent-space lighting).
	// The surface normal might also need to be normalised because it's either interpolated between
	// vertices (no normal map) or bilinearly interpolated during texture (normal map) lookup.
	// We can save one expensive 'normalize' by rearranging...
	// Lambert = dot(normalize(N),normalize(L))
	//		   = dot(N/|N|,L/|L|) 
	//		   = dot(N,L) / (|N| * |L|) 
	//		   = dot(N,L) / sqrt(dot(N,N) * dot(L,L)) 
	// NOTE: Using float instead of integer parameters to 'max' otherwise driver compiler
	// crashes on some systems complaining cannot find (integer overload of) function in 'stdlib'.
	return max(0.0,
		dot(normal, light_direction) * 
			inversesqrt(dot(normal, normal) * dot(light_direction, light_direction)));
}

/*
 * Mixes ambient lighting with diffuse lighting.
 *
 * ambient_with_diffuse_lighting = ambient_lighting + (1 - ambient_lighting) * diffuse_lighting
 *
 * 'ambient_lighting' is in the range [0,1].
 */
float
mix_ambient_with_diffuse_lighting(
		float diffuse_lighting,
		float ambient_lighting)
{
	// Blend between ambient and diffuse lighting - when ambient is 1.0 there is no diffuse.
	// NOTE: Using float instead of integer parameters to 'mix' otherwise driver compiler
	// crashes on some systems complaining cannot find (integer overload of) function in 'stdlib'.
	return mix(diffuse_lighting, 1.0, ambient_lighting);
}

/*
 * Colour space conversion from RGB to HSV.
 *
 * The hue channel of HSV is normalised to the range [0,1].
 *
 * References include wikipedia (http://en.wikipedia.org/wiki/HSL_and_HSV),
 * the Nvidia shader library (http://developer.download.nvidia.com/shaderlibrary/packages/post_RGB_to_HSV.fx.zip), and
 * http://chilliant.blogspot.com.au/2010/11/rgbhsv-in-hlsl.html
 */
vec3
rgb_to_hsv(
		vec3 rgb)
{
	vec3 hsv = vec3(0);
	
	float min_rgb_channel = min(rgb.r, min(rgb.g, rgb.b));
	float max_rgb_channel = max(rgb.r, max(rgb.g, rgb.b));
	float chroma = max_rgb_channel - min_rgb_channel;
	hsv.z = max_rgb_channel; // value
	
	if (chroma != 0)
	{
		hsv.y = chroma / max_rgb_channel; // saturation
		vec3 d = (((vec3(max_rgb_channel) - rgb) / 6.0) + vec3(chroma / 2.0)) / chroma;
		
		if (rgb.x == max_rgb_channel)
		{
			hsv.x = d.z - d.y; // hue
		}
		else if (rgb.y == max_rgb_channel)
		{
			hsv.x = (1.0 / 3.0) + d.x - d.z; // hue
		}
		else if (rgb.z == max_rgb_channel)
		{
			hsv.x = (2.0 / 3.0) + d.y - d.x; // hue
		}
		
		// Handle hue wrap around.
		if (hsv.x < 0.0)
		{
			hsv.x += 1.0;
		}
		if (hsv.x > 1.0)
		{
			hsv.x -= 1.0;
		}
	}

	return hsv;
}

/*
 * Colour space conversion from HSV to RGB.
 *
 * The hue channel of HSV is normalised to the range [0,1].
 *
 * References include wikipedia (http://en.wikipedia.org/wiki/HSL_and_HSV),
 * the Nvidia shader library (http://developer.download.nvidia.com/shaderlibrary/packages/post_RGB_to_HSV.fx.zip), and
 * http://chilliant.blogspot.com.au/2010/11/rgbhsv-in-hlsl.html
 */
vec3
hsv_to_rgb(
		vec3 hsv)
{
	vec3 rgb = vec3(hsv.z);
	
	if (hsv.y != 0)
	{
		float h = hsv.x * 6;
		float i = floor(h);
		
		float var_1 = hsv.z * (1.0 - hsv.y);
		float var_2 = hsv.z * (1.0 - hsv.y * (h - i));
		float var_3 = hsv.z * (1.0 - hsv.y * (1 - (h - i)));
		
		if (i == 0)
		{
			rgb = vec3(hsv.z, var_3, var_1);
		}
		else if (i == 1)
		{
			rgb = vec3(var_2, hsv.z, var_1);
		}
		else if (i == 2)
		{
			rgb = vec3(var_1, hsv.z, var_3);
		}
		else if (i == 3)
		{
			rgb = vec3(var_1, var_2, hsv.z);
		}
		else if (i == 4)
		{
			rgb = vec3(var_3, var_1, hsv.z);
		}
		else
		{
			rgb = vec3(hsv.z, var_1, var_2);
		}
	}

	return rgb;
}

struct Ray
{
  vec3 origin;
  vec3 direction;
};

// Position at given ray parameter
vec3
at(
		const Ray ray,
		float lambda)
{
  return ray.origin + lambda * ray.direction;
}

struct Interval
{
  float from;
  float to;
};

bool
is_valid(
		const Interval interval)
{
  return interval.from <= interval.to;
}

void
invalidate(
		Interval interval)
{
  interval.from = 1;
  interval.to = 0;
}

struct Sphere
{
  float radius;
};

// Intersect a ray with a sphere centred at the origin.
//
// If the ray intersects the sphere then true is returned, along with the
// distance along the ray to the first intersection.
bool
intersect(
		const Sphere sphere,
		const Ray ray,
		out float lambda)
{
  float D = dot(/*sphere centre vec3(0,0,0)*/ - ray.origin, ray.direction);
  float L2 =  dot(ray.origin, ray.origin);
  float R2 = sphere.radius * sphere.radius;
  if (D < 0 && L2 > R2)
  {
	  // Ray origin is outside sphere (L2>R2) and sphere is behind ray origin.
	  // So positive direction along ray cannot intersect sphere.
	  return false;
  }

  float M2 = L2 - D * D;
  if (M2 > R2)
  {
  	// Infinite line along ray does not intersect the sphere.
  	return false;
  }
  
  float Q = sqrt(R2 - M2);

  // If ray origin is outside sphere (L2>R2) then we know sphere is also in front of ray origin
  // (since we already know that 'D<0 && L2>R2' is not true, so must have 'D>=0').
  // In this case we choose the first intersection since it's closest to ray origin.
  // Otherwise ray origin is inside sphere (L2<=R2) and can only intersect sphere once.
  // In this case we choose the second intersection (because first intersection is behind ray origin).
  lambda = (L2 > R2) ? (D - Q) : (D + Q);

  return true;
}

// Intersect the infinite line of a ray with a sphere centred at the origin.
//
// If the infinite line intersects the sphere then true is returned, along with the
// interval containing the signed distance from ray origin to the two intersections
// (in order of increasing signed distance).
bool
intersect_line(
		const Sphere sphere,
		const Ray ray,
		out Interval interval)
{
  float D = dot(/*sphere centre vec3(0,0,0)*/ - ray.origin, ray.direction);
  float L2 =  dot(ray.origin, ray.origin);
  float R2 = sphere.radius * sphere.radius;

  float M2 = L2 - D * D;
  if (M2 > R2)
  {
  	// Infinite line along ray does not intersect the sphere.
  	return false;
  }
  
  float Q = sqrt(R2 - M2);

  interval = Interval(D - Q /*from*/, D + Q /*to*/);

  return true;
}

bool
contains(
		const Sphere sphere,
		const vec3 point)
{
  float distSq = dot(point, point);
  return distSq <= sphere.radius * sphere.radius;
}

// Convert window-space xy coordinates (eg, gl_FragCoord.xy) to NDC space (range [-1,1]).
//
// 'viewport.xy' should contain viewport x and y.
// 'viewport.zw' should contain viewport width and height.
vec2
window_to_NDC_xy(
		const vec2 window_coord,
		const vec4 viewport)
{
	return 2.0 * (window_coord - viewport.xy) / viewport.zw - 1.0;
}

// Convert screen NDC coordinates (in range [-1, 1]) to position in view space.
//
// This gives the same result as 'screen_to_view()' but uses the projection matrix (not its *inverse*).
// This was originally devised as a replacement to give more accurate results but appears to give same results.
// The accuracy problem ended up being due to using 'screen_to_world()' with combined model-view-projection inverse
// matrix instead of separate model-view inverse and projection inverse matrices.
//
// Handles standard perspective and orthographic projections.
vec4
view_from_screen(
		const vec3 screen_coord,
		const mat4 projection)
{
	//
	// For a standard *orthographic* projection matrix:
	//
	// P = P00   0   0 P30
	//       0 P11   0 P31
	//       0   0 P22 P32
	//       0   0   0 P33
	//
	// clip = P * view
	//
	// clip_x = P00 * view_x                               + P30 * view_w
	// clip_y =                P11 * view_y                + P31 * view_w
	// clip_z =                               P22 * view_z + P32 * view_w
	// clip_w =                                              P33 * view_w
	//
	// ndc = clip / clip.w
	//
	// Note: In the following we assume view_w = 1 ...
	//
	// ndc_x = clip_x / clip_w
	//       = (P00 * view_x + P30) / P33
	// view_x = ((ndc_x * P33) - P30) / P00
	//
	// ndc_y = clip_y / clip_w
	//       = (P11 * view_y + P31) / P33
	// view_y = ((ndc_y * P33) - P31) / P11
	//
	// ndc_z = clip_z / clip_w
	//       = (P22 * view_z + P32) / P33
	// view_z = ((ndc_z * P33) - P32) / P22
	//

	// We can distinguish between an orthographic and perspective projection by looking at P23.
	// For orthographic it is 0 and for perspective it is -1.
	if (projection[2][3] == 0)
	{
		float view_x = ((screen_coord.x * projection[3][3]) - projection[3][0]) / projection[0][0];
		float view_y = ((screen_coord.y * projection[3][3]) - projection[3][1]) / projection[1][1];
		float view_z = ((screen_coord.z * projection[3][3]) - projection[3][2]) / projection[2][2];

		return vec4(view_x, view_y, view_z, 1);
	}

	//
	// For a standard *perspective* projection matrix:
	//
	// P = P00   0 P20   0
	//       0 P11 P21   0
	//       0   0 P22 P32
	//       0   0 P23   0
	//
	// clip = P * view
	//
	// clip_x = P00 * view_x                + P20 * view_z
	// clip_y =                P11 * view_y + P21 * view_z
	// clip_z =                               P22 * view_z + P32 * view_w
	// clip_w =                               P23 * view_z
	//
	// ndc = clip / clip.w
	//
	// Note: In the following we assume view_w = 1 ...
	//
	// ndc_z = clip_z / clip_w
	//       = (P22 * view_z + P32) / (P23 * view_z)
	//       = (P22 + P32/view_z) / P23
	// view_z = P32 / ((ndc_z * P23) - P22)
	//
	// ndc_x = clip_x / clip_w
	//       = (P00 * view_x + P20 * view_z) / (P23 * view_z)
	//       = (P00 * view_x/view_z + P20) / P23
	// view_x = (((ndc_x * P23) - P20) / P00) * view_z
	//
	// ndc_y = clip_y / clip_w
	//       = (P11 * view_y + P21 * view_z) / (P23 * view_z)
	//       = (P11 * view_y/view_z + P21) / P23
	// view_y = (((ndc_y * P23) - P21) / P11) * view_z
	//
	float view_z = projection[3][2] / ((projection[2][3] * screen_coord.z) - projection[2][2]);

	float view_x = (((screen_coord.x * projection[2][3]) - projection[2][0]) / projection[0][0]) * view_z;
	float view_y = (((screen_coord.y * projection[2][3]) - projection[2][1]) / projection[1][1]) * view_z;

	return vec4(view_x, view_y, view_z, 1);
}

// Transformation of 3D screen-space coordinates (includes depth) to *view* coordinates.
// These screen-space coordinates are post-projection coordinates in the range [-1,1] for x, y and z components.
//
// This gives the same result as 'view_from_screen()' but uses the *inverse* of projection matrix (not projection matrix).
//
// Handles standard perspective and orthographic projections.
vec4
screen_to_view(
		const vec3 screen_coord,
		const mat4 projection_inverse)
{
	vec4 screen_position = vec4(screen_coord, 1.0);
	vec4 view_position = projection_inverse * screen_position;
	view_position /= view_position.w;

	return view_position;
}

// Transformation of 3D screen-space coordinates (includes depth) to *world* coordinates.
// These screen-space coordinates are post-projection coordinates in the range [-1,1] for x, y and z components.
//
// Handles standard perspective and orthographic projections.
//
// NOTE: We avoid using the combined model-view-projection inverse matrix (model-view inverse multiplied by projection inverse)
// since this can be less accurate in certain situations (eg, ray direction derived using this function can be slightly
// inaccurate when view is fully zoomed in and fully tilted in perspective viewing mode).
// This might be due to using single-precision floating-point in GPU (instead of the double precision used on CPU).
// So we get better accuracy at the expense of an extra matrix-vector multiply.
vec4
screen_to_world(
		const vec3 screen_coord,
		const mat4 model_view_inverse,
		const mat4 projection_inverse)
{
	return model_view_inverse * screen_to_view(screen_coord, projection_inverse);
}

// Ray initialization from screen-space coordinate.
//
// Create the ray starting on the near plane and moving towards the far plane.
//
// Normally the ray origin would be the eye/camera position, but for orthographic viewing the real eye position is at
// infinity and generates parallel rays for all pixels. Note that we also support perspective viewing, so it would be
// nice to use the same code for both orthographic and perspective viewing. The viewing transform is encoded in the
// model-view-projection transform so using that supports both orthographic and perspective viewing.
//
// The inverse model-view-projection transform is used to convert the current screen coordinate (x,y,-1) to
// world space for the ray origin and (x,y,1) to world space to calculate (normalised) ray direction.
// The screen-space depth of -1 is on the near clip plane of the view frustum [-1,1] (note: this is not eye location in perspective mode).
// The screen-space depth of 1 is on the far clip plane of the view frustum [-1,1].
Ray
get_ray(
		const vec2 screen_coord,
		const mat4 model_view_inverse,
		const mat4 projection_inverse)
{
	vec3 ray_origin = screen_to_world(vec3(screen_coord, -1.0), model_view_inverse, projection_inverse).xyz;

	return Ray(
		ray_origin,
		normalize(screen_to_world(vec3(screen_coord, 1.0), model_view_inverse, projection_inverse).xyz - ray_origin));
}

// Convert screen-space depth (in range [-1,1]) to ray distance/lambda.
float
convert_screen_space_depth_to_ray_lambda(
		const float screen_space_depth,
		const vec2 screen_coord,
		const mat4 model_view_inverse,
		const mat4 projection_inverse,
		const vec3 ray_origin)
{
	vec3 world_position = screen_to_world(vec3(screen_coord, screen_space_depth), model_view_inverse, projection_inverse).xyz;
	return length(world_position - ray_origin);
}
