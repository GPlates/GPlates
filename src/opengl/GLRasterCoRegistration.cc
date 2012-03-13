/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#include <cmath>
#include <limits>
#include <boost/cstdint.hpp>
#include <boost/foreach.hpp>
#include <boost/utility/in_place_factory.hpp>
/*
 * The OpenGL Extension Wrangler Library (GLEW).
 * Must be included before the OpenGL headers (which also means before Qt headers).
 * For this reason it's best to try and include it in ".cc" files only.
 */
#include <GL/glew.h>
#include <opengl/OpenGL.h>
#include <QImage>
#include <QString>

#include "GLRasterCoRegistration.h"

#include "GLBuffer.h"
#include "GLContext.h"
#include "GLCubeSubdivision.h"
#include "GLDataRasterSource.h"
#include "GLRenderer.h"
#include "GLShaderProgramUtils.h"
#include "GLStreamPrimitives.h"

#include "app-logic/ReconstructedFeatureGeometry.h"

#include "gui/Colour.h"

#include "maths/CubeCoordinateFrame.h"
#include "maths/types.h"

#include "utils/Base2Utils.h"
#include "utils/Profile.h"


namespace GPlatesOpenGL
{
	namespace
	{
		/**
		 * Fragment shader source to render region-of-interest geometries.
		 *
		 * 'ENABLE_SEED_FRUSTUM_CLIPPING' is used for clipping to seed frustums.
		 * It is used when the seed frustum is smaller than the view frustum otherwise
		 * the regular primitive clipping of the GPU (in NDC space) is all that's needed.
		 */
		const char *RENDER_REGION_OF_INTEREST_GEOMETRIES_FRAGMENT_SHADER_SOURCE =
				"#ifdef POINT_REGION_OF_INTEREST\n"
				"#ifdef SMALL_ROI_ANGLE\n"
				"	uniform float tan_squared_region_of_interest_angle;\n"
				"#endif\n"
				"#ifdef LARGE_ROI_ANGLE\n"
				"	uniform float cos_region_of_interest_angle;\n"
				"#endif\n"
				"#endif\n"

				"#ifdef LINE_REGION_OF_INTEREST\n"
				"#ifdef SMALL_ROI_ANGLE\n"
				"	uniform float sin_region_of_interest_angle;\n"
				"#endif\n"
				"#ifdef LARGE_ROI_ANGLE\n"
				"	uniform float tan_squared_region_of_interest_complementary_angle;\n"
				"#endif\n"
				"#endif\n"

				"#if defined(POINT_REGION_OF_INTEREST) || defined(LINE_REGION_OF_INTEREST)\n"
				"varying vec3 present_day_position;\n"
				"#endif\n"

				"#ifdef POINT_REGION_OF_INTEREST\n"
				"	varying vec3 present_day_point_centre;\n"
				"#endif\n"

				"#ifdef LINE_REGION_OF_INTEREST\n"
				"	varying vec3 present_day_line_arc_normal;\n"
				"#endif\n"

				"#ifdef ENABLE_SEED_FRUSTUM_CLIPPING\n"
				"	varying vec4 clip_position_params;\n"
				"#endif\n"

				"void main (void)\n"
				"{\n"

				"#ifdef POINT_REGION_OF_INTEREST\n"
				"	// Discard current pixel if outside region-of-interest radius about point centre.\n"
				"#ifdef SMALL_ROI_ANGLE\n"
				"	// Since acos (region-of-interest angle) is very inaccurate for very small angles we instead use:\n"
				"	//   tan(angle) = sin(angle) / cos(angle) = |cross(x1,x2)| / dot(x1,x2)\n"
				"	// 'present_day_point_centre' is constant (and unit length) across the primitive but\n"
				"	// 'present_day_position' varies and is not unit length (so must be normalised).\n"
				"	vec3 present_day_position_normalised = normalize(present_day_position);\n"
				"	vec3 cross_position_and_point_centre = cross(present_day_position_normalised, present_day_point_centre);\n"
				"	float sin_squared_angle = dot(cross_position_and_point_centre, cross_position_and_point_centre);\n"
				"	float cos_angle = dot(present_day_position_normalised, present_day_point_centre);\n"
				"	float cos_squared_angle = cos_angle * cos_angle;\n"
				"	if (sin_squared_angle > cos_squared_angle * tan_squared_region_of_interest_angle)\n"
				"		discard;\n"
				"#endif\n"
				"#ifdef LARGE_ROI_ANGLE\n"
				"	// However acos (region-of-interest angle) is fine for larger angles.\n"
				"	// Also the 'tan' (used for small angles) is not valid at 90 degrees.\n"
				"	// 'present_day_point_centre' is constant (and unit length) across the primitive but\n"
				"	// 'present_day_position' varies and is not unit length (so must be normalised).\n"
				"	if (dot(normalize(present_day_position), present_day_point_centre) < cos_region_of_interest_angle)\n"
				"		discard;\n"
				"#endif\n"
				"#endif\n"

				"#ifdef LINE_REGION_OF_INTEREST\n"
				"	// Discard current pixel if outside region-of-interest of line (great circle arc).\n"
				"#ifdef SMALL_ROI_ANGLE\n"
				"	// For very small region-of-interest angles sin(angle) is fine.\n"
				"	// 'present_day_line_arc_normal' is constant (and unit length) across the primitive but\n"
				"	// 'present_day_position' varies and is not unit length (so must be normalised).\n"
				"	if (abs(dot(normalize(present_day_position), present_day_line_arc_normal)) > sin_region_of_interest_angle)\n"
				"		discard;\n"
				"#endif\n"
				"#ifdef LARGE_ROI_ANGLE\n"
				"	// Since asin (region-of-interest angle) is very inaccurate for angles near 90 degrees we instead use:\n"
				"	//   tan(90-angle) = sin(90-angle) / cos(90-angle) = |cross(x,N)| / dot(x,N)\n"
				"	// where 'N' is the arc normal and 'x' is the position vector.\n"
				"	// 'present_day_point_centre' is constant (and unit length) across the primitive but\n"
				"	// 'present_day_position' varies and is not unit length (so must be normalised).\n"
				"	vec3 present_day_position_normalised = normalize(present_day_position);\n"
				"	vec3 cross_position_and_arc_normal = cross(present_day_position_normalised, present_day_line_arc_normal);\n"
				"	float sin_squared_complementary_angle = dot(cross_position_and_arc_normal, cross_position_and_arc_normal);\n"
				"	float cos_complementary_angle = dot(present_day_position_normalised, present_day_line_arc_normal);\n"
				"	float cos_squared_complementary_angle = cos_complementary_angle * cos_complementary_angle;\n"
				"	if (sin_squared_complementary_angle < \n"
				"			cos_squared_complementary_angle * tan_squared_region_of_interest_complementary_angle)\n"
				"		discard;\n"
				"#endif\n"
				"#endif\n"

				"#ifdef FILL_REGION_OF_INTEREST\n"
				"	// Nothing required for *fill* regions-of-interest - they are just normal geometry.\n"
				"#endif\n"

				"#ifdef ENABLE_SEED_FRUSTUM_CLIPPING\n"
				"	// Discard current pixel if outside the seed frustum side planes.\n"
				"	// Inside clip frustum means -1 < x/w < 1 and -1 < y/w < 1 which is same as\n"
				"	// -w < x < w and -w < y < w.\n"
				"	// 'clip_position_params' is (x, y, w, -w).\n"
				"	if (!all(lessThan(clip_position_params.wxwy, clip_position_params.xzyz)))\n"
				"		discard;\n"
				"#endif\n"

				"	// Output all channels as 1.0 to indicate inside region-of-interest.\n"
				"	// TODO: Output grayscale to account for partial pixel coverage or\n"
				"	// smoothing near boundary of region-of-interest (will require max blending).\n"
				"	gl_FragColor = vec4(1.0);\n"

				"}\n";

		//! Vertex shader source to render region-of-interest geometries.
		const char *RENDER_REGION_OF_INTEREST_GEOMETRIES_VERTEX_SHADER_SOURCE =
				"#ifdef POINT_REGION_OF_INTEREST\n"
				"	const vec3 north_pole = vec3(0, 0, 1);\n"
				"#endif\n"

				"attribute vec4 world_space_quaternion;\n"

				"#ifdef POINT_REGION_OF_INTEREST\n"
				"	attribute vec3 point_centre;\n"
				"	// The 'xyz' values are (weight_tangent_x, weight_tangent_y, weight_point_centre)\n"
				"	attribute vec3 tangent_frame_weights;\n"
				"#endif\n"

				"#ifdef LINE_REGION_OF_INTEREST\n"
				"	attribute vec3 line_arc_start_point;\n"
				"	attribute vec3 line_arc_normal;\n"
				"	// The 'xy' values are (weight_arc_normal, weight_start_point)\n"
				"	attribute vec2 tangent_frame_weights;\n"
				"#endif\n"

				"#ifdef FILL_REGION_OF_INTEREST\n"
				"	attribute vec3 fill_position;\n"
				"#endif\n"

				"#ifdef ENABLE_SEED_FRUSTUM_CLIPPING\n"
				"	// The 'xyz' values are (translate_x, translate_y, scale)\n"
				"	attribute vec3 raster_frustum_to_seed_frustum_clip_space_transform;\n"
				"	// The 'xyz' values are (translate_x, translate_y, scale)\n"
				"	attribute vec3 seed_frustum_to_render_target_clip_space_transform;\n"
				"#endif\n"

				"#if defined(POINT_REGION_OF_INTEREST) || defined(LINE_REGION_OF_INTEREST)\n"
				"varying vec3 present_day_position;\n"
				"#endif\n"

				"#ifdef POINT_REGION_OF_INTEREST\n"
				"	varying vec3 present_day_point_centre;\n"
				"#endif\n"

				"#ifdef LINE_REGION_OF_INTEREST\n"
				"	varying vec3 present_day_line_arc_normal;\n"
				"#endif\n"

				"#ifdef ENABLE_SEED_FRUSTUM_CLIPPING\n"
				"	varying vec4 clip_position_params;\n"
				"#endif\n"

				"void main (void)\n"
				"{\n"

				"#ifdef POINT_REGION_OF_INTEREST\n"
				"	// Pass present day point centre to the fragment shader.\n"
				"	present_day_point_centre = point_centre;\n"

				"	// Generate the tangent space frame around the point centre.\n"
				"	// Since the point is symmetric it doesn't matter which tangent frame we choose\n"
				"	// as long as it's orthonormal.\n"
				"	vec3 present_day_tangent_x = normalize(cross(north_pole, point_centre));\n"
				"	vec3 present_day_tangent_y = cross(point_centre, present_day_tangent_x);\n"

				"	// The weights are what actually determine which vertex of the quad primitive this vertex is.\n"
				"	// Eg, centre point has weights (0,0,1).\n"
				"	present_day_position =\n"
				"		tangent_frame_weights.x * present_day_tangent_x +\n"
				"		tangent_frame_weights.y * present_day_tangent_y +\n"
				"		tangent_frame_weights.z * present_day_point_centre;\n"

				"   // Transform present-day vertex position using finite rotation quaternion.\n"
				"	// It's ok that the position is not on the unit sphere (it'll still get rotated properly).\n"
				"	vec3 rotated_position = rotate_vector_by_quaternion(world_space_quaternion, present_day_position);\n"
				"#endif\n"

				"#ifdef LINE_REGION_OF_INTEREST\n"
				"	// Pass the present-day line arc normal to the fragment shader.\n"
				"	present_day_line_arc_normal = line_arc_normal;\n"

				"	// The weights (and order of start/end points) are what actually determine which \n"
				"	// vertex of the quad primitive this vertex is. Eg, centre point has weights (0,0,1).\n"
				"	present_day_position =\n"
				"		tangent_frame_weights.x * line_arc_normal +\n"
				"		tangent_frame_weights.y * line_arc_start_point;\n"

				"   // Transform present-day start point using finite rotation quaternion.\n"
				"	vec3 rotated_position = rotate_vector_by_quaternion(world_space_quaternion, present_day_position);\n"
				"#endif\n"

				"#ifdef FILL_REGION_OF_INTEREST\n"
				"   // Transform present-day position using finite rotation quaternion.\n"
				"	vec3 rotated_position = rotate_vector_by_quaternion(world_space_quaternion, fill_position);\n"
				"#endif\n"

				"	// Transform rotated position by the view/projection matrix.\n"
				"	// The view/projection matches the target raster tile.\n"
				"	vec4 raster_frustum_position = gl_ModelViewProjectionMatrix * vec4(rotated_position, 1);\n"

				"#ifdef ENABLE_SEED_FRUSTUM_CLIPPING\n"
				"	// Post-projection translate/scale to position NDC space around seed frustum...\n"
				"	vec4 loose_seed_frustum_position = vec4(\n"
				"		// Scale and translate x component...\n"
				"		dot(raster_frustum_to_seed_frustum_clip_space_transform.zx,\n"
				"				raster_frustum_position.xw),\n"
				"		// Scale and translate y component...\n"
				"		dot(raster_frustum_to_seed_frustum_clip_space_transform.zy,\n"
				"				raster_frustum_position.yw),\n"
				"		// z and w components unaffected...\n"
				"		raster_frustum_position.zw);\n"

				"	// This is also the clip-space the fragment shader uses to cull pixels outside\n"
				"	// the seed frustum - seed geometry should be bounded by frustum but just in case.\n"
				"	// Convert to a more convenient form for the fragment shader:\n"
				"	//   1) Only interested in clip position x, y, w and -w.\n"
				"	//   2) The z component is depth and we only need to clip to side planes not near/far plane.\n"
				"	clip_position_params = vec4(\n"
				"		loose_seed_frustum_position.xy,\n"
				"		loose_seed_frustum_position.w,\n"
				"		-loose_seed_frustum_position.w);\n"

				"	// Post-projection translate/scale to position NDC space around render target frustum...\n"
				"	vec4 render_target_frustum_position = vec4(\n"
				"		// Scale and translate x component...\n"
				"		dot(seed_frustum_to_render_target_clip_space_transform.zx,\n"
				"				loose_seed_frustum_position.xw),\n"
				"		// Scale and translate y component...\n"
				"		dot(seed_frustum_to_render_target_clip_space_transform.zy,\n"
				"				loose_seed_frustum_position.yw),\n"
				"		// z and w components unaffected...\n"
				"		loose_seed_frustum_position.zw);\n"

				"	gl_Position = render_target_frustum_position;\n"
				"#else\n"

				"	// When the seed frustum matches the target raster tile there is no need\n"
				"	// for seed frustum clipping (happens automatically due to view frustum).\n"
				"	// In this case both the raster frustum to seed frustum and seed frustum to\n"
				"	// render target frustum are identity transforms and are not needed.\n"
				"	// .\n"
				"	gl_Position = raster_frustum_position;\n"
				"#endif\n"

				"}\n";

		/**
		 * Fragment shader source to extract target raster in region-of-interest in preparation
		 * for reduction operations.
		 */
		const char *MASK_REGION_OF_INTEREST_FRAGMENT_SHADER_SOURCE =
				"uniform sampler2D target_raster_texture_sampler;\n"
				"uniform sampler2D region_of_interest_mask_texture_sampler;\n"

				"#ifdef FILTER_MOMENTS\n"
				"	uniform vec3 cube_face_centre;\n"
				"	varying vec4 view_position;\n"
				"#endif\n"

				"void main (void)\n"
				"{\n"

				"	float region_of_interest_mask =\n"
				"			texture2D(region_of_interest_mask_texture_sampler, gl_TexCoord[1].st).a;\n"
				"	if (region_of_interest_mask == 0)\n"
				"		discard;\n"

				"	// NOTE: There's no need to bilinear filter since the projection frustums should be\n"
				"	// such that we're sampling at texel centres.\n"
				"	vec4 target_raster = texture2D(target_raster_texture_sampler, gl_TexCoord[0].st);\n"
				"	// The red channel contains the raster data value and the green channel contains the coverage.\n"
				"	float data = target_raster.r;\n"
				"	float coverage = target_raster.g;\n"

				"	// Due to bilinear filtering of the source raster (data and coverage) the data\n"
				"	// value can be reduced depending on the bi-linearly filtered coverage value.\n"
				"	// So we need to undo that effect as best we can - this is important for MIN/MAX\n"
				"	// operations and also ensures MEAN correlates with MIN/MAX - ie, a single pixel\n"
				"	// ROI should give same value for MIN/MAX and MEAN.\n"
				"	// This typically occurs near a boundary between opaque and transparent regions.\n"
				"	if (coverage > 0)\n"
				"		data /= coverage;\n"

				"	// The coverage is modulated by the region-of-interest mask.\n"
				"	// Currently the ROI mask is either zero or one so this doesn't do anything\n"
				"	// (because of the above discard) but will if smoothing near ROI boundary is added.\n"
				"	coverage *= region_of_interest_mask;\n"

				"#ifdef FILTER_MOMENTS\n"
				"	// Adjust the coverage based on the area of the current pixel.\n"
				"	// The adjustment will be 1.0 at the cube face centre less than 1.0 elsewhere.\n"
				"	// NOTE: 'view_position' only needs to be a vec3 and not a vec4 because we do not\n"
				"	// need to do the projective divide by w because we are normalising anyway.\n"
				"	// We normalize to project the view position onto the surface of the globe.\n"
				"	// NOTE: We only need to do this adjustment for area-weighted operations.\n"
				"	coverage *= dot(cube_face_centre, normalize(view_position.xyz));\n"

				"	// Output (r, g, a) channels as (C*D, C*D*D, C).\n"
				"	// Where C is coverage and D is data value.\n"
				"	// This is enough to cover both mean and standard deviation.\n"
				"	gl_FragColor = vec4(coverage * data, coverage * data * data, 0, coverage);\n"
				"#endif\n"

				"#ifdef FILTER_MIN_MAX\n"
				"	// Output (r, a) channels as (D, C).\n"
				"	// Where C is coverage and D is data value.\n"
				"	// This is enough to cover both minimum and maximum.\n"
				"	gl_FragColor = vec4(data, 0, 0, coverage);\n"
				"#endif\n"

				"}\n";

		/**
		 * Vertex shader source to extract target raster in region-of-interest in preparation
		 * for reduction operations.
		 */
		const char *MASK_REGION_OF_INTEREST_VERTEX_SHADER_SOURCE =
				"// The 'xy' values are (translate, scale)\n"
				"const vec2 clip_space_to_texture_space_transform = vec2(0.5, 0.5);\n"

				"attribute vec4 screen_space_position;\n"
				"// The 'xyz' values are (translate_x, translate_y, scale)\n"
				"attribute vec3 raster_frustum_to_seed_frustum_clip_space_transform;\n"
				"// The 'xyz' values are (translate_x, translate_y, scale)\n"
				"attribute vec3 seed_frustum_to_render_target_clip_space_transform;\n"

				"#ifdef FILTER_MOMENTS\n"
				"	varying vec4 view_position;\n"
				"#endif\n"

				"void main (void)\n"
				"{\n"

				"	// Post-projection translate/scale to position NDC space around raster frustum.\n"
				"	// NOTE: We actually need to take the 'inverse' transform since we want to go from\n"
				"	// seed frustum to raster frustum rather than the opposite direction.\n"
				"	// See 'GPlatesUtils::QuadTreeClipSpaceTransform::get_inverse_translate_x()'\n"
				"	// for details of the inverse transform.\n"
				"	vec3 loose_seed_frustum_to_raster_frustum_clip_space_transform = vec3(\n"
				"			-raster_frustum_to_seed_frustum_clip_space_transform.x / \n"
				"				raster_frustum_to_seed_frustum_clip_space_transform.z,\n"
				"			-raster_frustum_to_seed_frustum_clip_space_transform.y / \n"
				"				raster_frustum_to_seed_frustum_clip_space_transform.z,\n"
				"			1.0 / raster_frustum_to_seed_frustum_clip_space_transform.z);\n"
				"	// This takes the 'screen_space_position' range [-1,1] and makes it cover the \n"
				"	// raster frustum (so [-1,1] covers the raster frustum).\n"
				"	vec4 raster_frustum_position = vec4(\n"
				"		// Scale and translate x component...\n"
				"		dot(loose_seed_frustum_to_raster_frustum_clip_space_transform.zx,\n"
				"				screen_space_position.xw),\n"
				"		// Scale and translate y component...\n"
				"		dot(loose_seed_frustum_to_raster_frustum_clip_space_transform.zy,\n"
				"				screen_space_position.yw),\n"
				"		// z and w components unaffected...\n"
				"		screen_space_position.zw);\n"

				"#ifdef FILTER_MOMENTS\n"
				"	// Convert from the screen-space of the raster frustum to view-space using\n"
				"	// the inverse view-projection *inverse* matrix.\n"
				"	// The view position is used in the fragment shader to adjust for cube map distortion.\n"
				"	view_position = gl_ModelViewProjectionMatrixInverse * raster_frustum_position;\n"
				"#endif\n"

				"	// Post-projection translate/scale to position NDC space around render target frustum.\n"
				"	// This takes the 'screen_space_position' range [-1,1] and makes it cover the \n"
				"	// render target frustum (so [-1,1] covers the render target frustum).\n"
				"	vec4 render_target_frustum_position = vec4(\n"
				"		// Scale and translate x component...\n"
				"		dot(seed_frustum_to_render_target_clip_space_transform.zx,\n"
				"				screen_space_position.xw),\n"
				"		// Scale and translate y component...\n"
				"		dot(seed_frustum_to_render_target_clip_space_transform.zy,\n"
				"				screen_space_position.yw),\n"
				"		// z and w components unaffected...\n"
				"		screen_space_position.zw);\n"

				"	// The target raster texture coordinates.\n"
				"	// Convert clip-space range [-1,1] to texture coordinate range [0,1].\n"
				"	gl_TexCoord[0] = vec4(\n"
				"		// Scale and translate s component...\n"
				"		dot(clip_space_to_texture_space_transform.yx,\n"
				"				raster_frustum_position.xw),\n"
				"		// Scale and translate t component...\n"
				"		dot(clip_space_to_texture_space_transform.yx,\n"
				"				raster_frustum_position.yw),\n"
				"		// p and q components unaffected...\n"
				"		raster_frustum_position.zw);\n"

				"	// The region-of-interest mask texture coordinates.\n"
				"	// Convert clip-space range [-1,1] to texture coordinate range [0,1].\n"
				"	gl_TexCoord[1] = vec4(\n"
				"		// Scale and translate s component...\n"
				"		dot(clip_space_to_texture_space_transform.yx,\n"
				"				render_target_frustum_position.xw),\n"
				"		// Scale and translate t component...\n"
				"		dot(clip_space_to_texture_space_transform.yx,\n"
				"				render_target_frustum_position.yw),\n"
				"		// p and q components unaffected...\n"
				"		render_target_frustum_position.zw);\n"

				"	gl_Position = render_target_frustum_position;\n"

				"}\n";

		/**
		 * Fragment shader source to reduce region-of-interest filter results.
		 */
		const char *REDUCTION_OF_REGION_OF_INTEREST_FRAGMENT_SHADER_SOURCE =
				"uniform sampler2D reduce_source_texture_sampler;\n"
				"// 'x' component is half texel offset and 'y' component is negative of that.\n"
				"uniform vec2 reduce_source_texture_half_texel_offset;\n"

				"void main (void)\n"
				"{\n"

				"	// Get the texture coordinates of the four source texels.\n"
				"	// Since it's a 2x2 -> 1x1 reduction the texture coordinate of the current pixel\n"
				"	// will be equidistant from four source texels (in each texel's corner).\n"
				"	vec2 st = gl_TexCoord[0].st;\n"
				"	vec2 st00 = st + reduce_source_texture_half_texel_offset.yy;\n"
				"	vec2 st01 = st + reduce_source_texture_half_texel_offset.yx;\n"
				"	vec2 st10 = st + reduce_source_texture_half_texel_offset.xy;\n"
				"	vec2 st11 = st + reduce_source_texture_half_texel_offset.xx;\n"

				"	vec4 src[4];\n"
				"	// Sample the four source texels.\n"
				"	src[0] = texture2D(reduce_source_texture_sampler, st00);\n"
				"	src[1] = texture2D(reduce_source_texture_sampler, st01);\n"
				"	src[2] = texture2D(reduce_source_texture_sampler, st10);\n"
				"	src[3] = texture2D(reduce_source_texture_sampler, st11);\n"

				"#ifdef REDUCTION_SUM\n"
				"	vec4 sum = vec4(0);\n"

				"	// Apply the reduction operation on the four source texels.\n"
				"	for (int n = 0; n < 4; ++n)\n"
				"	{\n"
				"		sum += src[n];\n"
				"	}\n"

				"	gl_FragColor = sum;\n"
				"#endif\n"

				"#ifdef REDUCTION_MIN\n"
				"	// First find the maximum value and coverage.\n"
				"	vec4 max_value = max(max(src[0], src[1]), max(src[2], src[3]));\n"

				"	// If the coverage values are all zero then discard this fragment.\n"
				"	// The framebuffer already has zero values meaning zero coverage.\n"
				"	float max_coverage = max_value.a;\n"
				"	if (max_coverage == 0)\n"
				"		discard;\n"

				"	// Apply the reduction operation on the four source texels.\n"
				"	vec3 min_covered_value = max_value.rgb;\n"
				"	for (int n = 0; n < 4; ++n)\n"
				"	{\n"
				"		// If the coverage is non-zero then find new minimum value, otherwise ignore.\n"
				"		if (src[n].a > 0)\n"
				"			min_covered_value = min(min_covered_value, src[n].rgb);\n"
				"	}\n"

				"	gl_FragColor = vec4(min_covered_value, max_coverage);\n"
				"#endif\n"

				"#ifdef REDUCTION_MAX\n"
				"	// First find the maximum coverage.\n"
				"	float max_coverage = max(max(src[0].a, src[1].a), max(src[2].a, src[3].a));\n"

				"	// If the coverage values are all zero then discard this fragment.\n"
				"	// The framebuffer already has zero values meaning zero coverage.\n"
				"	if (max_coverage == 0)\n"
				"		discard;\n"

				"	// First find the minimum value.\n"
				"	vec3 min_value = min(min(src[0].rgb, src[1].rgb), min(src[2].rgb, src[3].rgb));\n"

				"	// Apply the reduction operation on the four source texels.\n"
				"	vec3 max_covered_value = min_value;\n"
				"	for (int n = 0; n < 4; ++n)\n"
				"	{\n"
				"		// If the coverage is non-zero then find new maximum value, otherwise ignore.\n"
				"		if (src[n].a > 0)\n"
				"			max_covered_value = max(max_covered_value, src[n].rgb);\n"
				"	}\n"

				"	gl_FragColor = vec4(max_covered_value, max_coverage);\n"
				"#endif\n"

				"}\n";

		/**
		 * Vertex shader source to reduce region-of-interest filter results.
		 */
		const char *REDUCTION_OF_REGION_OF_INTEREST_VERTEX_SHADER_SOURCE =
				"// (x,y,z) is (translate_x, translate_y, scale).\n"
				"uniform vec3 target_quadrant_translate_scale;\n"

				"void main (void)\n"
				"{\n"

				"	gl_TexCoord[0] = gl_MultiTexCoord0;\n"

				"	// Scale and translate the pixel coordinates to the appropriate quadrant\n"
				"	// of the destination render target.\n"
				"	gl_Position.x = dot(target_quadrant_translate_scale.zx, gl_Vertex.xw);\n"
				"	gl_Position.y = dot(target_quadrant_translate_scale.zy, gl_Vertex.yw);\n"
				"	gl_Position.zw = gl_Vertex.zw;\n"

				"}\n";
	}
}


bool
GPlatesOpenGL::GLRasterCoRegistration::is_supported(
		GLRenderer &renderer)
{
	const GLContext::Parameters &context_parameters = GLContext::get_parameters();

	// Note that we don't specifically request GL_ARB_vertex_buffer_object and GL_ARB_pixel_buffer_object
	// because we have fall back paths for vertex and pixel buffers (using client memory instead of buffers)
	// in case those extensions are not supported on the run-time system. The fall back path is handled
	// by the interface classes GLVertexBuffer, GLVertexElementBuffer and GLPixelBuffer.
	//
	// In any case the most stringent requirement will likely be GL_ARB_texture_float.
	const bool supported =
			// Need floating-point textures...
			context_parameters.texture.gl_ARB_texture_float &&
			GLDataRasterSource::is_supported(renderer) &&
			// Max texture dimension supported should be large enough...
			context_parameters.texture.gl_max_texture_size >= TEXTURE_DIMENSION &&
			// Need vertex/fragment shader programs...
			context_parameters.shader.gl_ARB_shader_objects &&
			context_parameters.shader.gl_ARB_vertex_shader &&
			context_parameters.shader.gl_ARB_fragment_shader &&
			// Need framebuffer objects...
			context_parameters.framebuffer.gl_EXT_framebuffer_object;

	if (!supported)
	{
		// Only emit warning message once.
		static bool emitted_warning = false;
		if (!emitted_warning)
		{
			// It's most likely the graphics hardware doesn't support floating-point textures.
			// Most hardware that supports it also supports the other OpenGL extensions also.
			qWarning() <<
					"Raster co-registration NOT supported by this OpenGL system - requires floating-point texture support.\n"
					"  Your graphics hardware is most likely missing the 'GL_ARB_texture_float' OpenGL extension.";
			emitted_warning = true;
		}

		return false;
	}

	// Supported.
	return true;
}


GPlatesOpenGL::GLRasterCoRegistration::GLRasterCoRegistration(
		GLRenderer &renderer) :
	d_framebuffer_object(renderer.get_context().get_non_shared_state()->acquire_frame_buffer_object(renderer)),
	d_streaming_vertex_element_buffer(GLVertexElementBuffer::create(renderer, GLBuffer::create(renderer))),
	d_streaming_vertex_buffer(GLVertexBuffer::create(renderer, GLBuffer::create(renderer))),
	d_point_region_of_interest_vertex_array(GLVertexArray::create(renderer)),
	d_line_region_of_interest_vertex_array(GLVertexArray::create(renderer)),
	d_fill_region_of_interest_vertex_array(GLVertexArray::create(renderer)),
	d_mask_region_of_interest_vertex_array(GLVertexArray::create(renderer)),
	d_reduction_vertex_array(GLVertexArray::create(renderer)),
	d_identity_quaternion(GPlatesMaths::UnitQuaternion3D::create_identity_rotation())
{
	// Raster co-registration queries must be supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			is_supported(renderer),
			GPLATES_ASSERTION_SOURCE);

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			GPlatesUtils::Base2::is_power_of_two(TEXTURE_DIMENSION),
			GPLATES_ASSERTION_SOURCE);
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			GPlatesUtils::Base2::is_power_of_two(MINIMUM_SEED_GEOMETRIES_VIEWPORT_DIMENSION) &&
				// All seed geometries get reduced (to one pixel) so minimum viewport should be larger than one pixel...
				MINIMUM_SEED_GEOMETRIES_VIEWPORT_DIMENSION > 1 &&
				// Want multiple seed geometry minimum-size viewports to fit inside a texture...
				TEXTURE_DIMENSION > MINIMUM_SEED_GEOMETRIES_VIEWPORT_DIMENSION,
			GPLATES_ASSERTION_SOURCE);

	// A pixel buffer object for debugging render targets.
#if defined(DEBUG_RASTER_COREGISTRATION_RENDER_TARGET)
	GLBuffer::shared_ptr_type debug_pixel_buffer = GLBuffer::create(renderer);
	debug_pixel_buffer->gl_buffer_data(
			renderer,
			GLBuffer::TARGET_PIXEL_PACK_BUFFER,
			TEXTURE_DIMENSION * TEXTURE_DIMENSION * 4 * sizeof(GLfloat),
			NULL, // Uninitialised memory.
			GLBuffer::USAGE_STREAM_READ);
	d_debug_pixel_buffer = GLPixelBuffer::create(renderer, debug_pixel_buffer);
#endif

	// Initialise vertex arrays and shader programs to perform the various rendering tasks.
	initialise_vertex_arrays_and_shader_programs(renderer);
}


void
GPlatesOpenGL::GLRasterCoRegistration::initialise_vertex_arrays_and_shader_programs(
		GLRenderer &renderer)
{
	//
	// Allocate memory for the streaming vertex buffer.
	//

	// Allocate the buffer data in the seed geometries vertex element buffer.
	d_streaming_vertex_element_buffer->get_buffer()->gl_buffer_data(
			renderer,
			GLBuffer::TARGET_ELEMENT_ARRAY_BUFFER,
			NUM_BYTES_IN_STREAMING_VERTEX_ELEMENT_BUFFER,
			NULL,
			GLBuffer::USAGE_STREAM_DRAW);

	// Allocate the buffer data in the seed geometries vertex buffer.
	d_streaming_vertex_buffer->get_buffer()->gl_buffer_data(
			renderer,
			GLBuffer::TARGET_ARRAY_BUFFER,
			NUM_BYTES_IN_STREAMING_VERTEX_BUFFER,
			NULL,
			GLBuffer::USAGE_STREAM_DRAW);

	//
	// Create the shader programs (and configure vertex attributes in vertex arrays to match programs).
	//

	initialise_point_region_of_interest_shader_programs(renderer);

	initialise_line_region_of_interest_shader_program(renderer);

	initialise_fill_region_of_interest_shader_program(renderer);

	initialise_mask_region_of_interest_shader_program(renderer);

	initialise_reduction_of_region_of_interest_shader_programs(renderer);
	initialise_reduction_of_region_of_interest_vertex_array(renderer);
}


