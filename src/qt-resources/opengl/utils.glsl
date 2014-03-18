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
 * The first overload of 'bilinearly_interpolate' returns the interpolated texture result
 * while the second overload returns the four sampled texels and the interpolation coefficients.
 *
 * 'tex_dimensions' should contain the following (xyzw) components:
 *	x: texture width,
 *	y: texture height,
 *	z: inverse texture width,
 *	w: inverse texture height.
 *
 * This is useful for floating-point textures because bilinear filtering is not supported
 * in earlier hardware.
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

	// The first texture access starts a new indirection phase since it accesses a temporary
	// written in the current phase (see issue 24 in GL_ARB_fragment_program spec).
	tex11 = texture2D(tex_sampler, st.xy);
	tex21 = texture2D(tex_sampler, st.zy);
	tex12 = texture2D(tex_sampler, st.xw);
	tex22 = texture2D(tex_sampler, st.zw);
}

vec4
bilinearly_interpolate(
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

	// Call the other overload of 'bilinearly_interpolate()'.
	bilinearly_interpolate(
		tex_sampler, tex_coords, tex_dimensions, tex11, tex21, tex12, tex22, interp);

	// Bilinearly interpolate the four texels.
	return mix(mix(tex11, tex21, interp.x), mix(tex12, tex22, interp.x), interp.y);
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
	//		 = dot(N/|N|,L/|L|) 
	//		 = dot(N,L) / (|N| * |L|) 
	//		 = dot(N,L) / sqrt(dot(N,N) * dot(L,L)) 
	// NOTE: Using float instead of integer parameters to 'max' otherwise driver compiler
	// crashes on some systems complaining cannot find (integer overload of) function in 'stdlib'.
	return max(0.0,
		dot(normal, light_direction) * 
			inversesqrt(dot(normal, normal) * dot(light_direction, light_direction)));
}

/*
 * Mixes ambient lighting with diffuse lighting.
 *
 * ambient_with_diffuse_lighting = light_ambient_contribution + (1 - light_ambient_contribution) * diffuse_lighting
 *
 * 'light_ambient_contribution' is in the range [0,1].
 */
float
mix_ambient_with_diffuse_lighting(
		float diffuse_lighting,
		float light_ambient_contribution)
{
	// Blend between ambient and diffuse lighting - when ambient is 1.0 there is no diffuse.
	// NOTE: Using float instead of integer parameters to 'mix' otherwise driver compiler
	// crashes on some systems complaining cannot find (integer overload of) function in 'stdlib'.
	return mix(diffuse_lighting, 1.0, light_ambient_contribution);
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