void
GPlatesOpenGL::GLRasterCoRegistration::initialise_point_region_of_interest_shader_programs(
		GLRenderer &renderer)
{
	// Fragment shader to render points of seed geometries bounded by a loose target raster tile.
	// This version clips to the loose frustum (shouldn't be rendering pixels outside the loose frustum
	// anyway - since ROI-radius expanded seed geometry should be bounded by loose frustum - but
	// just in case this will prevent pixels being rendered into the sub-viewport, of render target,
	// of an adjacent seed geometry thus polluting its results).

	// For small region-of-interest angles (retains accuracy for very small angles).
	d_render_points_of_seed_geometries_with_small_roi_angle_program_object =
			create_region_of_interest_shader_program(
					renderer,
					"#define POINT_REGION_OF_INTEREST\n"
					"#define SMALL_ROI_ANGLE\n"
					"#define ENABLE_SEED_FRUSTUM_CLIPPING\n",
					"#define POINT_REGION_OF_INTEREST\n"
					"#define SMALL_ROI_ANGLE\n"
					"#define ENABLE_SEED_FRUSTUM_CLIPPING\n");

	// For larger region-of-interest angles (retains accuracy for angles very near 90 degrees).
	d_render_points_of_seed_geometries_with_large_roi_angle_program_object =
			create_region_of_interest_shader_program(
					renderer,
					"#define POINT_REGION_OF_INTEREST\n"
					"#define LARGE_ROI_ANGLE\n"
					"#define ENABLE_SEED_FRUSTUM_CLIPPING\n",
					"#define POINT_REGION_OF_INTEREST\n"
					"#define LARGE_ROI_ANGLE\n"
					"#define ENABLE_SEED_FRUSTUM_CLIPPING\n");

	// Attach vertex element buffer to the vertex array.
	d_point_region_of_interest_vertex_array->set_vertex_element_buffer(
			renderer,
			d_streaming_vertex_element_buffer);

	//
	// The following reflects the structure of 'struct PointRegionOfInterestVertex'.
	// It tells OpenGL how the elements of the vertex are packed together in the vertex and
	// which parts of the vertex bind to the named attributes in the shader program.
	//

	// sizeof() only works on data members if you have an object instantiated...
	PointRegionOfInterestVertex vertex_for_sizeof; 
	// Avoid unused variable warning on some compilers not recognising sizeof() as usage.
	static_cast<void>(vertex_for_sizeof.point_centre);
	// Offset of attribute data from start of a vertex.
	GLint offset = 0;

	// NOTE: We don't need to worry about attribute aliasing (see comment in
	// 'GLProgramObject::gl_bind_attrib_location') because we are not using any of the built-in
	// attributes (like 'gl_Vertex').
	// However we'll start attribute indices at 1 (instead of 0) in case we later decide to use
	// the most common built-in attribute 'gl_Vertex' (which aliases to attribute index 0).
	// If we use more built-in attributes then we'll need to modify the attribute indices we use here.
	// UPDATE: It turns out some hardware (nVidia 7400M) does not function unless the index starts
	// at zero (it's probably expecting either a generic vertex attribute at index zero or 'gl_Vertex').
	GLuint attribute_index = 0;

	// The "point_centre" attribute data...
	d_render_points_of_seed_geometries_with_small_roi_angle_program_object->gl_bind_attrib_location(
			"point_centre", attribute_index);
	d_render_points_of_seed_geometries_with_large_roi_angle_program_object->gl_bind_attrib_location(
			"point_centre", attribute_index);
	d_point_region_of_interest_vertex_array->set_enable_vertex_attrib_array(
			renderer, attribute_index, true/*enable*/);
	d_point_region_of_interest_vertex_array->set_vertex_attrib_pointer(
			renderer,
			d_streaming_vertex_buffer,
			attribute_index,
			sizeof(vertex_for_sizeof.point_centre) / sizeof(vertex_for_sizeof.point_centre[0]),
			GL_FLOAT,
			GL_FALSE/*normalized*/,
			sizeof(PointRegionOfInterestVertex),
			offset);

	++attribute_index;
	offset += sizeof(vertex_for_sizeof.point_centre);

	// The "tangent_frame_weights" attribute data...
	d_render_points_of_seed_geometries_with_small_roi_angle_program_object->gl_bind_attrib_location(
			"tangent_frame_weights", attribute_index);
	d_render_points_of_seed_geometries_with_large_roi_angle_program_object->gl_bind_attrib_location(
			"tangent_frame_weights", attribute_index);
	d_point_region_of_interest_vertex_array->set_enable_vertex_attrib_array(
			renderer, attribute_index, true/*enable*/);
	d_point_region_of_interest_vertex_array->set_vertex_attrib_pointer(
			renderer,
			d_streaming_vertex_buffer,
			attribute_index,
			sizeof(vertex_for_sizeof.tangent_frame_weights) / sizeof(vertex_for_sizeof.tangent_frame_weights[0]),
			GL_FLOAT,
			GL_FALSE/*normalized*/,
			sizeof(PointRegionOfInterestVertex),
			offset);

	++attribute_index;
	offset += sizeof(vertex_for_sizeof.tangent_frame_weights);

	// The "world_space_quaternion" attribute data...
	d_render_points_of_seed_geometries_with_small_roi_angle_program_object->gl_bind_attrib_location(
			"world_space_quaternion", attribute_index);
	d_render_points_of_seed_geometries_with_large_roi_angle_program_object->gl_bind_attrib_location(
			"world_space_quaternion", attribute_index);
	d_point_region_of_interest_vertex_array->set_enable_vertex_attrib_array(
			renderer, attribute_index, true/*enable*/);
	d_point_region_of_interest_vertex_array->set_vertex_attrib_pointer(
			renderer,
			d_streaming_vertex_buffer,
			attribute_index,
			sizeof(vertex_for_sizeof.world_space_quaternion) / sizeof(vertex_for_sizeof.world_space_quaternion[0]),
			GL_FLOAT,
			GL_FALSE/*normalized*/,
			sizeof(PointRegionOfInterestVertex),
			offset);

	++attribute_index;
	offset += sizeof(vertex_for_sizeof.world_space_quaternion);

	// The "seed_frustum_to_render_target_clip_space_transform" attribute data...
	d_render_points_of_seed_geometries_with_small_roi_angle_program_object->gl_bind_attrib_location(
			"raster_frustum_to_seed_frustum_clip_space_transform", attribute_index);
	d_render_points_of_seed_geometries_with_large_roi_angle_program_object->gl_bind_attrib_location(
			"raster_frustum_to_seed_frustum_clip_space_transform", attribute_index);
	d_point_region_of_interest_vertex_array->set_enable_vertex_attrib_array(
			renderer, attribute_index, true/*enable*/);
	d_point_region_of_interest_vertex_array->set_vertex_attrib_pointer(
			renderer,
			d_streaming_vertex_buffer,
			attribute_index,
			sizeof(vertex_for_sizeof.raster_frustum_to_seed_frustum_clip_space_transform) /
				sizeof(vertex_for_sizeof.raster_frustum_to_seed_frustum_clip_space_transform[0]),
			GL_FLOAT,
			GL_FALSE/*normalized*/,
			sizeof(PointRegionOfInterestVertex),
			offset);

	++attribute_index;
	offset += sizeof(vertex_for_sizeof.raster_frustum_to_seed_frustum_clip_space_transform);

	// The "seed_frustum_to_render_target_clip_space_transform" attribute data...
	d_render_points_of_seed_geometries_with_small_roi_angle_program_object->gl_bind_attrib_location(
			"seed_frustum_to_render_target_clip_space_transform", attribute_index);
	d_render_points_of_seed_geometries_with_large_roi_angle_program_object->gl_bind_attrib_location(
			"seed_frustum_to_render_target_clip_space_transform", attribute_index);
	d_point_region_of_interest_vertex_array->set_enable_vertex_attrib_array(
			renderer, attribute_index, true/*enable*/);
	d_point_region_of_interest_vertex_array->set_vertex_attrib_pointer(
			renderer,
			d_streaming_vertex_buffer,
			attribute_index,
			sizeof(vertex_for_sizeof.seed_frustum_to_render_target_clip_space_transform) /
				sizeof(vertex_for_sizeof.seed_frustum_to_render_target_clip_space_transform[0]),
			GL_FLOAT,
			GL_FALSE/*normalized*/,
			sizeof(PointRegionOfInterestVertex),
			offset);


	// Now that we've changed the attribute bindings in the program object we need to
	// re-link it in order for them to take effect.
	bool link_status;
	link_status = d_render_points_of_seed_geometries_with_small_roi_angle_program_object->gl_link_program(renderer);
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			link_status,
			GPLATES_ASSERTION_SOURCE);
	link_status = d_render_points_of_seed_geometries_with_large_roi_angle_program_object->gl_link_program(renderer);
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			link_status,
			GPLATES_ASSERTION_SOURCE);
}


void
GPlatesOpenGL::GLRasterCoRegistration::initialise_line_region_of_interest_shader_program(
		GLRenderer &renderer)
{
	// Fragment shader to render lines (GCAs) of seed geometries bounded by a loose target raster tile.
	// This version clips to the loose frustum (shouldn't be rendering pixels outside the loose frustum
	// anyway - since ROI-radius expanded seed geometry should be bounded by loose frustum - but
	// just in case this will prevent pixels being rendered into the sub-viewport, of render target,
	// of an adjacent seed geometry thus polluting its results).

	// For small region-of-interest angles (retains accuracy for very small angles).
	d_render_lines_of_seed_geometries_with_small_roi_angle_program_object =
			create_region_of_interest_shader_program(
					renderer,
					"#define LINE_REGION_OF_INTEREST\n"
					"#define SMALL_ROI_ANGLE\n"
					"#define ENABLE_SEED_FRUSTUM_CLIPPING\n",
					"#define LINE_REGION_OF_INTEREST\n"
					"#define SMALL_ROI_ANGLE\n"
					"#define ENABLE_SEED_FRUSTUM_CLIPPING\n");

	// For larger region-of-interest angles (retains accuracy for angles very near 90 degrees).
	d_render_lines_of_seed_geometries_with_large_roi_angle_program_object =
			create_region_of_interest_shader_program(
					renderer,
					"#define LINE_REGION_OF_INTEREST\n"
					"#define LARGE_ROI_ANGLE\n"
					"#define ENABLE_SEED_FRUSTUM_CLIPPING\n",
					"#define LINE_REGION_OF_INTEREST\n"
					"#define LARGE_ROI_ANGLE\n"
					"#define ENABLE_SEED_FRUSTUM_CLIPPING\n");

	// Attach vertex element buffer to the vertex array.
	d_line_region_of_interest_vertex_array->set_vertex_element_buffer(
			renderer,
			d_streaming_vertex_element_buffer);

	//
	// The following reflects the structure of 'struct LineRegionOfInterestVertex'.
	// It tells OpenGL how the elements of the vertex are packed together in the vertex and
	// which parts of the vertex bind to the named attributes in the shader program.
	//

	// sizeof() only works on data members if you have an object instantiated...
	LineRegionOfInterestVertex vertex_for_sizeof;
	// Avoid unused variable warning on some compilers not recognising sizeof() as usage.
	static_cast<void>(vertex_for_sizeof.line_arc_start_point);
	// Offset of attribute data from start of a vertex.
	GLint offset = 0;

	// NOTE: We don't need to worry about attribute aliasing (see comment in
	// 'GLProgramObject::gl_bind_attrib_location') because we are not using any of the built-in
	// attributes (like 'gl_Vertex').
	// However we'll start attribute indices at 1 (instead of 0) in case we later decide to use
	// the most common built-in attribute 'gl_Vertex' (which aliases to attribute index 0).
	// If we use more built-in attributes then we'll need to modify the attribute indices we use here.
	// UPDATE: It turns out some hardware (nVidia 7400M) does not function unless the index starts
	// at zero (it's probably expecting either a generic vertex attribute at index zero or 'gl_Vertex').
	GLuint attribute_index = 0;

	// The "line_arc_start_point" attribute data...
	d_render_lines_of_seed_geometries_with_small_roi_angle_program_object->gl_bind_attrib_location(
			"line_arc_start_point", attribute_index);
	d_render_lines_of_seed_geometries_with_large_roi_angle_program_object->gl_bind_attrib_location(
			"line_arc_start_point", attribute_index);
	d_line_region_of_interest_vertex_array->set_enable_vertex_attrib_array(
			renderer, attribute_index, true/*enable*/);
	d_line_region_of_interest_vertex_array->set_vertex_attrib_pointer(
			renderer,
			d_streaming_vertex_buffer,
			attribute_index,
			sizeof(vertex_for_sizeof.line_arc_start_point) / sizeof(vertex_for_sizeof.line_arc_start_point[0]),
			GL_FLOAT,
			GL_FALSE/*normalized*/,
			sizeof(LineRegionOfInterestVertex),
			offset);

	++attribute_index;
	offset += sizeof(vertex_for_sizeof.line_arc_start_point);

	// The "line_arc_normal" attribute data...
	d_render_lines_of_seed_geometries_with_small_roi_angle_program_object->gl_bind_attrib_location(
			"line_arc_normal", attribute_index);
	d_render_lines_of_seed_geometries_with_large_roi_angle_program_object->gl_bind_attrib_location(
			"line_arc_normal", attribute_index);
	d_line_region_of_interest_vertex_array->set_enable_vertex_attrib_array(
			renderer, attribute_index, true/*enable*/);
	d_line_region_of_interest_vertex_array->set_vertex_attrib_pointer(
			renderer,
			d_streaming_vertex_buffer,
			attribute_index,
			sizeof(vertex_for_sizeof.line_arc_normal) / sizeof(vertex_for_sizeof.line_arc_normal[0]),
			GL_FLOAT,
			GL_FALSE/*normalized*/,
			sizeof(LineRegionOfInterestVertex),
			offset);

	++attribute_index;
	offset += sizeof(vertex_for_sizeof.line_arc_normal);

	// The "tangent_frame_weights" attribute data...
	d_render_lines_of_seed_geometries_with_small_roi_angle_program_object->gl_bind_attrib_location(
			"tangent_frame_weights", attribute_index);
	d_render_lines_of_seed_geometries_with_large_roi_angle_program_object->gl_bind_attrib_location(
			"tangent_frame_weights", attribute_index);
	d_line_region_of_interest_vertex_array->set_enable_vertex_attrib_array(
			renderer, attribute_index, true/*enable*/);
	d_line_region_of_interest_vertex_array->set_vertex_attrib_pointer(
			renderer,
			d_streaming_vertex_buffer,
			attribute_index,
			sizeof(vertex_for_sizeof.tangent_frame_weights) / sizeof(vertex_for_sizeof.tangent_frame_weights[0]),
			GL_FLOAT,
			GL_FALSE/*normalized*/,
			sizeof(LineRegionOfInterestVertex),
			offset);

	++attribute_index;
	offset += sizeof(vertex_for_sizeof.tangent_frame_weights);

	// The "world_space_quaternion" attribute data...
	d_render_lines_of_seed_geometries_with_small_roi_angle_program_object->gl_bind_attrib_location(
			"world_space_quaternion", attribute_index);
	d_render_lines_of_seed_geometries_with_large_roi_angle_program_object->gl_bind_attrib_location(
			"world_space_quaternion", attribute_index);
	d_line_region_of_interest_vertex_array->set_enable_vertex_attrib_array(
			renderer, attribute_index, true/*enable*/);
	d_line_region_of_interest_vertex_array->set_vertex_attrib_pointer(
			renderer,
			d_streaming_vertex_buffer,
			attribute_index,
			sizeof(vertex_for_sizeof.world_space_quaternion) / sizeof(vertex_for_sizeof.world_space_quaternion[0]),
			GL_FLOAT,
			GL_FALSE/*normalized*/,
			sizeof(LineRegionOfInterestVertex),
			offset);

	++attribute_index;
	offset += sizeof(vertex_for_sizeof.world_space_quaternion);

	// The "seed_frustum_to_render_target_clip_space_transform" attribute data...
	d_render_lines_of_seed_geometries_with_small_roi_angle_program_object->gl_bind_attrib_location(
			"raster_frustum_to_seed_frustum_clip_space_transform", attribute_index);
	d_render_lines_of_seed_geometries_with_large_roi_angle_program_object->gl_bind_attrib_location(
			"raster_frustum_to_seed_frustum_clip_space_transform", attribute_index);
	d_line_region_of_interest_vertex_array->set_enable_vertex_attrib_array(
			renderer, attribute_index, true/*enable*/);
	d_line_region_of_interest_vertex_array->set_vertex_attrib_pointer(
			renderer,
			d_streaming_vertex_buffer,
			attribute_index,
			sizeof(vertex_for_sizeof.raster_frustum_to_seed_frustum_clip_space_transform) /
				sizeof(vertex_for_sizeof.raster_frustum_to_seed_frustum_clip_space_transform[0]),
			GL_FLOAT,
			GL_FALSE/*normalized*/,
			sizeof(LineRegionOfInterestVertex),
			offset);

	++attribute_index;
	offset += sizeof(vertex_for_sizeof.raster_frustum_to_seed_frustum_clip_space_transform);

	// The "seed_frustum_to_render_target_clip_space_transform" attribute data...
	d_render_lines_of_seed_geometries_with_small_roi_angle_program_object->gl_bind_attrib_location(
			"seed_frustum_to_render_target_clip_space_transform", attribute_index);
	d_render_lines_of_seed_geometries_with_large_roi_angle_program_object->gl_bind_attrib_location(
			"seed_frustum_to_render_target_clip_space_transform", attribute_index);
	d_line_region_of_interest_vertex_array->set_enable_vertex_attrib_array(
			renderer, attribute_index, true/*enable*/);
	d_line_region_of_interest_vertex_array->set_vertex_attrib_pointer(
			renderer,
			d_streaming_vertex_buffer,
			attribute_index,
			sizeof(vertex_for_sizeof.seed_frustum_to_render_target_clip_space_transform) /
				sizeof(vertex_for_sizeof.seed_frustum_to_render_target_clip_space_transform[0]),
			GL_FLOAT,
			GL_FALSE/*normalized*/,
			sizeof(LineRegionOfInterestVertex),
			offset);


	// Now that we've changed the attribute bindings in the program object we need to
	// re-link it in order for them to take effect.
	bool link_status;
	link_status = d_render_lines_of_seed_geometries_with_small_roi_angle_program_object->gl_link_program(renderer);
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			link_status,
			GPLATES_ASSERTION_SOURCE);
	link_status = d_render_lines_of_seed_geometries_with_large_roi_angle_program_object->gl_link_program(renderer);
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			link_status,
			GPLATES_ASSERTION_SOURCE);
}


void
GPlatesOpenGL::GLRasterCoRegistration::initialise_fill_region_of_interest_shader_program(
		GLRenderer &renderer)
{
	// Shader program to render interior of seed polygons bounded by a loose target raster tile.
	// Also used when rasterizing point and line primitive (ie, GL_POINTS and GL_LINES, not GL_TRIANGLES).
	d_render_fill_of_seed_geometries_program_object =
			create_region_of_interest_shader_program(
					renderer,
					"#define FILL_REGION_OF_INTEREST\n"
					"#define ENABLE_SEED_FRUSTUM_CLIPPING\n",
					"#define FILL_REGION_OF_INTEREST\n"
					"#define ENABLE_SEED_FRUSTUM_CLIPPING\n");

	// Attach vertex element buffer to the vertex array.
	d_fill_region_of_interest_vertex_array->set_vertex_element_buffer(
			renderer,
			d_streaming_vertex_element_buffer);

	//
	// The following reflects the structure of 'struct FillRegionOfInterestVertex'.
	// It tells OpenGL how the elements of the vertex are packed together in the vertex and
	// which parts of the vertex bind to the named attributes in the shader program.
	//

	// sizeof() only works on data members if you have an object instantiated...
	FillRegionOfInterestVertex vertex_for_sizeof;
	// Avoid unused variable warning on some compilers not recognising sizeof() as usage.
	static_cast<void>(vertex_for_sizeof.fill_position);
	// Offset of attribute data from start of a vertex.
	GLint offset = 0;

	// NOTE: We don't need to worry about attribute aliasing (see comment in
	// 'GLProgramObject::gl_bind_attrib_location') because we are not using any of the built-in
	// attributes (like 'gl_Vertex').
	// However we'll start attribute indices at 1 (instead of 0) in case we later decide to use
	// the most common built-in attribute 'gl_Vertex' (which aliases to attribute index 0).
	// If we use more built-in attributes then we'll need to modify the attribute indices we use here.
	// UPDATE: It turns out some hardware (nVidia 7400M) does not function unless the index starts
	// at zero (it's probably expecting either a generic vertex attribute at index zero or 'gl_Vertex').
	GLuint attribute_index = 0;

	// The "fill_position" attribute data...
	d_render_fill_of_seed_geometries_program_object->gl_bind_attrib_location(
			"fill_position", attribute_index);
	d_fill_region_of_interest_vertex_array->set_enable_vertex_attrib_array(
			renderer, attribute_index, true/*enable*/);
	d_fill_region_of_interest_vertex_array->set_vertex_attrib_pointer(
			renderer,
			d_streaming_vertex_buffer,
			attribute_index,
			sizeof(vertex_for_sizeof.fill_position) / sizeof(vertex_for_sizeof.fill_position[0]),
			GL_FLOAT,
			GL_FALSE/*normalized*/,
			sizeof(FillRegionOfInterestVertex),
			offset);

	++attribute_index;
	offset += sizeof(vertex_for_sizeof.fill_position);

	// The "world_space_quaternion" attribute data...
	d_render_fill_of_seed_geometries_program_object->gl_bind_attrib_location(
			"world_space_quaternion", attribute_index);
	d_fill_region_of_interest_vertex_array->set_enable_vertex_attrib_array(
			renderer, attribute_index, true/*enable*/);
	d_fill_region_of_interest_vertex_array->set_vertex_attrib_pointer(
			renderer,
			d_streaming_vertex_buffer,
			attribute_index,
			sizeof(vertex_for_sizeof.world_space_quaternion) / sizeof(vertex_for_sizeof.world_space_quaternion[0]),
			GL_FLOAT,
			GL_FALSE/*normalized*/,
			sizeof(FillRegionOfInterestVertex),
			offset);

	++attribute_index;
	offset += sizeof(vertex_for_sizeof.world_space_quaternion);

	// The "seed_frustum_to_render_target_clip_space_transform" attribute data...
	d_render_fill_of_seed_geometries_program_object->gl_bind_attrib_location(
			"raster_frustum_to_seed_frustum_clip_space_transform", attribute_index);
	d_fill_region_of_interest_vertex_array->set_enable_vertex_attrib_array(
			renderer, attribute_index, true/*enable*/);
	d_fill_region_of_interest_vertex_array->set_vertex_attrib_pointer(
			renderer,
			d_streaming_vertex_buffer,
			attribute_index,
			sizeof(vertex_for_sizeof.raster_frustum_to_seed_frustum_clip_space_transform) /
				sizeof(vertex_for_sizeof.raster_frustum_to_seed_frustum_clip_space_transform[0]),
			GL_FLOAT,
			GL_FALSE/*normalized*/,
			sizeof(FillRegionOfInterestVertex),
			offset);

	++attribute_index;
	offset += sizeof(vertex_for_sizeof.raster_frustum_to_seed_frustum_clip_space_transform);

	// The "seed_frustum_to_render_target_clip_space_transform" attribute data...
	d_render_fill_of_seed_geometries_program_object->gl_bind_attrib_location(
			"seed_frustum_to_render_target_clip_space_transform", attribute_index);
	d_fill_region_of_interest_vertex_array->set_enable_vertex_attrib_array(
			renderer, attribute_index, true/*enable*/);
	d_fill_region_of_interest_vertex_array->set_vertex_attrib_pointer(
			renderer,
			d_streaming_vertex_buffer,
			attribute_index,
			sizeof(vertex_for_sizeof.seed_frustum_to_render_target_clip_space_transform) /
				sizeof(vertex_for_sizeof.seed_frustum_to_render_target_clip_space_transform[0]),
			GL_FLOAT,
			GL_FALSE/*normalized*/,
			sizeof(FillRegionOfInterestVertex),
			offset);


	// Now that we've changed the attribute bindings in the program object we need to
	// re-link it in order for them to take effect.
	const bool link_status = d_render_fill_of_seed_geometries_program_object->gl_link_program(renderer);
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			link_status,
			GPLATES_ASSERTION_SOURCE);
}


void
GPlatesOpenGL::GLRasterCoRegistration::initialise_mask_region_of_interest_shader_program(
		GLRenderer &renderer)
{
	// Vertex shader to copy target raster moments into seed sub-viewport with region-of-interest masking.
	GLShaderProgramUtils::ShaderSource mask_region_of_interest_moments_vertex_shader_source;
	// Add the '#define' first.
	mask_region_of_interest_moments_vertex_shader_source.add_shader_source("#define FILTER_MOMENTS\n");
	// Then add the GLSL 'main()' function.
	mask_region_of_interest_moments_vertex_shader_source.add_shader_source(
			MASK_REGION_OF_INTEREST_VERTEX_SHADER_SOURCE);
	// Compile the vertex shader.
	boost::optional<GLShaderObject::shared_ptr_type> mask_region_of_interest_moments_vertex_shader =
			GLShaderProgramUtils::compile_vertex_shader(
					renderer,
					mask_region_of_interest_moments_vertex_shader_source);
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			mask_region_of_interest_moments_vertex_shader ,
			GPLATES_ASSERTION_SOURCE);

	// Fragment shader to copy target raster moments into seed sub-viewport with region-of-interest masking.
	GLShaderProgramUtils::ShaderSource mask_region_of_interest_moments_fragment_shader_source;
	// Add the '#define' first.
	mask_region_of_interest_moments_fragment_shader_source.add_shader_source("#define FILTER_MOMENTS\n");
	// Then add the GLSL 'main()' function.
	mask_region_of_interest_moments_fragment_shader_source.add_shader_source(
			MASK_REGION_OF_INTEREST_FRAGMENT_SHADER_SOURCE);
	// Compile the fragment shader.
	boost::optional<GLShaderObject::shared_ptr_type> mask_region_of_interest_moments_fragment_shader =
			GLShaderProgramUtils::compile_fragment_shader(
					renderer,
					mask_region_of_interest_moments_fragment_shader_source);
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			mask_region_of_interest_moments_fragment_shader ,
			GPLATES_ASSERTION_SOURCE);
	// Link the shader program.
	boost::optional<GLProgramObject::shared_ptr_type> mask_region_of_interest_moments_program_object =
			GLShaderProgramUtils::link_vertex_fragment_program(
					renderer,
					*mask_region_of_interest_moments_vertex_shader.get(),
					*mask_region_of_interest_moments_fragment_shader.get());
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			mask_region_of_interest_moments_program_object,
			GPLATES_ASSERTION_SOURCE);
	d_mask_region_of_interest_moments_program_object = mask_region_of_interest_moments_program_object.get();

	// Vertex shader to copy target raster min/max into seed sub-viewport with region-of-interest masking.
	GLShaderProgramUtils::ShaderSource mask_region_of_interest_minmax_vertex_shader_source;
	// Add the '#define' first.
	mask_region_of_interest_minmax_vertex_shader_source.add_shader_source("#define FILTER_MIN_MAX\n");
	// Then add the GLSL 'main()' function.
	mask_region_of_interest_minmax_vertex_shader_source.add_shader_source(
			MASK_REGION_OF_INTEREST_VERTEX_SHADER_SOURCE);
	// Compile the vertex shader.
	boost::optional<GLShaderObject::shared_ptr_type> mask_region_of_interest_minmax_vertex_shader =
			GLShaderProgramUtils::compile_vertex_shader(
					renderer,
					mask_region_of_interest_minmax_vertex_shader_source);
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			mask_region_of_interest_minmax_vertex_shader ,
			GPLATES_ASSERTION_SOURCE);

	// Fragment shader to copy target raster min/max into seed sub-viewport with region-of-interest masking.
	GLShaderProgramUtils::ShaderSource mask_region_of_interest_minmax_fragment_shader_source;
	// Add the '#define' first.
	mask_region_of_interest_minmax_fragment_shader_source.add_shader_source("#define FILTER_MIN_MAX\n");
	// Then add the GLSL 'main()' function.
	mask_region_of_interest_minmax_fragment_shader_source.add_shader_source(
			MASK_REGION_OF_INTEREST_FRAGMENT_SHADER_SOURCE);
	// Compile the fragment shader.
	boost::optional<GLShaderObject::shared_ptr_type> mask_region_of_interest_minmax_fragment_shader =
			GLShaderProgramUtils::compile_fragment_shader(
					renderer,
					mask_region_of_interest_minmax_fragment_shader_source);
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			mask_region_of_interest_minmax_fragment_shader ,
			GPLATES_ASSERTION_SOURCE);
	// Link the shader program.
	boost::optional<GLProgramObject::shared_ptr_type> mask_region_of_interest_minmax_program_object =
			GLShaderProgramUtils::link_vertex_fragment_program(
					renderer,
					*mask_region_of_interest_minmax_vertex_shader.get(),
					*mask_region_of_interest_minmax_fragment_shader.get());
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			mask_region_of_interest_minmax_program_object,
			GPLATES_ASSERTION_SOURCE);
	d_mask_region_of_interest_minmax_program_object = mask_region_of_interest_minmax_program_object.get();


	// Attach vertex element buffer to the vertex array.
	// All mask region-of-interest shader programs use the same attribute data and hence the same vertex array.
	d_mask_region_of_interest_vertex_array->set_vertex_element_buffer(
			renderer,
			d_streaming_vertex_element_buffer);

	//
	// The following reflects the structure of 'struct MaskRegionOfInterestVertex'.
	// It tells OpenGL how the elements of the vertex are packed together in the vertex and
	// which parts of the vertex bind to the named attributes in the shader program.
	//

	// sizeof() only works on data members if you have an object instantiated...
	MaskRegionOfInterestVertex vertex_for_sizeof;
	// Avoid unused variable warning on some compilers not recognising sizeof() as usage.
	static_cast<void>(vertex_for_sizeof.screen_space_position);
	// Offset of attribute data from start of a vertex.
	GLint offset = 0;

	// NOTE: We don't need to worry about attribute aliasing (see comment in
	// 'GLProgramObject::gl_bind_attrib_location') because we are not using any of the built-in
	// attributes (like 'gl_Vertex').
	// However we'll start attribute indices at 1 (instead of 0) in case we later decide to use
	// the most common built-in attribute 'gl_Vertex' (which aliases to attribute index 0).
	// If we use more built-in attributes then we'll need to modify the attribute indices we use here.
	// UPDATE: It turns out some hardware (nVidia 7400M) does not function unless the index starts
	// at zero (it's probably expecting either a generic vertex attribute at index zero or 'gl_Vertex').
	GLuint attribute_index = 0;

	// The "screen_space_position" attribute data...
	d_mask_region_of_interest_moments_program_object->gl_bind_attrib_location(
			"screen_space_position", attribute_index);
	d_mask_region_of_interest_minmax_program_object->gl_bind_attrib_location(
			"screen_space_position", attribute_index);
	d_mask_region_of_interest_vertex_array->set_enable_vertex_attrib_array(
			renderer, attribute_index, true/*enable*/);
	d_mask_region_of_interest_vertex_array->set_vertex_attrib_pointer(
			renderer,
			d_streaming_vertex_buffer,
			attribute_index,
			sizeof(vertex_for_sizeof.screen_space_position) / sizeof(vertex_for_sizeof.screen_space_position[0]),
			GL_FLOAT,
			GL_FALSE/*normalized*/,
			sizeof(MaskRegionOfInterestVertex),
			offset);

	++attribute_index;
	offset += sizeof(vertex_for_sizeof.screen_space_position);

	// The "raster_frustum_to_seed_frustum_clip_space_transform" attribute data...
	d_mask_region_of_interest_moments_program_object->gl_bind_attrib_location(
			"raster_frustum_to_seed_frustum_clip_space_transform", attribute_index);
	d_mask_region_of_interest_minmax_program_object->gl_bind_attrib_location(
			"raster_frustum_to_seed_frustum_clip_space_transform", attribute_index);
	d_mask_region_of_interest_vertex_array->set_enable_vertex_attrib_array(
			renderer, attribute_index, true/*enable*/);
	d_mask_region_of_interest_vertex_array->set_vertex_attrib_pointer(
			renderer,
			d_streaming_vertex_buffer,
			attribute_index,
			sizeof(vertex_for_sizeof.raster_frustum_to_seed_frustum_clip_space_transform) /
				sizeof(vertex_for_sizeof.raster_frustum_to_seed_frustum_clip_space_transform[0]),
			GL_FLOAT,
			GL_FALSE/*normalized*/,
			sizeof(MaskRegionOfInterestVertex),
			offset);

	++attribute_index;
	offset += sizeof(vertex_for_sizeof.raster_frustum_to_seed_frustum_clip_space_transform);

	// The "seed_frustum_to_render_target_clip_space_transform" attribute data...
	d_mask_region_of_interest_moments_program_object->gl_bind_attrib_location(
			"seed_frustum_to_render_target_clip_space_transform", attribute_index);
	d_mask_region_of_interest_minmax_program_object->gl_bind_attrib_location(
			"seed_frustum_to_render_target_clip_space_transform", attribute_index);
	d_mask_region_of_interest_vertex_array->set_enable_vertex_attrib_array(
			renderer, attribute_index, true/*enable*/);
	d_mask_region_of_interest_vertex_array->set_vertex_attrib_pointer(
			renderer,
			d_streaming_vertex_buffer,
			attribute_index,
			sizeof(vertex_for_sizeof.seed_frustum_to_render_target_clip_space_transform) /
				sizeof(vertex_for_sizeof.seed_frustum_to_render_target_clip_space_transform[0]),
			GL_FLOAT,
			GL_FALSE/*normalized*/,
			sizeof(MaskRegionOfInterestVertex),
			offset);

	// Now that we've changed the attribute bindings in the program object we need to
	// re-link it in order for them to take effect.
	bool link_status;
	link_status = d_mask_region_of_interest_moments_program_object->gl_link_program(renderer);
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			link_status,
			GPLATES_ASSERTION_SOURCE);
	link_status = d_mask_region_of_interest_minmax_program_object->gl_link_program(renderer);
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			link_status,
			GPLATES_ASSERTION_SOURCE);
}


GPlatesOpenGL::GLProgramObject::shared_ptr_type
GPlatesOpenGL::GLRasterCoRegistration::create_region_of_interest_shader_program(
		GLRenderer &renderer,
		const char *vertex_shader_defines,
		const char *fragment_shader_defines)
{
	// Vertex shader source.
	GLShaderProgramUtils::ShaderSource vertex_shader_source;
	// Add the '#define'.
	vertex_shader_source.add_shader_source(vertex_shader_defines);
	// Then add the GLSL function to rotate by quaternion.
	vertex_shader_source.add_shader_source(
			GLShaderProgramUtils::ROTATE_VECTOR_BY_QUATERNION_SHADER_SOURCE);
	// Then add the GLSL 'main()' function.
	vertex_shader_source.add_shader_source(
			RENDER_REGION_OF_INTEREST_GEOMETRIES_VERTEX_SHADER_SOURCE);

	GLShaderProgramUtils::ShaderSource fragment_shader_source;
	// Add the '#define' first.
	fragment_shader_source.add_shader_source(fragment_shader_defines);
	// Then add the GLSL 'main()' function.
	fragment_shader_source.add_shader_source(
			RENDER_REGION_OF_INTEREST_GEOMETRIES_FRAGMENT_SHADER_SOURCE);

	// Link the shader program.
	boost::optional<GLProgramObject::shared_ptr_type> program_object =
			GLShaderProgramUtils::compile_and_link_vertex_fragment_program(
					renderer,
					vertex_shader_source,
					fragment_shader_source);

	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			program_object,
			GPLATES_ASSERTION_SOURCE);

	return program_object.get();
}


void
GPlatesOpenGL::GLRasterCoRegistration::initialise_reduction_of_region_of_interest_shader_programs(
		GLRenderer &renderer)
{
	// Compile the common vertex shader used by all reduction operation shader programs.
	boost::optional<GLShaderObject::shared_ptr_type> reduction_vertex_shader =
			GLShaderProgramUtils::compile_vertex_shader(
					renderer,
					REDUCTION_OF_REGION_OF_INTEREST_VERTEX_SHADER_SOURCE);
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			reduction_vertex_shader ,
			GPLATES_ASSERTION_SOURCE);

	// Fragment shader to calculate the sum of region-of-interest filter results.
	GLShaderProgramUtils::ShaderSource reduction_sum_fragment_shader_source;
	// Add the '#define' first.
	reduction_sum_fragment_shader_source.add_shader_source("#define REDUCTION_SUM\n");
	// Then add the GLSL 'main()' function.
	reduction_sum_fragment_shader_source.add_shader_source(
			REDUCTION_OF_REGION_OF_INTEREST_FRAGMENT_SHADER_SOURCE);
	// Compile the fragment shader to calculate the sum of region-of-interest filter results.
	boost::optional<GLShaderObject::shared_ptr_type> reduction_sum_fragment_shader =
			GLShaderProgramUtils::compile_fragment_shader(
					renderer,
					reduction_sum_fragment_shader_source);
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			reduction_sum_fragment_shader ,
			GPLATES_ASSERTION_SOURCE);
	// Link the shader program to calculate the sum of region-of-interest filter results.
	boost::optional<GLProgramObject::shared_ptr_type> reduction_sum_program_object =
			GLShaderProgramUtils::link_vertex_fragment_program(
					renderer,
					*reduction_vertex_shader.get(),
					*reduction_sum_fragment_shader.get());
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			reduction_sum_program_object,
			GPLATES_ASSERTION_SOURCE);
	d_reduction_sum_program_object = reduction_sum_program_object.get();

	// Fragment shader to calculate the minimum of region-of-interest filter results.
	GLShaderProgramUtils::ShaderSource reduction_min_fragment_shader_source;
	// Add the '#define' first.
	reduction_min_fragment_shader_source.add_shader_source("#define REDUCTION_MIN\n");
	// Then add the GLSL 'main()' function.
	reduction_min_fragment_shader_source.add_shader_source(
			REDUCTION_OF_REGION_OF_INTEREST_FRAGMENT_SHADER_SOURCE);
	// Compile the fragment shader to calculate the minimum of region-of-interest filter results.
	boost::optional<GLShaderObject::shared_ptr_type> reduction_min_fragment_shader =
			GLShaderProgramUtils::compile_fragment_shader(
					renderer,
					reduction_min_fragment_shader_source);
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			reduction_min_fragment_shader ,
			GPLATES_ASSERTION_SOURCE);
	// Link the shader program to calculate the minimum of region-of-interest filter results.
	boost::optional<GLProgramObject::shared_ptr_type> reduction_min_program_object =
			GLShaderProgramUtils::link_vertex_fragment_program(
					renderer,
					*reduction_vertex_shader.get(),
					*reduction_min_fragment_shader.get());
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			reduction_min_program_object,
			GPLATES_ASSERTION_SOURCE);
	d_reduction_min_program_object = reduction_min_program_object.get();

	// Fragment shader to calculate the maximum of region-of-interest filter results.
	GLShaderProgramUtils::ShaderSource reduction_max_fragment_shader_source;
	// Add the '#define' first.
	reduction_max_fragment_shader_source.add_shader_source("#define REDUCTION_MAX\n");
	// Then add the GLSL 'main()' function.
	reduction_max_fragment_shader_source.add_shader_source(
			REDUCTION_OF_REGION_OF_INTEREST_FRAGMENT_SHADER_SOURCE);
	// Compile the fragment shader to calculate the maximum of region-of-interest filter results.
	boost::optional<GLShaderObject::shared_ptr_type> reduction_max_fragment_shader =
			GLShaderProgramUtils::compile_fragment_shader(
					renderer,
					reduction_max_fragment_shader_source);
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			reduction_max_fragment_shader ,
			GPLATES_ASSERTION_SOURCE);
	// Link the shader program to calculate the maximum of region-of-interest filter results.
	boost::optional<GLProgramObject::shared_ptr_type> reduction_max_program_object =
			GLShaderProgramUtils::link_vertex_fragment_program(
					renderer,
					*reduction_vertex_shader.get(),
					*reduction_max_fragment_shader.get());
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			reduction_max_program_object,
			GPLATES_ASSERTION_SOURCE);
	d_reduction_max_program_object = reduction_max_program_object.get();
}


void
GPlatesOpenGL::GLRasterCoRegistration::initialise_reduction_of_region_of_interest_vertex_array(
		GLRenderer &renderer)
{
	std::vector<GLTextureVertex> vertices;
	std::vector<reduction_vertex_element_type> vertex_elements;

	const unsigned int total_number_quads =
			NUM_REDUCE_VERTEX_ARRAY_QUADS_ACROSS_TEXTURE * NUM_REDUCE_VERTEX_ARRAY_QUADS_ACROSS_TEXTURE;
	vertices.reserve(4 * total_number_quads); // Four vertices per quad.
	vertex_elements.reserve(6 * total_number_quads); // Size indices per quad (two triangles, three per triangle).

	// Initialise the vertices in quad-tree traversal order - this is done because the reduce textures
	// are filled up in quad-tree order - so we can reduce a partially filled reduce texture simply
	// by determining how many quads (from beginning of vertex array) to render and submit in one draw call.
	initialise_reduction_vertex_array_in_quad_tree_traversal_order(
			vertices,
			vertex_elements,
			0/*x_quad_offset*/,
			0/*y_quad_offset*/,
			NUM_REDUCE_VERTEX_ARRAY_QUADS_ACROSS_TEXTURE/*width_in_quads*/);

	// Store the vertices/indices in new vertex/index buffers and attach to the reduction vertex array.
	set_vertex_array_data(renderer, *d_reduction_vertex_array, vertices, vertex_elements);
}


void
GPlatesOpenGL::GLRasterCoRegistration::initialise_reduction_vertex_array_in_quad_tree_traversal_order(
		std::vector<GLTextureVertex> &vertices,
		std::vector<reduction_vertex_element_type> &vertex_elements,
		unsigned int x_quad_offset,
		unsigned int y_quad_offset,
		unsigned int width_in_quads)
{
	// If we've reached the leaf nodes of the quad tree traversal.
	if (width_in_quads == 1)
	{
		//
		// Write one quad primitive (two triangles) to the list of vertices/indices.
		//

		const double inverse_num_reduce_quads = 1.0 / NUM_REDUCE_VERTEX_ARRAY_QUADS_ACROSS_TEXTURE;

		const double u0 = x_quad_offset * inverse_num_reduce_quads;
		const double u1 = (x_quad_offset + 1) * inverse_num_reduce_quads;
		const double v0 = y_quad_offset * inverse_num_reduce_quads;
		const double v1 = (y_quad_offset + 1) * inverse_num_reduce_quads;

		// Screen space position is similar to texture coordinates but in range [-1,1] instead of [0,1].
		const double x0 = 2 * u0 - 1;
		const double x1 = 2 * u1 - 1;
		const double y0 = 2 * v0 - 1;
		const double y1 = 2 * v1 - 1;

		const unsigned int quad_start_vertex_index = vertices.size();

		vertices.push_back(GLTextureVertex(x0, y0, 0, u0, v0));
		vertices.push_back(GLTextureVertex(x0, y1, 0, u0, v1));
		vertices.push_back(GLTextureVertex(x1, y1, 0, u1, v1));
		vertices.push_back(GLTextureVertex(x1, y0, 0, u1, v0));

		// First quad triangle.
		vertex_elements.push_back(quad_start_vertex_index);
		vertex_elements.push_back(quad_start_vertex_index + 1);
		vertex_elements.push_back(quad_start_vertex_index + 2);
		// Second quad triangle.
		vertex_elements.push_back(quad_start_vertex_index);
		vertex_elements.push_back(quad_start_vertex_index + 2);
		vertex_elements.push_back(quad_start_vertex_index + 3);

		return;
	}

	// Recurse into the child quad tree nodes.
	for (unsigned int child_y_offset = 0; child_y_offset < 2; ++child_y_offset)
	{
		for (unsigned int child_x_offset = 0; child_x_offset < 2; ++child_x_offset)
		{
			const unsigned int child_x_quad_offset = 2 * x_quad_offset + child_x_offset;
			const unsigned int child_y_quad_offset = 2 * y_quad_offset + child_y_offset;
			const unsigned int child_width_in_quads = width_in_quads / 2;

			initialise_reduction_vertex_array_in_quad_tree_traversal_order(
					vertices,
					vertex_elements,
					child_x_quad_offset,
					child_y_quad_offset,
					child_width_in_quads);
		}
	}
}


void
GPlatesOpenGL::GLRasterCoRegistration::initialise_texture_level_of_detail_parameters(
		GLRenderer &renderer,
		const GLMultiResolutionRasterInterface::non_null_ptr_type &target_raster,
		const unsigned int raster_level_of_detail,
		unsigned int &raster_texture_cube_quad_tree_depth,
		unsigned int &seed_geometries_spatial_partition_depth)
{
	GLCubeSubdivision::non_null_ptr_to_const_type cube_subdivision = GLCubeSubdivision::create();

	// Get the projection transforms of an entire cube face (the lowest resolution level-of-detail).
	const GLTransform::non_null_ptr_to_const_type projection_transform =
			cube_subdivision->get_projection_transform(
					0/*level_of_detail*/, 0/*tile_u_offset*/, 0/*tile_v_offset*/);

	// Get the view transform - it doesn't matter which cube face we choose because, although
	// the view transforms are different, it won't matter to us since we're projecting onto
	// a spherical globe from its centre and all faces project the same way.
	const GLTransform::non_null_ptr_to_const_type view_transform =
			cube_subdivision->get_view_transform(
					GPlatesMaths::CubeCoordinateFrame::POSITIVE_X);

	// Determine the scale factor for our viewport dimensions required to capture the resolution
	// of target raster level-of-detail into an entire cube face.
	//
	// This tells us how many textures of square dimension 'TEXTURE_DIMENSION' will be needed
	// to tile a single cube face.
	double viewport_dimension_scale =
			target_raster->get_viewport_dimension_scale(
					view_transform->get_matrix(),
					projection_transform->get_matrix(),
					GLViewport(0, 0, TEXTURE_DIMENSION, TEXTURE_DIMENSION),
					raster_level_of_detail);

	double log2_viewport_dimension_scale = std::log(viewport_dimension_scale) / std::log(2.0);

	// We always acquire the same-sized textures instead of creating the optimal non-power-of-two
	// texture for the target raster because we want raster queries for all size target rasters to
	// re-use the same textures - these textures use quite a lot of memory so we really need to
	// be able to re-use them once they are created.
	// In any case our reduction textures need to be power-of-two since they are subdivided using
	// a quad tree and the reduction texture dimensions need to match the raster texture dimensions.
	//
	// Determine the cube quad tree level-of-detail at which to render the target raster into
	// the processing texture.
	// Note that this is only used if there are seed geometries stored in the cube quad tree partition
	// in levels [0, raster_texture_cube_quad_tree_depth].
	// If there are seed geometries stored in the cube quad tree partition in levels
	// [raster_texture_cube_quad_tree_depth + 1, inf) then they'll get processed using 'loose' textures.
	raster_texture_cube_quad_tree_depth =
			(log2_viewport_dimension_scale < 0)
					// The entire cube face can fit in a single TEXTURE_DIMENSION x TEXTURE_DIMENSION texture.
					? 0
					// The '1 - 1e-4' rounds up to the next integer level-of-detail.
					: static_cast<int>(log2_viewport_dimension_scale + 1 - 1e-4);

	//
	// NOTE: Previously we only rendered to part of the TEXTURE_DIMENSION x TEXTURE_DIMENSION
	// render texture - only enough to adequately capture the resolution of the target raster.
	//
	// However this presented various problems and so now we just render to the entire texture
	// even though it means some sized target rasters will get more resolution than they need.
	//
	// Some of the problems encountered (and solved by always using a power-of-two dimension) were:
	//  - difficultly having a reduce quad tree (a quad tree is a simple and elegant solution to this problem),
	//  - dealing with reduced viewports that were not of integer dimensions,
	//  - having to deal with odd dimension viewports and their effect on the 2x2 reduce filter,
	//  - having to keep track of more detailed mesh quads (used when reducing rendered seed geometries)
	//    which is simplified greatly by using a quad tree.
	//
#if 0
	// The target raster is rendered into the...
	//
	//    'target_raster_viewport_dimension' x 'target_raster_viewport_dimension'
	//
	// ...region of the 'TEXTURE_DIMENSION' x 'TEXTURE_DIMENSION' processing texture.
	// This is enough to retain the resolution of the target raster.
	target_raster_viewport_dimension = static_cast<int>(
			viewport_dimension_scale / std::pow(2.0, double(raster_texture_cube_quad_tree_depth))
					* TEXTURE_DIMENSION
							// Equivalent to rounding up to the next texel...
							+ 1 - 1e-4);
#endif

	// The maximum depth of the seed geometries spatial partition is enough to render seed
	// geometries (at the maximum depth) such that the pixel dimension of the 'loose' tile needed
	// to bound them covers 'MINIMUM_SEED_GEOMETRIES_VIEWPORT_DIMENSION' pixels.
	// We don't need a deeper spatial partition than this in order to get good batching of seed geometries.
	//
	// The '+1' is because at depth 'd_raster_texture_cube_quad_tree_depth' the non-loose tile has
	// dimension TEXTURE_DIMENSION and at depth 'd_raster_texture_cube_quad_tree_depth + 1' the
	// *loose* tile (that's the depth at which we switch from non-loose to loose tiles) also has
	// dimension TEXTURE_DIMENSION (after which the tile dimension then halves with each depth increment).
	seed_geometries_spatial_partition_depth =
			raster_texture_cube_quad_tree_depth + 1 +
				GPlatesUtils::Base2::log2_power_of_two(
					TEXTURE_DIMENSION / MINIMUM_SEED_GEOMETRIES_VIEWPORT_DIMENSION);
}


void
GPlatesOpenGL::GLRasterCoRegistration::co_register(
		GLRenderer &renderer,
		std::vector<Operation> &operations,
		const std::vector<GPlatesAppLogic::ReconstructContext::ReconstructedFeature> &seed_features,
		const GLMultiResolutionRasterInterface::non_null_ptr_type &target_raster,
		unsigned int raster_level_of_detail)
{
	PROFILE_FUNC();

	// Make sure we leave the OpenGL state the way it was.
	// We don't really need this (since we already save/restore where needed) but put it here just in case.
	GLRenderer::StateBlockScope save_restore_state(renderer);


	//
	// The following is *preparation* for co-registration processing...
	//

	// Ensure the raster level of detail is within a valid range.
	raster_level_of_detail = target_raster->clamp_level_of_detail(raster_level_of_detail);

	// Initialise details to do with texture viewports and cube quad tree level-of-detail
	// transitions that depend on the target raster *resolution*.
	unsigned int raster_texture_cube_quad_tree_depth;
	unsigned int seed_geometries_spatial_partition_depth;
	initialise_texture_level_of_detail_parameters(
			renderer,
			target_raster,
			raster_level_of_detail,
			raster_texture_cube_quad_tree_depth,
			seed_geometries_spatial_partition_depth);

	// Intermediate co-registration results - each seed feature can have multiple (partial)
	// co-registration results that need to be combined into a single result for each seed feature
	// before returning results to the caller.
	std::vector<OperationSeedFeaturePartialResults> seed_feature_partial_results(operations.size());

	// Clear/initialise the caller's operations' result arrays.
	for (unsigned int operation_index = 0; operation_index < operations.size(); ++operation_index)
	{
		Operation &operation = operations[operation_index];

		// There is one result for each seed feature.
		// Initially all the results are N/A (equal to boost::none).
		operation.d_results.clear();
		operation.d_results.resize(seed_features.size());

		// There is one list of (partial) co-registration results for each seed feature.
		OperationSeedFeaturePartialResults &operation_seed_feature_partial_results =
				seed_feature_partial_results[operation_index];
		operation_seed_feature_partial_results.partial_result_lists.resize(seed_features.size());
	}

	// Queues asynchronous reading back of results from GPU to CPU memory.
	ResultsQueue results_queue(renderer);


	//
	// Co-registration processing...
	//

	// From the seed geometries create a spatial partition of SeedCoRegistration objects.
	const seed_geometries_spatial_partition_type::non_null_ptr_type
			seed_geometries_spatial_partition =
					create_reconstructed_seed_geometries_spatial_partition(
							operations,
							seed_features,
							seed_geometries_spatial_partition_depth);

	// This simply avoids having to pass each parameter as function parameters during traversal.
	CoRegistrationParameters co_registration_parameters(
			seed_features,
			target_raster,
			raster_level_of_detail,
			raster_texture_cube_quad_tree_depth,
			seed_geometries_spatial_partition_depth,
			seed_geometries_spatial_partition,
			operations,
			seed_feature_partial_results,
			results_queue);

	// Start co-registering the seed geometries with the raster.
	// The co-registration results are generated here.
	filter_reduce_seed_geometries_spatial_partition(renderer, co_registration_parameters);

	// Finally make sure the results from the GPU are flushed before we return results to the caller.
	// This is done last to minimise any blocking required to wait for the GPU to finish generating
	// the result data and transferring it to CPU memory.
	//
	// TODO: Delay this until the caller actually retrieves the results - then we can advise clients
	// to delay the retrieval of results by doing other work in between initiating the co-registration
	// (this method) and actually reading the results (allowing greater parallelism between GPU and CPU).
	results_queue.flush_results(renderer, seed_feature_partial_results);

	// Now that the results have all been retrieved from the GPU we need combine multiple
	// (potentially partial) co-registration results into a single result per seed feature.
	return_co_registration_results_to_caller(co_registration_parameters);


	//
	// The following is *cleanup* after co-registration processing...
	//

	// Clear the attachments of our acquired framebuffer object so when it's returned it is not
	// sitting around attached to a texture (normally GLContext only detaches when another
	// client requests a framebuffer object).
	d_framebuffer_object->gl_detach_all(renderer);
}


GPlatesOpenGL::GLRasterCoRegistration::seed_geometries_spatial_partition_type::non_null_ptr_type
GPlatesOpenGL::GLRasterCoRegistration::create_reconstructed_seed_geometries_spatial_partition(
		std::vector<Operation> &operations,
		const std::vector<GPlatesAppLogic::ReconstructContext::ReconstructedFeature> &seed_features,
		const unsigned int seed_geometries_spatial_partition_depth)
{
	//PROFILE_FUNC();

	// Create a reconstructed seed geometries spatial partition.
	seed_geometries_spatial_partition_type::non_null_ptr_type
			seed_geometries_spatial_partition =
					seed_geometries_spatial_partition_type::create(
							seed_geometries_spatial_partition_depth);

	// Each operation specifies a region-of-interest radius so convert this to a bounding circle expansion.
	std::vector<seed_geometries_spatial_partition_type::BoundingCircleExtent> operation_regions_of_interest;
	BOOST_FOREACH(const Operation &operation, operations)
	{
		operation_regions_of_interest.push_back(
				seed_geometries_spatial_partition_type::BoundingCircleExtent(
						std::cos(operation.d_region_of_interest_radius)/*cosine_extend_angle_*/));
	}

	// Add the seed feature geometries to the spatial partition.
	for (unsigned int feature_index = 0; feature_index < seed_features.size(); ++feature_index)
	{
		const GPlatesAppLogic::ReconstructContext::ReconstructedFeature &reconstructed_feature = seed_features[feature_index];

		// Each seed feature could have multiple geometries.
		const GPlatesAppLogic::ReconstructContext::ReconstructedFeature::reconstruction_seq_type &reconstructions =
				reconstructed_feature.get_reconstructions();
		BOOST_FOREACH(const GPlatesAppLogic::ReconstructContext::Reconstruction &reconstruction, reconstructions)
		{
			// NOTE: To avoid reconstructing geometries (it's faster if we transform using GPU) we
			// add the *unreconstructed* geometry (and a finite rotation) to the spatial partition.
			// The spatial partition will rotate only the centroid of the *unreconstructed*
			// geometry (instead of reconstructing the entire geometry) and then use that as the
			// insertion location (along with the *unreconstructed* geometry's bounding circle extents).

			const GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type &rfg =
					reconstruction.get_reconstructed_feature_geometry();

			// See if the reconstruction can be represented as a finite rotation.
			const boost::optional<GPlatesAppLogic::ReconstructedFeatureGeometry::FiniteRotationReconstruction> &
					finite_rotation_reconstruction = rfg->finite_rotation_reconstruction();
			if (finite_rotation_reconstruction)
			{
				// The resolved geometry is the *unreconstructed* geometry (but still possibly
				// the result of a lookup of a time-dependent geometry property).
				const GPlatesMaths::GeometryOnSphere &resolved_geometry =
						*finite_rotation_reconstruction->get_resolved_geometry();

				// The finite rotation.
				const GPlatesMaths::FiniteRotation &finite_rotation =
						finite_rotation_reconstruction->get_reconstruct_method_finite_rotation()->get_finite_rotation();

				// Iterate over the operations and insert the same geometry for each operation.
				// Each operation might have a different region-of-interest though which could
				// place the same geometry at different locations in the spatial partition.
				for (unsigned int operation_index = 0; operation_index < operations.size(); ++operation_index)
				{
					// Add to the spatial partition.
					seed_geometries_spatial_partition->add(
							SeedCoRegistration(
									operation_index,
									feature_index,
									resolved_geometry,
									finite_rotation.unit_quat()),
							resolved_geometry,
							operation_regions_of_interest[operation_index],
							finite_rotation);
				}
			}
			else
			{
				const GPlatesMaths::GeometryOnSphere &reconstructed_geometry = *rfg->reconstructed_geometry();

				// It's not a finite rotation so we can't assume the geometry has rigidly rotated.
				// Hence we can't assume it's shape is the same and hence can't assume the
				// small circle bounding radius is the same.
				// So just get the reconstructed geometry and insert it into the spatial partition.
				// The appropriate bounding small circle will be generated for it when it's added.
				//
				// Iterate over the operations and insert the same geometry for each operation.
				for (unsigned int operation_index = 0; operation_index < operations.size(); ++operation_index)
				{
					// Add to the spatial partition.
					seed_geometries_spatial_partition->add(
							SeedCoRegistration(
									operation_index,
									feature_index,
									reconstructed_geometry,
									d_identity_quaternion),
							reconstructed_geometry,
							operation_regions_of_interest[operation_index]);
				}
			}
		}
	}

	return seed_geometries_spatial_partition;
}


void
GPlatesOpenGL::GLRasterCoRegistration::filter_reduce_seed_geometries_spatial_partition(
		GLRenderer &renderer,
		const CoRegistrationParameters &co_registration_parameters)
{
	//PROFILE_FUNC();

	// Create a subdivision cube quad tree traversal.
	// No caching is required since we're only visiting each subdivision node once.
	//
	// We don't need to remove seams between adjacent target raster tiles (due to bilinear filtering)
	// like we do when visualising a raster. So we don't need half-texel expanded tiles (view frustums).
	cube_subdivision_cache_type::non_null_ptr_type
			cube_subdivision_cache =
					cube_subdivision_cache_type::create(
							GPlatesOpenGL::GLCubeSubdivision::create());

	//
	// Traverse the spatial partition of reconstructed seed geometries.
	//

	// Traverse the quad trees of the cube faces.
	for (unsigned int face = 0; face < 6; ++face)
	{
		const GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face =
				static_cast<GPlatesMaths::CubeCoordinateFrame::CubeFaceType>(face);

		// This is used to find those nodes of the reconstructed seed geometries spatial partition
		// that intersect the target raster cube quad tree.
		seed_geometries_intersecting_nodes_type
				seed_geometries_intersecting_nodes(
						*co_registration_parameters.seed_geometries_spatial_partition,
						cube_face);

		// The root node of the seed geometries spatial partition.
		// NOTE: The node reference could be null (meaning there's no seed geometries in the current
		// loose cube face) but we'll still recurse because neighbouring nodes can still intersect
		// the current cube face of the target raster.
		const seed_geometries_spatial_partition_type::node_reference_type
				seed_geometries_spatial_partition_root_node =
						co_registration_parameters.seed_geometries_spatial_partition->get_quad_tree_root_node(cube_face);

		// Get the cube subdivision root node.
		const cube_subdivision_cache_type::node_reference_type
				cube_subdivision_cache_root_node =
						cube_subdivision_cache->get_quad_tree_root_node(cube_face);

		// Initially there are no intersecting nodes...
		seed_geometries_spatial_partition_node_list_type seed_geometries_spatial_partition_node_list;

		filter_reduce_seed_geometries(
				renderer,
				co_registration_parameters,
				seed_geometries_spatial_partition_root_node,
				seed_geometries_spatial_partition_node_list,
				seed_geometries_intersecting_nodes,
				*cube_subdivision_cache,
				cube_subdivision_cache_root_node,
				0/*level_of_detail*/);
	}
}


void
GPlatesOpenGL::GLRasterCoRegistration::filter_reduce_seed_geometries(
		GLRenderer &renderer,
		const CoRegistrationParameters &co_registration_parameters,
		seed_geometries_spatial_partition_type::node_reference_type seed_geometries_spatial_partition_node,
		const seed_geometries_spatial_partition_node_list_type &parent_seed_geometries_intersecting_node_list,
		const seed_geometries_intersecting_nodes_type &seed_geometries_intersecting_nodes,
		cube_subdivision_cache_type &cube_subdivision_cache,
		const cube_subdivision_cache_type::node_reference_type &cube_subdivision_cache_node,
		unsigned int level_of_detail)
{
	// If we've reached the level-of-detail at which to render the target raster .
	if (level_of_detail == co_registration_parameters.d_raster_texture_cube_quad_tree_depth)
	{
		co_register_seed_geometries(
				renderer,
				co_registration_parameters,
				seed_geometries_spatial_partition_node,
				parent_seed_geometries_intersecting_node_list,
				seed_geometries_intersecting_nodes,
				cube_subdivision_cache,
				cube_subdivision_cache_node);

		return;
	}

	//
	// Iterate over the child quad tree nodes.
	//

	for (unsigned int child_v_offset = 0; child_v_offset < 2; ++child_v_offset)
	{
		for (unsigned int child_u_offset = 0; child_u_offset < 2; ++child_u_offset)
		{
			// Used to determine which seed geometries intersect the child quad tree node.
			seed_geometries_intersecting_nodes_type
					child_seed_geometries_intersecting_nodes(
							seed_geometries_intersecting_nodes,
							child_u_offset,
							child_v_offset);

			// Construct linked list nodes on the runtime stack as it simplifies memory management.
			// When the stack unwinds, the list(s) referencing these nodes, as well as the nodes themselves,
			// will disappear together (leaving any lists higher up in the stack still intact) - this happens
			// because this list implementation supports tail-sharing.
			SeedGeometriesNodeListNode child_seed_geometries_list_nodes[
					seed_geometries_intersecting_nodes_type::parent_intersecting_nodes_type::MAX_NUM_NODES];

			// A tail-shared list to contain the seed geometries nodes that intersect the
			// current node. The parent list contains the nodes we've been
			// accumulating so far during our quad tree traversal.
			seed_geometries_spatial_partition_node_list_type
					child_seed_geometries_intersecting_node_list(
							parent_seed_geometries_intersecting_node_list);

			// Add any new intersecting nodes from the seed geometries spatial partition.
			// These new nodes are the nodes that intersect the tile at the current quad tree depth.
			const seed_geometries_intersecting_nodes_type::parent_intersecting_nodes_type &
					parent_intersecting_nodes =
							child_seed_geometries_intersecting_nodes.get_parent_intersecting_nodes();

			// Now add those neighbours nodes that exist (not all areas of the spatial partition will be
			// populated with seed geometries).
			const unsigned int num_parent_nodes = parent_intersecting_nodes.get_num_nodes();
			for (unsigned int parent_node_index = 0; parent_node_index < num_parent_nodes; ++parent_node_index)
			{
				const seed_geometries_spatial_partition_type::node_reference_type
						intersecting_parent_node_reference = parent_intersecting_nodes.get_node(parent_node_index);
				// Only need to add nodes that actually contain seed geometries.
				// NOTE: We still recurse into child nodes though - an empty internal node does not
				// mean the child nodes are necessarily empty.
				if (!intersecting_parent_node_reference.empty())
				{
					child_seed_geometries_list_nodes[parent_node_index].node_reference =
							intersecting_parent_node_reference;

					// Add to the list of seed geometries spatial partition nodes that
					// intersect the current tile.
					child_seed_geometries_intersecting_node_list.push_front(
							&child_seed_geometries_list_nodes[parent_node_index]);
				}
			}

			// See if there is a child node in the seed geometries spatial partition.
			// We might not even have the parent node though - in this case we got here
			// because there are neighbouring nodes that overlap the current target raster tile.
			seed_geometries_spatial_partition_type::node_reference_type child_seed_geometries_spatial_partition_node;
			if (seed_geometries_spatial_partition_node)
			{
				child_seed_geometries_spatial_partition_node =
						seed_geometries_spatial_partition_node.get_child_node(
								child_u_offset, child_v_offset);
			}

			// Get the child cube subdivision cache node.
			const cube_subdivision_cache_type::node_reference_type
					child_cube_subdivision_cache_node =
							cube_subdivision_cache.get_child_node(
									cube_subdivision_cache_node,
									child_u_offset,
									child_v_offset);

			// Recurse into child node.
			filter_reduce_seed_geometries(
					renderer,
					co_registration_parameters,
					child_seed_geometries_spatial_partition_node,
					child_seed_geometries_intersecting_node_list,
					child_seed_geometries_intersecting_nodes,
					cube_subdivision_cache,
					child_cube_subdivision_cache_node,
					level_of_detail + 1);
		}
	}
}


void
GPlatesOpenGL::GLRasterCoRegistration::co_register_seed_geometries(
		GLRenderer &renderer,
		const CoRegistrationParameters &co_registration_parameters,
		seed_geometries_spatial_partition_type::node_reference_type seed_geometries_spatial_partition_node,
		const seed_geometries_spatial_partition_node_list_type &parent_seed_geometries_intersecting_node_list,
		const seed_geometries_intersecting_nodes_type &seed_geometries_intersecting_nodes,
		cube_subdivision_cache_type &cube_subdivision_cache,
		const cube_subdivision_cache_type::node_reference_type &cube_subdivision_cache_node)
{
	// Co-register any seed geometries collected so far during the cube quad tree traversal.
	co_register_seed_geometries_with_target_raster(
			renderer,
			co_registration_parameters,
			parent_seed_geometries_intersecting_node_list,
			seed_geometries_intersecting_nodes,
			cube_subdivision_cache,
			cube_subdivision_cache_node);

	// Continue traversing the seed geometries spatial partition in order to co-register them by
	// switching to rendering the target raster as 'loose' tiles instead of regular, non-overlapping
	// tiles (it means the seed geometries only need be rendered/processed once each).
	//
	// NOTE: We only recurse if the seed geometries spatial partition exists at the current
	// cube quad tree location. If the spatial partition node is null then it means there are no
	// seed geometries in the current sub-tree of the spatial partition.
	if (seed_geometries_spatial_partition_node)
	{
		for (unsigned int child_v_offset = 0; child_v_offset < 2; ++child_v_offset)
		{
			for (unsigned int child_u_offset = 0; child_u_offset < 2; ++child_u_offset)
			{
				// See if there is a child node in the seed geometries spatial partition.
				const seed_geometries_spatial_partition_type::node_reference_type
						child_seed_geometries_spatial_partition_node =
								seed_geometries_spatial_partition_node.get_child_node(
										child_u_offset, child_v_offset);

				// No need to recurse into child node if no seed geometries in current *loose* tile.
				if (!child_seed_geometries_spatial_partition_node)
				{
					continue;
				}

				const cube_subdivision_cache_type::node_reference_type
						child_cube_subdivision_cache_node = cube_subdivision_cache.get_child_node(
								cube_subdivision_cache_node, child_u_offset, child_v_offset);

				co_register_seed_geometries_with_loose_target_raster(
						renderer,
						co_registration_parameters,
						child_seed_geometries_spatial_partition_node,
						cube_subdivision_cache,
						child_cube_subdivision_cache_node);
			}
		}
	}
}


void
GPlatesOpenGL::GLRasterCoRegistration::co_register_seed_geometries_with_target_raster(
		GLRenderer &renderer,
		const CoRegistrationParameters &co_registration_parameters,
		const seed_geometries_spatial_partition_node_list_type &parent_seed_geometries_intersecting_node_list,
		const seed_geometries_intersecting_nodes_type &seed_geometries_intersecting_nodes,
		cube_subdivision_cache_type &cube_subdivision_cache,
		const cube_subdivision_cache_type::node_reference_type &cube_subdivision_cache_node)
{
	// Construct linked list nodes on the runtime stack as it simplifies memory management.
	// When the stack unwinds, the list(s) referencing these nodes, as well as the nodes themselves,
	// will disappear together (leaving any lists higher up in the stack still intact) - this happens
	// because this list implementation supports tail-sharing.
	SeedGeometriesNodeListNode seed_geometries_list_nodes[
			seed_geometries_intersecting_nodes_type::intersecting_nodes_type::MAX_NUM_NODES];

	// A tail-shared list to contain the reconstructed seed geometry nodes that intersect the
	// current target raster frustum. The parent list contains the nodes we've been
	// accumulating so far during our quad tree traversal.
	seed_geometries_spatial_partition_node_list_type
			seed_geometries_intersecting_node_list(
					parent_seed_geometries_intersecting_node_list);

	// Add any new intersecting nodes from the reconstructed seed geometries spatial partition.
	// These new nodes are the nodes that intersect the raster frustum at the current quad tree depth.
	const seed_geometries_intersecting_nodes_type::intersecting_nodes_type &intersecting_nodes =
			seed_geometries_intersecting_nodes.get_intersecting_nodes();

	// Now add those intersecting nodes that exist (not all areas of the spatial partition will be
	// populated with reconstructed seed geometries).
	const unsigned int num_intersecting_nodes = intersecting_nodes.get_num_nodes();
	for (unsigned int list_node_index = 0; list_node_index < num_intersecting_nodes; ++list_node_index)
	{
		const seed_geometries_spatial_partition_type::node_reference_type
				intersecting_node_reference = intersecting_nodes.get_node(list_node_index);

		// Only need to add nodes that actually contain reconstructed seed geometries.
		// NOTE: We still recurse into child nodes though - an empty internal node does not
		// mean the child nodes are necessarily empty.
		if (!intersecting_node_reference.empty())
		{
			// Create the list node.
			seed_geometries_list_nodes[list_node_index].node_reference = intersecting_node_reference;

			// Add to the list of seed geometries spatial partition nodes that intersect the current raster frustum.
			seed_geometries_intersecting_node_list.push_front(&seed_geometries_list_nodes[list_node_index]);
		}
	}

	// If there are no seed geometries collected so far then there's nothing to do so return early.
	if (seed_geometries_intersecting_node_list.empty() &&
		co_registration_parameters.seed_geometries_spatial_partition->begin_root_elements() ==
			co_registration_parameters.seed_geometries_spatial_partition->end_root_elements())
	{
		return;
	}

	//
	// Now traverse the list of intersecting reconstructed seed geometries and co-register them.
	//

	const GLTransform::non_null_ptr_to_const_type view_transform =
			cube_subdivision_cache.get_view_transform(cube_subdivision_cache_node);
	const GLTransform::non_null_ptr_to_const_type projection_transform =
			cube_subdivision_cache.get_projection_transform(cube_subdivision_cache_node);

	// The centre of the cube fact currently being visited.
	// This is used to adjust for the area-sampling distortion of pixels introduced by the cube map.
	const GPlatesMaths::UnitVector3D &cube_face_centre =
		GPlatesMaths::CubeCoordinateFrame::get_cube_face_coordinate_frame_axis(
					cube_subdivision_cache_node.get_cube_face(),
					GPlatesMaths::CubeCoordinateFrame::Z_AXIS);

	// Now that we have a list of seed geometries we can co-register them with the current target raster tile.
	co_register_seed_geometries_with_target_raster(
			renderer,
			co_registration_parameters,
			seed_geometries_intersecting_node_list,
			cube_face_centre,
			view_transform,
			projection_transform);
}


void
GPlatesOpenGL::GLRasterCoRegistration::co_register_seed_geometries_with_target_raster(
		GLRenderer &renderer,
		const CoRegistrationParameters &co_registration_parameters,
		const seed_geometries_spatial_partition_node_list_type &seed_geometries_intersecting_node_list,
		const GPlatesMaths::UnitVector3D &cube_face_centre,
		const GLTransform::non_null_ptr_to_const_type &view_transform,
		const GLTransform::non_null_ptr_to_const_type &projection_transform)
{
	// Acquire a floating-point texture to render the target raster into.
	GLTexture::shared_ptr_type target_raster_texture = acquire_rgba_float_texture(renderer);

	// Render the target raster into the view frustum (into render texture).
	if (!render_target_raster(renderer, co_registration_parameters, target_raster_texture, *view_transform, *projection_transform))
	{
		// There was no rendering of target raster into the current view frustum so there's no
		// co-registration of seed geometries in the current view frustum.
		return;
	}

	// Working lists used during co-registration processing.
	// Each operation has a list for each reduce stage.
	//
	// NOTE: However we only use reduce stage index 0 (the other stages are only needed,
	// for rendering seed geometries to, when rendering seeds in the smaller 'loose' tiles).
	std::vector<SeedCoRegistrationReduceStageLists> operations_reduce_stage_lists(
			co_registration_parameters.operations.size());

	// Iterate over the list of seed geometries and group by operation.
	// This is because the reducing is done per-operation (cannot mix operations while reducing).
	// Note that seed geometries from the root of the spatial partition as well as the node list are grouped.
	group_seed_co_registrations_by_operation_to_reduce_stage_zero(
			operations_reduce_stage_lists,
			*co_registration_parameters.seed_geometries_spatial_partition,
			seed_geometries_intersecting_node_list);

	// Iterate over the operations and co-register the seed geometries associated with each operation.
	for (unsigned int operation_index = 0;
		operation_index < co_registration_parameters.operations.size();
		++operation_index)
	{
		// Note that we always render the seed geometries into reduce stage *zero* here - it's only
		// when we recurse further down and render *loose* target raster tiles that we start rendering
		// to the other reduce stages (because the loose raster tiles are smaller - need less reducing).
		render_seed_geometries_to_reduce_pyramids(
				renderer,
				co_registration_parameters,
				operation_index,
				cube_face_centre,
				target_raster_texture,
				view_transform,
				projection_transform,
				operations_reduce_stage_lists,
				// Seed geometries are *not* bounded by loose cube quad tree tiles...
				false/*are_seed_geometries_bounded*/);
	}
}


void
GPlatesOpenGL::GLRasterCoRegistration::group_seed_co_registrations_by_operation_to_reduce_stage_zero(
		std::vector<SeedCoRegistrationReduceStageLists> &operations_reduce_stage_lists,
		seed_geometries_spatial_partition_type &seed_geometries_spatial_partition,
		const seed_geometries_spatial_partition_node_list_type &seed_geometries_intersecting_node_list)
{
	//PROFILE_FUNC();

	// Iterate over the seed geometries in the root (unpartitioned) of the spatial partition.
	seed_geometries_spatial_partition_type::element_iterator root_seeds_iter =
			seed_geometries_spatial_partition.begin_root_elements();
	seed_geometries_spatial_partition_type::element_iterator root_seeds_end =
			seed_geometries_spatial_partition.end_root_elements();
	for ( ; root_seeds_iter != root_seeds_end; ++root_seeds_iter)
	{
		SeedCoRegistration &seed_co_registration = *root_seeds_iter;

		// NOTE: There's no need to change the default clip-space scale/translate since these seed
		// geometries are rendered into the entire view frustum of the target raster tile
		// and not a subsection of it (like the seed geometries rendered into 'loose' tiles).

		// Add the current seed co-registration to the working list of its operation.
		// Adding to the top-level reduce stage (reduce stage index 0).
		const unsigned int operation_index = seed_co_registration.operation_index;
		operations_reduce_stage_lists[operation_index].reduce_stage_lists[0/*reduce_stage_index*/].push_front(&seed_co_registration);
	}

	// Iterate over the nodes in the seed geometries spatial partition.
	seed_geometries_spatial_partition_node_list_type::const_iterator seeds_node_iter =
			seed_geometries_intersecting_node_list.begin();
	seed_geometries_spatial_partition_node_list_type::const_iterator seeds_node_end =
			seed_geometries_intersecting_node_list.end();
	for ( ; seeds_node_iter != seeds_node_end; ++seeds_node_iter)
	{
		const seed_geometries_spatial_partition_type::node_reference_type &node_reference =
				seeds_node_iter->node_reference;

		// Iterate over the seed co-registrations of the current node.
		seed_geometries_spatial_partition_type::element_iterator seeds_iter = node_reference.begin();
		seed_geometries_spatial_partition_type::element_iterator seeds_end = node_reference.end();
		for ( ; seeds_iter != seeds_end; ++seeds_iter)
		{
			SeedCoRegistration &seed_co_registration = *seeds_iter;

			// NOTE: There's no need to change the default clip-space scale/translate since these seed
			// geometries are rendered into the entire view frustum of the target raster tile
			// and not a subsection of it (like the seed geometries rendered into 'loose' tiles).

			// Add the current seed co-registration to the working list of its operation.
			// Adding to the top-level reduce stage (reduce stage index 0).
			const unsigned int operation_index = seed_co_registration.operation_index;
			operations_reduce_stage_lists[operation_index].reduce_stage_lists[0/*reduce_stage_index*/].push_front(&seed_co_registration);
		}
	}
}


void
GPlatesOpenGL::GLRasterCoRegistration::co_register_seed_geometries_with_loose_target_raster(
		GLRenderer &renderer,
		const CoRegistrationParameters &co_registration_parameters,
		seed_geometries_spatial_partition_type::node_reference_type seed_geometries_spatial_partition_node,
		cube_subdivision_cache_type &cube_subdivision_cache,
		const cube_subdivision_cache_type::node_reference_type &cube_subdivision_cache_node)
{
	// Acquire a floating-point texture to render the target raster into.
	GLTexture::shared_ptr_type target_raster_texture = acquire_rgba_float_texture(renderer);

	const GLTransform::non_null_ptr_to_const_type view_transform =
			cube_subdivision_cache.get_view_transform(cube_subdivision_cache_node);
	// NOTE: We are now rendering to *loose* tiles (frustums) so use loose projection transform.
	const GLTransform::non_null_ptr_to_const_type projection_transform =
			cube_subdivision_cache.get_loose_projection_transform(cube_subdivision_cache_node);

	// Render the target raster into the view frustum (into render texture).
	if (!render_target_raster(renderer, co_registration_parameters, target_raster_texture, *view_transform, *projection_transform))
	{
		// There was no rendering of target raster into the current view frustum so there's no
		// co-registration of seed geometries in the current view frustum.
		return;
	}

	// Working lists used during co-registration processing.
	// Each operation has a list for each reduce stage.
	std::vector<SeedCoRegistrationReduceStageLists> operations_reduce_stage_lists(
			co_registration_parameters.operations.size());

	// As we recurse into the seed geometries spatial partition we need to translate/scale the
	// clip-space (post-projection space) to account for progressively smaller *loose* tile regions.
	// Note that we don't have a half-texel overlap in these frustums - so 'expand_tile_ratio' is '1.0'.
	const GLUtils::QuadTreeClipSpaceTransform raster_frustum_to_loose_seed_frustum_clip_space_transform;

	// Recurse into the current seed geometries spatial partition sub-tree and group
	// seed co-registrations by operation.
	// This is because the reducing is done per-operation (cannot mix operations while reducing).
	group_seed_co_registrations_by_operation(
			co_registration_parameters,
			operations_reduce_stage_lists,
			seed_geometries_spatial_partition_node,
			raster_frustum_to_loose_seed_frustum_clip_space_transform,
			// At the current quad tree depth we are rendering seed geometries into a
			// TEXTURE_DIMENSION tile which means it's the highest resolution reduce stage...
			0/*reduce_stage_index*/);

	// The centre of the cube fact currently being visited.
	// This is used to adjust for the area-sampling distortion of pixels introduced by the cube map.
	const GPlatesMaths::UnitVector3D &cube_face_centre =
		GPlatesMaths::CubeCoordinateFrame::get_cube_face_coordinate_frame_axis(
					cube_subdivision_cache_node.get_cube_face(),
					GPlatesMaths::CubeCoordinateFrame::Z_AXIS);

	// Iterate over the operations and co-register the seed geometries associated with each operation.
	for (unsigned int operation_index = 0;
		operation_index < co_registration_parameters.operations.size();
		++operation_index)
	{
		render_seed_geometries_to_reduce_pyramids(
				renderer,
				co_registration_parameters,
				operation_index,
				cube_face_centre,
				target_raster_texture,
				view_transform,
				projection_transform,
				operations_reduce_stage_lists,
				// Seed geometries are bounded by loose cube quad tree tiles (even reduce stage zero)...
				true/*are_seed_geometries_bounded*/);
	}
}


void
GPlatesOpenGL::GLRasterCoRegistration::group_seed_co_registrations_by_operation(
		const CoRegistrationParameters &co_registration_parameters,
		std::vector<SeedCoRegistrationReduceStageLists> &operations_reduce_stage_lists,
		seed_geometries_spatial_partition_type::node_reference_type seed_geometries_spatial_partition_node,
		const GLUtils::QuadTreeClipSpaceTransform &raster_frustum_to_loose_seed_frustum_clip_space_transform,
		unsigned int reduce_stage_index)
{
	//PROFILE_FUNC();

	// Things are set up so that seed geometries at the maximum spatial partition depth will
	// render into the reduce stage that has dimension MINIMUM_SEED_GEOMETRIES_VIEWPORT_DIMENSION.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			reduce_stage_index < NUM_REDUCE_STAGES - boost::static_log2<MINIMUM_SEED_GEOMETRIES_VIEWPORT_DIMENSION>::value,
			GPLATES_ASSERTION_SOURCE);

	// The clip-space scale/translate for the current *loose* spatial partition node.
	const double raster_frustum_to_loose_seed_frustum_post_projection_scale =
			raster_frustum_to_loose_seed_frustum_clip_space_transform.get_loose_scale();
	const double raster_frustum_to_loose_seed_frustum_post_projection_translate_x =
			raster_frustum_to_loose_seed_frustum_clip_space_transform.get_loose_translate_x();
	const double raster_frustum_to_loose_seed_frustum_post_projection_translate_y =
			raster_frustum_to_loose_seed_frustum_clip_space_transform.get_loose_translate_y();

	// Iterate over the current node in the seed geometries spatial partition.
	seed_geometries_spatial_partition_type::element_iterator seeds_iter = seed_geometries_spatial_partition_node.begin();
	seed_geometries_spatial_partition_type::element_iterator seeds_end = seed_geometries_spatial_partition_node.end();
	for ( ; seeds_iter != seeds_end; ++seeds_iter)
	{
		SeedCoRegistration &seed_co_registration = *seeds_iter;

		// Save the clip-space scale/translate for the current *loose* spatial partition node.
		seed_co_registration.raster_frustum_to_seed_frustum_post_projection_scale =
				raster_frustum_to_loose_seed_frustum_post_projection_scale;
		seed_co_registration.raster_frustum_to_seed_frustum_post_projection_translate_x =
				raster_frustum_to_loose_seed_frustum_post_projection_translate_x;
		seed_co_registration.raster_frustum_to_seed_frustum_post_projection_translate_y =
				raster_frustum_to_loose_seed_frustum_post_projection_translate_y;

		// Add the current seed co-registration to the working list of its operation.
		const unsigned int operation_index = seed_co_registration.operation_index;
		operations_reduce_stage_lists[operation_index].reduce_stage_lists[reduce_stage_index].push_front(&seed_co_registration);
	}

	//
	// Iterate over the child quad tree nodes.
	//

	for (unsigned int child_y_offset = 0; child_y_offset < 2; ++child_y_offset)
	{
		for (unsigned int child_x_offset = 0; child_x_offset < 2; ++child_x_offset)
		{
			// See if there is a child node in the seed geometries spatial partition.
			const seed_geometries_spatial_partition_type::node_reference_type
					child_seed_geometries_spatial_partition_node =
							seed_geometries_spatial_partition_node.get_child_node(
									child_x_offset, child_y_offset);

			// No need to recurse into child node if no seed geometries in current *loose* sub-tree.
			if (!child_seed_geometries_spatial_partition_node)
			{
				continue;
			}

			group_seed_co_registrations_by_operation(
					co_registration_parameters,
					operations_reduce_stage_lists,
					child_seed_geometries_spatial_partition_node,
					// Child is the next reduce stage...
					GLUtils::QuadTreeClipSpaceTransform(
							raster_frustum_to_loose_seed_frustum_clip_space_transform,
							child_x_offset,
							child_y_offset),
					reduce_stage_index + 1);
		}
	}
}


void
GPlatesOpenGL::GLRasterCoRegistration::render_seed_geometries_to_reduce_pyramids(
		GLRenderer &renderer,
		const CoRegistrationParameters &co_registration_parameters,
		unsigned int operation_index,
		const GPlatesMaths::UnitVector3D &cube_face_centre,
		const GLTexture::shared_ptr_type &target_raster_texture,
		const GLTransform::non_null_ptr_to_const_type &target_raster_view_transform,
		const GLTransform::non_null_ptr_to_const_type &target_raster_projection_transform,
		std::vector<SeedCoRegistrationReduceStageLists> &operation_reduce_stage_lists,
		bool are_seed_geometries_bounded)
{
	//PROFILE_FUNC();

	SeedCoRegistrationReduceStageLists &operation_reduce_stage_list =
			operation_reduce_stage_lists[operation_index];

	// We start with reduce stage zero and increase until stage 'NUM_REDUCE_STAGES - 1' is reached.
	// This ensures that the reduce quad tree traversal fills up properly optimally and it also
	// keeps the reduce stage textures in sync with the reduce quad tree(s).
	unsigned int reduce_stage_index = 0;

	// Advance to the first *non-empty* reduce stage.
	while (operation_reduce_stage_list.reduce_stage_lists[reduce_stage_index].begin() ==
		operation_reduce_stage_list.reduce_stage_lists[reduce_stage_index].end())
	{
		++reduce_stage_index;
		if (reduce_stage_index == NUM_REDUCE_STAGES)
		{
			// There were no geometries to begin with.
			// Shouldn't really be able to get here since should only be called if have geometries.
			return;
		}
	}

	// Get the list of seed geometries for the current reduce stage to start things off.
	// NOTE: These iterators will change reduce stages as the reduce stage index changes during traversal.
	seed_co_registration_reduce_stage_list_type::iterator seed_co_registration_iter =
			operation_reduce_stage_list.reduce_stage_lists[reduce_stage_index].begin();
	seed_co_registration_reduce_stage_list_type::iterator seed_co_registration_end =
			operation_reduce_stage_list.reduce_stage_lists[reduce_stage_index].end();

	// Seed geometry render lists for each reduce stage.
	SeedCoRegistrationGeometryLists seed_co_registration_geometry_lists[NUM_REDUCE_STAGES];

	// Keep rendering into reduce quad trees until we've run out of seed geometries in all reduce stages.
	// Each reduce quad tree can handle TEXTURE_DIMENSION x TEXTURE_DIMENSION seed geometries.
	do
	{
		// Create a reduce quad tree to track the final co-registration results.
		// Each reduce quad tree maps to a TEXTURE_DIMENSION x TEXTURE_DIMENSION texture
		// and carries as many co-registration results as pixels in the texture.
		ReduceQuadTree::non_null_ptr_type reduce_quad_tree = ReduceQuadTree::create();

		// A set of reduce textures to generate/reduce co-registration results associated with 'reduce_quad_tree'.
		// All are null but will get initialised as needed during reduce quad tree traversal.
		// The last reduce stage will contain the final (reduced) results and the location of
		// each seed's result is determined by the reduce quad tree.
		boost::optional<GLTexture::shared_ptr_type> reduce_stage_textures[NUM_REDUCE_STAGES];

		// Offsets of reduce quad tree nodes relative to the root node.
		// This is used to generate appropriate scale/translate parameters for rendering seed geometries
		// into the reduce stage textures when keeping track of which quad-tree sub-viewport of a
		// reduce stage render target a seed geometry should be rendered into.
		unsigned int node_x_offsets_relative_to_root[NUM_REDUCE_STAGES];
		unsigned int node_y_offsets_relative_to_root[NUM_REDUCE_STAGES];
		// Initialise to zero to begin with.
		for (unsigned int n = 0; n < NUM_REDUCE_STAGES; ++n)
		{
			node_x_offsets_relative_to_root[n] = node_y_offsets_relative_to_root[n] = 0;
		}

		// Parameters used during traversal of reduce quad tree. Saves having to pass them as function
		// parameters during traversal (the ones that are not dependent on quad tree depth).
		RenderSeedCoRegistrationParameters render_parameters(
				co_registration_parameters.operations[operation_index],
				cube_face_centre,
				target_raster_texture,
				target_raster_view_transform,
				target_raster_projection_transform,
				*reduce_quad_tree,
				node_x_offsets_relative_to_root,
				node_y_offsets_relative_to_root,
				reduce_stage_textures,
				reduce_stage_index/*passed by *non-const* reference*/,
				operation_reduce_stage_list,
				seed_co_registration_iter/*passed by *non-const* reference*/,
				seed_co_registration_end/*passed by *non-const* reference*/,
				seed_co_registration_geometry_lists,
				are_seed_geometries_bounded);

		// Recursively render the seed geometries and perform reduction as we traverse back up
		// the quad tree to the root.
		const unsigned int num_new_leaf_nodes =
				render_seed_geometries_to_reduce_quad_tree_internal_node(
						renderer,
						render_parameters,
						reduce_quad_tree->get_root_node());

		// Keep track of leaf node numbers so we can determine when the reduce quad tree is full.
		reduce_quad_tree->get_root_node().accumulate_descendant_leaf_node_count(num_new_leaf_nodes);

		//
		// Queue the current reduce quad tree for read back from GPU to CPU.
		//

		// The final reduce stage texture should exist.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				reduce_stage_textures[NUM_REDUCE_STAGES - 1],
				GPLATES_ASSERTION_SOURCE);
		// We must be no partial results left in any other reduce stage textures.
		for (unsigned int n = 0; n < NUM_REDUCE_STAGES - 1; ++n)
		{
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					!reduce_stage_textures[n],
					GPLATES_ASSERTION_SOURCE);
		}

		// The reduce quad tree should not be empty.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				!reduce_quad_tree->empty(),
				GPLATES_ASSERTION_SOURCE);

		// If the reduce quad tree is *not* full then it means we must have finished.
		// If it is full then we also might have finished but it's more likely that
		// we need another reduce quad tree.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				reduce_quad_tree->get_root_node().is_sub_tree_full() ||
					// Finished ? ...
					reduce_stage_index == NUM_REDUCE_STAGES,
				GPLATES_ASSERTION_SOURCE);

		// Queue the results (stored in the final reduce texture).
		// This starts asynchronous read back of the texture to CPU memory via a pixel buffer.
		co_registration_parameters.d_results_queue.queue_reduce_pyramid_output(
				renderer,
				d_framebuffer_object,
				reduce_stage_textures[NUM_REDUCE_STAGES - 1].get(),
				reduce_quad_tree,
				co_registration_parameters.seed_feature_partial_results);
	}
	while (reduce_stage_index < NUM_REDUCE_STAGES); // While not yet finished with all reduce stages.
}


unsigned int
GPlatesOpenGL::GLRasterCoRegistration::render_seed_geometries_to_reduce_quad_tree_internal_node(
		GLRenderer &renderer,
		RenderSeedCoRegistrationParameters &render_params,
		ReduceQuadTreeInternalNode &reduce_quad_tree_internal_node)
{
	unsigned int num_new_leaf_nodes = 0;

	const unsigned int parent_reduce_stage_index = reduce_quad_tree_internal_node.get_reduce_stage_index();
	const unsigned int child_reduce_stage_index = parent_reduce_stage_index - 1;

	// Recurse into the child reduce quad tree nodes.
	for (unsigned int child_y_offset = 0; child_y_offset < 2; ++child_y_offset)
	{
		// Keep track of the location of the current child node relative to the root node.
		render_params.node_y_offsets_relative_to_root[child_reduce_stage_index] =
				(render_params.node_y_offsets_relative_to_root[parent_reduce_stage_index] << 1) +
						child_y_offset;

		for (unsigned int child_x_offset = 0; child_x_offset < 2; ++child_x_offset)
		{
			// Keep track of the location of the current child node relative to the root node.
			render_params.node_x_offsets_relative_to_root[child_reduce_stage_index] =
					(render_params.node_x_offsets_relative_to_root[parent_reduce_stage_index] << 1) +
							child_x_offset;

			// If the child layer is the leaf node layer...
			if (child_reduce_stage_index == 0)
			{
				//
				// Create a child leaf node and add the next seed co-registration to it.
				//

				// Remove a seed geometry from the list.
				SeedCoRegistration &seed_co_registration = *render_params.seed_co_registration_iter;
				++render_params.seed_co_registration_iter;

				// Create the child *leaf* node.
				// We don't use it now but we will later when we read back the results from GPU.
				render_params.reduce_quad_tree.create_child_leaf_node(
						reduce_quad_tree_internal_node,
						child_x_offset,
						child_y_offset,
						seed_co_registration);

				// Add the seed geometry to the list of point/outline/fill primitives to be rendered
				// depending on the seed geometry type.
				AddSeedCoRegistrationToGeometryLists add_geometry_to_list_visitor(
						render_params.seed_co_registration_geometry_lists[render_params.reduce_stage_index],
						seed_co_registration);
				seed_co_registration.geometry.accept_visitor(add_geometry_to_list_visitor);

				//
				// Determine the quad-tree sub-viewport of the reduce stage render target that this
				// seed geometry will be rendered into.
				//

				const unsigned int node_x_offset_relative_to_reduce_stage =
						render_params.node_x_offsets_relative_to_root[0/*child_reduce_stage_index*/] -
							(render_params.node_x_offsets_relative_to_root[render_params.reduce_stage_index]
									<< render_params.reduce_stage_index);
				const unsigned int node_y_offset_relative_to_reduce_stage =
						render_params.node_y_offsets_relative_to_root[0/*child_reduce_stage_index*/] -
							(render_params.node_y_offsets_relative_to_root[render_params.reduce_stage_index]
									<< render_params.reduce_stage_index);

				const double reduce_stage_inverse_scale = 1.0 / (1 << render_params.reduce_stage_index);

				// Record the transformation from clip-space of the (loose or non-loose - both source
				// code paths go through here) seed frustum to the sub-viewport of render target to
				// render seed geometry into.
				//
				// This code mirrors that of the *inverse* transform in class GLUtils::QuadTreeClipSpaceTransform.
				//
				// NOTE: We use the 'inverse' since takes the clip-space range [-1,1] covering the
				// (loose or non-loose) seed frustum and makes it cover the render target frustum -
				// so this is descendant -> ancestor (rather than ancestor -> descendant).
				//
				// NOTE: Even though both loose and non-loose source code paths come through here
				// we do *not* use the *loose* inverse transform because the notion of looseness
				// only applies when transforming from loose *raster* frustum to loose seed frustum
				// (also the other path is regular raster frustum to regular seed frustum) and this
				// is the transform from seed frustum to *render target* frustum.
				seed_co_registration.seed_frustum_to_render_target_post_projection_scale = reduce_stage_inverse_scale;
				seed_co_registration.seed_frustum_to_render_target_post_projection_translate_x =
						-1 + reduce_stage_inverse_scale * (1 + 2 * node_x_offset_relative_to_reduce_stage);
				seed_co_registration.seed_frustum_to_render_target_post_projection_translate_y =
						-1 + reduce_stage_inverse_scale * (1 + 2 * node_y_offset_relative_to_reduce_stage);


				// Keep track of leaf node numbers so we can determine when sub-trees fill up.
				++num_new_leaf_nodes;
				reduce_quad_tree_internal_node.accumulate_descendant_leaf_node_count(1);
			}
			else // Child node is an internal node (not a leaf node)...
			{
				// Create a child *internal* node.
				ReduceQuadTreeInternalNode &child_reduce_quad_tree_internal_node =
						render_params.reduce_quad_tree.create_child_internal_node(
									reduce_quad_tree_internal_node,
									child_x_offset,
									child_y_offset);

				// Recurse into the child reduce quad tree *internal* node.
				const unsigned int num_new_leaf_nodes_from_child =
						render_seed_geometries_to_reduce_quad_tree_internal_node(
								renderer,
								render_params,
								child_reduce_quad_tree_internal_node);

				// Keep track of leaf node numbers so we can determine when sub-trees fill up.
				num_new_leaf_nodes += num_new_leaf_nodes_from_child;
				reduce_quad_tree_internal_node.accumulate_descendant_leaf_node_count(num_new_leaf_nodes_from_child);
			}

			// If the child sub-tree just visited has geometries in its render list then we need to
			// render that list of points/outlines/fills.
			//
			// The reason for the render list (instead of rendering each seed geometry as we
			// encounter it) is to minimise the number of draw calls (improved OpenGL batching) -
			// each draw call submission to OpenGL is quite expensive (in CPU cycles) especially
			// if we're rendering thousands or hundreds of thousands of seed geometries in which
			// case it could get quite overwhelming.
			if (!render_params.seed_co_registration_geometry_lists[child_reduce_stage_index].empty())
			{
				// If a reduce stage texture currently exists then it means it contains
				// partial results (is waiting to be fully filled before being reduced and released).
				// Which means it shouldn't be cleared before rendering more results into it.
				bool clear_reduce_texture = false;

				// Get the reduce stage texture.
				if (!render_params.reduce_stage_textures[child_reduce_stage_index])
				{
					// Acquire a reduce texture.
					render_params.reduce_stage_textures[child_reduce_stage_index] =
							acquire_rgba_float_texture(renderer);

					// Clear acquired texture - there are no partial results.
					clear_reduce_texture = true;
				}

				// Render the geometries into the reduce stage texture.
				render_seed_geometries_in_reduce_stage_render_list(
						renderer,
						render_params.reduce_stage_textures[child_reduce_stage_index].get(),
						clear_reduce_texture,
						render_params.operation,
						render_params.cube_face_centre,
						render_params.target_raster_texture,
						render_params.target_raster_view_transform,
						render_params.target_raster_projection_transform,
						render_params.seed_co_registration_geometry_lists[child_reduce_stage_index],
						render_params.are_seed_geometries_bounded);

				// We've finished rendering the lists so clear them for the next batch in the current reduce stage.
				render_params.seed_co_registration_geometry_lists[child_reduce_stage_index].clear();
			}

			// If there's a child reduce stage texture then it means we need to perform a 2x2 -> 1x1
			// reduction of the child reduce stage texture into our (parent) reduce stage texture.
			if (render_params.reduce_stage_textures[child_reduce_stage_index])
			{
				// If a parent reduce stage texture currently exists then it means it contains
				// partial results (is waiting to be fully filled before being reduced and released).
				// Which means it shouldn't be cleared before reducing more results into it.
				bool clear_parent_reduce_texture = false;

				// Get the parent reduce stage texture.
				if (!render_params.reduce_stage_textures[parent_reduce_stage_index])
				{
					// Acquire a parent reduce texture.
					render_params.reduce_stage_textures[parent_reduce_stage_index] =
							acquire_rgba_float_texture(renderer);

					// Clear acquired texture - there are no partial results.
					clear_parent_reduce_texture = true;
				}

				// Do the 2x2 -> 1x1 reduction.
				//
				// NOTE: If we ran out of geometries before the child sub-tree could be filled then
				// this could be a reduction of *less* than TEXTURE_DIMENSION x TEXTURE_DIMENSION pixels.
				render_reduction_of_reduce_stage(
						renderer,
						render_params.operation,
						reduce_quad_tree_internal_node,
						child_x_offset,
						child_y_offset,
						clear_parent_reduce_texture,
						// The destination (1x1) stage...
						render_params.reduce_stage_textures[parent_reduce_stage_index].get(),
						// The source (2x2) stage...
						render_params.reduce_stage_textures[child_reduce_stage_index].get());

				// The child texture has been reduced so we can release it for re-use.
				// This also signals the next acquire to clear the texture before re-using.
				render_params.reduce_stage_textures[child_reduce_stage_index] = boost::none;
			}

			// If there are no more seed geometries in *any* reduce stages then return early
			// (all the way back to the root node without visiting any more sub-trees - but we still
			// perform any unflushed rendering and reduction on the way back up to the root though).
			if (render_params.reduce_stage_index == NUM_REDUCE_STAGES)
			{
				return num_new_leaf_nodes;
			}

			// Advance to the next *non-empty* reduce stage if there are no more seed geometries
			// in the current reduce stage.
			while (render_params.seed_co_registration_iter == render_params.seed_co_registration_end)
			{
				++render_params.reduce_stage_index;
				if (render_params.reduce_stage_index == NUM_REDUCE_STAGES)
				{
					// No seed geometries left in any reduce stages - we're finished.
					return num_new_leaf_nodes;
				}

				// Change the seed co-registration iterators to refer to the next reduce stage.
				render_params.seed_co_registration_iter =
						render_params.operation_reduce_stage_list.reduce_stage_lists[
								render_params.reduce_stage_index].begin();
				render_params.seed_co_registration_end =
						render_params.operation_reduce_stage_list.reduce_stage_lists[
								render_params.reduce_stage_index].end();
			}
		}
	}

	return num_new_leaf_nodes;
}


void
GPlatesOpenGL::GLRasterCoRegistration::render_seed_geometries_in_reduce_stage_render_list(
		GLRenderer &renderer,
		const GLTexture::shared_ptr_type &reduce_stage_texture,
		bool clear_reduce_stage_texture,
		const Operation &operation,
		const GPlatesMaths::UnitVector3D &cube_face_centre,
		const GLTexture::shared_ptr_type &target_raster_texture,
		const GLTransform::non_null_ptr_to_const_type &target_raster_view_transform,
		const GLTransform::non_null_ptr_to_const_type &target_raster_projection_transform,
		const SeedCoRegistrationGeometryLists &geometry_lists,
		bool are_seed_geometries_bounded)
{
	//PROFILE_FUNC();

	//
	// Set up for streaming vertices/indices into region-of-interest vertex/index buffers.
	//

	// Used when mapping the vertex/index buffers for streaming.
	GLBuffer::MapBufferScope map_vertex_element_buffer_scope(
			renderer,
			*d_streaming_vertex_element_buffer->get_buffer(),
			GLBuffer::TARGET_ELEMENT_ARRAY_BUFFER);
	GLBuffer::MapBufferScope map_vertex_buffer_scope(
			renderer,
			*d_streaming_vertex_buffer->get_buffer(),
			GLBuffer::TARGET_ARRAY_BUFFER);

	//
	// Prepare for rendering into the region-of-interest mask fixed-point texture.
	//

	// Make sure we leave the OpenGL state the way it was.
	GLRenderer::StateBlockScope save_restore_state_region_of_interest_mask(
			renderer,
			// We're rendering to a render target so reset to the default OpenGL state...
			true/*reset_to_default_state*/);

	// Acquire a fixed-point texture to render the region-of-interest masks into.
	//
	// The reason for acquiring a separate fixed-point texture for masking is the polygon fill is
	// implemented with alpha-blending and alpha-blending with floating-point textures is
	// unsupported on a lot of hardware - so we use a fixed-point texture instead.
	GLTexture::shared_ptr_type region_of_interest_mask_texture = acquire_rgba_fixed_texture(renderer);

	// Render to the fixed-point region-of-interest mask texture.
	d_framebuffer_object->gl_attach(
			renderer, GL_TEXTURE_2D, region_of_interest_mask_texture, 0/*level*/, GL_COLOR_ATTACHMENT0_EXT);
	renderer.gl_bind_frame_buffer(d_framebuffer_object);

	// Render to the entire regions-of-interest texture - same dimensions as reduce stage textures.
	renderer.gl_viewport(0, 0, TEXTURE_DIMENSION, TEXTURE_DIMENSION);

	// Clear the region-of-interest mask fixed-point texture.
	// Clear colour to all zeros - only those areas inside the regions-of-interest will be non-zero.
	renderer.gl_clear_color();
	renderer.gl_clear(GL_COLOR_BUFFER_BIT); // Clear only the colour buffer.

	// All seed geometries will use the same view/projection matrices and simply make
	// post-projection adjustments in the vertex shader as needed (using vertex data - attributes).
	// NOTE: This greatly minimises the number of OpenGL calls we need to make (each OpenGL call
	// can be quite expensive in terms of CPU cost - very little GPU cost though) since it avoids
	// per-seed-geometry OpenGL calls and there could be *lots* of seed geometries.
	renderer.gl_load_matrix(GL_MODELVIEW, target_raster_view_transform->get_matrix());
	renderer.gl_load_matrix(GL_PROJECTION, target_raster_projection_transform->get_matrix());

	//
	// Render the fill, if specified by the current operation, of all seed geometries.
	// This means geometries that are polygons.
	//
	// NOTE: We do this before rendering point and line regions-of-interest because the method
	// of rendering polygons interiors requires a clear framebuffer to start with. Rendering the
	// point and line regions-of-interest can then accumulate into the final polygon-fill result.
	//

	// If the operation specified fill for polygon interiors then that will be in addition to the regular
	// region-of-interest fill (ie, distance from polygon outline) around a polygon's line (arc) segments.
	if (operation.d_fill_polygons)
	{
		render_fill_region_of_interest_geometries(
				renderer,
				map_vertex_element_buffer_scope,
				map_vertex_buffer_scope,
				geometry_lists);
	}

	//
	// Render the line-segment regions-of-interest of all seed geometries.
	// This means geometries that are polylines and polygons.
	//

	// We only need to render the region-of-interest geometries if the ROI radius is non-zero.
	// If it's zero then only rendering of single pixel points and single pixel-wide lines is necessary.
	if (GPlatesMaths::real_t(operation.d_region_of_interest_radius) > 0)
	{
		if (are_seed_geometries_bounded)
		{
			// Render the line region-of-interest geometries (quads).
			// The seed geometry is bounded by a loose cube quad tree tile.
			render_bounded_line_region_of_interest_geometries(
					renderer,
					map_vertex_element_buffer_scope,
					map_vertex_buffer_scope,
					geometry_lists,
					operation.d_region_of_interest_radius);
		}
		else
		{
			// Render the line region-of-interest geometries (meshes).
			// The seed geometry is *not* bounded by a loose cube quad tree tile.
			render_unbounded_line_region_of_interest_geometries(
					renderer,
					map_vertex_element_buffer_scope,
					map_vertex_buffer_scope,
					geometry_lists,
					operation.d_region_of_interest_radius);
		}

		//
		// Render the point regions-of-interest of all seed geometries.
		// This means geometries that are points, multipoints, polylines and polygons.
		//

		if (are_seed_geometries_bounded)
		{
			// Render the point region-of-interest geometries (quads).
			// The seed geometry is bounded by a loose cube quad tree tile.
			render_bounded_point_region_of_interest_geometries(
					renderer,
					map_vertex_element_buffer_scope,
					map_vertex_buffer_scope,
					geometry_lists,
					operation.d_region_of_interest_radius);
		}
		else
		{
			// Render the point region-of-interest geometries (meshes).
			// The seed geometry is *not* bounded by a loose cube quad tree tile.
			render_unbounded_point_region_of_interest_geometries(
					renderer,
					map_vertex_element_buffer_scope,
					map_vertex_buffer_scope,
					geometry_lists,
					operation.d_region_of_interest_radius);
		}
	}

	// As an extra precaution we also render the line region-of-interest geometries as lines (not quads) of
	// line width 1 (with no anti-aliasing). This is done in case the region-of-interest radius is so
	// small that the line quads fall between pixels in the render target because the lines are so thin.
	//
	// UPDATE: This is also required for the case when a zero region-of-interest radius is specified
	// which can be used to indicate point-sampling (along the line) rather than area sampling.
	render_single_pixel_wide_line_region_of_interest_geometries(
			renderer,
			map_vertex_element_buffer_scope,
			map_vertex_buffer_scope,
			geometry_lists);

	// As an extra precaution we also render the point region-of-interest geometries as points (not quads) of
	// point size 1 (with no anti-aliasing). This is done in case the region-of-interest radius is so
	// small that the point quads fall between pixels in the render target because the points are so small.
	//
	// UPDATE: This is also required for the case when a zero region-of-interest radius is specified
	// which can be used to indicate point-sampling rather than area sampling.
	render_single_pixel_size_point_region_of_interest_geometries(
			renderer,
			map_vertex_element_buffer_scope,
			map_vertex_buffer_scope,
			geometry_lists);

	//debug_fixed_point_render_target(renderer, "region_of_interest_mask");

	//
	// Now that we've generated the region-of-interest masks we can copy seed sub-viewport sections
	// of the target raster texture into the reduce stage texture with region-of-interest masking.
	//
	// Prepare for rendering into the reduce stage floating-point texture.
	//

	// Render to the floating-point reduce stage texture.
	d_framebuffer_object->gl_attach(
			renderer, GL_TEXTURE_2D, reduce_stage_texture, 0/*level*/, GL_COLOR_ATTACHMENT0_EXT);
	renderer.gl_bind_frame_buffer(d_framebuffer_object);

	// No need to change the viewport - it's already TEXTURE_DIMENSION x TEXTURE_DIMENSION;

	// If the reduce stage texture does not contain partial results then it'll need to be cleared.
	// This happens when starting afresh with a newly acquired reduce stage texture.
	if (clear_reduce_stage_texture)
	{
		// Clear colour to all zeros - this means pixels outside the regions-of-interest will
		// have coverage values of zero (causing them to not contribute to the co-registration result).
		renderer.gl_clear_color();
		renderer.gl_clear(GL_COLOR_BUFFER_BIT); // Clear only the colour buffer.
	}

	// We use the same view and projection matrices (set for the target raster) but,
	// in the vertex shader, we don't transform the vertex position using them.
	// This is because the vertices will be in screen-space (range [-1,1]) and will only need
	// a translate/scale adjustment of the 'x' and 'y' components to effectively map its texture
	// coordinates to the appropriate sub-viewport of the target raster texture and map its position
	// to the appropriate sub-viewport of the render target.
	// And these adjustments will be transferred to the vertex shader using vertex data (attributes).
	//
	// We do however use the inverse view and projection matrices, provided as built-in uniform
	// shader constants from our GL_MODELVIEW and GL_PROJECTION transforms courtesy of OpenGL, to
	// inverse transform from screen-space back to view space so we can then perform a dot-product
	// of the (normalised) view space position with the cube face centre in order to adjust the
	// raster coverage to counteract the distortion of a pixel's area on the surface of the globe.
	// Near the face corners a cube map pixel projects to a smaller area on the globe than at the
	// cube face centre. This can affect area-weighted operations like mean and standard deviation
	// which assume each pixel projects to the same area on the globe. The dot product or cosine
	// adjustment counteracts that (assuming each pixel is infinitesimally small - which is close
	// enough for small pixels).
	//
	// We could have done the cube map distortion adjustment when rendering the region-of-interest
	// geometries but that's done to a fixed-point (8-bit per component) render target and the
	// lack of precision would introduce noise into the co-registration operation (eg, mean, std-dev).
	// So we do it with the floating-point render target here instead.
	//

	//
	// Render the seed sub-viewports of the regions-of-interest of all seed geometries.
	// This means geometries that are points, multipoints, polylines and polygons.
	//

	// Copy the target raster to the reduce stage texture with region-of-interest masking.
	mask_target_raster_with_regions_of_interest(
			renderer,
			operation,
			cube_face_centre,
			target_raster_texture,
			region_of_interest_mask_texture,
			map_vertex_element_buffer_scope,
			map_vertex_buffer_scope,
			geometry_lists);

	//debug_floating_point_render_target(
	//		renderer, "region_of_interest_masked_raster", false/*coverage_is_in_green_channel*/);
}


void
GPlatesOpenGL::GLRasterCoRegistration::render_bounded_point_region_of_interest_geometries(
		GLRenderer &renderer,
		GLBuffer::MapBufferScope &map_vertex_element_buffer_scope,
		GLBuffer::MapBufferScope &map_vertex_buffer_scope,
		const SeedCoRegistrationGeometryLists &geometry_lists,
		const double &region_of_interest_radius)
{
	//PROFILE_FUNC();

	//
	// Some uniform shader parameters.
	//
	// NOTE: The region-of-interest angular extent will be less than 90 degrees because otherwise
	// we wouldn't be here - the region-of-interest-expanded bounding regions around each seed
	// geometry fits in the spatial partition (in one 'loose' cube face of the partition) - and it's
	// not possible for those half-extents to exceed 90 degrees (it shouldn't even get close to 90 degrees).
	// So we can use the small region-of-interest angle shader program that should be accurate since
	// we're not going near 90 degrees and we also don't have to worry about an undefined 'tan' at 90 degrees.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			// Let's make sure it doesn't even get close to 90 degrees...
			region_of_interest_radius < 0.95 * GPlatesMaths::HALF_PI,
			GPLATES_ASSERTION_SOURCE);

	// Bind the shader program for rendering point regions-of-interest with smaller region-of-interest angles.
	renderer.gl_bind_program_object(d_render_points_of_seed_geometries_with_small_roi_angle_program_object);

	const double tan_region_of_interest_angle = std::tan(region_of_interest_radius);
	const double tan_squared_region_of_interest_angle = tan_region_of_interest_angle * tan_region_of_interest_angle;

	// Set the region-of-interest radius.
	d_render_points_of_seed_geometries_with_small_roi_angle_program_object->gl_uniform1f(
			renderer, "tan_squared_region_of_interest_angle", tan_squared_region_of_interest_angle);

	// Bind the point region-of-interest vertex array.
	d_point_region_of_interest_vertex_array->gl_bind(renderer);

	// For streaming PointRegionOfInterestVertex vertices.
	point_region_of_interest_stream_primitives_type point_stream;
	point_region_of_interest_stream_primitives_type::StreamTarget point_stream_target(point_stream);

	// Start streaming point region-of-interest geometries.
	begin_vertex_array_streaming<PointRegionOfInterestVertex>(
			renderer,
			point_stream_target,
			map_vertex_element_buffer_scope,
			map_vertex_buffer_scope);

	point_region_of_interest_stream_primitives_type::Primitives point_stream_quads(point_stream);

	// Iterate over the point geometries.
	seed_co_registration_points_list_type::const_iterator points_iter = geometry_lists.points_list.begin();
	seed_co_registration_points_list_type::const_iterator points_end = geometry_lists.points_list.end();
	for ( ; points_iter != points_end; ++points_iter)
	{
		const SeedCoRegistration &seed_co_registration = *points_iter;

		// We're currently traversing the 'GPlatesMaths::PointOnSphere' list so the dynamic_cast should not fail.
		const GPlatesMaths::PointOnSphere &point_on_sphere =
				dynamic_cast<const GPlatesMaths::PointOnSphere &>(seed_co_registration.geometry);

		// Most of the vertex data is the same for all vertices in the seed geometry.
		PointRegionOfInterestVertex vertex;
		vertex.initialise_seed_geometry_constants(seed_co_registration);

		render_bounded_point_region_of_interest_geometry(
				renderer,
				map_vertex_element_buffer_scope,
				map_vertex_buffer_scope,
				point_stream_target,
				point_stream_quads,
				point_on_sphere.position_vector(),
				vertex,
				tan_region_of_interest_angle);
	}

	// Iterate over the multipoint geometries.
	seed_co_registration_multi_points_list_type::const_iterator multi_points_iter = geometry_lists.multi_points_list.begin();
	seed_co_registration_multi_points_list_type::const_iterator multi_points_end = geometry_lists.multi_points_list.end();
	for ( ; multi_points_iter != multi_points_end; ++multi_points_iter)
	{
		const SeedCoRegistration &seed_co_registration = *multi_points_iter;

		// We're currently traversing the 'GPlatesMaths::MultiPointOnSphere' list so the dynamic_cast should not fail.
		const GPlatesMaths::MultiPointOnSphere &multi_point_on_sphere =
				dynamic_cast<const GPlatesMaths::MultiPointOnSphere &>(seed_co_registration.geometry);

		// Most of the vertex data is the same for all vertices in the seed geometry.
		PointRegionOfInterestVertex vertex;
		vertex.initialise_seed_geometry_constants(seed_co_registration);

		// Iterate over the points of the current multipoint.
		GPlatesMaths::MultiPointOnSphere::const_iterator multi_point_iter = multi_point_on_sphere.begin();
		GPlatesMaths::MultiPointOnSphere::const_iterator multi_point_end = multi_point_on_sphere.end();
		for ( ; multi_point_iter != multi_point_end; ++multi_point_iter)
		{
			const GPlatesMaths::PointOnSphere &point = *multi_point_iter;

			render_bounded_point_region_of_interest_geometry(
					renderer,
					map_vertex_element_buffer_scope,
					map_vertex_buffer_scope,
					point_stream_target,
					point_stream_quads,
					point.position_vector(),
					vertex,
					tan_region_of_interest_angle);
		}
	}

	// Iterate over the polyline geometries.
	seed_co_registration_polylines_list_type::const_iterator polylines_iter = geometry_lists.polylines_list.begin();
	seed_co_registration_polylines_list_type::const_iterator polylines_end = geometry_lists.polylines_list.end();
	for ( ; polylines_iter != polylines_end; ++polylines_iter)
	{
		const SeedCoRegistration &seed_co_registration = *polylines_iter;

		// We're currently traversing the 'GPlatesMaths::PolylineOnSphere' list so the dynamic_cast should not fail.
		const GPlatesMaths::PolylineOnSphere &polyline_on_sphere =
				dynamic_cast<const GPlatesMaths::PolylineOnSphere &>(seed_co_registration.geometry);

		// Most of the vertex data is the same for all vertices in the seed geometry.
		PointRegionOfInterestVertex vertex;
		vertex.initialise_seed_geometry_constants(seed_co_registration);

		// Iterate over the points of the current polyline.
		GPlatesMaths::PolylineOnSphere::vertex_const_iterator polyline_points_iter = polyline_on_sphere.vertex_begin();
		GPlatesMaths::PolylineOnSphere::vertex_const_iterator polyline_points_end = polyline_on_sphere.vertex_end();
		for ( ; polyline_points_iter != polyline_points_end; ++polyline_points_iter)
		{
			const GPlatesMaths::PointOnSphere &point = *polyline_points_iter;

			// Render the point region-of-interest geometry filling in the vertex data attributes
			// that are *not* constant across the seed geometry.
			render_bounded_point_region_of_interest_geometry(
					renderer,
					map_vertex_element_buffer_scope,
					map_vertex_buffer_scope,
					point_stream_target,
					point_stream_quads,
					point.position_vector(),
					vertex,
					tan_region_of_interest_angle);
		}
	}

	// Iterate over the polygon geometries.
	seed_co_registration_polygons_list_type::const_iterator polygons_iter = geometry_lists.polygons_list.begin();
	seed_co_registration_polygons_list_type::const_iterator polygons_end = geometry_lists.polygons_list.end();
	for ( ; polygons_iter != polygons_end; ++polygons_iter)
	{
		const SeedCoRegistration &seed_co_registration = *polygons_iter;

		// We're currently traversing the 'GPlatesMaths::PolygonOnSphere' list so the dynamic_cast should not fail.
		const GPlatesMaths::PolygonOnSphere &polygon_on_sphere =
				dynamic_cast<const GPlatesMaths::PolygonOnSphere &>(seed_co_registration.geometry);

		// Most of the vertex data is the same for all vertices in the seed geometry.
		PointRegionOfInterestVertex vertex;
		vertex.initialise_seed_geometry_constants(seed_co_registration);

		// Iterate over the points of the current polygon.
		GPlatesMaths::PolygonOnSphere::vertex_const_iterator polygon_points_iter = polygon_on_sphere.vertex_begin();
		GPlatesMaths::PolygonOnSphere::vertex_const_iterator polygon_points_end = polygon_on_sphere.vertex_end();
		for ( ; polygon_points_iter != polygon_points_end; ++polygon_points_iter)
		{
			const GPlatesMaths::PointOnSphere &point = *polygon_points_iter;

			// Render the point region-of-interest geometry filling in the vertex data attributes
			// that are *not* constant across the seed geometry.
			render_bounded_point_region_of_interest_geometry(
					renderer,
					map_vertex_element_buffer_scope,
					map_vertex_buffer_scope,
					point_stream_target,
					point_stream_quads,
					point.position_vector(),
					vertex,
					tan_region_of_interest_angle);
		}
	}

	// Stop streaming point region-of-interest geometries so we can render the last batch.
	end_vertex_array_streaming<PointRegionOfInterestVertex>(
			renderer,
			point_stream_target,
			map_vertex_element_buffer_scope,
			map_vertex_buffer_scope);

	// Render the last batch of streamed point region-of-interest geometries (if any).
	render_vertex_array_stream<PointRegionOfInterestVertex>(
			renderer,
			point_stream_target,
			d_point_region_of_interest_vertex_array,
			GL_TRIANGLES);
}


void
GPlatesOpenGL::GLRasterCoRegistration::render_bounded_point_region_of_interest_geometry(
		GLRenderer &renderer,
		GLBuffer::MapBufferScope &map_vertex_element_buffer_scope,
		GLBuffer::MapBufferScope &map_vertex_buffer_scope,
		point_region_of_interest_stream_primitives_type::StreamTarget &point_stream_target,
		point_region_of_interest_stream_primitives_type::Primitives &point_stream_quads,
		const GPlatesMaths::UnitVector3D &point,
		PointRegionOfInterestVertex &vertex,
		const double &tan_region_of_interest_angle)
{
	//PROFILE_FUNC();

	// There are four vertices for the current point (each point gets a quad) and
	// two triangles (three indices each).
	if (!point_stream_quads.begin_primitive(
			4/*max_num_vertices*/,
			6/*max_num_vertex_elements*/))
	{
		// There's not enough vertices or indices so render what we have so far and
		// obtain new stream buffers.
		suspend_render_resume_vertex_array_streaming<PointRegionOfInterestVertex>(
				renderer,
				point_stream_target,
				map_vertex_element_buffer_scope,
				map_vertex_buffer_scope,
				d_point_region_of_interest_vertex_array,
				GL_TRIANGLES);
	}

	vertex.point_centre[0] = point.x().dval();
	vertex.point_centre[1] = point.y().dval();
	vertex.point_centre[2] = point.z().dval();
	vertex.tangent_frame_weights[2] = 1;

	vertex.tangent_frame_weights[0] = -tan_region_of_interest_angle;
	vertex.tangent_frame_weights[1] = -tan_region_of_interest_angle;
	point_stream_quads.add_vertex(vertex);

	vertex.tangent_frame_weights[0] = -tan_region_of_interest_angle;
	vertex.tangent_frame_weights[1] = tan_region_of_interest_angle;
	point_stream_quads.add_vertex(vertex);

	vertex.tangent_frame_weights[0] = tan_region_of_interest_angle;
	vertex.tangent_frame_weights[1] = tan_region_of_interest_angle;
	point_stream_quads.add_vertex(vertex);

	vertex.tangent_frame_weights[0] = tan_region_of_interest_angle;
	vertex.tangent_frame_weights[1] = -tan_region_of_interest_angle;
	point_stream_quads.add_vertex(vertex);

	//
	// Add the quad triangles.
	//

	point_stream_quads.add_vertex_element(0);
	point_stream_quads.add_vertex_element(1);
	point_stream_quads.add_vertex_element(2);

	point_stream_quads.add_vertex_element(0);
	point_stream_quads.add_vertex_element(2);
	point_stream_quads.add_vertex_element(3);

	point_stream_quads.end_primitive();
}


void
GPlatesOpenGL::GLRasterCoRegistration::render_unbounded_point_region_of_interest_geometries(
		GLRenderer &renderer,
		GLBuffer::MapBufferScope &map_vertex_element_buffer_scope,
		GLBuffer::MapBufferScope &map_vertex_buffer_scope,
		const SeedCoRegistrationGeometryLists &geometry_lists,
		const double &region_of_interest_radius)
{
	//PROFILE_FUNC();

	//
	// Some uniform shader parameters.
	//

	// The seed geometries are unbounded which means they were too big to fit into a cube face of
	// the spatial partition which means the region-of-interest angle could be large or small.
	// Actually for seed geometries that are unbounded *points* the angles cannot be small otherwise
	// they would be bounded - however we'll go ahead and test for large and small angles anyway.

	const double cos_region_of_interest_angle = std::cos(region_of_interest_radius);
	const double sin_region_of_interest_angle = std::sin(region_of_interest_radius);

	// For smaller angles (less than 45 degrees) use a shader program that's accurate for very small angles.
	if (region_of_interest_radius < GPlatesMaths::PI / 4)
	{
		// Bind the shader program for rendering point regions-of-interest.
		renderer.gl_bind_program_object(d_render_points_of_seed_geometries_with_small_roi_angle_program_object);

		// Note that 'tan' is undefined at 90 degrees but we're safe since we're restricted to 45 degrees or less.
		const double tan_region_of_interest_angle = std::tan(region_of_interest_radius);
		const double tan_squared_region_of_interest_angle = tan_region_of_interest_angle * tan_region_of_interest_angle;

		// Set the region-of-interest radius.
		d_render_points_of_seed_geometries_with_small_roi_angle_program_object->gl_uniform1f(
				renderer, "tan_squared_region_of_interest_angle", tan_squared_region_of_interest_angle);
	}
	else // Use a shader program that's accurate for angles very near 90 degrees...
	{
		// Bind the shader program for rendering point regions-of-interest.
		renderer.gl_bind_program_object(d_render_points_of_seed_geometries_with_large_roi_angle_program_object);

		// Set the region-of-interest radius.
		d_render_points_of_seed_geometries_with_large_roi_angle_program_object->gl_uniform1f(
				renderer, "cos_region_of_interest_angle", cos_region_of_interest_angle);
	}

	// Tangent frame weights used for each 'point' to determine position of a point's fan mesh vertices.
	// Aside from the factor of sqrt(2), these weights place the fan mesh vertices on the unit sphere.
	// The sqrt(2) is used when the region-of-interest is smaller than a hemisphere - this factor ensures
	// the fan mesh covers at least the region-of-interest when the fan mesh is projected onto the globe.
	// This effectively moves the vertices (except the fan apex vertex) off the sphere - picture the
	// point as the north pole and the fan mesh is a pyramid with apex at north pole and quad base lies
	// on the small circle plane at a particular latitude (less than 90 degrees from north pole) - the
	// factor of sqrt(2) ensures the quad base touches the sphere (small circle) at the midpoints of the
	// four quad edges - so if you project this pyramid onto the sphere then it will completely
	// cover the entire upper latitude region (and a bit more near the base quad corners).
	const double centre_point_weight = cos_region_of_interest_angle;
	const double tangent_weight =
			(cos_region_of_interest_angle > 0)
			? std::sqrt(2.0) * sin_region_of_interest_angle
			: sin_region_of_interest_angle;

	// Bind the point region-of-interest vertex array.
	d_point_region_of_interest_vertex_array->gl_bind(renderer);

	// For streaming PointRegionOfInterestVertex vertices.
	point_region_of_interest_stream_primitives_type point_stream;
	point_region_of_interest_stream_primitives_type::StreamTarget point_stream_target(point_stream);

	// Start streaming point region-of-interest geometries.
	begin_vertex_array_streaming<PointRegionOfInterestVertex>(
			renderer,
			point_stream_target,
			map_vertex_element_buffer_scope,
			map_vertex_buffer_scope);

	point_region_of_interest_stream_primitives_type::Primitives point_stream_meshes(point_stream);

	// Iterate over the point geometries.
	seed_co_registration_points_list_type::const_iterator points_iter = geometry_lists.points_list.begin();
	seed_co_registration_points_list_type::const_iterator points_end = geometry_lists.points_list.end();
	for ( ; points_iter != points_end; ++points_iter)
	{
		const SeedCoRegistration &seed_co_registration = *points_iter;

		// We're currently traversing the 'GPlatesMaths::PointOnSphere' list so the dynamic_cast should not fail.
		const GPlatesMaths::PointOnSphere &point_on_sphere =
				dynamic_cast<const GPlatesMaths::PointOnSphere &>(seed_co_registration.geometry);

		// Most of the vertex data is the same for all vertices in the seed geometry.
		PointRegionOfInterestVertex vertex;
		vertex.initialise_seed_geometry_constants(seed_co_registration);

		render_unbounded_point_region_of_interest_geometry(
				renderer,
				map_vertex_element_buffer_scope,
				map_vertex_buffer_scope,
				point_stream_target,
				point_stream_meshes,
				point_on_sphere.position_vector(),
				vertex,
				centre_point_weight,
				tangent_weight);
	}

	// Iterate over the multipoint geometries.
	seed_co_registration_multi_points_list_type::const_iterator multi_points_iter = geometry_lists.multi_points_list.begin();
	seed_co_registration_multi_points_list_type::const_iterator multi_points_end = geometry_lists.multi_points_list.end();
	for ( ; multi_points_iter != multi_points_end; ++multi_points_iter)
	{
		const SeedCoRegistration &seed_co_registration = *multi_points_iter;

		// We're currently traversing the 'GPlatesMaths::MultiPointOnSphere' list so the dynamic_cast should not fail.
		const GPlatesMaths::MultiPointOnSphere &multi_point_on_sphere =
				dynamic_cast<const GPlatesMaths::MultiPointOnSphere &>(seed_co_registration.geometry);

		// Most of the vertex data is the same for all vertices in the seed geometry.
		PointRegionOfInterestVertex vertex;
		vertex.initialise_seed_geometry_constants(seed_co_registration);

		// Iterate over the points of the current multipoint.
		GPlatesMaths::MultiPointOnSphere::const_iterator multi_point_iter = multi_point_on_sphere.begin();
		GPlatesMaths::MultiPointOnSphere::const_iterator multi_point_end = multi_point_on_sphere.end();
		for ( ; multi_point_iter != multi_point_end; ++multi_point_iter)
		{
			const GPlatesMaths::PointOnSphere &point = *multi_point_iter;

			render_unbounded_point_region_of_interest_geometry(
					renderer,
					map_vertex_element_buffer_scope,
					map_vertex_buffer_scope,
					point_stream_target,
					point_stream_meshes,
					point.position_vector(),
					vertex,
					centre_point_weight,
					tangent_weight);
		}
	}

	// Iterate over the polyline geometries.
	seed_co_registration_polylines_list_type::const_iterator polylines_iter = geometry_lists.polylines_list.begin();
	seed_co_registration_polylines_list_type::const_iterator polylines_end = geometry_lists.polylines_list.end();
	for ( ; polylines_iter != polylines_end; ++polylines_iter)
	{
		const SeedCoRegistration &seed_co_registration = *polylines_iter;

		// We're currently traversing the 'GPlatesMaths::PolylineOnSphere' list so the dynamic_cast should not fail.
		const GPlatesMaths::PolylineOnSphere &polyline_on_sphere =
				dynamic_cast<const GPlatesMaths::PolylineOnSphere &>(seed_co_registration.geometry);

		// Most of the vertex data is the same for all vertices in the seed geometry.
		PointRegionOfInterestVertex vertex;
		vertex.initialise_seed_geometry_constants(seed_co_registration);

		// Iterate over the points of the current polyline.
		GPlatesMaths::PolylineOnSphere::vertex_const_iterator polyline_points_iter = polyline_on_sphere.vertex_begin();
		GPlatesMaths::PolylineOnSphere::vertex_const_iterator polyline_points_end = polyline_on_sphere.vertex_end();
		for ( ; polyline_points_iter != polyline_points_end; ++polyline_points_iter)
		{
			const GPlatesMaths::PointOnSphere &point = *polyline_points_iter;

			// Render the point region-of-interest geometry filling in the vertex data attributes
			// that are *not* constant across the seed geometry.
			render_unbounded_point_region_of_interest_geometry(
					renderer,
					map_vertex_element_buffer_scope,
					map_vertex_buffer_scope,
					point_stream_target,
					point_stream_meshes,
					point.position_vector(),
					vertex,
					centre_point_weight,
					tangent_weight);
		}
	}

	// Iterate over the polygon geometries.
	seed_co_registration_polygons_list_type::const_iterator polygons_iter = geometry_lists.polygons_list.begin();
	seed_co_registration_polygons_list_type::const_iterator polygons_end = geometry_lists.polygons_list.end();
	for ( ; polygons_iter != polygons_end; ++polygons_iter)
	{
		const SeedCoRegistration &seed_co_registration = *polygons_iter;

		// We're currently traversing the 'GPlatesMaths::PolygonOnSphere' list so the dynamic_cast should not fail.
		const GPlatesMaths::PolygonOnSphere &polygon_on_sphere =
				dynamic_cast<const GPlatesMaths::PolygonOnSphere &>(seed_co_registration.geometry);

		// Most of the vertex data is the same for all vertices in the seed geometry.
		PointRegionOfInterestVertex vertex;
		vertex.initialise_seed_geometry_constants(seed_co_registration);

		// Iterate over the points of the current polygon.
		GPlatesMaths::PolygonOnSphere::vertex_const_iterator polygon_points_iter = polygon_on_sphere.vertex_begin();
		GPlatesMaths::PolygonOnSphere::vertex_const_iterator polygon_points_end = polygon_on_sphere.vertex_end();
		for ( ; polygon_points_iter != polygon_points_end; ++polygon_points_iter)
		{
			const GPlatesMaths::PointOnSphere &point = *polygon_points_iter;

			// Render the point region-of-interest geometry filling in the vertex data attributes
			// that are *not* constant across the seed geometry.
			render_unbounded_point_region_of_interest_geometry(
					renderer,
					map_vertex_element_buffer_scope,
					map_vertex_buffer_scope,
					point_stream_target,
					point_stream_meshes,
					point.position_vector(),
					vertex,
					centre_point_weight,
					tangent_weight);
		}
	}

	// Stop streaming point region-of-interest geometries so we can render the last batch.
	end_vertex_array_streaming<PointRegionOfInterestVertex>(
			renderer,
			point_stream_target,
			map_vertex_element_buffer_scope,
			map_vertex_buffer_scope);

	// Render the last batch of streamed point region-of-interest geometries (if any).
	render_vertex_array_stream<PointRegionOfInterestVertex>(
			renderer,
			point_stream_target,
			d_point_region_of_interest_vertex_array,
			GL_TRIANGLES);
}


void
GPlatesOpenGL::GLRasterCoRegistration::render_unbounded_point_region_of_interest_geometry(
		GLRenderer &renderer,
		GLBuffer::MapBufferScope &map_vertex_element_buffer_scope,
		GLBuffer::MapBufferScope &map_vertex_buffer_scope,
		point_region_of_interest_stream_primitives_type::StreamTarget &point_stream_target,
		point_region_of_interest_stream_primitives_type::Primitives &point_stream_meshes,
		const GPlatesMaths::UnitVector3D &point,
		PointRegionOfInterestVertex &vertex,
		const double &centre_point_weight,
		const double &tangent_weight)
{
	//PROFILE_FUNC();

	// There are five vertices for the current point (each point gets a fan mesh) and
	// four triangles (three indices each).
	if (!point_stream_meshes.begin_primitive(
			5/*max_num_vertices*/,
			12/*max_num_vertex_elements*/))
	{
		// There's not enough vertices or indices so render what we have so far and
		// obtain new stream buffers.
		suspend_render_resume_vertex_array_streaming<PointRegionOfInterestVertex>(
				renderer,
				point_stream_target,
				map_vertex_element_buffer_scope,
				map_vertex_buffer_scope,
				d_point_region_of_interest_vertex_array,
				GL_TRIANGLES);
	}

	vertex.point_centre[0] = point.x().dval();
	vertex.point_centre[1] = point.y().dval();
	vertex.point_centre[2] = point.z().dval();

	// Add the apex vertex - the vertex that remains at the point centre.
	vertex.tangent_frame_weights[0] = 0;
	vertex.tangent_frame_weights[1] = 0;
	vertex.tangent_frame_weights[2] = 1;
	point_stream_meshes.add_vertex(vertex);

	//
	// Add the four fan mesh outer vertices.
	//
	// Unlike the *bounded* shader program the *unbounded* one does not extrude *off* the sphere.
	// This is because the angles are too large and even having an infinite plane tangent to the
	// surface of the sphere (ie, infinite extrusion) will still not project onto the globe
	// (when projected to the centre of the globe) a surface coverage that is sufficient to cover
	// the large region-of-interest.
	// Instead of extrusion, a fan mesh is created where all vertices lie *on* the sphere
	// (effectively wrapping around the sphere) - the triangle faces of the mesh will still cut
	// into the sphere but they will get normalised (to the sphere surface) in the pixel shader.
	// The main purpose here is to ensure enough coverage of the globe is achieved - too much is also
	// fine because the pixel shader does a region-of-interest test to exclude extraneous coverage pixels.
	// Not having enough coverage is a problem though.
	//

	vertex.tangent_frame_weights[2] = centre_point_weight;

	vertex.tangent_frame_weights[0] = -tangent_weight;
	vertex.tangent_frame_weights[1] = -tangent_weight;
	point_stream_meshes.add_vertex(vertex);

	vertex.tangent_frame_weights[0] = -tangent_weight;
	vertex.tangent_frame_weights[1] = tangent_weight;
	point_stream_meshes.add_vertex(vertex);

	vertex.tangent_frame_weights[0] = tangent_weight;
	vertex.tangent_frame_weights[1] = tangent_weight;
	point_stream_meshes.add_vertex(vertex);

	vertex.tangent_frame_weights[0] = tangent_weight;
	vertex.tangent_frame_weights[1] = -tangent_weight;
	point_stream_meshes.add_vertex(vertex);

	//
	// Add the mesh triangles.
	//

	point_stream_meshes.add_vertex_element(0);
	point_stream_meshes.add_vertex_element(1);
	point_stream_meshes.add_vertex_element(2);

	point_stream_meshes.add_vertex_element(0);
	point_stream_meshes.add_vertex_element(2);
	point_stream_meshes.add_vertex_element(3);

	point_stream_meshes.add_vertex_element(0);
	point_stream_meshes.add_vertex_element(3);
	point_stream_meshes.add_vertex_element(4);

	point_stream_meshes.add_vertex_element(0);
	point_stream_meshes.add_vertex_element(4);
	point_stream_meshes.add_vertex_element(1);

	point_stream_meshes.end_primitive();
}


void
GPlatesOpenGL::GLRasterCoRegistration::render_bounded_line_region_of_interest_geometries(
		GLRenderer &renderer,
		GLBuffer::MapBufferScope &map_vertex_element_buffer_scope,
		GLBuffer::MapBufferScope &map_vertex_buffer_scope,
		const SeedCoRegistrationGeometryLists &geometry_lists,
		const double &region_of_interest_radius)
{
	//PROFILE_FUNC();

	// Nothing to do if there's no polylines and no polygons.
	if (geometry_lists.polylines_list.empty() && geometry_lists.polygons_list.empty())
	{
		return;
	}

	//
	// Some uniform shader parameters.
	//
	// NOTE: The region-of-interest angular extent will be less than 90 degrees because otherwise
	// we wouldn't be here - the region-of-interest-expanded bounding regions around each seed
	// geometry fits in the spatial partition (in one 'loose' cube face of the partition) - and it's
	// not possible for those half-extents to exceed 90 degrees (it shouldn't even get close to 90 degrees).
	// So we can use the small region-of-interest angle shader program that should be accurate since
	// we're not going near 90 degrees and we also don't have to worry about an undefined 'tan' at 90 degrees.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			// Let's make sure it doesn't even get close to 90 degrees...
			region_of_interest_radius < 0.95 * GPlatesMaths::HALF_PI,
			GPLATES_ASSERTION_SOURCE);

	const double sin_region_of_interest_angle = std::sin(region_of_interest_radius);
	const double tan_region_of_interest_angle = std::tan(region_of_interest_radius);

	// Bind the shader program for rendering line regions-of-interest with smaller region-of-interest angles.
	renderer.gl_bind_program_object(d_render_lines_of_seed_geometries_with_small_roi_angle_program_object);

	// Set the region-of-interest radius.
	d_render_lines_of_seed_geometries_with_small_roi_angle_program_object->gl_uniform1f(
			renderer, "sin_region_of_interest_angle", sin_region_of_interest_angle);

	// Bind the line region-of-interest vertex array.
	d_line_region_of_interest_vertex_array->gl_bind(renderer);

	// For streaming LineRegionOfInterestVertex vertices.
	line_region_of_interest_stream_primitives_type line_stream;
	line_region_of_interest_stream_primitives_type::StreamTarget line_stream_target(line_stream);

	// Start streaming line region-of-interest geometries.
	begin_vertex_array_streaming<LineRegionOfInterestVertex>(
			renderer,
			line_stream_target,
			map_vertex_element_buffer_scope,
			map_vertex_buffer_scope);

	line_region_of_interest_stream_primitives_type::Primitives line_stream_quads(line_stream);

	// Iterate over the polyline geometries.
	seed_co_registration_polylines_list_type::const_iterator polylines_iter = geometry_lists.polylines_list.begin();
	seed_co_registration_polylines_list_type::const_iterator polylines_end = geometry_lists.polylines_list.end();
	for ( ; polylines_iter != polylines_end; ++polylines_iter)
	{
		const SeedCoRegistration &seed_co_registration = *polylines_iter;

		// We're currently traversing the 'GPlatesMaths::PolylineOnSphere' list so the dynamic_cast should not fail.
		const GPlatesMaths::PolylineOnSphere &polyline_on_sphere =
				dynamic_cast<const GPlatesMaths::PolylineOnSphere &>(seed_co_registration.geometry);

		// Most of the vertex data is the same for all vertices in the seed geometry.
		LineRegionOfInterestVertex vertex;
		vertex.initialise_seed_geometry_constants(seed_co_registration);

		// Iterate over the lines (great circle arcs) of the current polyline.
		GPlatesMaths::PolylineOnSphere::const_iterator polyline_arcs_iter = polyline_on_sphere.begin();
		GPlatesMaths::PolylineOnSphere::const_iterator polyline_arcs_end = polyline_on_sphere.end();
		for ( ; polyline_arcs_iter != polyline_arcs_end; ++polyline_arcs_iter)
		{
			const GPlatesMaths::GreatCircleArc &line = *polyline_arcs_iter;

			// If the line is degenerate (within numerical precision) then it's endpoints are
			// too close together to get a rotation axis. We can ignore these lines since the
			// region-of-interest quad that would've been generated would also be degenerate (zero-area).
			// And the two end-point *point* region-of-interest geometries cover the region-of-interest nicely.
			if (line.is_zero_length())
			{
				continue;
			}

			render_bounded_line_region_of_interest_geometry(
					renderer,
					map_vertex_element_buffer_scope,
					map_vertex_buffer_scope,
					line_stream_target,
					line_stream_quads,
					line,
					vertex,
					tan_region_of_interest_angle);
		}
	}

	// Iterate over the polygon geometries.
	seed_co_registration_polygons_list_type::const_iterator polygons_iter = geometry_lists.polygons_list.begin();
	seed_co_registration_polygons_list_type::const_iterator polygons_end = geometry_lists.polygons_list.end();
	for ( ; polygons_iter != polygons_end; ++polygons_iter)
	{
		const SeedCoRegistration &seed_co_registration = *polygons_iter;

		// We're currently traversing the 'GPlatesMaths::PolygonOnSphere' list so the dynamic_cast should not fail.
		const GPlatesMaths::PolygonOnSphere &polygon_on_sphere =
				dynamic_cast<const GPlatesMaths::PolygonOnSphere &>(seed_co_registration.geometry);

		// Most of the vertex data is the same for all vertices in the seed geometry.
		LineRegionOfInterestVertex vertex;
		vertex.initialise_seed_geometry_constants(seed_co_registration);

		// Iterate over the lines (great circle arcs) of the current polygon.
		GPlatesMaths::PolygonOnSphere::const_iterator polygon_arcs_iter = polygon_on_sphere.begin();
		GPlatesMaths::PolygonOnSphere::const_iterator polygon_arcs_end = polygon_on_sphere.end();
		for ( ; polygon_arcs_iter != polygon_arcs_end; ++polygon_arcs_iter)
		{
			const GPlatesMaths::GreatCircleArc &line = *polygon_arcs_iter;

			// If the line is degenerate (within numerical precision) then it's endpoints are
			// too close together to get a rotation axis. We can ignore these lines since the
			// region-of-interest quad that would've been generated would also be degenerate (zero-area).
			// And the two end-point *point* region-of-interest geometries cover the region-of-interest nicely.
			if (line.is_zero_length())
			{
				continue;
			}

			render_bounded_line_region_of_interest_geometry(
					renderer,
					map_vertex_element_buffer_scope,
					map_vertex_buffer_scope,
					line_stream_target,
					line_stream_quads,
					line,
					vertex,
					tan_region_of_interest_angle);
		}
	}

	// Stop streaming line region-of-interest geometries so we can render the last batch.
	end_vertex_array_streaming<LineRegionOfInterestVertex>(
			renderer,
			line_stream_target,
			map_vertex_element_buffer_scope,
			map_vertex_buffer_scope);

	// Render the last batch of streamed line region-of-interest geometries (if any).
	render_vertex_array_stream<LineRegionOfInterestVertex>(
			renderer,
			line_stream_target,
			d_line_region_of_interest_vertex_array,
			GL_TRIANGLES);
}


void
GPlatesOpenGL::GLRasterCoRegistration::render_bounded_line_region_of_interest_geometry(
		GLRenderer &renderer,
		GLBuffer::MapBufferScope &map_vertex_element_buffer_scope,
		GLBuffer::MapBufferScope &map_vertex_buffer_scope,
		line_region_of_interest_stream_primitives_type::StreamTarget &line_stream_target,
		line_region_of_interest_stream_primitives_type::Primitives &line_stream_quads,
		const GPlatesMaths::GreatCircleArc &line,
		LineRegionOfInterestVertex &vertex,
		const double &tan_region_of_interest_angle)
{
	// There are four vertices for the current line (each line gets a quad) and
	// two triangles (three indices each).
	if (!line_stream_quads.begin_primitive(
			4/*max_num_vertices*/,
			6/*max_num_vertex_elements*/))
	{
		// There's not enough vertices or indices so render what we have so far and
		// obtain new stream buffers.
		suspend_render_resume_vertex_array_streaming<LineRegionOfInterestVertex>(
				renderer,
				line_stream_target,
				map_vertex_element_buffer_scope,
				map_vertex_buffer_scope,
				d_line_region_of_interest_vertex_array,
				GL_TRIANGLES);
	}

	// We should only be called if line (arc) has a rotation axis.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			!line.is_zero_length(),
			GPLATES_ASSERTION_SOURCE);

	const GPlatesMaths::UnitVector3D &first_point = line.start_point().position_vector();
	const GPlatesMaths::UnitVector3D &second_point = line.end_point().position_vector();
	const GPlatesMaths::UnitVector3D &arc_normal = line.rotation_axis();

	const GLfloat first_point_gl[3] = { first_point.x().dval(), first_point.y().dval(), first_point.z().dval() };
	const GLfloat second_point_gl[3] = { second_point.x().dval(), second_point.y().dval(), second_point.z().dval() };

	// All four vertices have the same arc normal.
	vertex.line_arc_normal[0] = arc_normal.x().dval();
	vertex.line_arc_normal[1] = arc_normal.y().dval();
	vertex.line_arc_normal[2] = arc_normal.z().dval();
	vertex.tangent_frame_weights[1] = 1; // 'weight_start_point'.

	// The first two vertices have the start point as the *first* GCA point.
	vertex.line_arc_start_point[0] = first_point_gl[0];
	vertex.line_arc_start_point[1] = first_point_gl[1];
	vertex.line_arc_start_point[2] = first_point_gl[2];

	vertex.tangent_frame_weights[0] = -tan_region_of_interest_angle;
	line_stream_quads.add_vertex(vertex);

	vertex.tangent_frame_weights[0] = tan_region_of_interest_angle;
	line_stream_quads.add_vertex(vertex);

	// The last two vertices have the start point as the *second* GCA point.
	vertex.line_arc_start_point[0] = second_point_gl[0];
	vertex.line_arc_start_point[1] = second_point_gl[1];
	vertex.line_arc_start_point[2] = second_point_gl[2];

	vertex.tangent_frame_weights[0] = tan_region_of_interest_angle;
	line_stream_quads.add_vertex(vertex);

	vertex.tangent_frame_weights[0] = -tan_region_of_interest_angle;
	line_stream_quads.add_vertex(vertex);

	//
	// Add the mesh triangles.
	//

	line_stream_quads.add_vertex_element(0);
	line_stream_quads.add_vertex_element(1);
	line_stream_quads.add_vertex_element(2);

	line_stream_quads.add_vertex_element(0);
	line_stream_quads.add_vertex_element(2);
	line_stream_quads.add_vertex_element(3);

	line_stream_quads.end_primitive();
}


void
GPlatesOpenGL::GLRasterCoRegistration::render_unbounded_line_region_of_interest_geometries(
		GLRenderer &renderer,
		GLBuffer::MapBufferScope &map_vertex_element_buffer_scope,
		GLBuffer::MapBufferScope &map_vertex_buffer_scope,
		const SeedCoRegistrationGeometryLists &geometry_lists,
		const double &region_of_interest_radius)
{
	//PROFILE_FUNC();

	// Nothing to do if there's no polylines and no polygons.
	if (geometry_lists.polylines_list.empty() && geometry_lists.polygons_list.empty())
	{
		return;
	}
	//
	// Some uniform shader parameters.
	//

	// The seed geometries are unbounded which means they were too big to fit into a cube face of
	// the spatial partition which means the region-of-interest angle could be large or small.
	// The line region-of-interest geometries come from polylines and polygons and they can be
	// arbitrarily large but still have small region-of-interest radii associated with them -
	// so the fact that they're unbounded does not rule out small angles.

	const double cos_region_of_interest_angle = std::cos(region_of_interest_radius);
	const double sin_region_of_interest_angle = std::sin(region_of_interest_radius);

	// For smaller angles (less than 45 degrees) use a shader program that's accurate for very small angles.
	if (region_of_interest_radius < GPlatesMaths::PI / 4)
	{
		// Bind the shader program for rendering point regions-of-interest.
		renderer.gl_bind_program_object(d_render_lines_of_seed_geometries_with_small_roi_angle_program_object);

		// Set the region-of-interest radius.
		d_render_lines_of_seed_geometries_with_small_roi_angle_program_object->gl_uniform1f(
				renderer, "sin_region_of_interest_angle", sin_region_of_interest_angle);
	}
	else // Use a shader program that's accurate for angles very near 90 degrees...
	{
		// Bind the shader program for rendering point regions-of-interest.
		renderer.gl_bind_program_object(d_render_lines_of_seed_geometries_with_large_roi_angle_program_object);

		// Set the region-of-interest radius.
		// Note that 'tan' is undefined at 90 degrees but we're safe since we're restricted to 45 degrees or more
		// and we're calculating 'tan' of the *complimentary* angle (which is 90 degrees minus the angle).
		// Also we limit the maximum region-of-interest angle to 90 degrees - this is because angles
		// greater than 90 degrees are not necessary - they are taken care of by the *point*
		// region-of-interest regions (the end points of the line, or arc, section) - and the shader
		// program can only handle angles up to 90 degrees since it calculates distance to the arc plane.
		// Note that the shader program does not actually exclude regions that are close to the arc
		// plane but nevertheless too far from either arc endpoint to be considered inside the region-of-interest.
		// The region-of-interest geometry (coverage) itself does this exclusion.
		const double tan_region_of_interest_complementary_angle =
				(region_of_interest_radius < GPlatesMaths::HALF_PI)
				? std::tan(GPlatesMaths::HALF_PI - region_of_interest_radius)
				: std::tan(GPlatesMaths::HALF_PI);
		d_render_lines_of_seed_geometries_with_large_roi_angle_program_object->gl_uniform1f(
				renderer,
				"tan_squared_region_of_interest_complementary_angle",
				tan_region_of_interest_complementary_angle * tan_region_of_interest_complementary_angle);
	}

	// Tangent frame weights used for each 'line' to determine position of the line's mesh vertices.
	// These weights place the mesh vertices on the unit sphere.
	const double arc_point_weight = cos_region_of_interest_angle;
	const double tangent_weight = sin_region_of_interest_angle;

	// Bind the line region-of-interest vertex array.
	d_line_region_of_interest_vertex_array->gl_bind(renderer);

	// For streaming LineRegionOfInterestVertex vertices.
	line_region_of_interest_stream_primitives_type line_stream;
	line_region_of_interest_stream_primitives_type::StreamTarget line_stream_target(line_stream);

	// Start streaming line region-of-interest geometries.
	begin_vertex_array_streaming<LineRegionOfInterestVertex>(
			renderer,
			line_stream_target,
			map_vertex_element_buffer_scope,
			map_vertex_buffer_scope);

	line_region_of_interest_stream_primitives_type::Primitives line_stream_meshes(line_stream);

	// Iterate over the polyline geometries.
	seed_co_registration_polylines_list_type::const_iterator polylines_iter = geometry_lists.polylines_list.begin();
	seed_co_registration_polylines_list_type::const_iterator polylines_end = geometry_lists.polylines_list.end();
	for ( ; polylines_iter != polylines_end; ++polylines_iter)
	{
		const SeedCoRegistration &seed_co_registration = *polylines_iter;

		// We're currently traversing the 'GPlatesMaths::PolylineOnSphere' list so the dynamic_cast should not fail.
		const GPlatesMaths::PolylineOnSphere &polyline_on_sphere =
				dynamic_cast<const GPlatesMaths::PolylineOnSphere &>(seed_co_registration.geometry);

		// Most of the vertex data is the same for all vertices in the seed geometry.
		LineRegionOfInterestVertex vertex;
		vertex.initialise_seed_geometry_constants(seed_co_registration);

		// Iterate over the lines (great circle arcs) of the current polyline.
		GPlatesMaths::PolylineOnSphere::const_iterator polyline_arcs_iter = polyline_on_sphere.begin();
		GPlatesMaths::PolylineOnSphere::const_iterator polyline_arcs_end = polyline_on_sphere.end();
		for ( ; polyline_arcs_iter != polyline_arcs_end; ++polyline_arcs_iter)
		{
			const GPlatesMaths::GreatCircleArc &line = *polyline_arcs_iter;

			// If the line is degenerate (within numerical precision) then it's endpoints are
			// too close together to get a rotation axis. We can ignore these lines since the
			// region-of-interest mesh that would've been generated would also be degenerate (zero-area).
			// And the two end-point *point* region-of-interest geometries cover the region-of-interest nicely.
			if (line.is_zero_length())
			{
				continue;
			}

			render_unbounded_line_region_of_interest_geometry(
					renderer,
					map_vertex_element_buffer_scope,
					map_vertex_buffer_scope,
					line_stream_target,
					line_stream_meshes,
					line,
					vertex,
					arc_point_weight,
					tangent_weight);
		}
	}

	// Iterate over the polygon geometries.
	seed_co_registration_polygons_list_type::const_iterator polygons_iter = geometry_lists.polygons_list.begin();
	seed_co_registration_polygons_list_type::const_iterator polygons_end = geometry_lists.polygons_list.end();
	for ( ; polygons_iter != polygons_end; ++polygons_iter)
	{
		const SeedCoRegistration &seed_co_registration = *polygons_iter;

		// We're currently traversing the 'GPlatesMaths::PolygonOnSphere' list so the dynamic_cast should not fail.
		const GPlatesMaths::PolygonOnSphere &polygon_on_sphere =
				dynamic_cast<const GPlatesMaths::PolygonOnSphere &>(seed_co_registration.geometry);

		// Most of the vertex data is the same for all vertices in the seed geometry.
		LineRegionOfInterestVertex vertex;
		vertex.initialise_seed_geometry_constants(seed_co_registration);

		// Iterate over the lines (great circle arcs) of the current polygon.
		GPlatesMaths::PolygonOnSphere::const_iterator polygon_arcs_iter = polygon_on_sphere.begin();
		GPlatesMaths::PolygonOnSphere::const_iterator polygon_arcs_end = polygon_on_sphere.end();
		for ( ; polygon_arcs_iter != polygon_arcs_end; ++polygon_arcs_iter)
		{
			const GPlatesMaths::GreatCircleArc &line = *polygon_arcs_iter;

			// If the line is degenerate (within numerical precision) then it's endpoints are
			// too close together to get a rotation axis. We can ignore these lines since the
			// region-of-interest mesh that would've been generated would also be degenerate (zero-area).
			// And the two end-point *point* region-of-interest geometries cover the region-of-interest nicely.
			if (line.is_zero_length())
			{
				continue;
			}

			render_unbounded_line_region_of_interest_geometry(
					renderer,
					map_vertex_element_buffer_scope,
					map_vertex_buffer_scope,
					line_stream_target,
					line_stream_meshes,
					line,
					vertex,
					arc_point_weight,
					tangent_weight);
		}
	}

	// Stop streaming line region-of-interest geometries so we can render the last batch.
	end_vertex_array_streaming<LineRegionOfInterestVertex>(
			renderer,
			line_stream_target,
			map_vertex_element_buffer_scope,
			map_vertex_buffer_scope);

	// Render the last batch of streamed line region-of-interest geometries (if any).
	render_vertex_array_stream<LineRegionOfInterestVertex>(
			renderer,
			line_stream_target,
			d_line_region_of_interest_vertex_array,
			GL_TRIANGLES);
}


void
GPlatesOpenGL::GLRasterCoRegistration::render_unbounded_line_region_of_interest_geometry(
		GLRenderer &renderer,
		GLBuffer::MapBufferScope &map_vertex_element_buffer_scope,
		GLBuffer::MapBufferScope &map_vertex_buffer_scope,
		line_region_of_interest_stream_primitives_type::StreamTarget &line_stream_target,
		line_region_of_interest_stream_primitives_type::Primitives &line_stream_meshes,
		const GPlatesMaths::GreatCircleArc &line,
		LineRegionOfInterestVertex &vertex,
		const double &arc_point_weight,
		const double &tangent_weight)
{
	// There are six vertices for the current line (each line gets a mesh) and 
	// four triangles (three indices each).
	if (!line_stream_meshes.begin_primitive(
			6/*max_num_vertices*/,
			12/*max_num_vertex_elements*/))
	{
		// There's not enough vertices or indices so render what we have so far and
		// obtain new stream buffers.
		suspend_render_resume_vertex_array_streaming<LineRegionOfInterestVertex>(
				renderer,
				line_stream_target,
				map_vertex_element_buffer_scope,
				map_vertex_buffer_scope,
				d_line_region_of_interest_vertex_array,
				GL_TRIANGLES);
	}

	// We should only be called if line (arc) has a rotation axis.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			!line.is_zero_length(),
			GPLATES_ASSERTION_SOURCE);

	const GPlatesMaths::UnitVector3D &first_point = line.start_point().position_vector();
	const GPlatesMaths::UnitVector3D &second_point = line.end_point().position_vector();
	const GPlatesMaths::UnitVector3D &arc_normal = line.rotation_axis();

	const GLfloat first_point_gl[3] = { first_point.x().dval(), first_point.y().dval(), first_point.z().dval() };
	const GLfloat second_point_gl[3] = { second_point.x().dval(), second_point.y().dval(), second_point.z().dval() };

	//
	// Two mesh vertices remain at the line (arc) end points, another two lie in the plane containing
	// the arc start point and arc normal (and origin) and another two lie in the plane containing
	// the arc end point and arc normal (and origin).
	//
	// Unlike the *bounded* shader program the *unbounded* one does not extrude *off* the sphere.
	// This is because the angles are too large and even having an infinite plane tangent to the
	// surface of the sphere (ie, infinite extrusion) will still not project onto the globe
	// (when projected to the centre of the globe) a surface coverage that is sufficient to cover
	// the large region-of-interest.
	// Instead of extrusion, a mesh is created where all vertices lie *on* the sphere
	// (effectively wrapping around the sphere) - the triangle faces of the mesh will still cut
	// into the sphere but they will get normalised (to the sphere surface) in the pixel shader.
	// The main purpose here is to ensure enough coverage of the globe is achieved - too much
	// coverage (with one exception noted below) is also fine because the pixel shader does a
	// region-of-interest test to exclude extraneous coverage pixels - not having enough coverage
	// is a problem though.
	// The one exception to allowed extraneous coverage is the area within the region-of-interest
	// distance from the arc's plane (ie, the full great circle not just the great circle arc) *but*
	// still outside the region-of-interest (ie, not between the two arc end points). Here the
	// mesh geometry is carefully constructed to not cover this area. So the region-of-interest test,
	// for lines (arcs), is a combination of geometry coverage and arc-plane tests in the pixel shader.
	//

	// All six vertices have the same arc normal.
	vertex.line_arc_normal[0] = arc_normal.x().dval();
	vertex.line_arc_normal[1] = arc_normal.y().dval();
	vertex.line_arc_normal[2] = arc_normal.z().dval();

	// The first three vertices have the start point as the *first* GCA point.
	vertex.line_arc_start_point[0] = first_point_gl[0];
	vertex.line_arc_start_point[1] = first_point_gl[1];
	vertex.line_arc_start_point[2] = first_point_gl[2];

	// First vertex is weighted to remain at the *first* GCA point.
	vertex.tangent_frame_weights[0] = 0;
	vertex.tangent_frame_weights[1] = 1; // 'weight_start_point'.
	line_stream_meshes.add_vertex(vertex);

	// The next two vertices are either side of the *first* GCA point (and in the plane
	// containing the *first* GCA point and the arc normal point - this is what achieves
	// the geometry part of the region-of-interest test).

	vertex.tangent_frame_weights[0] = -tangent_weight;
	vertex.tangent_frame_weights[1] = arc_point_weight;
	line_stream_meshes.add_vertex(vertex);

	vertex.tangent_frame_weights[0] = tangent_weight;
	vertex.tangent_frame_weights[1] = arc_point_weight;
	line_stream_meshes.add_vertex(vertex);

	// The last three vertices have the start point as the *second* GCA point.
	vertex.line_arc_start_point[0] = second_point_gl[0];
	vertex.line_arc_start_point[1] = second_point_gl[1];
	vertex.line_arc_start_point[2] = second_point_gl[2];

	// Fourth vertex is weighted to remain at the *second* GCA point.
	vertex.tangent_frame_weights[0] = 0;
	vertex.tangent_frame_weights[1] = 1; // 'weight_start_point'.
	line_stream_meshes.add_vertex(vertex);

	// The next two vertices are either side of the *second* GCA point (and in the plane
	// containing the *second* GCA point and the arc normal point - this is what achieves
	// the geometry part of the region-of-interest test).

	vertex.tangent_frame_weights[0] = -tangent_weight;
	vertex.tangent_frame_weights[1] = arc_point_weight;
	line_stream_meshes.add_vertex(vertex);

	vertex.tangent_frame_weights[0] = tangent_weight;
	vertex.tangent_frame_weights[1] = arc_point_weight;
	line_stream_meshes.add_vertex(vertex);

	//
	// Add the four mesh triangles.
	//
	// 2-5
	// |/|
	// 0-3
	// |/|
	// 1-4

	line_stream_meshes.add_vertex_element(0);
	line_stream_meshes.add_vertex_element(1);
	line_stream_meshes.add_vertex_element(3);

	line_stream_meshes.add_vertex_element(1);
	line_stream_meshes.add_vertex_element(4);
	line_stream_meshes.add_vertex_element(3);

	line_stream_meshes.add_vertex_element(0);
	line_stream_meshes.add_vertex_element(3);
	line_stream_meshes.add_vertex_element(5);

	line_stream_meshes.add_vertex_element(0);
	line_stream_meshes.add_vertex_element(5);
	line_stream_meshes.add_vertex_element(2);

	line_stream_meshes.end_primitive();
}


void
GPlatesOpenGL::GLRasterCoRegistration::render_single_pixel_size_point_region_of_interest_geometries(
		GLRenderer &renderer,
		GLBuffer::MapBufferScope &map_vertex_element_buffer_scope,
		GLBuffer::MapBufferScope &map_vertex_buffer_scope,
		const SeedCoRegistrationGeometryLists &geometry_lists)
{
	//PROFILE_FUNC();

	//
	// Leave the point size state as the default (point size of 1 and no anti-aliasing).
	// We're rendering actual points here instead of small quads (to ensure the small quads didn't fall
	// between pixels in the render target because they were too small). Doing this ensures we
	// always sample at least one pixel at the point position.
	//

	// Bind the shader program for rendering fill regions-of-interest.
	renderer.gl_bind_program_object(d_render_fill_of_seed_geometries_program_object);

	// Bind the fill region-of-interest vertex array.
	d_fill_region_of_interest_vertex_array->gl_bind(renderer);

	// For streaming FillRegionOfInterestVertex vertices.
	fill_region_of_interest_stream_primitives_type fill_stream;
	fill_region_of_interest_stream_primitives_type::StreamTarget fill_stream_target(fill_stream);

	// Start streaming fill region-of-interest geometries.
	begin_vertex_array_streaming<FillRegionOfInterestVertex>(
			renderer,
			fill_stream_target,
			map_vertex_element_buffer_scope,
			map_vertex_buffer_scope);

	// Input points for the vertices of the seed geometries.
	fill_region_of_interest_stream_primitives_type::Points fill_stream_points(fill_stream);

	fill_stream_points.begin_points();

	// Iterate over the point geometries.
	seed_co_registration_points_list_type::const_iterator points_iter = geometry_lists.points_list.begin();
	seed_co_registration_points_list_type::const_iterator points_end = geometry_lists.points_list.end();
	for ( ; points_iter != points_end; ++points_iter)
	{
		const SeedCoRegistration &seed_co_registration = *points_iter;

		// We're currently traversing the 'GPlatesMaths::PointOnSphere' list so the dynamic_cast should not fail.
		const GPlatesMaths::PointOnSphere &point_on_sphere =
				dynamic_cast<const GPlatesMaths::PointOnSphere &>(seed_co_registration.geometry);

		const GPlatesMaths::UnitVector3D &point_position = point_on_sphere.position_vector();

		FillRegionOfInterestVertex vertex;
		vertex.initialise_seed_geometry_constants(seed_co_registration);
		vertex.fill_position[0] = point_position.x().dval();
		vertex.fill_position[1] = point_position.y().dval();
		vertex.fill_position[2] = point_position.z().dval();

		if (!fill_stream_points.add_vertex(vertex))
		{
			suspend_render_resume_vertex_array_streaming<FillRegionOfInterestVertex>(
					renderer,
					fill_stream_target,
					map_vertex_element_buffer_scope,
					map_vertex_buffer_scope,
					d_fill_region_of_interest_vertex_array,
					// These are actually rasterised points not quads (triangles)...
					GL_POINTS);
			fill_stream_points.add_vertex(vertex);
		}
	}

	// Iterate over the multipoint geometries.
	seed_co_registration_multi_points_list_type::const_iterator multi_points_iter = geometry_lists.multi_points_list.begin();
	seed_co_registration_multi_points_list_type::const_iterator multi_points_end = geometry_lists.multi_points_list.end();
	for ( ; multi_points_iter != multi_points_end; ++multi_points_iter)
	{
		const SeedCoRegistration &seed_co_registration = *multi_points_iter;

		// We're currently traversing the 'GPlatesMaths::MultiPointOnSphere' list so the dynamic_cast should not fail.
		const GPlatesMaths::MultiPointOnSphere &multi_point_on_sphere =
				dynamic_cast<const GPlatesMaths::MultiPointOnSphere &>(seed_co_registration.geometry);

		// Most of the vertex data is the same for all vertices in the seed geometry.
		FillRegionOfInterestVertex vertex;
		vertex.initialise_seed_geometry_constants(seed_co_registration);

		// Iterate over the points of the current multipoint.
		GPlatesMaths::MultiPointOnSphere::const_iterator multi_point_iter = multi_point_on_sphere.begin();
		GPlatesMaths::MultiPointOnSphere::const_iterator multi_point_end = multi_point_on_sphere.end();
		for ( ; multi_point_iter != multi_point_end; ++multi_point_iter)
		{
			const GPlatesMaths::UnitVector3D &point_position = multi_point_iter->position_vector();

			vertex.fill_position[0] = point_position.x().dval();
			vertex.fill_position[1] = point_position.y().dval();
			vertex.fill_position[2] = point_position.z().dval();

			if (!fill_stream_points.add_vertex(vertex))
			{
				suspend_render_resume_vertex_array_streaming<FillRegionOfInterestVertex>(
						renderer,
						fill_stream_target,
						map_vertex_element_buffer_scope,
						map_vertex_buffer_scope,
						d_fill_region_of_interest_vertex_array,
						// These are actually rasterised points not quads (triangles)...
						GL_POINTS);
				fill_stream_points.add_vertex(vertex);
			}
		}
	}

	// Iterate over the polyline geometries.
	seed_co_registration_polylines_list_type::const_iterator polylines_iter = geometry_lists.polylines_list.begin();
	seed_co_registration_polylines_list_type::const_iterator polylines_end = geometry_lists.polylines_list.end();
	for ( ; polylines_iter != polylines_end; ++polylines_iter)
	{
		const SeedCoRegistration &seed_co_registration = *polylines_iter;

		// We're currently traversing the 'GPlatesMaths::PolylineOnSphere' list so the dynamic_cast should not fail.
		const GPlatesMaths::PolylineOnSphere &polyline_on_sphere =
				dynamic_cast<const GPlatesMaths::PolylineOnSphere &>(seed_co_registration.geometry);

		// Most of the vertex data is the same for all vertices in the seed geometry.
		FillRegionOfInterestVertex vertex;
		vertex.initialise_seed_geometry_constants(seed_co_registration);

		// Iterate over the points of the current polyline.
		GPlatesMaths::PolylineOnSphere::vertex_const_iterator polyline_points_iter = polyline_on_sphere.vertex_begin();
		GPlatesMaths::PolylineOnSphere::vertex_const_iterator polyline_points_end = polyline_on_sphere.vertex_end();
		for ( ; polyline_points_iter != polyline_points_end; ++polyline_points_iter)
		{
			const GPlatesMaths::UnitVector3D &point_position = polyline_points_iter->position_vector();

			vertex.fill_position[0] = point_position.x().dval();
			vertex.fill_position[1] = point_position.y().dval();
			vertex.fill_position[2] = point_position.z().dval();

			if (!fill_stream_points.add_vertex(vertex))
			{
				suspend_render_resume_vertex_array_streaming<FillRegionOfInterestVertex>(
						renderer,
						fill_stream_target,
						map_vertex_element_buffer_scope,
						map_vertex_buffer_scope,
						d_fill_region_of_interest_vertex_array,
						// These are actually rasterised points not quads (triangles)...
						GL_POINTS);
				fill_stream_points.add_vertex(vertex);
			}
		}
	}

	// Iterate over the polygon geometries.
	seed_co_registration_polygons_list_type::const_iterator polygons_iter = geometry_lists.polygons_list.begin();
	seed_co_registration_polygons_list_type::const_iterator polygons_end = geometry_lists.polygons_list.end();
	for ( ; polygons_iter != polygons_end; ++polygons_iter)
	{
		const SeedCoRegistration &seed_co_registration = *polygons_iter;

		// We're currently traversing the 'GPlatesMaths::PolygonOnSphere' list so the dynamic_cast should not fail.
		const GPlatesMaths::PolygonOnSphere &polygon_on_sphere =
				dynamic_cast<const GPlatesMaths::PolygonOnSphere &>(seed_co_registration.geometry);

		// Most of the vertex data is the same for all vertices in the seed geometry.
		FillRegionOfInterestVertex vertex;
		vertex.initialise_seed_geometry_constants(seed_co_registration);

		// Iterate over the points of the current polygon.
		GPlatesMaths::PolygonOnSphere::vertex_const_iterator polygon_points_iter = polygon_on_sphere.vertex_begin();
		GPlatesMaths::PolygonOnSphere::vertex_const_iterator polygon_points_end = polygon_on_sphere.vertex_end();
		for ( ; polygon_points_iter != polygon_points_end; ++polygon_points_iter)
		{
			const GPlatesMaths::UnitVector3D &point_position = polygon_points_iter->position_vector();

			vertex.fill_position[0] = point_position.x().dval();
			vertex.fill_position[1] = point_position.y().dval();
			vertex.fill_position[2] = point_position.z().dval();

			if (!fill_stream_points.add_vertex(vertex))
			{
				suspend_render_resume_vertex_array_streaming<FillRegionOfInterestVertex>(
						renderer,
						fill_stream_target,
						map_vertex_element_buffer_scope,
						map_vertex_buffer_scope,
						d_fill_region_of_interest_vertex_array,
						// These are actually rasterised points not quads (triangles)...
						GL_POINTS);
				fill_stream_points.add_vertex(vertex);
			}
		}
	}

	fill_stream_points.end_points();

	// Stop streaming so we can render the last batch.
	end_vertex_array_streaming<FillRegionOfInterestVertex>(
			renderer,
			fill_stream_target,
			map_vertex_element_buffer_scope,
			map_vertex_buffer_scope);

	// Render the last batch (if any).
	render_vertex_array_stream<FillRegionOfInterestVertex>(
			renderer,
			fill_stream_target,
			d_fill_region_of_interest_vertex_array,
			// These are actually rasterised points not quads (triangles)...
			GL_POINTS);
}


void
GPlatesOpenGL::GLRasterCoRegistration::render_single_pixel_wide_line_region_of_interest_geometries(
		GLRenderer &renderer,
		GLBuffer::MapBufferScope &map_vertex_element_buffer_scope,
		GLBuffer::MapBufferScope &map_vertex_buffer_scope,
		const SeedCoRegistrationGeometryLists &geometry_lists)
{
	//PROFILE_FUNC();

	// Nothing to do if there's no polylines and no polygons.
	if (geometry_lists.polylines_list.empty() && geometry_lists.polygons_list.empty())
	{
		return;
	}

	//
	// Leave the line width state as the default (line width of 1 and no anti-aliasing).
	// We're rendering lines here instead of thin quads (to ensure the thin quads didn't fall
	// between pixels in the render target because they were too thin). Doing this ensures we
	// always sample at least one pixel right along the polyline or polygon boundary without
	// skipping pixels along the lines.
	//

	// Bind the shader program for rendering fill regions-of-interest.
	renderer.gl_bind_program_object(d_render_fill_of_seed_geometries_program_object);

	// Bind the fill region-of-interest vertex array.
	d_fill_region_of_interest_vertex_array->gl_bind(renderer);

	// For streaming FillRegionOfInterestVertex vertices.
	fill_region_of_interest_stream_primitives_type fill_stream;
	fill_region_of_interest_stream_primitives_type::StreamTarget fill_stream_target(fill_stream);

	// Start streaming fill region-of-interest geometries.
	begin_vertex_array_streaming<FillRegionOfInterestVertex>(
			renderer,
			fill_stream_target,
			map_vertex_element_buffer_scope,
			map_vertex_buffer_scope);

	// Input a line strip for each polyline.
	fill_region_of_interest_stream_primitives_type::LineStrips fill_stream_line_strips(fill_stream);

	// Iterate over the polyline geometries.
	seed_co_registration_polylines_list_type::const_iterator polylines_iter = geometry_lists.polylines_list.begin();
	seed_co_registration_polylines_list_type::const_iterator polylines_end = geometry_lists.polylines_list.end();
	for ( ; polylines_iter != polylines_end; ++polylines_iter)
	{
		const SeedCoRegistration &seed_co_registration = *polylines_iter;

		// We're currently traversing the 'GPlatesMaths::PolylineOnSphere' list so the dynamic_cast should not fail.
		const GPlatesMaths::PolylineOnSphere &polyline_on_sphere =
				dynamic_cast<const GPlatesMaths::PolylineOnSphere &>(seed_co_registration.geometry);

		fill_stream_line_strips.begin_line_strip();

		// Most of the vertex data is the same for all vertices in the seed geometry.
		FillRegionOfInterestVertex vertex;
		vertex.initialise_seed_geometry_constants(seed_co_registration);

		// Iterate over the points of the current polyline.
		GPlatesMaths::PolylineOnSphere::vertex_const_iterator polyline_points_iter = polyline_on_sphere.vertex_begin();
		GPlatesMaths::PolylineOnSphere::vertex_const_iterator polyline_points_end = polyline_on_sphere.vertex_end();
		for ( ; polyline_points_iter != polyline_points_end; ++polyline_points_iter)
		{
			const GPlatesMaths::UnitVector3D &point_position = polyline_points_iter->position_vector();

			vertex.fill_position[0] = point_position.x().dval();
			vertex.fill_position[1] = point_position.y().dval();
			vertex.fill_position[2] = point_position.z().dval();
			if (!fill_stream_line_strips.add_vertex(vertex))
			{
				suspend_render_resume_vertex_array_streaming<FillRegionOfInterestVertex>(
						renderer,
						fill_stream_target,
						map_vertex_element_buffer_scope,
						map_vertex_buffer_scope,
						d_fill_region_of_interest_vertex_array,
						// These are actually rasterised lines not quads (triangles)...
						GL_LINES);
				fill_stream_line_strips.add_vertex(vertex);
			}
		}

		fill_stream_line_strips.end_line_strip();
	}

	// Input a line loop for each polygon.
	fill_region_of_interest_stream_primitives_type::LineLoops fill_stream_line_loops(fill_stream);

	// Iterate over the polygon geometries.
	seed_co_registration_polygons_list_type::const_iterator polygons_iter = geometry_lists.polygons_list.begin();
	seed_co_registration_polygons_list_type::const_iterator polygons_end = geometry_lists.polygons_list.end();
	for ( ; polygons_iter != polygons_end; ++polygons_iter)
	{
		const SeedCoRegistration &seed_co_registration = *polygons_iter;

		// We're currently traversing the 'GPlatesMaths::PolygonOnSphere' list so the dynamic_cast should not fail.
		const GPlatesMaths::PolygonOnSphere &polygon_on_sphere =
				dynamic_cast<const GPlatesMaths::PolygonOnSphere &>(seed_co_registration.geometry);

		fill_stream_line_loops.begin_line_loop();

		// Most of the vertex data is the same for all vertices in the seed geometry.
		FillRegionOfInterestVertex vertex;
		vertex.initialise_seed_geometry_constants(seed_co_registration);

		// Iterate over the points of the current polygon.
		GPlatesMaths::PolygonOnSphere::vertex_const_iterator polygon_points_iter = polygon_on_sphere.vertex_begin();
		GPlatesMaths::PolygonOnSphere::vertex_const_iterator polygon_points_end = polygon_on_sphere.vertex_end();
		for ( ; polygon_points_iter != polygon_points_end; ++polygon_points_iter)
		{
			const GPlatesMaths::UnitVector3D &point_position = polygon_points_iter->position_vector();

			vertex.fill_position[0] = point_position.x().dval();
			vertex.fill_position[1] = point_position.y().dval();
			vertex.fill_position[2] = point_position.z().dval();
			if (!fill_stream_line_loops.add_vertex(vertex))
			{
				suspend_render_resume_vertex_array_streaming<FillRegionOfInterestVertex>(
						renderer,
						fill_stream_target,
						map_vertex_element_buffer_scope,
						map_vertex_buffer_scope,
						d_fill_region_of_interest_vertex_array,
						// These are actually rasterised lines not quads (triangles)...
						GL_LINES);
				fill_stream_line_strips.add_vertex(vertex);
			}
		}

		fill_stream_line_loops.end_line_loop();
	}

	// Stop streaming so we can render the last batch.
	end_vertex_array_streaming<FillRegionOfInterestVertex>(
			renderer,
			fill_stream_target,
			map_vertex_element_buffer_scope,
			map_vertex_buffer_scope);

	// Render the last batch (if any).
	render_vertex_array_stream<FillRegionOfInterestVertex>(
			renderer,
			fill_stream_target,
			d_fill_region_of_interest_vertex_array,
			// These are actually rasterised lines not quads (triangles)...
			GL_LINES);
}


void
GPlatesOpenGL::GLRasterCoRegistration::render_fill_region_of_interest_geometries(
		GLRenderer &renderer,
		GLBuffer::MapBufferScope &map_vertex_element_buffer_scope,
		GLBuffer::MapBufferScope &map_vertex_buffer_scope,
		const SeedCoRegistrationGeometryLists &geometry_lists)
{
	//PROFILE_FUNC();

	// Nothing to do if there's no polygons.
	if (geometry_lists.polygons_list.empty())
	{
		return;
	}

	// Bind the shader program for rendering fill regions-of-interest.
	renderer.gl_bind_program_object(d_render_fill_of_seed_geometries_program_object);

	// Bind the fill region-of-interest vertex array.
	d_fill_region_of_interest_vertex_array->gl_bind(renderer);

	// Alpha-blend state set to invert destination alpha (and colour) every time a pixel
	// is rendered (this means we get 1 where a pixel is covered by an odd number of triangles
	// and 0 by an even number of triangles).
	// The end result is zero outside the polygon and one inside.
	renderer.gl_enable(GL_BLEND);
	renderer.gl_blend_func(GL_ONE_MINUS_DST_ALPHA, GL_ZERO);

	// For streaming LineRegionOfInterestVertex vertices.
	fill_region_of_interest_stream_primitives_type fill_stream;
	fill_region_of_interest_stream_primitives_type::StreamTarget fill_stream_target(fill_stream);

	// Start streaming fill region-of-interest geometries.
	begin_vertex_array_streaming<FillRegionOfInterestVertex>(
			renderer,
			fill_stream_target,
			map_vertex_element_buffer_scope,
			map_vertex_buffer_scope);

	// Render each polygon as a triangle fan with the fan apex being the polygon centroid.
	fill_region_of_interest_stream_primitives_type::TriangleFans fill_stream_triangle_fans(fill_stream);

	// Iterate over the polygon geometries - the only geometry type that supports fill (has an interior).
	seed_co_registration_polygons_list_type::const_iterator polygons_iter = geometry_lists.polygons_list.begin();
	seed_co_registration_polygons_list_type::const_iterator polygons_end = geometry_lists.polygons_list.end();
	for ( ; polygons_iter != polygons_end; ++polygons_iter)
	{
		const SeedCoRegistration &seed_co_registration = *polygons_iter;

		// We're currently traversing the 'GPlatesMaths::PolygonOnSphere' list so the dynamic_cast should not fail.
		const GPlatesMaths::PolygonOnSphere &polygon_on_sphere =
				dynamic_cast<const GPlatesMaths::PolygonOnSphere &>(seed_co_registration.geometry);

		fill_stream_triangle_fans.begin_triangle_fan();

		// Most of the vertex data is the same for all vertices for polygon triangle fan.
		FillRegionOfInterestVertex vertex;
		vertex.initialise_seed_geometry_constants(seed_co_registration);

		// The first vertex is the polygon centroid.
		const GPlatesMaths::UnitVector3D &centroid = polygon_on_sphere.get_centroid();
		vertex.fill_position[0] = centroid.x().dval();
		vertex.fill_position[1] = centroid.y().dval();
		vertex.fill_position[2] = centroid.z().dval();
		if (!fill_stream_triangle_fans.add_vertex(vertex))
		{
			suspend_render_resume_vertex_array_streaming<FillRegionOfInterestVertex>(
					renderer,
					fill_stream_target,
					map_vertex_element_buffer_scope,
					map_vertex_buffer_scope,
					d_fill_region_of_interest_vertex_array,
					GL_TRIANGLES);
			fill_stream_triangle_fans.add_vertex(vertex);
		}

		// Iterate over the points of the current polygon.
		const GPlatesMaths::PolygonOnSphere::vertex_const_iterator points_begin = polygon_on_sphere.vertex_begin();
		const GPlatesMaths::PolygonOnSphere::vertex_const_iterator points_end = polygon_on_sphere.vertex_end();
		for (GPlatesMaths::PolygonOnSphere::vertex_const_iterator points_iter = points_begin;
			points_iter != points_end;
			++points_iter)
		{
			const GPlatesMaths::UnitVector3D &point_position = points_iter->position_vector();

			vertex.fill_position[0] = point_position.x().dval();
			vertex.fill_position[1] = point_position.y().dval();
			vertex.fill_position[2] = point_position.z().dval();
			if (!fill_stream_triangle_fans.add_vertex(vertex))
			{
				suspend_render_resume_vertex_array_streaming<FillRegionOfInterestVertex>(
						renderer,
						fill_stream_target,
						map_vertex_element_buffer_scope,
						map_vertex_buffer_scope,
						d_fill_region_of_interest_vertex_array,
						GL_TRIANGLES);
				fill_stream_triangle_fans.add_vertex(vertex);
			}
		}

		// Wraparound back to the first polygon vertex to close off the polygon.
		const GPlatesMaths::UnitVector3D &first_point_position = points_begin->position_vector();
		vertex.fill_position[0] = first_point_position.x().dval();
		vertex.fill_position[1] = first_point_position.y().dval();
		vertex.fill_position[2] = first_point_position.z().dval();
		if (!fill_stream_triangle_fans.add_vertex(vertex))
		{
			suspend_render_resume_vertex_array_streaming<FillRegionOfInterestVertex>(
					renderer,
					fill_stream_target,
					map_vertex_element_buffer_scope,
					map_vertex_buffer_scope,
					d_fill_region_of_interest_vertex_array,
					GL_TRIANGLES);
			fill_stream_triangle_fans.add_vertex(vertex);
		}

		fill_stream_triangle_fans.end_triangle_fan();
	}

	// Stop streaming so we can render the last batch.
	end_vertex_array_streaming<FillRegionOfInterestVertex>(
			renderer,
			fill_stream_target,
			map_vertex_element_buffer_scope,
			map_vertex_buffer_scope);

	// Render the last batch streamed (if any).
	render_vertex_array_stream<FillRegionOfInterestVertex>(
			renderer,
			fill_stream_target,
			d_fill_region_of_interest_vertex_array,
			GL_TRIANGLES);

	// Set the blend state back to the default state.
	renderer.gl_enable(GL_BLEND, false);
	renderer.gl_blend_func();
}


void
GPlatesOpenGL::GLRasterCoRegistration::mask_target_raster_with_regions_of_interest(
		GLRenderer &renderer,
		const Operation &operation,
		const GPlatesMaths::UnitVector3D &cube_face_centre,
		const GLTexture::shared_ptr_type &target_raster_texture,
		const GLTexture::shared_ptr_type &region_of_interest_mask_texture,
		GLBuffer::MapBufferScope &map_vertex_element_buffer_scope,
		GLBuffer::MapBufferScope &map_vertex_buffer_scope,
		const SeedCoRegistrationGeometryLists &geometry_lists)
{
	//PROFILE_FUNC();

	// Determine which filter operation to use.
	GLProgramObject::shared_ptr_type mask_region_of_interest_program_object;
	switch (operation.d_operation)
	{
		// Both mean and standard deviation are filtered using moments.
	case OPERATION_MEAN:
	case OPERATION_STANDARD_DEVIATION:
		mask_region_of_interest_program_object = d_mask_region_of_interest_moments_program_object;
		// Set the cube face centre - needed to adjust for cube map area-weighting distortion.
		mask_region_of_interest_program_object->gl_uniform3f(renderer, "cube_face_centre", cube_face_centre);
		break;

		// Both min and max are filtered using minmax.
	case OPERATION_MINIMUM:
	case OPERATION_MAXIMUM:
		mask_region_of_interest_program_object = d_mask_region_of_interest_minmax_program_object;
		break;

	default:
		// Shouldn't get here.
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
		break;
	}

	// Bind the shader program for masking target raster with regions-of-interest.
	renderer.gl_bind_program_object(mask_region_of_interest_program_object);

	// Set the target raster texture sampler to texture unit 0.
	mask_region_of_interest_program_object->gl_uniform1i(
			renderer, "target_raster_texture_sampler", 0/*texture unit*/);
	// Bind the target raster texture to texture unit 0.
	renderer.gl_bind_texture(target_raster_texture, GL_TEXTURE0, GL_TEXTURE_2D);

	// Set the region-of-interest mask texture sampler to texture unit 1.
	mask_region_of_interest_program_object->gl_uniform1i(
			renderer, "region_of_interest_mask_texture_sampler", 1/*texture unit*/);
	// Bind the region-of-interest mask texture to texture unit 1.
	renderer.gl_bind_texture(region_of_interest_mask_texture, GL_TEXTURE1, GL_TEXTURE_2D);

	// Bind the mask target raster with regions-of-interest vertex array.
	d_mask_region_of_interest_vertex_array->gl_bind(renderer);

	// For streaming MaskRegionOfInterestVertex vertices.
	mask_region_of_interest_stream_primitives_type mask_stream;
	mask_region_of_interest_stream_primitives_type::StreamTarget mask_stream_target(mask_stream);

	// Start streaming point region-of-interest geometries.
	begin_vertex_array_streaming<MaskRegionOfInterestVertex>(
			renderer,
			mask_stream_target,
			map_vertex_element_buffer_scope,
			map_vertex_buffer_scope);

	mask_region_of_interest_stream_primitives_type::Primitives mask_stream_quads(mask_stream);

	// Iterate over the seed points.
	seed_co_registration_points_list_type::const_iterator points_iter = geometry_lists.points_list.begin();
	seed_co_registration_points_list_type::const_iterator points_end = geometry_lists.points_list.end();
	for ( ; points_iter != points_end; ++points_iter)
	{
		const SeedCoRegistration &seed_co_registration = *points_iter;

		// Copy the seed geometry's frustum region of the target raster.
		mask_target_raster_with_region_of_interest(
				renderer,
				map_vertex_element_buffer_scope,
				map_vertex_buffer_scope,
				mask_stream_target,
				mask_stream_quads,
				seed_co_registration);
	}

	// Iterate over the seed multipoints.
	seed_co_registration_multi_points_list_type::const_iterator multi_points_iter = geometry_lists.multi_points_list.begin();
	seed_co_registration_multi_points_list_type::const_iterator multi_points_end = geometry_lists.multi_points_list.end();
	for ( ; multi_points_iter != multi_points_end; ++multi_points_iter)
	{
		const SeedCoRegistration &seed_co_registration = *multi_points_iter;

		// Copy the seed geometry's frustum region of the target raster.
		mask_target_raster_with_region_of_interest(
				renderer,
				map_vertex_element_buffer_scope,
				map_vertex_buffer_scope,
				mask_stream_target,
				mask_stream_quads,
				seed_co_registration);
	}

	// Iterate over the seed polylines.
	seed_co_registration_polylines_list_type::const_iterator polylines_iter = geometry_lists.polylines_list.begin();
	seed_co_registration_polylines_list_type::const_iterator polylines_end = geometry_lists.polylines_list.end();
	for ( ; polylines_iter != polylines_end; ++polylines_iter)
	{
		const SeedCoRegistration &seed_co_registration = *polylines_iter;

		// Copy the seed geometry's frustum region of the target raster.
		mask_target_raster_with_region_of_interest(
				renderer,
				map_vertex_element_buffer_scope,
				map_vertex_buffer_scope,
				mask_stream_target,
				mask_stream_quads,
				seed_co_registration);
	}

	// Iterate over the seed polygons.
	seed_co_registration_polygons_list_type::const_iterator polygons_iter = geometry_lists.polygons_list.begin();
	seed_co_registration_polygons_list_type::const_iterator polygons_end = geometry_lists.polygons_list.end();
	for ( ; polygons_iter != polygons_end; ++polygons_iter)
	{
		const SeedCoRegistration &seed_co_registration = *polygons_iter;

		// Copy the seed geometry's frustum region of the target raster.
		mask_target_raster_with_region_of_interest(
				renderer,
				map_vertex_element_buffer_scope,
				map_vertex_buffer_scope,
				mask_stream_target,
				mask_stream_quads,
				seed_co_registration);
	}

	// Stop streaming so we can render the last batch.
	end_vertex_array_streaming<MaskRegionOfInterestVertex>(
			renderer,
			mask_stream_target,
			map_vertex_element_buffer_scope,
			map_vertex_buffer_scope);

	// Render the last batch of streamed primitives (if any).
	render_vertex_array_stream<MaskRegionOfInterestVertex>(
			renderer,
			mask_stream_target,
			d_mask_region_of_interest_vertex_array,
			GL_TRIANGLES);
}


void
GPlatesOpenGL::GLRasterCoRegistration::mask_target_raster_with_region_of_interest(
		GLRenderer &renderer,
		GLBuffer::MapBufferScope &map_vertex_element_buffer_scope,
		GLBuffer::MapBufferScope &map_vertex_buffer_scope,
		mask_region_of_interest_stream_primitives_type::StreamTarget &mask_stream_target,
		mask_region_of_interest_stream_primitives_type::Primitives &mask_stream_quads,
		const SeedCoRegistration &seed_co_registration)
{
	// There are four vertices for the current quad and two triangles (three indices each).
	if (!mask_stream_quads.begin_primitive(
			4/*max_num_vertices*/,
			6/*max_num_vertex_elements*/))
	{
		// There's not enough vertices or indices so render what we have so far and
		// obtain new stream buffers.
		suspend_render_resume_vertex_array_streaming<MaskRegionOfInterestVertex>(
				renderer,
				mask_stream_target,
				map_vertex_element_buffer_scope,
				map_vertex_buffer_scope,
				d_mask_region_of_interest_vertex_array,
				GL_TRIANGLES);
	}

	// Some of the vertex data is the same for all vertices for the current quad.
	// The quad maps to the subsection used for the current seed geometry.
	MaskRegionOfInterestVertex vertex;

	vertex.raster_frustum_to_seed_frustum_clip_space_transform[0] =
			seed_co_registration.raster_frustum_to_seed_frustum_post_projection_translate_x;
	vertex.raster_frustum_to_seed_frustum_clip_space_transform[1] =
			seed_co_registration.raster_frustum_to_seed_frustum_post_projection_translate_y;
	vertex.raster_frustum_to_seed_frustum_clip_space_transform[2] =
			seed_co_registration.raster_frustum_to_seed_frustum_post_projection_scale;

	vertex.seed_frustum_to_render_target_clip_space_transform[0] =
			seed_co_registration.seed_frustum_to_render_target_post_projection_translate_x;
	vertex.seed_frustum_to_render_target_clip_space_transform[1] =
			seed_co_registration.seed_frustum_to_render_target_post_projection_translate_y;
	vertex.seed_frustum_to_render_target_clip_space_transform[2] =
			seed_co_registration.seed_frustum_to_render_target_post_projection_scale;

	vertex.screen_space_position[0] = -1;
	vertex.screen_space_position[1] = -1;
	mask_stream_quads.add_vertex(vertex);

	vertex.screen_space_position[0] = -1;
	vertex.screen_space_position[1] = 1;
	mask_stream_quads.add_vertex(vertex);

	vertex.screen_space_position[0] = 1;
	vertex.screen_space_position[1] = 1;
	mask_stream_quads.add_vertex(vertex);

	vertex.screen_space_position[0] = 1;
	vertex.screen_space_position[1] = -1;
	mask_stream_quads.add_vertex(vertex);

	//
	// Add the quad triangles.
	//

	mask_stream_quads.add_vertex_element(0);
	mask_stream_quads.add_vertex_element(1);
	mask_stream_quads.add_vertex_element(2);

	mask_stream_quads.add_vertex_element(0);
	mask_stream_quads.add_vertex_element(2);
	mask_stream_quads.add_vertex_element(3);

	mask_stream_quads.end_primitive();
}


template <typename StreamingVertexType>
void
GPlatesOpenGL::GLRasterCoRegistration::begin_vertex_array_streaming(
		GLRenderer &renderer,
		typename GLStaticStreamPrimitives<StreamingVertexType, streaming_vertex_element_type>::StreamTarget &stream_target,
		GLBuffer::MapBufferScope &map_vertex_element_buffer_scope,
		GLBuffer::MapBufferScope &map_vertex_buffer_scope)
{
	//PROFILE_FUNC();

	// Start the vertex element stream mapping.
	unsigned int vertex_element_stream_offset;
	unsigned int vertex_element_stream_bytes_available;
	void *vertex_element_data = map_vertex_element_buffer_scope.gl_map_buffer_stream(
			MINIMUM_BYTES_TO_STREAM_IN_VERTEX_ELEMENT_BUFFER,
			sizeof(streaming_vertex_element_type)/*stream_alignment*/,
			vertex_element_stream_offset,
			vertex_element_stream_bytes_available);

	// Start the vertex stream mapping.
	unsigned int vertex_stream_offset;
	unsigned int vertex_stream_bytes_available;
	void *vertex_data = map_vertex_buffer_scope.gl_map_buffer_stream(
			MINIMUM_BYTES_TO_STREAM_IN_VERTEX_BUFFER,
			sizeof(StreamingVertexType)/*stream_alignment*/,
			vertex_stream_offset,
			vertex_stream_bytes_available);

	// Convert bytes to vertex/index counts.
	unsigned int base_vertex_element_offset =
			vertex_element_stream_offset / sizeof(streaming_vertex_element_type);
	unsigned int num_vertex_elements_available =
			vertex_element_stream_bytes_available / sizeof(streaming_vertex_element_type);
	unsigned int base_vertex_offset =
			vertex_stream_offset / sizeof(StreamingVertexType);
	unsigned int num_vertices_available =
			vertex_stream_bytes_available / sizeof(StreamingVertexType);

	// Start streaming into the newly mapped vertex/index buffers.
	stream_target.start_streaming(
			// Setting 'initial_count' for vertices ensures the vertex indices are correct...
			boost::in_place(
					static_cast<StreamingVertexType *>(vertex_data),
					num_vertices_available,
					base_vertex_offset/*initial_count*/),
			boost::in_place(
					static_cast<streaming_vertex_element_type *>(vertex_element_data),
					num_vertex_elements_available,
					base_vertex_element_offset/*initial_count*/));
}


template <typename StreamingVertexType>
void
GPlatesOpenGL::GLRasterCoRegistration::end_vertex_array_streaming(
		GLRenderer &renderer,
		typename GLStaticStreamPrimitives<StreamingVertexType, streaming_vertex_element_type>::StreamTarget &stream_target,
		GLBuffer::MapBufferScope &map_vertex_element_buffer_scope,
		GLBuffer::MapBufferScope &map_vertex_buffer_scope)
{
	//PROFILE_FUNC();

	stream_target.stop_streaming();

	// Flush the data streamed so far (which could be no data).
	map_vertex_element_buffer_scope.gl_flush_buffer_stream(
			stream_target.get_num_streamed_vertex_elements() * sizeof(streaming_vertex_element_type));
	map_vertex_buffer_scope.gl_flush_buffer_stream(
			stream_target.get_num_streamed_vertices() * sizeof(StreamingVertexType));

	// Check return code in case mapped data got corrupted.
	// This shouldn't happen but we'll emit a warning message if it does.
	const bool vertex_element_buffer_unmap_result = map_vertex_element_buffer_scope.gl_unmap_buffer();
	const bool vertex_buffer_unmap_result = map_vertex_buffer_scope.gl_unmap_buffer();
	if (!vertex_element_buffer_unmap_result || !vertex_buffer_unmap_result)
	{
		qWarning() << "GLRasterCoRegistration: Failed to unmap vertex stream.";
	}
}


template <typename StreamingVertexType>
void
GPlatesOpenGL::GLRasterCoRegistration::render_vertex_array_stream(
		GLRenderer &renderer,
		typename GLStaticStreamPrimitives<StreamingVertexType, streaming_vertex_element_type>::StreamTarget &stream_target,
		const GLVertexArray::shared_ptr_type &vertex_array,
		GLenum primitive_mode)
{
	//PROFILE_FUNC();

	// Only render if we've got some data to render.
	if (stream_target.get_num_streamed_vertex_elements() == 0)
	{
		return;
	}

	// Draw the primitives.
	// NOTE: The caller should have already bound this vertex array.
	vertex_array->gl_draw_range_elements(
			renderer,
			primitive_mode,
			stream_target.get_start_streaming_vertex_count()/*start*/,
			stream_target.get_start_streaming_vertex_count() +
					stream_target.get_num_streamed_vertices() - 1/*end*/,
			stream_target.get_num_streamed_vertex_elements()/*count*/,
			GLVertexElementTraits<streaming_vertex_element_type>::type,
			stream_target.get_start_streaming_vertex_element_count() *
					sizeof(streaming_vertex_element_type)/*indices_offset*/);
}


template <typename StreamingVertexType>
void
GPlatesOpenGL::GLRasterCoRegistration::suspend_render_resume_vertex_array_streaming(
		GLRenderer &renderer,
		typename GLStaticStreamPrimitives<StreamingVertexType, streaming_vertex_element_type>::StreamTarget &stream_target,
		GLBuffer::MapBufferScope &map_vertex_element_buffer_scope,
		GLBuffer::MapBufferScope &map_vertex_buffer_scope,
		const GLVertexArray::shared_ptr_type &vertex_array,
		GLenum primitive_mode)
{
	// Temporarily suspend streaming.
	end_vertex_array_streaming<StreamingVertexType>(
			renderer, stream_target, map_vertex_element_buffer_scope, map_vertex_buffer_scope);

	// Render the primitives streamed so far.
	render_vertex_array_stream<StreamingVertexType>(renderer, stream_target, vertex_array, primitive_mode);

	// Resume streaming.
	begin_vertex_array_streaming<StreamingVertexType>(
			renderer, stream_target, map_vertex_element_buffer_scope, map_vertex_buffer_scope);
}


void
GPlatesOpenGL::GLRasterCoRegistration::render_reduction_of_reduce_stage(
		GLRenderer &renderer,
		const Operation &operation,
		const ReduceQuadTreeInternalNode &dst_reduce_quad_tree_node,
		unsigned int src_child_x_offset,
		unsigned int src_child_y_offset,
		bool clear_dst_reduce_stage_texture,
		const GLTexture::shared_ptr_type &dst_reduce_stage_texture,
		const GLTexture::shared_ptr_type &src_reduce_stage_texture)
{
	//PROFILE_FUNC();

	// Make sure we leave the OpenGL state the way it was.
	GLRenderer::StateBlockScope save_restore_state(
			renderer,
			// We're rendering to a render target so reset to the default OpenGL state...
			true/*reset_to_default_state*/);

	// Begin rendering to the destination reduce stage texture.
	d_framebuffer_object->gl_attach(
			renderer, GL_TEXTURE_2D, dst_reduce_stage_texture, 0/*level*/, GL_COLOR_ATTACHMENT0_EXT);
	renderer.gl_bind_frame_buffer(d_framebuffer_object);

	// Render to the entire reduce stage texture.
	renderer.gl_viewport(0, 0, TEXTURE_DIMENSION, TEXTURE_DIMENSION);

	// If the destination reduce stage texture does not contain partial results then it'll need to be cleared.
	// This happens when starting afresh with a newly acquired destination reduce stage texture.
	if (clear_dst_reduce_stage_texture)
	{
		// Clear colour to all zeros - this means when texels with zero coverage get discarded the framebuffer
		// will have coverage values of zero (causing them to not contribute to the co-registration result).
		renderer.gl_clear_color();
		renderer.gl_clear(GL_COLOR_BUFFER_BIT); // Clear only the colour buffer.
	}

	// Determine which reduction operation to use.
	GLProgramObject::shared_ptr_type reduction_program_object;
	switch (operation.d_operation)
	{
		// Both mean and standard deviation can be reduced using summation.
	case OPERATION_MEAN:
	case OPERATION_STANDARD_DEVIATION:
		reduction_program_object = d_reduction_sum_program_object;
		break;

	case OPERATION_MINIMUM:
		reduction_program_object = d_reduction_min_program_object;
		break;

	case OPERATION_MAXIMUM:
		reduction_program_object = d_reduction_max_program_object;
		break;

	default:
		// Shouldn't get here.
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
		break;
	}

	// Bind the shader program for reducing the regions-of-interest filter results.
	renderer.gl_bind_program_object(reduction_program_object);

	// Set the reduce source texture sampler to texture unit 0.
	reduction_program_object->gl_uniform1i(
			renderer, "reduce_source_texture_sampler", 0/*texture unit*/);
	// Bind the source reduce stage texture to texture unit 0.
	renderer.gl_bind_texture(src_reduce_stage_texture, GL_TEXTURE0, GL_TEXTURE_2D);

	// Set the half-texel offset of the reduce source texture (all reduce textures have same dimension).
	const double half_texel_offset = 0.5 / TEXTURE_DIMENSION;
	reduction_program_object->gl_uniform2f(
			renderer, "reduce_source_texture_half_texel_offset", half_texel_offset, -half_texel_offset);
	// Determine which quadrant of the destination reduce texture to render to.
	// Map the range [-1,1] to one of [-1,0] or [0,1] for both x and y directions.
	reduction_program_object->gl_uniform3f(
			renderer,
			"target_quadrant_translate_scale",
			0.5 * (src_child_x_offset ? 1 : -1), // translate_x
			0.5 * (src_child_y_offset ? 1 : -1),  // translate_y
			0.5); // scale

	// Bind the reduction vertex array.
	d_reduction_vertex_array->gl_bind(renderer);

	// Determine how many quads, in the reduction vertex array, to render based on how much data
	// needs to be reduced (which is determined by how full the reduce quad-subtree begin rendered is).
	const unsigned int num_reduce_quads_spanned =
			find_number_reduce_vertex_array_quads_spanned_by_child_reduce_quad_tree_node(
					dst_reduce_quad_tree_node,
					src_child_x_offset,
					src_child_y_offset,
					NUM_REDUCE_VERTEX_ARRAY_QUADS_ACROSS_TEXTURE/*child_quad_tree_node_width_in_quads*/);
	// Shouldn't get zero quads.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			num_reduce_quads_spanned > 0,
			GPLATES_ASSERTION_SOURCE);

	// Draw the required number of quads in the reduction vertex array.
	d_reduction_vertex_array->gl_draw_range_elements(
			renderer,
			GL_TRIANGLES,
			0/*start*/,
			4 * num_reduce_quads_spanned - 1/*end*/, // Each quad has four vertices.
			6 * num_reduce_quads_spanned/*count*/,   // Each quad has two triangles of three indices each.
			GLVertexElementTraits<reduction_vertex_element_type>::type,
			0/*indices_offset*/);

	//debug_floating_point_render_target(
	//		renderer, "reduction_raster", false/*coverage_is_in_green_channel*/);
}


unsigned int
GPlatesOpenGL::GLRasterCoRegistration::find_number_reduce_vertex_array_quads_spanned_by_child_reduce_quad_tree_node(
		const ReduceQuadTreeInternalNode &parent_reduce_quad_tree_node,
		unsigned int child_x_offset,
		unsigned int child_y_offset,
		unsigned int child_quad_tree_node_width_in_quads)
{
	// Should never get zero coverage of quads across child quad tree node.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			child_quad_tree_node_width_in_quads > 0,
			GPLATES_ASSERTION_SOURCE);

	const unsigned child_reduce_stage_index = parent_reduce_quad_tree_node.get_reduce_stage_index() - 1;

	// We've reached a leaf node.
	if (child_reduce_stage_index == 0)
	{
		// If there's no child (leaf) node then return zero.
		if (!parent_reduce_quad_tree_node.get_child_leaf_node(child_x_offset, child_y_offset))
		{
			return 0;
		}

		// All of a leaf node must be reduced.
		return child_quad_tree_node_width_in_quads * child_quad_tree_node_width_in_quads;
	}

	// The child node is an *internal* node.
	const ReduceQuadTreeInternalNode *child_reduce_quad_tree_internal_node =
			parent_reduce_quad_tree_node.get_child_internal_node(child_x_offset, child_y_offset);

	// If there's no child (internal) node then return zero.
	if (!child_reduce_quad_tree_internal_node)
	{
		return 0;
	}

	// If the child node subtree is full then all of it needs to be reduced.
	if (child_reduce_quad_tree_internal_node->is_sub_tree_full())
	{
		return child_quad_tree_node_width_in_quads * child_quad_tree_node_width_in_quads;
	}

	// Each quad in the reduce vertex array can only span MINIMUM_SEED_GEOMETRIES_VIEWPORT_DIMENSION
	// pixels dimension. Whereas the reduction operation eventually reduces each seed co-registration
	// results down to a single pixel. So the quads cannot represent this fine a detail so we just
	// work in blocks of dimension MINIMUM_SEED_GEOMETRIES_VIEWPORT_DIMENSION. When the block is not
	// full (ie, not all of a block contains data being reduced) it just means that OpenGL is
	// processing/reducing some pixels that it doesn't need to (but they don't get used anyway).
	if (child_quad_tree_node_width_in_quads == 1)
	{
		// One MINIMUM_SEED_GEOMETRIES_VIEWPORT_DIMENSION x MINIMUM_SEED_GEOMETRIES_VIEWPORT_DIMENSION.
		return 1;
	}

	// The number of quads spanned by the current child node.
	unsigned int num_quads_spanned = 0;

	// Recurse into the grand child reduce quad tree nodes.
	for (unsigned int grand_child_y_offset = 0; grand_child_y_offset < 2; ++grand_child_y_offset)
	{
		for (unsigned int grand_child_x_offset = 0; grand_child_x_offset < 2; ++grand_child_x_offset)
		{
			num_quads_spanned +=
					find_number_reduce_vertex_array_quads_spanned_by_child_reduce_quad_tree_node(
							*child_reduce_quad_tree_internal_node,
							grand_child_x_offset,
							grand_child_y_offset,
							// Child nodes cover half the dimension of the texture...
							child_quad_tree_node_width_in_quads / 2);
		}
	}

	return num_quads_spanned;
}


bool
GPlatesOpenGL::GLRasterCoRegistration::render_target_raster(
		GLRenderer &renderer,
		const CoRegistrationParameters &co_registration_parameters,
		const GLTexture::shared_ptr_type &target_raster_texture,
		const GLTransform &view_transform,
		const GLTransform &projection_transform)
{
	//PROFILE_FUNC();

	// Make sure we leave the OpenGL state the way it was.
	GLRenderer::StateBlockScope save_restore_state(
			renderer,
			// We're rendering to a render target so reset to the default OpenGL state...
			true/*reset_to_default_state*/);

	// Begin rendering to the 2D texture.
	d_framebuffer_object->gl_attach(renderer, GL_TEXTURE_2D, target_raster_texture, 0/*level*/, GL_COLOR_ATTACHMENT0_EXT);
	renderer.gl_bind_frame_buffer(d_framebuffer_object);

	// Render to the entire texture.
	renderer.gl_viewport(0, 0, TEXTURE_DIMENSION, TEXTURE_DIMENSION);

	renderer.gl_clear_color(); // Clear colour to all zeros.
	renderer.gl_clear(GL_COLOR_BUFFER_BIT); // Clear only the colour buffer.

	renderer.gl_load_matrix(GL_MODELVIEW, view_transform.get_matrix());
	renderer.gl_load_matrix(GL_PROJECTION, projection_transform.get_matrix());

	// Render the target raster into the view frustum.
	GLMultiResolutionRasterInterface::cache_handle_type cache_handle;
	// Render target raster and return true if there was any rendering into the view frustum.
	const bool raster_rendered = co_registration_parameters.d_target_raster->render(
			renderer,
			co_registration_parameters.d_raster_level_of_detail,
			cache_handle);

	//debug_floating_point_render_target(
	//		renderer, "raster", true/*coverage_is_in_green_channel*/);

	return raster_rendered;
}


GPlatesOpenGL::GLTexture::shared_ptr_type
GPlatesOpenGL::GLRasterCoRegistration::acquire_rgba_float_texture(
		GLRenderer &renderer)
{
	// Acquire a cached floating-point texture.
	// It'll get returned to its cache when we no longer reference it.
	const GLTexture::shared_ptr_type texture =
			renderer.get_context().get_shared_state()->acquire_texture(
					renderer,
					GL_TEXTURE_2D,
					GL_RGBA32F_ARB,
					TEXTURE_DIMENSION,
					TEXTURE_DIMENSION);

	// 'acquire_texture' initialises the texture memory (to empty) but does not set the filtering
	// state when it creates a new texture.
	// Also even if the texture was cached it might have been used by another client that specified
	// different filtering settings for it.
	// So we set the filtering settings each time we acquire.

	// For floating-point textures turn off any linear/anisotropic filtering (earlier floating-point
	// texture hardware does not support it).
	texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	if (GLEW_EXT_texture_filter_anisotropic)
	{
		texture->gl_tex_parameterf(renderer, GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);
	}

	// Clamp texture coordinates to centre of edge texels -
	// it's easier for hardware to implement - and doesn't affect our calculations.
	if (GLEW_EXT_texture_edge_clamp || GLEW_SGIS_texture_edge_clamp)
	{
		texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	else
	{
		texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	}

	return texture;
}


GPlatesOpenGL::GLTexture::shared_ptr_type
GPlatesOpenGL::GLRasterCoRegistration::acquire_rgba_fixed_texture(
		GLRenderer &renderer)
{
	// Acquire a cached fixed-point texture.
	// It'll get returned to its cache when we no longer reference it.
	const GLTexture::shared_ptr_type texture =
			renderer.get_context().get_shared_state()->acquire_texture(
					renderer,
					GL_TEXTURE_2D,
					GL_RGBA8,
					TEXTURE_DIMENSION,
					TEXTURE_DIMENSION);

	// 'acquire_texture' initialises the texture memory (to empty) but does not set the filtering
	// state when it creates a new texture.
	// Also even if the texture was cached it might have been used by another client that specified
	// different filtering settings for it.
	// So we set the filtering settings each time we acquire.

	// Turn off any linear/anisotropic filtering - we're using one-to-one texel-to-pixel mapping.
	texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	if (GLEW_EXT_texture_filter_anisotropic)
	{
		texture->gl_tex_parameterf(renderer, GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);
	}

	// Clamp texture coordinates to centre of edge texels -
	// it's easier for hardware to implement - and doesn't affect our calculations.
	if (GLEW_EXT_texture_edge_clamp || GLEW_SGIS_texture_edge_clamp)
	{
		texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	else
	{
		texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	}

	return texture;
}


void
GPlatesOpenGL::GLRasterCoRegistration::return_co_registration_results_to_caller(
		const CoRegistrationParameters &co_registration_parameters)
{
	// Now that the results have all been retrieved from the GPU we need combine multiple
	// (potentially partial) co-registration results into a single result per seed feature.
	for (unsigned int operation_index = 0;
		operation_index < co_registration_parameters.operations.size();
		++operation_index)
	{
		Operation &operation = co_registration_parameters.operations[operation_index];

		// There is one list of (partial) co-registration results for each seed feature.
		const OperationSeedFeaturePartialResults &operation_seed_feature_partial_results =
				co_registration_parameters.seed_feature_partial_results[operation_index];

		const unsigned int num_seed_features = co_registration_parameters.d_seed_features.size();
		for (unsigned int feature_index = 0; feature_index < num_seed_features; ++feature_index)
		{
			const seed_co_registration_partial_result_list_type &partial_results_list =
					operation_seed_feature_partial_results.partial_result_lists[feature_index];

			// If there are no results for the current seed feature then either the seed feature
			// doesn't exist (at the current reconstruction time) or the target raster did not
			// overlap the seed feature's geometry(s) - in either case leave result as 'boost::none'.
			if (partial_results_list.empty())
			{
				continue;
			}

			// Combine the partial results for the current seed feature depending on the operation type.
			switch (operation.d_operation)
			{
			case OPERATION_MEAN:
				{
					double coverage = 0;
					double coverage_weighted_mean = 0;

					seed_co_registration_partial_result_list_type::const_iterator partial_results_iter =
							partial_results_list.begin();
					seed_co_registration_partial_result_list_type::const_iterator partial_results_end =
							partial_results_list.end();
					for ( ; partial_results_iter != partial_results_end; ++partial_results_iter)
					{
						const SeedCoRegistrationPartialResult &partial_result = *partial_results_iter;

						// The partial result only contributes if it has non-zero coverage.
						if (GPlatesMaths::real_t(partial_result.result_pixel.alpha) != 0)
						{
							// The alpha and red components are coverage and coverage_weighted_mean.
							coverage += partial_result.result_pixel.alpha;
							coverage_weighted_mean += partial_result.result_pixel.red;
						}
					}

					// If the coverage is zero then it means the seed geometry(s) did not overlap
					// with the target raster and hence we should leave the result as 'boost::none'.
					if (GPlatesMaths::real_t(coverage) != 0)
					{
						// Store final mean result.
						operation.d_results[feature_index] = coverage_weighted_mean / coverage;
					}
				}
				break;

			case OPERATION_STANDARD_DEVIATION:
				{
					double coverage = 0;
					double coverage_weighted_mean = 0;
					double coverage_weighted_second_moment = 0;

					seed_co_registration_partial_result_list_type::const_iterator partial_results_iter =
							partial_results_list.begin();
					seed_co_registration_partial_result_list_type::const_iterator partial_results_end =
							partial_results_list.end();
					for ( ; partial_results_iter != partial_results_end; ++partial_results_iter)
					{
						const SeedCoRegistrationPartialResult &partial_result = *partial_results_iter;

						// The partial result only contributes if it has non-zero coverage.
						if (GPlatesMaths::real_t(partial_result.result_pixel.alpha) != 0)
						{
							// The alpha/red/green components are coverage/coverage_weighted_mean/coverage_weighted_second_moment.
							coverage += partial_result.result_pixel.alpha;
							coverage_weighted_mean += partial_result.result_pixel.red;
							coverage_weighted_second_moment += partial_result.result_pixel.green;
						}
					}

					// If the coverage is zero then it means the seed geometry(s) did not overlap
					// with the target raster and hence we should leave the result as 'boost::none'.
					if (GPlatesMaths::real_t(coverage) != 0)
					{
						// mean = M = sum(Ci * Xi) / sum(Ci)
						// std_dev  = sqrt[sum(Ci * (Xi - M)^2) / sum(Ci)]
						//          = sqrt[(sum(Ci * Xi^2) - 2 * M * sum(Ci * Xi) + M^2 * sum(Ci)) / sum(Ci)]
						//          = sqrt[(sum(Ci * Xi^2) - 2 * M * M * sum(Ci) + M^2 * sum(Ci)) / sum(Ci)]
						//          = sqrt[(sum(Ci * Xi^2) - M^2 * sum(Ci)) / sum(Ci)]
						//          = sqrt[(sum(Ci * Xi^2) / sum(Ci) - M^2]
						const double inverse_coverage = 1.0 / coverage;
						const double mean = inverse_coverage * coverage_weighted_mean;

						// Store final standard deviation result.
						const double variance = inverse_coverage * coverage_weighted_second_moment - mean * mean;
						// Protect 'sqrt' in case variance is slightly negative due to numerical precision.
						operation.d_results[feature_index] = (variance > 0) ? std::sqrt(variance) : 0;
					}
				}
				break;

			case OPERATION_MINIMUM:
				{
					double max_coverage = 0;
					// The parentheses around max prevent windows max macro from stuffing numeric_limits' max.
					double min_value = (std::numeric_limits<double>::max)();

					seed_co_registration_partial_result_list_type::const_iterator partial_results_iter =
							partial_results_list.begin();
					seed_co_registration_partial_result_list_type::const_iterator partial_results_end =
							partial_results_list.end();
					for ( ; partial_results_iter != partial_results_end; ++partial_results_iter)
					{
						const SeedCoRegistrationPartialResult &partial_result = *partial_results_iter;

						// The partial result only contributes if it has non-zero coverage.
						if (GPlatesMaths::real_t(partial_result.result_pixel.alpha) != 0)
						{
							// The alpha and red components are coverage and min_value.
							if (max_coverage < partial_result.result_pixel.alpha)
							{
								max_coverage = partial_result.result_pixel.alpha;
							}
							if (min_value > partial_result.result_pixel.red)
							{
								min_value = partial_result.result_pixel.red;
							}
						}
					}

					// If the coverage is zero then it means the seed geometry(s) did not overlap
					// with the target raster and hence we should leave the result as 'boost::none'.
					if (GPlatesMaths::real_t(max_coverage) != 0)
					{
						// Store final minimum result.
						operation.d_results[feature_index] = min_value;
					}
				}
				break;

			case OPERATION_MAXIMUM:
				{
					double max_coverage = 0;
					// The parentheses around max prevent windows max macro from stuffing numeric_limits' max.
					double max_value = -(std::numeric_limits<double>::max)();

					seed_co_registration_partial_result_list_type::const_iterator partial_results_iter =
							partial_results_list.begin();
					seed_co_registration_partial_result_list_type::const_iterator partial_results_end =
							partial_results_list.end();
					for ( ; partial_results_iter != partial_results_end; ++partial_results_iter)
					{
						const SeedCoRegistrationPartialResult &partial_result = *partial_results_iter;

						// The partial result only contributes if it has non-zero coverage.
						if (GPlatesMaths::real_t(partial_result.result_pixel.alpha) != 0)
						{
							// The alpha and red components are coverage and max_value.
							if (max_coverage < partial_result.result_pixel.alpha)
							{
								max_coverage = partial_result.result_pixel.alpha;
							}
							if (max_value < partial_result.result_pixel.red)
							{
								max_value = partial_result.result_pixel.red;
							}
						}
					}

					// If the coverage is zero then it means the seed geometry(s) did not overlap
					// with the target raster and hence we should leave the result as 'boost::none'.
					if (GPlatesMaths::real_t(max_coverage) != 0)
					{
						// Store final maximum result.
						operation.d_results[feature_index] = max_value;
					}
				}
				break;

			default:
				// Shouldn't get here.
				GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
				break;
			}
		}
	}
}


#if defined (DEBUG_RASTER_COREGISTRATION_RENDER_TARGET)
void
GPlatesOpenGL::GLRasterCoRegistration::debug_fixed_point_render_target(
		GLRenderer &renderer,
		const QString &image_file_basename)
{
	// Make sure we leave the OpenGL state the way it was.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// Bind the pixel buffer so that all subsequent 'gl_read_pixels()' calls go into that buffer.
	d_debug_pixel_buffer->gl_bind_pack(renderer);

	// NOTE: We don't need to worry about changing the default GL_PACK_ALIGNMENT (rows aligned to 4 bytes)
	// since our data is RGBA (already 4-byte aligned).
	d_debug_pixel_buffer->gl_read_pixels(
			renderer,
			0,
			0,
			TEXTURE_DIMENSION,
			TEXTURE_DIMENSION,
			GL_RGBA,
			GL_UNSIGNED_BYTE,
			0);

	// Map the pixel buffer to access its data.
	GLBuffer::MapBufferScope map_pixel_buffer_scope(
			renderer,
			*d_debug_pixel_buffer->get_buffer(),
			GLBuffer::TARGET_PIXEL_PACK_BUFFER);

	// Map the pixel buffer data.
	const void *result_data = map_pixel_buffer_scope.gl_map_buffer_static(GLBuffer::ACCESS_READ_ONLY);
	const GPlatesGui::rgba8_t *result_rgba8_data = static_cast<const GPlatesGui::rgba8_t *>(result_data);

	boost::scoped_array<GPlatesGui::rgba8_t> rgba8_data(new GPlatesGui::rgba8_t[TEXTURE_DIMENSION * TEXTURE_DIMENSION]);

	for (unsigned int y = 0; y < TEXTURE_DIMENSION; ++y)
	{
		for (unsigned int x = 0; x < TEXTURE_DIMENSION; ++x)
		{
			const GPlatesGui::rgba8_t &result_pixel = result_rgba8_data[y * TEXTURE_DIMENSION + x];

			GPlatesGui::rgba8_t colour(0, 0, 0, 255);
			if (result_pixel.alpha == 0)
			{
				// Use blue to represent areas not in region-of-interest.
				colour.blue = 255;
			}
			else
			{
				colour.red = colour.green = colour.blue = 255;
			}

			rgba8_data.get()[y * TEXTURE_DIMENSION + x] = colour;
		}
	}

	const bool unmap_success = map_pixel_buffer_scope.gl_unmap_buffer();

	boost::scoped_array<boost::uint32_t> argb32_data(new boost::uint32_t[TEXTURE_DIMENSION * TEXTURE_DIMENSION]);

	// Convert to a format supported by QImage.
	GPlatesGui::convert_rgba8_to_argb32(
			rgba8_data.get(),
			argb32_data.get(),
			TEXTURE_DIMENSION * TEXTURE_DIMENSION);

	QImage qimage(
			static_cast<const uchar *>(static_cast<const void *>(argb32_data.get())),
			TEXTURE_DIMENSION,
			TEXTURE_DIMENSION,
			QImage::Format_ARGB32);

	static unsigned int s_image_count = 0;
	++s_image_count;

	// Save the image to a file.
	QString image_filename = QString("%1%2.png").arg(image_file_basename).arg(s_image_count);
	qimage.save(image_filename, "PNG");
}


void
GPlatesOpenGL::GLRasterCoRegistration::debug_floating_point_render_target(
		GLRenderer &renderer,
		const QString &image_file_basename,
		bool coverage_is_in_green_channel)
{
	// Make sure we leave the OpenGL state the way it was.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// Bind the pixel buffer so that all subsequent 'gl_read_pixels()' calls go into that buffer.
	d_debug_pixel_buffer->gl_bind_pack(renderer);

	// NOTE: We don't need to worry about changing the default GL_PACK_ALIGNMENT (rows aligned to 4 bytes)
	// since our data is floats (each float is already 4-byte aligned).
	d_debug_pixel_buffer->gl_read_pixels(
			renderer,
			0,
			0,
			TEXTURE_DIMENSION,
			TEXTURE_DIMENSION,
			GL_RGBA,
			GL_FLOAT,
			0);

	// Map the pixel buffer to access its data.
	GLBuffer::MapBufferScope map_pixel_buffer_scope(
			renderer,
			*d_debug_pixel_buffer->get_buffer(),
			GLBuffer::TARGET_PIXEL_PACK_BUFFER);

	// Map the pixel buffer data.
	const void *result_data = map_pixel_buffer_scope.gl_map_buffer_static(GLBuffer::ACCESS_READ_ONLY);
	const ResultPixel *result_pixel_data = static_cast<const ResultPixel *>(result_data);

	boost::scoped_array<GPlatesGui::rgba8_t> rgba8_data(new GPlatesGui::rgba8_t[TEXTURE_DIMENSION * TEXTURE_DIMENSION]);

	// Convert data from floating-point to fixed-point.
	const float range = 100.0f; // Change this depending on the range of the specific raster being debugged.
	const float inv_range = 1 / range;
	for (unsigned int y = 0; y < TEXTURE_DIMENSION; ++y)
	{
		for (unsigned int x = 0; x < TEXTURE_DIMENSION; ++x)
		{
			const ResultPixel &result_pixel = result_pixel_data[y * TEXTURE_DIMENSION + x];

			GPlatesGui::rgba8_t colour(0, 0, 0, 255);
			if ((coverage_is_in_green_channel && GPlatesMaths::are_almost_exactly_equal(result_pixel.green, 0)) ||
				(!coverage_is_in_green_channel && GPlatesMaths::are_almost_exactly_equal(result_pixel.alpha, 0)))
			{
				// Use blue to represent transparent areas or areas not in region-of-interest.
				colour.blue = 255;
			}
			else
			{
				// Transition from red to green over a periodic range to visualise raster pattern.
				const float rem = std::fabs(std::fmod(result_pixel.red, range));
				colour.red = boost::uint8_t(255 * rem * inv_range);
				colour.green = 255 - colour.red;
			}

			rgba8_data.get()[y * TEXTURE_DIMENSION + x] = colour;
		}
	}

	const bool unmap_success = map_pixel_buffer_scope.gl_unmap_buffer();

	boost::scoped_array<boost::uint32_t> argb32_data(new boost::uint32_t[TEXTURE_DIMENSION * TEXTURE_DIMENSION]);

	// Convert to a format supported by QImage.
	GPlatesGui::convert_rgba8_to_argb32(
			rgba8_data.get(),
			argb32_data.get(),
			TEXTURE_DIMENSION * TEXTURE_DIMENSION);

	QImage qimage(
			static_cast<const uchar *>(static_cast<const void *>(argb32_data.get())),
			TEXTURE_DIMENSION,
			TEXTURE_DIMENSION,
			QImage::Format_ARGB32);

	static unsigned int s_image_count = 0;
	++s_image_count;

	// Save the image to a file.
	QString image_filename = QString("%1%2.png").arg(image_file_basename).arg(s_image_count);
	qimage.save(image_filename, "PNG");
}
#endif


void
GPlatesOpenGL::GLRasterCoRegistration::PointRegionOfInterestVertex::initialise_seed_geometry_constants(
		const SeedCoRegistration &seed_co_registration)
{
	world_space_quaternion[0] = seed_co_registration.transform.x().dval();
	world_space_quaternion[1] = seed_co_registration.transform.y().dval();
	world_space_quaternion[2] = seed_co_registration.transform.z().dval();
	world_space_quaternion[3] = seed_co_registration.transform.w().dval();

	raster_frustum_to_seed_frustum_clip_space_transform[0] =
			seed_co_registration.raster_frustum_to_seed_frustum_post_projection_translate_x;
	raster_frustum_to_seed_frustum_clip_space_transform[1] =
			seed_co_registration.raster_frustum_to_seed_frustum_post_projection_translate_y;
	raster_frustum_to_seed_frustum_clip_space_transform[2] =
			seed_co_registration.raster_frustum_to_seed_frustum_post_projection_scale;

	seed_frustum_to_render_target_clip_space_transform[0] =
			seed_co_registration.seed_frustum_to_render_target_post_projection_translate_x;
	seed_frustum_to_render_target_clip_space_transform[1] =
			seed_co_registration.seed_frustum_to_render_target_post_projection_translate_y;
	seed_frustum_to_render_target_clip_space_transform[2] =
			seed_co_registration.seed_frustum_to_render_target_post_projection_scale;
}


void
GPlatesOpenGL::GLRasterCoRegistration::LineRegionOfInterestVertex::initialise_seed_geometry_constants(
		const SeedCoRegistration &seed_co_registration)
{
	world_space_quaternion[0] = seed_co_registration.transform.x().dval();
	world_space_quaternion[1] = seed_co_registration.transform.y().dval();
	world_space_quaternion[2] = seed_co_registration.transform.z().dval();
	world_space_quaternion[3] = seed_co_registration.transform.w().dval();

	raster_frustum_to_seed_frustum_clip_space_transform[0] =
			seed_co_registration.raster_frustum_to_seed_frustum_post_projection_translate_x;
	raster_frustum_to_seed_frustum_clip_space_transform[1] =
			seed_co_registration.raster_frustum_to_seed_frustum_post_projection_translate_y;
	raster_frustum_to_seed_frustum_clip_space_transform[2] =
			seed_co_registration.raster_frustum_to_seed_frustum_post_projection_scale;

	seed_frustum_to_render_target_clip_space_transform[0] =
			seed_co_registration.seed_frustum_to_render_target_post_projection_translate_x;
	seed_frustum_to_render_target_clip_space_transform[1] =
			seed_co_registration.seed_frustum_to_render_target_post_projection_translate_y;
	seed_frustum_to_render_target_clip_space_transform[2] =
			seed_co_registration.seed_frustum_to_render_target_post_projection_scale;
}


void
GPlatesOpenGL::GLRasterCoRegistration::FillRegionOfInterestVertex::initialise_seed_geometry_constants(
		const SeedCoRegistration &seed_co_registration)
{
	world_space_quaternion[0] = seed_co_registration.transform.x().dval();
	world_space_quaternion[1] = seed_co_registration.transform.y().dval();
	world_space_quaternion[2] = seed_co_registration.transform.z().dval();
	world_space_quaternion[3] = seed_co_registration.transform.w().dval();

	raster_frustum_to_seed_frustum_clip_space_transform[0] =
			seed_co_registration.raster_frustum_to_seed_frustum_post_projection_translate_x;
	raster_frustum_to_seed_frustum_clip_space_transform[1] =
			seed_co_registration.raster_frustum_to_seed_frustum_post_projection_translate_y;
	raster_frustum_to_seed_frustum_clip_space_transform[2] =
			seed_co_registration.raster_frustum_to_seed_frustum_post_projection_scale;

	seed_frustum_to_render_target_clip_space_transform[0] =
			seed_co_registration.seed_frustum_to_render_target_post_projection_translate_x;
	seed_frustum_to_render_target_clip_space_transform[1] =
			seed_co_registration.seed_frustum_to_render_target_post_projection_translate_y;
	seed_frustum_to_render_target_clip_space_transform[2] =
			seed_co_registration.seed_frustum_to_render_target_post_projection_scale;
}


const GPlatesOpenGL::GLRasterCoRegistration::ReduceQuadTreeInternalNode *
GPlatesOpenGL::GLRasterCoRegistration::ReduceQuadTreeInternalNode::get_child_internal_node(
		unsigned int child_x_offset,
		unsigned int child_y_offset) const
{
	const ReduceQuadTreeNode *child_node = d_children[child_y_offset][child_x_offset];
	if (!child_node)
	{
		return NULL;
	}

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			!child_node->is_leaf_node,
			GPLATES_ASSERTION_SOURCE);
	return static_cast<const ReduceQuadTreeInternalNode *>(child_node);
}


const GPlatesOpenGL::GLRasterCoRegistration::ReduceQuadTreeLeafNode *
GPlatesOpenGL::GLRasterCoRegistration::ReduceQuadTreeInternalNode::get_child_leaf_node(
		unsigned int child_x_offset,
		unsigned int child_y_offset) const
{
	const ReduceQuadTreeNode *child_node = d_children[child_y_offset][child_x_offset];
	if (!child_node)
	{
		return NULL;
	}

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			child_node->is_leaf_node,
			GPLATES_ASSERTION_SOURCE);
	return static_cast<const ReduceQuadTreeLeafNode *>(child_node);
}


GPlatesOpenGL::GLRasterCoRegistration::ReduceQuadTree::ReduceQuadTree() :
	d_root_node(*d_reduce_quad_tree_internal_node_pool.construct(NUM_REDUCE_STAGES - 1))
{
}


GPlatesOpenGL::GLRasterCoRegistration::ResultsQueue::ResultsQueue(
		GLRenderer &renderer)
{
	for (unsigned int n = 0; n < NUM_PIXEL_BUFFERS; ++n)
	{
		// Allocate enough memory in each pixel buffer to read back a floating-point texture.
		GLBuffer::shared_ptr_type buffer = GLBuffer::create(renderer);
		buffer->gl_buffer_data(
				renderer,
				GLBuffer::TARGET_PIXEL_PACK_BUFFER,
				PIXEL_BUFFER_SIZE_IN_BYTES,
				NULL, // Uninitialised memory.
				GLBuffer::USAGE_STREAM_READ);

		// Add to our free list of pixel buffers.
		d_free_pixel_buffers.push_back(GLPixelBuffer::create(renderer, buffer));
	}

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			GPlatesUtils::Base2::is_power_of_two(MIN_DISTRIBUTE_READ_BACK_PIXEL_DIMENSION),
			GPLATES_ASSERTION_SOURCE);
}


void
GPlatesOpenGL::GLRasterCoRegistration::ResultsQueue::queue_reduce_pyramid_output(
		GLRenderer &renderer,
		const GLFrameBufferObject::shared_ptr_type &framebuffer_object,
		const GLTexture::shared_ptr_to_const_type &results_texture,
		const ReduceQuadTree::non_null_ptr_to_const_type &reduce_quad_tree,
		std::vector<OperationSeedFeaturePartialResults> &seed_feature_partial_results)
{
	//PROFILE_FUNC();

	// Make sure we leave the OpenGL state the way it was.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	if (d_free_pixel_buffers.empty())
	{
		// Free up a pixel buffer by extracting the results from the least-recently queued pixel buffer.
		flush_least_recently_queued_result(renderer, seed_feature_partial_results);
	}

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			!d_free_pixel_buffers.empty(),
			GPLATES_ASSERTION_SOURCE);

	// Remove an unused pixel buffer from the free list.
	GLPixelBuffer::shared_ptr_type pixel_buffer = d_free_pixel_buffers.back();
	d_free_pixel_buffers.pop_back();

	// Bind our framebuffer object to the results texture so that 'glReadPixels' will read from it.
	//
	// Note that since we're using 'GL_COLOR_ATTACHMENT0_EXT' we don't need to call 'glReadBuffer'
	// because binding to a framebuffer object automatically does that for us.
	framebuffer_object->gl_attach(renderer, GL_TEXTURE_2D, results_texture, 0/*level*/, GL_COLOR_ATTACHMENT0_EXT);
	renderer.gl_bind_frame_buffer(framebuffer_object);

	// Start an asynchronous read back of the results texture to CPU memory (the pixel buffer).
	// OpenGL won't block until we attempt to read from the pixel buffer (so we delay that as much as possible).
	//
	// Bind the pixel buffer so that all subsequent 'gl_read_pixels()' calls go into that buffer.
	pixel_buffer->gl_bind_pack(renderer);

	// Recurse into the reduce quad tree to determine which parts of the results texture need to be read back.
	//
	// Normally it's better to have one larger 'glReadPixels' call instead of many small ones.
	// However our 'gl_read_pixels()' calls are non-blocking since they're targeting a pixel buffer (async)
	// so they're not nearly as expensive as a 'glReadPixels' to raw client memory (which would cause
	// the CPU to sync with the GPU thus leaving the GPU pipeline empty and hence stalling the GPU
	// until we can start feeding it again).
	// So the only cost for us, per 'gl_read_pixels', is the time spent in the OpenGL driver setting
	// up the read command which, while not insignificant, is not as significant as a GPU stall so we
	// don't want to go overboard with the number of read calls but we do want to avoid downloading
	// TEXTURE_DIMENSION x TEXTURE_DIMENSION pixels of data (with one large read call) when only a
	// small portion of that contains actual result data (downloading a 1024x1024 texture can take
	// a few milliseconds which is a relatively long time when you think of how many CPU cycles
	// that is the equivalent of).
	distribute_async_read_back(renderer, *reduce_quad_tree, *pixel_buffer);

	// Add to the front of the results queue - we'll read the results later to avoid blocking.
	d_results_queue.push_front(ReducePyramidOutput(reduce_quad_tree, pixel_buffer));
}


void
GPlatesOpenGL::GLRasterCoRegistration::ResultsQueue::flush_results(
		GLRenderer &renderer,
		std::vector<OperationSeedFeaturePartialResults> &seed_feature_partial_results)
{
	while (!d_results_queue.empty())
	{
		flush_least_recently_queued_result(renderer, seed_feature_partial_results);
	}
}


void
GPlatesOpenGL::GLRasterCoRegistration::ResultsQueue::flush_least_recently_queued_result(
		GLRenderer &renderer,
		std::vector<OperationSeedFeaturePartialResults> &seed_feature_partial_results)
{
	//PROFILE_FUNC();

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			!d_results_queue.empty(),
			GPLATES_ASSERTION_SOURCE);

	// Pop the least-recently queued results first.
	const ReducePyramidOutput result = d_results_queue.back();
	d_results_queue.pop_back();

	// The pixel buffer has been (will be) read so now it can be returned to the free list.
	// We do this here instead of after reading the pixel buffer in case the reading throws an error -
	// keeps our state more consistent in presence of exceptions.
	d_free_pixel_buffers.push_back(result.pixel_buffer);

	// Map the pixel buffer to access its data.
	// Note that this is where blocking occurs if the data is not ready yet (eg, because GPU
	// is still generating it or still transferring to pixel buffer memory).
	GLBuffer::MapBufferScope map_pixel_buffer_scope(
			renderer,
			*result.pixel_buffer->get_buffer(),
			GLBuffer::TARGET_PIXEL_PACK_BUFFER);

	// Map the pixel buffer data (note that 'map_pixel_buffer_scope' takes care of unmapping for us).
	//
	// FIXME: What to do if the 'gl_unmap_buffer' returns GL_FALSE (indicating buffer corruption) ?
	// I think the buffer corruption mainly applies when writing data *to* the GPU (not reading *from* GPU).
	// So since we should be reading from CPU memory we shouldn't have a problem (buffer corruption happens
	// to video memory) - but we can't be sure. Problem is we don't know of the corruption until
	// *after* distributing all the results (at unmap) - do we use 'gl_get_buffer_sub_data' and do it again ?
	void *result_data = map_pixel_buffer_scope.gl_map_buffer_static(GLBuffer::ACCESS_READ_ONLY);

	// Traverse the reduce quad tree and distribute the pixel buffer results to SeedCoRegistration objects.
	distribute_result_data(renderer, result_data, *result.reduce_quad_tree, seed_feature_partial_results);
}


void
GPlatesOpenGL::GLRasterCoRegistration::ResultsQueue::distribute_async_read_back(
		GLRenderer &renderer,
		const ReduceQuadTree &reduce_quad_tree,
		GLPixelBuffer &pixel_buffer)
{
	// Start reading to the beginning of the buffer.
	GLint pixel_buffer_offset = 0;

	distribute_async_read_back(
			renderer,
			reduce_quad_tree.get_root_node(),
			pixel_buffer,
			pixel_buffer_offset,
			0/*pixel_rect_offset_x*/,
			0/*pixel_rect_offset_y*/,
			TEXTURE_DIMENSION/*pixel_rect_dimension*/);
}


void
GPlatesOpenGL::GLRasterCoRegistration::ResultsQueue::distribute_async_read_back(
		GLRenderer &renderer,
		const ReduceQuadTreeInternalNode &reduce_quad_tree_internal_node,
		GLPixelBuffer &pixel_buffer,
		GLint &pixel_buffer_offset,
		GLint pixel_rect_offset_x,
		GLint pixel_rect_offset_y,
		GLsizei pixel_rect_dimension)
{
	// If the current sub-tree is full then read back all pixels in the rectangular region covered by it.
	//
	// NOTE: If the rectangular region is small enough then we read it back anyway (even if it's not full).
	// This is because the cost of reading back extra data (that contains no results) is less than
	// the cost of setting up the read back.
	if (reduce_quad_tree_internal_node.is_sub_tree_full() ||
		pixel_rect_dimension <= static_cast<GLsizei>(MIN_DISTRIBUTE_READ_BACK_PIXEL_DIMENSION))
	{
		// NOTE: We don't need to worry about changing the default GL_PACK_ALIGNMENT (rows aligned to 4 bytes)
		// since our data is floats (each float is already 4-byte aligned).
		pixel_buffer.gl_read_pixels(
				renderer,
				pixel_rect_offset_x,
				pixel_rect_offset_y,
				pixel_rect_dimension,
				pixel_rect_dimension,
				GL_RGBA,
				GL_FLOAT,
				pixel_buffer_offset);

		// Advance the pixel buffer offset for the next read.
		pixel_buffer_offset += pixel_rect_dimension * pixel_rect_dimension * sizeof(ResultPixel);

		return;
	}

	const GLsizei child_pixel_rect_dimension = (pixel_rect_dimension >> 1);

	// Recurse into the child reduce quad tree nodes.
	for (unsigned int child_y_offset = 0; child_y_offset < 2; ++child_y_offset)
	{
		for (unsigned int child_x_offset = 0; child_x_offset < 2; ++child_x_offset)
		{
			// If the current child node exists then recurse into it.
			// If it doesn't exist it means there is no result data in that sub-tree.
			const ReduceQuadTreeInternalNode *child_reduce_quad_tree_internal_node =
					reduce_quad_tree_internal_node.get_child_internal_node(
							child_x_offset, child_y_offset);
			if (child_reduce_quad_tree_internal_node)
			{
				const GLint child_pixel_rect_offset_x =
						pixel_rect_offset_x + child_x_offset * child_pixel_rect_dimension;
				const GLint child_pixel_rect_offset_y =
						pixel_rect_offset_y + child_y_offset * child_pixel_rect_dimension;

				distribute_async_read_back(
						renderer,
						*child_reduce_quad_tree_internal_node,
						pixel_buffer,
						pixel_buffer_offset,
						child_pixel_rect_offset_x,
						child_pixel_rect_offset_y,
						child_pixel_rect_dimension);
			}
		}
	}
}


void
GPlatesOpenGL::GLRasterCoRegistration::ResultsQueue::distribute_result_data(
		GLRenderer &renderer,
		const void *result_data,
		const ReduceQuadTree &reduce_quad_tree,
		std::vector<OperationSeedFeaturePartialResults> &seed_feature_partial_results)
{
	const ResultPixel *result_pixel_data = static_cast<const ResultPixel *>(result_data);
	// Start reading from the beginning of the result data buffer in units of *pixels* (not bytes).
	unsigned int result_data_pixel_offset = 0;

	distribute_result_data(
			renderer,
			reduce_quad_tree.get_root_node(),
			result_pixel_data,
			result_data_pixel_offset,
			TEXTURE_DIMENSION/*pixel_rect_dimension*/,
			seed_feature_partial_results);
}


void
GPlatesOpenGL::GLRasterCoRegistration::ResultsQueue::distribute_result_data(
		GLRenderer &renderer,
		const ReduceQuadTreeInternalNode &reduce_quad_tree_internal_node,
		const ResultPixel *const result_pixel_data,
		unsigned int &result_data_pixel_offset,
		GLsizei pixel_rect_dimension,
		std::vector<OperationSeedFeaturePartialResults> &seed_feature_partial_results)
{
	//
	//
	// NOTE: Here we must follow the same path as 'distribute_async_read_back()' in order to
	// correctly retrieve the data read back.
	// So this code path should be kept in sync with that of 'distribute_async_read_back()'.
	//
	//

	// If the current sub-tree is full then read back all pixels in the rectangular region covered by it.
	//
	// NOTE: If the rectangular region is small enough then we read it back anyway (even if it's not full).
	// This is because the cost of reading back extra data (that contains no results) is less than
	// the cost of setting up the read back.
	if (reduce_quad_tree_internal_node.is_sub_tree_full() ||
		pixel_rect_dimension <= static_cast<GLsizei>(MIN_DISTRIBUTE_READ_BACK_PIXEL_DIMENSION))
	{
		// The beginning of the result data for the current 'gl_read_pixels()' pixel rectangle.
		const ResultPixel *gl_read_pixels_result_data = result_pixel_data + result_data_pixel_offset;
		// The dimension of the current 'gl_read_pixels()' pixel rectangle.
		const GLsizei gl_read_pixels_rect_dimension = pixel_rect_dimension;

		// Recurse into the reduce quad tree to extract data from the current pixel rectangle
		// that was originally read by a single 'gl_read_pixels()' call.
		//
		// NOTE: The pixel x/y offsets are relative to the 'gl_read_pixels()' pixel rectangle.
		distribute_result_data_from_gl_read_pixels_rect(
				renderer,
				reduce_quad_tree_internal_node,
				gl_read_pixels_result_data,
				gl_read_pixels_rect_dimension,
				0/*pixel_rect_offset_x*/,
				0/*pixel_rect_offset_y*/,
				pixel_rect_dimension,
				seed_feature_partial_results);

		// Advance the result data offset for the next read.
		// NOTE: The offsets is in units of pixels (not bytes).
		result_data_pixel_offset += pixel_rect_dimension * pixel_rect_dimension;

		return;
	}

	const GLsizei child_pixel_rect_dimension = (pixel_rect_dimension >> 1);

	// Recurse into the child reduce quad tree nodes.
	for (unsigned int child_y_offset = 0; child_y_offset < 2; ++child_y_offset)
	{
		for (unsigned int child_x_offset = 0; child_x_offset < 2; ++child_x_offset)
		{
			// If the current child node exists then recurse into it.
			// If it doesn't exist it means there is no result data in that sub-tree.
			const ReduceQuadTreeInternalNode *child_reduce_quad_tree_internal_node =
					reduce_quad_tree_internal_node.get_child_internal_node(
							child_x_offset, child_y_offset);
			if (child_reduce_quad_tree_internal_node)
			{
				distribute_result_data(
						renderer,
						*child_reduce_quad_tree_internal_node,
						result_pixel_data,
						result_data_pixel_offset,
						child_pixel_rect_dimension,
						seed_feature_partial_results);
			}
		}
	}
}


void
GPlatesOpenGL::GLRasterCoRegistration::ResultsQueue::distribute_result_data_from_gl_read_pixels_rect(
		GLRenderer &renderer,
		const ReduceQuadTreeInternalNode &reduce_quad_tree_internal_node,
		const ResultPixel *const gl_read_pixels_result_data,
		const GLsizei gl_read_pixels_rect_dimension,
		GLint pixel_rect_offset_x,
		GLint pixel_rect_offset_y,
		GLsizei pixel_rect_dimension,
		std::vector<OperationSeedFeaturePartialResults> &seed_feature_partial_results)
{
	const GLsizei child_pixel_rect_dimension = (pixel_rect_dimension >> 1);

	// Recurse into the child reduce quad tree nodes.
	for (unsigned int child_y_offset = 0; child_y_offset < 2; ++child_y_offset)
	{
		for (unsigned int child_x_offset = 0; child_x_offset < 2; ++child_x_offset)
		{
			// If the next layer deep in the reduce quad tree is the leaf node layer...
			if (reduce_quad_tree_internal_node.get_reduce_stage_index() == 1)
			{
				// If the current child node exists then distribute its result.
				// If it doesn't exist it means there is no result data.
				const ReduceQuadTreeLeafNode *child_reduce_quad_tree_leaf_node =
						reduce_quad_tree_internal_node.get_child_leaf_node(
								child_x_offset, child_y_offset);
				if (child_reduce_quad_tree_leaf_node)
				{
					const GLint child_pixel_rect_offset_x =
							pixel_rect_offset_x + child_x_offset * child_pixel_rect_dimension;
					const GLint child_pixel_rect_offset_y =
							pixel_rect_offset_y + child_y_offset * child_pixel_rect_dimension;

					// The result pixel - index into the *original* 'gl_read_pixels' rectangle...
					const ResultPixel &result_pixel =
							gl_read_pixels_result_data[
									child_pixel_rect_offset_x +
										child_pixel_rect_offset_y * gl_read_pixels_rect_dimension];

					// Get the seed co-registration associated with the result.
					SeedCoRegistration &seed_co_registration =
							child_reduce_quad_tree_leaf_node->seed_co_registration;

					// Get the partial results for the operation associated with the co-registration result.
					OperationSeedFeaturePartialResults &operation_seed_feature_partial_results =
							seed_feature_partial_results[seed_co_registration.operation_index];

					// Add the co-registration result to the list of partial results for the
					// seed feature associated with the co-registration result.
					operation_seed_feature_partial_results.add_partial_result(
							result_pixel, seed_co_registration.feature_index);
				}
			}
			else // Child node is an internal node (not a leaf node)...
			{
				// If the current child node exists then recurse into it.
				// If it doesn't exist it means there is no result data in that sub-tree.
				const ReduceQuadTreeInternalNode *child_reduce_quad_tree_internal_node =
						reduce_quad_tree_internal_node.get_child_internal_node(
								child_x_offset, child_y_offset);
				if (child_reduce_quad_tree_internal_node)
				{
					const GLint child_pixel_rect_offset_x =
							pixel_rect_offset_x + child_x_offset * child_pixel_rect_dimension;
					const GLint child_pixel_rect_offset_y =
							pixel_rect_offset_y + child_y_offset * child_pixel_rect_dimension;

					distribute_result_data_from_gl_read_pixels_rect(
							renderer,
							*child_reduce_quad_tree_internal_node,
							gl_read_pixels_result_data,
							gl_read_pixels_rect_dimension,
							child_pixel_rect_offset_x,
							child_pixel_rect_offset_y,
							child_pixel_rect_dimension,
							seed_feature_partial_results);
				}
			}
		}
	}
}
