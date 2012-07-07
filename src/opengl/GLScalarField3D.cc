/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#include <limits>
/*
 * The OpenGL Extension Wrangler Library (GLEW).
 * Must be included before the OpenGL headers (which also means before Qt headers).
 * For this reason it's best to try and include it in ".cc" files only.
 */
#include <GL/glew.h>
#include <opengl/OpenGL.h>
#include <QDataStream>
#include <QDebug>
#include <QFile>
#include <QString>

#include "GLScalarField3D.h"

#include "GLBuffer.h"
#include "GLContext.h"
#include "GLPixelBuffer.h"
#include "GLRenderer.h"
#include "GLShaderProgramUtils.h"
#include "GLUtils.h"

#include "file-io/ErrorOpeningFileForWritingException.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/LogException.h"
#include "global/PreconditionViolationError.h"

#include "gui/RasterColourPalette.h"

#include "maths/Vector3D.h"

#include "utils/Base2Utils.h"
#include "utils/Endian.h"

// Uses a shader-generated test scalar field instead of loading from a scalar field file.
// Also writes the (shader-generated test) scalar field to file to test file loading code path.
// WARNING: Turning this on will overwrite the scalar field file passed into GPlates so it should
// start out as an empty file.
// TODO: Later on this will be moved into the source code that imports scalar fields.
//#define DEBUG_USING_SHADER_GENERATED_TEST_SCALAR_FIELD


namespace GPlatesOpenGL
{
	namespace
	{
		//! Shader source code utilities used for scalar field ray-tracing.
		const QString SCALAR_FIELD_UTILS_SOURCE_FILE_NAME =
				":/opengl/scalar_field_3d/utils.glsl";

		//! Vertex shader source code to render isosurface.
		const QString ISO_SURFACE_VERTEX_SHADER_SOURCE_FILE_NAME =
				":/opengl/scalar_field_3d/iso_surface_vertex_shader.glsl";

		//! Fragment shader source code to render isosurface.
		const QString ISO_SURFACE_FRAGMENT_SHADER_SOURCE_FILE_NAME =
				":/opengl/scalar_field_3d/iso_surface_fragment_shader.glsl";

		//! Vertex shader source code to render vertical cross-section of scalar field.
		const QString CROSS_SECTION_VERTEX_SHADER_SOURCE_FILE_NAME =
				":/opengl/scalar_field_3d/cross_section_vertex_shader.glsl";

		//! Fragment shader source code to render vertical cross-section of scalar field.
		const QString CROSS_SECTION_FRAGMENT_SHADER_SOURCE_FILE_NAME =
				":/opengl/scalar_field_3d/cross_section_fragment_shader.glsl";

#ifdef DEBUG_USING_SHADER_GENERATED_TEST_SCALAR_FIELD
		const char *RENDER_TEST_SCALAR_FIELD_VERTEX_SHADER =
				"varying vec3 world_space_position;\n"

				"void main (void)\n"
				"{\n"
				"	// Screen-space position.\n"
				"	gl_Position = gl_Vertex;\n"

				"	// The 'z' component of gl_Vertex is not important since normalize in pixel shader.\n"
				"	vec4 world_space_position4 = gl_ModelViewProjectionMatrixInverse * gl_Vertex;\n"
				"	world_space_position = world_space_position4.xyz / world_space_position4.w;\n"
				"}\n";

		const char *RENDER_TEST_SCALAR_FIELD_FRAGMENT_SHADER =
				"uniform float layer_depth_radius;\n"

				"varying vec3 world_space_position;\n"

				"//#define TEST_SPHERE\n"
				"//#define TEST_PLANE\n"
				"#define TEST_CUBE\n"
				"//#define TEST_FRACTAL\n" // Needs missing 'cnoise()' function.

				"float\n"
				"scalar_field(\n"
				"		vec3 position)\n"
				"{\n"

				"	// Sphere with isovalue 0 at radius 0.75.\n"
				"#ifdef TEST_SPHERE\n"
				"	return length(position) - 0.75;\n"
				"#endif\n"

				"	// Arbitrary plane with isovalue 0 passing through origin.\n"
				"#ifdef TEST_PLANE\n"
				"	return dot(position, normalize(vec3(1,1,1)));\n"
				"#endif\n"

				"	// Cube with isovalue 0 having side length '2 * HL'.\n"
				"#ifdef TEST_CUBE\n"
				"	const float HL = 0.35;\n"
				"	float d1 = dot(position, vec3(1,0,0));\n"
				"	float d2 = dot(position, vec3(0,1,0));\n"
				"	float d3 = dot(position, vec3(0,0,1));\n"
				"	float d4 = dot(position, vec3(-1,0,0));\n"
				"	float d5 = dot(position, vec3(0,-1,0));\n"
				"	float d6 = dot(position, vec3(0,0,-1));\n"
				"	if (d1 < HL && d2 < HL && d3 < HL && d4 < HL && d5 < HL && d6 < HL)\n"
				"	{\n"
				"		// Inside cube - choose closest distance to cube.\n"
				"		return max(d1, max(d2, max(d3, max(d4, max(d5, d6))))) - HL;\n"
				"	}\n"
				"	// Outside cube - choose closest distance to cube.\n"
				"	float d = 0;\n"
				"	if (d1 > HL) d += (d1-HL)*(d1-HL);\n"
				"	if (d2 > HL) d += (d2-HL)*(d2-HL);\n"
				"	if (d3 > HL) d += (d3-HL)*(d3-HL);\n"
				"	if (d4 > HL) d += (d4-HL)*(d4-HL);\n"
				"	if (d5 > HL) d += (d5-HL)*(d5-HL);\n"
				"	if (d6 > HL) d += (d6-HL)*(d6-HL);\n"
				"	return sqrt(d);\n"
				"#endif\n"

				"#ifdef TEST_FRACTAL\n"
				"	// Do a fractal summation of the 3D noise function.\n"
				"	const float persistence = 0.6;\n"
				"	const float base_freq = 0.85;\n"
				"	float noise = 0;\n"
				"	float p = 1;\n"
				"	for (int i = 0; i < 4; ++i)\n"
				"	{\n"
				"		float fac = pow(2.0, float(i));\n"
				"		p *= persistence;\n"
				"		float noise_contrib = p * cnoise(fac * base_freq * position);\n"
				"		noise += noise_contrib;\n"
				"	}\n"
				"	return noise;\n"
				"#endif\n"

				"}\n"

				"void main (void)\n"
				"{\n"

				"	// Set the current world space position to the depth of the current layer being rendered to.\n"
				"	vec3 world_space_position_at_layer_depth = layer_depth_radius * normalize(world_space_position);\n"

				"	// Field scalar value.\n"
				"   gl_FragColor.r = scalar_field(world_space_position_at_layer_depth);\n"

				"	// Field gradient value.\n"
				"	const float grad_delta = 1e-3;\n"
				"	const float inv_grad_delta = 1.0 / grad_delta;\n"
				"   float grad_x = 0.5 * inv_grad_delta * (\n"
				"		scalar_field(world_space_position_at_layer_depth + vec3(grad_delta,0,0)) - \n"
				"		scalar_field(world_space_position_at_layer_depth - vec3(grad_delta,0,0)));\n"
				"   float grad_y = 0.5 * inv_grad_delta * (\n"
				"		scalar_field(world_space_position_at_layer_depth + vec3(0,grad_delta,0)) - \n"
				"		scalar_field(world_space_position_at_layer_depth - vec3(0,grad_delta,0)));\n"
				"   float grad_z = 0.5 * inv_grad_delta * (\n"
				"		scalar_field(world_space_position_at_layer_depth + vec3(0,0,grad_delta)) - \n"
				"		scalar_field(world_space_position_at_layer_depth - vec3(0,0,grad_delta)));\n"
				"	gl_FragColor.gba = vec3(grad_x, grad_y, grad_z);\n"
				"}\n";
#endif
	}
}


bool
GPlatesOpenGL::GLScalarField3D::is_supported(
		GLRenderer &renderer)
{
	static bool supported = false;

	// Only test for support the first time we're called.
	static bool tested_for_support = false;
	if (!tested_for_support)
	{
		tested_for_support = true;

		const GLContext::Parameters &context_params = GLContext::get_parameters();

		// We essentially need graphics hardware supporting OpenGL 3.0.
		//
		// Instead of testing for GLEW_VERSION_3_0 we test for GL_EXT_texture_array
		// (which was introduced in OpenGL 3.0) - and is the main requirement for the ray-tracing shader.
		// This is done because OpenGL 3.0 is not officially supported on MacOS Snow Leopard - in that
		// it supports OpenGL 3.0 extensions but not the specific OpenGL 3.0 functions (according to
		// http://www.cultofmac.com/26065/snow-leopard-10-6-3-update-significantly-improves-opengl-3-0-support/).
		if (!context_params.texture.gl_EXT_texture_array ||
			// Shader relies on hardware bilinear filtering of floating-point textures...
			!context_params.texture.gl_supports_floating_point_filtering_and_blending ||
			// Using floating-point textures...
			!context_params.texture.gl_ARB_texture_float ||
			// We want floating-point RG texture format...
			!context_params.texture.gl_ARB_texture_rg ||
			!context_params.texture.gl_ARB_texture_non_power_of_two ||
			!context_params.shader.gl_ARB_vertex_shader ||
			!context_params.shader.gl_ARB_fragment_shader)
		{
			qWarning() << "3D scalar fields NOT supported by this OpenGL system - requires OpenGL 3.0.";
			return false;
		}

		//
		// Try to compile our most complex ray-tracing shader program.
		//
		// If this fails then it could be exceeding some resource limit on the runtime system.
		//

		GLShaderProgramUtils::ShaderSource fragment_shader_source(SHADER_VERSION);
		fragment_shader_source.add_shader_source_from_file(GLShaderProgramUtils::UTILS_SHADER_SOURCE_FILE_NAME);
		fragment_shader_source.add_shader_source_from_file(SCALAR_FIELD_UTILS_SOURCE_FILE_NAME);
		fragment_shader_source.add_shader_source_from_file(ISO_SURFACE_FRAGMENT_SHADER_SOURCE_FILE_NAME);
		
		GLShaderProgramUtils::ShaderSource vertex_shader_source(SHADER_VERSION);
		vertex_shader_source.add_shader_source_from_file(GLShaderProgramUtils::UTILS_SHADER_SOURCE_FILE_NAME);
		vertex_shader_source.add_shader_source_from_file(SCALAR_FIELD_UTILS_SOURCE_FILE_NAME);
		vertex_shader_source.add_shader_source_from_file(ISO_SURFACE_VERTEX_SHADER_SOURCE_FILE_NAME);

		// Attempt to create the test shader program.
		if (!GLShaderProgramUtils::compile_and_link_vertex_fragment_program(
				renderer, vertex_shader_source, fragment_shader_source))
		{
			qWarning() << "3D scalar fields NOT supported by this OpenGL system - failed to compile shader program.";
			return false;
		}

		// If we get this far then we have support.
		supported = true;
	}

	return supported;
}


GPlatesOpenGL::GLScalarField3D::non_null_ptr_type
GPlatesOpenGL::GLScalarField3D::create(
		GLRenderer &renderer,
		const QString &scalar_field_filename,
		const GLLight::non_null_ptr_type &light)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			is_supported(renderer),
			GPLATES_ASSERTION_SOURCE);

	return non_null_ptr_type(
			new GLScalarField3D(
					renderer,
					scalar_field_filename,
					light));
}


GPlatesOpenGL::GLScalarField3D::GLScalarField3D(
		GLRenderer &renderer,
		const QString &scalar_field_filename,
		const GLLight::non_null_ptr_type &light) :
	d_light(light),
	d_tile_meta_data_resolution(0),
	d_tile_resolution(0),
	d_num_active_tiles(0),
	d_num_depth_layers(0),
	d_min_depth_layer_radius(0),
	d_max_depth_layer_radius(1),
	d_scalar_min(0),
	d_scalar_max(0),
	d_scalar_mean(0),
	d_scalar_standard_deviation(0),
	d_gradient_magnitude_min(0),
	d_gradient_magnitude_max(0),
	d_gradient_magnitude_mean(0),
	d_gradient_magnitude_standard_deviation(0),
	d_tile_meta_data_texture_array(GLTexture::create(renderer)),
	d_field_data_texture_array(GLTexture::create(renderer)),
	d_mask_data_texture_array(GLTexture::create(renderer)),
	d_depth_radius_to_layer_texture(GLTexture::create(renderer)),
	d_colour_palette_texture(GLTexture::create(renderer)),
	d_current_scalar_iso_surface_value(0),
	d_current_colour_palette(GPlatesGui::DefaultNormalisedRasterColourPalette::create())
{
#ifdef DEBUG_USING_SHADER_GENERATED_TEST_SCALAR_FIELD
	// Ignore the scalar field file and load a test scalar field instead.
	// Set some parameters for the test scalar field.
	d_tile_meta_data_resolution = 2; // Must be power-of-two only because using a quad-tree to generate test field.
	d_tile_resolution = 64;
	d_num_active_tiles = 6/*six cube faces*/ * d_tile_meta_data_resolution * d_tile_meta_data_resolution;
	d_num_depth_layers = 40;
	d_min_depth_layer_radius = 0.25f;
	d_max_depth_layer_radius = 1.0f;
#else
	// Reader to access data in scalar field file.
	GPlatesFileIO::ScalarField3DFileFormat::Reader scalar_field_reader(scalar_field_filename);

	// Load the parameters of the scalar field.
	d_tile_meta_data_resolution = scalar_field_reader.get_tile_meta_data_resolution(); // Doesn't have to be power-of-two.
	d_tile_resolution = scalar_field_reader.get_tile_resolution();
	d_num_active_tiles = scalar_field_reader.get_num_active_tiles();
	d_num_depth_layers = scalar_field_reader.get_num_depth_layers_per_tile();
	d_min_depth_layer_radius = scalar_field_reader.get_minimum_depth_layer_radius();
	d_max_depth_layer_radius = scalar_field_reader.get_maximum_depth_layer_radius();
	d_depth_layer_radii = scalar_field_reader.get_depth_layer_radii();
	d_scalar_min = scalar_field_reader.get_scalar_min();
	d_scalar_max = scalar_field_reader.get_scalar_max();
	d_scalar_mean = scalar_field_reader.get_scalar_mean();
	d_scalar_standard_deviation = scalar_field_reader.get_scalar_standard_deviation();
	d_gradient_magnitude_min = scalar_field_reader.get_gradient_magnitude_min();
	d_gradient_magnitude_max = scalar_field_reader.get_gradient_magnitude_max();
	d_gradient_magnitude_mean = scalar_field_reader.get_gradient_magnitude_mean();
	d_gradient_magnitude_standard_deviation = scalar_field_reader.get_gradient_magnitude_standard_deviation();
#endif

	// Check that the number of texture array layers does not exceed the maximum supported by
	// the GPU on the runtime system.
	// TODO: For now we'll just report an error but later we'll need to adapt somehow.
	GPlatesGlobal::Assert<GPlatesGlobal::LogException>(
			d_num_active_tiles * d_num_depth_layers <=
				GLContext::get_parameters().texture.gl_max_texture_array_layers,
			GPLATES_ASSERTION_SOURCE,
			"GLScalarField3D: number texture layers in scalar field file exceeded GPU limit.");

	create_shader_programs(renderer);

	create_tile_meta_data_texture_array(renderer);
	create_field_data_texture_array(renderer);
	create_mask_data_texture_array(renderer);
	create_depth_radius_to_layer_texture(renderer);
	create_colour_palette_texture(renderer);

#ifdef DEBUG_USING_SHADER_GENERATED_TEST_SCALAR_FIELD
	// Ignore the scalar field file and load a test scalar field instead.
	// NOTE: Also writes generated scalar field to the file 'scalar_field_filename'.
	load_shader_generated_test_scalar_field(renderer, scalar_field_filename);
#else
	load_scalar_field(renderer, scalar_field_reader);
#endif

	// Load the colour palette texture.
	load_colour_palette_texture(renderer);
}


void
GPlatesOpenGL::GLScalarField3D::set_iso_value(
		GLRenderer &renderer,
		float iso_value)
{
	d_current_scalar_iso_surface_value = iso_value;
}


void
GPlatesOpenGL::GLScalarField3D::set_colour_palette(
		GLRenderer &renderer,
		const GPlatesGui::ColourPalette<double>::non_null_ptr_to_const_type &colour_palette)
{
	d_current_colour_palette = colour_palette;
	load_colour_palette_texture(renderer);
}


void
GPlatesOpenGL::GLScalarField3D::set_shader_test_variables(
		GLRenderer &renderer,
		const std::vector<float> &test_variables)
{
	// We should always have a valid shader program but test just in case.
	if (!d_render_iso_surface_program_object)
	{
		return;
	}

	for (unsigned int variable_index = 0; variable_index < test_variables.size(); ++variable_index)
	{
		const QString variable_name = QString("test_variable_%1").arg(variable_index);;

		// Set the shader test variable.
		// If the variable doesn't exist in the shader program or is not used then
		// a warning is emitted (but can be ignored).
		d_render_iso_surface_program_object.get()->gl_uniform1f(
				renderer,
				variable_name.toAscii().constData(),
				test_variables[variable_index]);
	}
}


bool
GPlatesOpenGL::GLScalarField3D::change_scalar_field(
		GLRenderer &renderer,
		const QString &scalar_field_filename)
{
#ifdef DEBUG_USING_SHADER_GENERATED_TEST_SCALAR_FIELD
	// Just continue to use the same test scalar field.
	return true;
#endif

	// Reader to access data in scalar field file.
	GPlatesFileIO::ScalarField3DFileFormat::Reader scalar_field_reader(scalar_field_filename);

	// Return false if any scalar field parameters differ from the current scalar field.
	// We need to be able to load the data into our existing texture arrays.
	// Returning false tells the caller to rebuild this GLScalarField3D object from scratch.
	if (d_tile_meta_data_resolution != scalar_field_reader.get_tile_meta_data_resolution() ||
		d_tile_resolution != scalar_field_reader.get_tile_resolution() ||
		d_num_depth_layers != scalar_field_reader.get_num_depth_layers_per_tile() ||
		d_num_active_tiles != scalar_field_reader.get_num_active_tiles())
	{
		return false;
	}

	// Upload the new scalar field data.
	load_scalar_field(renderer, scalar_field_reader);

	return true;
}


const GPlatesUtils::SubjectToken &
GPlatesOpenGL::GLScalarField3D::get_subject_token() const
{
	//
	// This covers changes to the inputs that don't require completely re-creating the inputs.
	// That is beyond our scope and is detected and managed by our owners (and owners of our inputs).
	//

	// If the light has changed.
	if (d_light)
	{
		if (!d_light.get()->get_subject_token().is_observer_up_to_date(d_light_observer_token))
		{
			d_subject_token.invalidate();

			d_light.get()->get_subject_token().update_observer(d_light_observer_token);
		}
	}

	return d_subject_token;
}


void
GPlatesOpenGL::GLScalarField3D::render(
		GLRenderer &renderer,
		cache_handle_type &cache_handle,
		boost::optional<GLTexture::shared_ptr_to_const_type> surface_occlusion_texture,
		boost::optional<GLTexture::shared_ptr_to_const_type> depth_read_texture)
{
	// Make sure we leave the OpenGL state the way it was.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// We should always have a valid shader program but test just in case.
	if (!d_render_iso_surface_program_object)
	{
		return;
	}

	// Used to draw a full-screen quad.
	const GLCompiledDrawState::non_null_ptr_to_const_type full_screen_quad_drawable =
			renderer.get_context().get_shared_state()->get_full_screen_2D_textured_quad(renderer);

	// Bind the shader program for rendering iso-surface using an orthographic view projection.
	renderer.gl_bind_program_object(d_render_iso_surface_program_object.get());

	// Currently always using orthographic projection.
	// TODO: Add support for perspective projection.
	d_render_iso_surface_program_object.get()->gl_uniform1i(
			renderer,
			"using_ortho_projection",
			true);

	unsigned int current_texture_unit = 0;

	// Set tile metadata texture sampler to current texture unit.
	renderer.gl_bind_texture(d_tile_meta_data_texture_array, GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_2D_ARRAY_EXT);
	d_render_iso_surface_program_object.get()->gl_uniform1i(
			renderer,
			"tile_meta_data_sampler",
			current_texture_unit);
	// Move to the next texture unit.
	++current_texture_unit;

	// Set field data texture sampler to current texture unit.
	renderer.gl_bind_texture(d_field_data_texture_array, GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_2D_ARRAY_EXT);
	d_render_iso_surface_program_object.get()->gl_uniform1i(
			renderer,
			"field_data_sampler",
			current_texture_unit);
	// Move to the next texture unit.
	++current_texture_unit;

	// Set mask data texture sampler to current texture unit.
	renderer.gl_bind_texture(d_mask_data_texture_array, GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_2D_ARRAY_EXT);
	d_render_iso_surface_program_object.get()->gl_uniform1i(
			renderer,
			"mask_data_sampler",
			current_texture_unit);
	// Move to the next texture unit.
	++current_texture_unit;

	// Set 1D depth radius to layer texture sampler to current texture unit.
	renderer.gl_bind_texture(d_depth_radius_to_layer_texture, GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_1D);
	d_render_iso_surface_program_object.get()->gl_uniform1i(
			renderer,
			"depth_radius_to_layer_sampler",
			current_texture_unit);
	// Move to the next texture unit.
	++current_texture_unit;

	// Set 1D depth radius to layer texture sampler to current texture unit.
	renderer.gl_bind_texture(d_colour_palette_texture, GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_1D);
	d_render_iso_surface_program_object.get()->gl_uniform1i(
			renderer,
			"colour_palette_sampler",
			current_texture_unit);
	// Move to the next texture unit.
	++current_texture_unit;

	if (surface_occlusion_texture)
	{
		// Set surface occlusion texture sampler to current texture unit.
		renderer.gl_bind_texture(surface_occlusion_texture.get(), GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_2D);
		d_render_iso_surface_program_object.get()->gl_uniform1i(
				renderer,
				"surface_occlusion_texture_sampler",
				current_texture_unit);
		// Move to the next texture unit.
		++current_texture_unit;

		// Enable reads from surface occlusion texture.
		d_render_iso_surface_program_object.get()->gl_uniform1i(
				renderer,
				"read_from_surface_occlusion_texture",
				true);
	}
	else
	{
		// Disable reads from surface occlusion texture.
		d_render_iso_surface_program_object.get()->gl_uniform1i(
				renderer,
				"read_from_surface_occlusion_texture",
				false);
	}

	if (depth_read_texture)
	{
		// Set depth texture sampler to current texture unit.
		renderer.gl_bind_texture(depth_read_texture.get(), GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_2D);
		d_render_iso_surface_program_object.get()->gl_uniform1i(
				renderer,
				"depth_texture_sampler",
				current_texture_unit);
		// Move to the next texture unit.
		++current_texture_unit;

		// Enable reads from depth texture.
		d_render_iso_surface_program_object.get()->gl_uniform1i(
				renderer,
				"read_from_depth_texture",
				true);
	}
	else
	{
		// Disable reads from depth texture.
		d_render_iso_surface_program_object.get()->gl_uniform1i(
				renderer,
				"read_from_depth_texture",
				false);
	}

	// Set the tile metadata resolution.
	d_render_iso_surface_program_object.get()->gl_uniform1i(
			renderer,
			"tile_meta_data_resolution",
			d_tile_meta_data_resolution);

	// Set the tile resolution.
	d_render_iso_surface_program_object.get()->gl_uniform1i(
			renderer,
			"tile_resolution",
			d_tile_resolution);

	// Set the 1D texture depth-radius-to-layer resolution.
	d_render_iso_surface_program_object.get()->gl_uniform1i(
			renderer,
			"depth_radius_to_layer_resolution",
			d_depth_radius_to_layer_texture->get_width().get());

	// Set the 1D texture colour palette resolution.
	d_render_iso_surface_program_object.get()->gl_uniform1i(
			renderer,
			"colour_palette_resolution",
			d_colour_palette_texture->get_width().get());

	// Set the ray-tracing inner/outer sphere radius corresponding.
	d_render_iso_surface_program_object.get()->gl_uniform2f(
			renderer,
			"min_max_depth_radius",
			d_min_depth_layer_radius,
			d_max_depth_layer_radius);

	// Set the number of depth layers.
	d_render_iso_surface_program_object.get()->gl_uniform1i(
			renderer,
			"num_depth_layers",
			d_num_depth_layers);

	// Set the ray-tracing min/max scalar value.
	d_render_iso_surface_program_object.get()->gl_uniform2f(
			renderer,
			"min_max_scalar_value",
			d_scalar_min,
			d_scalar_max);

	// Set the ray-tracing min/max gradient magnitude.
	d_render_iso_surface_program_object.get()->gl_uniform2f(
			renderer,
			"min_max_gradient_magnitude",
			d_gradient_magnitude_min,
			d_gradient_magnitude_max);

	// Set the current iso-surface scalar value.
	d_render_iso_surface_program_object.get()->gl_uniform1f(
			renderer,
			"scalar_iso_surface_value",
			d_current_scalar_iso_surface_value);

	// Set boolean flag if lighting is enabled.
	d_render_iso_surface_program_object.get()->gl_uniform1i(
			renderer,
			"lighting_enabled",
			d_light->get_scene_lighting_parameters().is_lighting_enabled());

	// Set the world-space light direction.
	d_render_iso_surface_program_object.get()->gl_uniform3f(
			renderer,
			"world_space_light_direction",
			d_light->get_globe_view_light_direction(renderer));

	// Set the light ambient contribution.
	d_render_iso_surface_program_object.get()->gl_uniform1f(
			renderer,
			"light_ambient_contribution",
			d_light->get_scene_lighting_parameters().get_ambient_light_contribution());

	// Render the full-screen quad.
	renderer.apply_compiled_draw_state(*full_screen_quad_drawable);

	cache_handle = cache_handle_type();
}


void
GPlatesOpenGL::GLScalarField3D::create_shader_programs(
		GLRenderer &renderer)
{
	d_render_iso_surface_program_object =
			create_shader_program(
					renderer,
					ISO_SURFACE_VERTEX_SHADER_SOURCE_FILE_NAME,
					ISO_SURFACE_FRAGMENT_SHADER_SOURCE_FILE_NAME);
}


boost::optional<GPlatesOpenGL::GLProgramObject::shared_ptr_type>
GPlatesOpenGL::GLScalarField3D::create_shader_program(
		GLRenderer &renderer,
		const QString &vertex_shader_source_file_name,
		const QString &fragment_shader_source_file_name)
{
	// Vertex shader source.
	GLShaderProgramUtils::ShaderSource vertex_shader_source(SHADER_VERSION);
	// Then add the general utilities.
	vertex_shader_source.add_shader_source_from_file(GLShaderProgramUtils::UTILS_SHADER_SOURCE_FILE_NAME);
	// Then add the scalar field utilities.
	vertex_shader_source.add_shader_source_from_file(SCALAR_FIELD_UTILS_SOURCE_FILE_NAME);
	// Then add the GLSL 'main()' function.
	vertex_shader_source.add_shader_source_from_file(vertex_shader_source_file_name);

	GLShaderProgramUtils::ShaderSource fragment_shader_source(SHADER_VERSION);
	// Then add the general utilities.
	fragment_shader_source.add_shader_source_from_file(GLShaderProgramUtils::UTILS_SHADER_SOURCE_FILE_NAME);
	// Then add the scalar field utilities.
	fragment_shader_source.add_shader_source_from_file(SCALAR_FIELD_UTILS_SOURCE_FILE_NAME);
	// Then add the GLSL 'main()' function.
	fragment_shader_source.add_shader_source_from_file(fragment_shader_source_file_name);

	// Link the shader program.
	boost::optional<GLProgramObject::shared_ptr_type> program_object =
			GLShaderProgramUtils::compile_and_link_vertex_fragment_program(
					renderer,
					vertex_shader_source,
					fragment_shader_source);

	return program_object;
}


void
GPlatesOpenGL::GLScalarField3D::create_tile_meta_data_texture_array(
		GLRenderer &renderer)
{
	// Using nearest-neighbour filtering since don't want to filter pixel metadata.
	d_tile_meta_data_texture_array->gl_tex_parameteri(
			renderer, GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	d_tile_meta_data_texture_array->gl_tex_parameteri(
			renderer, GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Clamp texture coordinates to centre of edge texels.
	// Not strictly necessary for nearest-neighbour filtering.
	if (GLEW_EXT_texture_edge_clamp || GLEW_SGIS_texture_edge_clamp)
	{
		d_tile_meta_data_texture_array->gl_tex_parameteri(
				renderer, GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		d_tile_meta_data_texture_array->gl_tex_parameteri(
				renderer, GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	else
	{
		d_tile_meta_data_texture_array->gl_tex_parameteri(
				renderer, GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_WRAP_S, GL_CLAMP);
		d_tile_meta_data_texture_array->gl_tex_parameteri(
				renderer, GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_WRAP_T, GL_CLAMP);
	}

	// Create the texture but don't load any data into it.
	//
	// NOTE: Since the image data is NULL it doesn't really matter what 'format' and 'type' are -
	// just use values that are compatible with all internal formats to avoid a possible error.

	d_tile_meta_data_texture_array->gl_tex_image_3D(
			renderer, GL_TEXTURE_2D_ARRAY_EXT, 0, GL_RGB32F_ARB,
			d_tile_meta_data_resolution,
			d_tile_meta_data_resolution,
			6, // One layer per cube face.
			0, GL_RGB, GL_FLOAT, NULL);

	// Check there are no OpenGL errors.
	GLUtils::assert_no_gl_errors(GPLATES_ASSERTION_SOURCE);
}


void
GPlatesOpenGL::GLScalarField3D::create_field_data_texture_array(
		GLRenderer &renderer)
{
	// Using linear filtering.
	// We've tested for support for filtering of floating-point textures in 'is_supported()'.
	d_field_data_texture_array->gl_tex_parameteri(
			renderer, GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	d_field_data_texture_array->gl_tex_parameteri(
			renderer, GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Clamp texture coordinates to centre of edge texels.
	if (GLEW_EXT_texture_edge_clamp || GLEW_SGIS_texture_edge_clamp)
	{
		d_field_data_texture_array->gl_tex_parameteri(
				renderer, GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		d_field_data_texture_array->gl_tex_parameteri(
				renderer, GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	else
	{
		d_field_data_texture_array->gl_tex_parameteri(
				renderer, GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_WRAP_S, GL_CLAMP);
		d_field_data_texture_array->gl_tex_parameteri(
				renderer, GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_WRAP_T, GL_CLAMP);
	}

	// Create the texture but don't load any data into it.
	//
	// NOTE: Since the image data is NULL it doesn't really matter what 'format' and 'type' are -
	// just use values that are compatible with all internal formats to avoid a possible error.

	d_field_data_texture_array->gl_tex_image_3D(
			renderer, GL_TEXTURE_2D_ARRAY_EXT, 0, GL_RGBA32F_ARB,
			d_tile_resolution,
			d_tile_resolution,
			d_num_active_tiles * d_num_depth_layers,
			0, GL_RGBA, GL_FLOAT, NULL);

	// Check there are no OpenGL errors.
	GLUtils::assert_no_gl_errors(GPLATES_ASSERTION_SOURCE);
}


void
GPlatesOpenGL::GLScalarField3D::create_mask_data_texture_array(
		GLRenderer &renderer)
{
	// Using linear filtering.
	// We've tested for support for filtering of floating-point textures in 'is_supported()'.
	d_mask_data_texture_array->gl_tex_parameteri(
			renderer, GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	d_mask_data_texture_array->gl_tex_parameteri(
			renderer, GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Clamp texture coordinates to centre of edge texels.
	if (GLEW_EXT_texture_edge_clamp || GLEW_SGIS_texture_edge_clamp)
	{
		d_mask_data_texture_array->gl_tex_parameteri(
				renderer, GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		d_mask_data_texture_array->gl_tex_parameteri(
				renderer, GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	else
	{
		d_mask_data_texture_array->gl_tex_parameteri(
				renderer, GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_WRAP_S, GL_CLAMP);
		d_mask_data_texture_array->gl_tex_parameteri(
				renderer, GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_WRAP_T, GL_CLAMP);
	}

	// Create the texture but don't load any data into it.
	//
	// NOTE: Since the image data is NULL it doesn't really matter what 'format' and 'type' are -
	// just use values that are compatible with all internal formats to avoid a possible error.

	d_mask_data_texture_array->gl_tex_image_3D(
			renderer, GL_TEXTURE_2D_ARRAY_EXT, 0, GL_R32F,
			d_tile_resolution,
			d_tile_resolution,
			d_num_active_tiles,
			0, GL_RED, GL_FLOAT, NULL);

	// Check there are no OpenGL errors.
	GLUtils::assert_no_gl_errors(GPLATES_ASSERTION_SOURCE);
}


void
GPlatesOpenGL::GLScalarField3D::create_depth_radius_to_layer_texture(
		GLRenderer &renderer)
{
	// Using linear filtering.
	// We've tested for support for filtering of floating-point textures in 'is_supported()'.
	d_depth_radius_to_layer_texture->gl_tex_parameteri(
			renderer, GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	d_depth_radius_to_layer_texture->gl_tex_parameteri(
			renderer, GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Clamp texture coordinates to centre of edge texels.
	if (GLEW_EXT_texture_edge_clamp || GLEW_SGIS_texture_edge_clamp)
	{
		d_depth_radius_to_layer_texture->gl_tex_parameteri(
				renderer, GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	}
	else
	{
		d_depth_radius_to_layer_texture->gl_tex_parameteri(
				renderer, GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	}

	// Create the texture but don't load any data into it.
	//
	// NOTE: Since the image data is NULL it doesn't really matter what 'format' and 'type' are -
	// just use values that are compatible with all internal formats to avoid a possible error.

	unsigned int depth_radius_to_layer_resolution = DEPTH_RADIUS_TO_LAYER_RESOLUTION;
	// Limit to max texture size if exceeds.
	if (depth_radius_to_layer_resolution > GLContext::get_parameters().texture.gl_max_texture_size)
	{
		depth_radius_to_layer_resolution = GLContext::get_parameters().texture.gl_max_texture_size;
	}

	d_depth_radius_to_layer_texture->gl_tex_image_1D(
			renderer, GL_TEXTURE_1D, 0, GL_R32F,
			depth_radius_to_layer_resolution,
			0, GL_RED, GL_FLOAT, NULL);

	// Check there are no OpenGL errors.
	GLUtils::assert_no_gl_errors(GPLATES_ASSERTION_SOURCE);
}


void
GPlatesOpenGL::GLScalarField3D::create_colour_palette_texture(
		GLRenderer &renderer)
{
	// Using linear filtering.
	// We've tested for support for filtering of floating-point textures in 'is_supported()'.
	d_colour_palette_texture->gl_tex_parameteri(
			renderer, GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	d_colour_palette_texture->gl_tex_parameteri(
			renderer, GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Clamp texture coordinates to centre of edge texels.
	if (GLEW_EXT_texture_edge_clamp || GLEW_SGIS_texture_edge_clamp)
	{
		d_colour_palette_texture->gl_tex_parameteri(
				renderer, GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	}
	else
	{
		d_colour_palette_texture->gl_tex_parameteri(
				renderer, GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	}

	// Create the texture but don't load any data into it.
	//
	// NOTE: Since the image data is NULL it doesn't really matter what 'format' and 'type' are -
	// just use values that are compatible with all internal formats to avoid a possible error.

	unsigned int colour_palette_resolution = COLOUR_PALETTE_RESOLUTION;
	// Limit to max texture size if exceeds.
	if (colour_palette_resolution > GLContext::get_parameters().texture.gl_max_texture_size)
	{
		colour_palette_resolution = GLContext::get_parameters().texture.gl_max_texture_size;
	}

	d_colour_palette_texture->gl_tex_image_1D(
			renderer, GL_TEXTURE_1D, 0, GL_RGBA32F_ARB,
			colour_palette_resolution,
			0, GL_RGBA, GL_FLOAT, NULL);

	// Check there are no OpenGL errors.
	GLUtils::assert_no_gl_errors(GPLATES_ASSERTION_SOURCE);
}


void
GPlatesOpenGL::GLScalarField3D::load_scalar_field(
		GLRenderer &renderer,
		const GPlatesFileIO::ScalarField3DFileFormat::Reader &scalar_field_reader)
{
	// Load the depth-radius-to-layer 1D texture mapping.
	load_depth_radius_to_layer_texture(renderer);


	//
	// Read the tile metadata from the file.
	//
	// This is a relatively small amount of data so we don't need to worry about memory usage.
	boost::shared_array<GPlatesFileIO::ScalarField3DFileFormat::TileMetaData>
			tile_meta_data = scalar_field_reader.read_tile_meta_data();

	// Upload the tile metadata into the texture array.
	d_tile_meta_data_texture_array->gl_tex_sub_image_3D(
			renderer, GL_TEXTURE_2D_ARRAY_EXT, 0,
			0, 0, 0, // x,y,z offsets
			scalar_field_reader.get_tile_meta_data_resolution(),
			scalar_field_reader.get_tile_meta_data_resolution(),
			6, // One layer per cube face.
			GL_RGB, GL_FLOAT, tile_meta_data.get());


	//
	// Read the field data from the file.
	//
	// Avoid excessive memory use from reading entire field into a single large memory array by
	// reading sub-sections in multiple iterations.
	const unsigned int field_bytes_per_layer = sizeof(GPlatesFileIO::ScalarField3DFileFormat::FieldDataSample) *
			scalar_field_reader.get_tile_resolution() * scalar_field_reader.get_tile_resolution();
	// Limit to roughly 64Mb (the '1' ensures we read at least one layer per iteration).
	const unsigned int max_layers_read_per_iteration = 1 + (64 * 1024 * 1024 / field_bytes_per_layer);
	const unsigned int num_layers = scalar_field_reader.get_num_layers();
	unsigned int layer_index = 0;
	while (layer_index < num_layers)
	{
		const unsigned int num_layers_remaining = num_layers - layer_index;
		const unsigned int num_layers_to_read = (std::min)(max_layers_read_per_iteration, num_layers_remaining);

		boost::shared_array<GPlatesFileIO::ScalarField3DFileFormat::FieldDataSample>
				field_data = scalar_field_reader.read_field_data(layer_index, num_layers_to_read);

		// Upload the current range of field data layers into the texture array.
		d_field_data_texture_array->gl_tex_sub_image_3D(
				renderer, GL_TEXTURE_2D_ARRAY_EXT, 0,
				0, 0, // x,y offsets
				layer_index, // z offset
				scalar_field_reader.get_tile_resolution(),
				scalar_field_reader.get_tile_resolution(),
				num_layers_to_read,
				GL_RGBA, GL_FLOAT, field_data.get());

		layer_index += num_layers_to_read;
	}


	//
	// Read the mask data from the file.
	//
	// Avoid excessive memory use from reading entire mask into a single large memory array by
	// reading sub-sections in multiple iterations.
	const unsigned int mask_bytes_per_tile = sizeof(GPlatesFileIO::ScalarField3DFileFormat::MaskDataSample) *
			scalar_field_reader.get_tile_resolution() * scalar_field_reader.get_tile_resolution();
	// Limit to roughly 64Mb (the '1' ensures we read at least one tile per iteration).
	const unsigned int max_mask_tiles_read_per_iteration = 1 + (64 * 1024 * 1024 / mask_bytes_per_tile);
	unsigned int mask_tile_index = 0;
	while (mask_tile_index < d_num_active_tiles)
	{
		const unsigned int num_tiles_remaining = d_num_active_tiles - mask_tile_index;
		const unsigned int num_tiles_to_read = (std::min)(max_mask_tiles_read_per_iteration, num_tiles_remaining);

		boost::shared_array<GPlatesFileIO::ScalarField3DFileFormat::MaskDataSample>
				mask_data = scalar_field_reader.read_mask_data(mask_tile_index, num_tiles_to_read);

		// Upload the current range of mask data into the texture array.
		d_mask_data_texture_array->gl_tex_sub_image_3D(
				renderer, GL_TEXTURE_2D_ARRAY_EXT, 0,
				0, 0, // x,y offsets
				mask_tile_index, // z offset
				scalar_field_reader.get_tile_resolution(),
				scalar_field_reader.get_tile_resolution(),
				num_tiles_to_read,
				GL_RED, GL_FLOAT, mask_data.get());

		mask_tile_index += num_tiles_to_read;
	}
}


void
GPlatesOpenGL::GLScalarField3D::load_depth_radius_to_layer_texture(
		GLRenderer &renderer)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_depth_layer_radii.size() == d_num_depth_layers,
			GPLATES_ASSERTION_SOURCE);

	// Number of texels in the depth-radius-to-layer mapping.
	const unsigned int depth_radius_to_layer_resolution =
			d_depth_radius_to_layer_texture->get_width().get();

	// The texels of the depth-radius-to-layer mapping.
	std::vector<GLfloat> depth_layer_mapping;

	// First texel is layer zero.
	depth_layer_mapping.push_back(0/*layer*/);
	// Index into 'depth_layer_radii'.
	unsigned int layer_index = 0;
	// Iterate over non-boundary texels (ie, skip first and last texel).
	for (unsigned int texel = 1; texel < depth_radius_to_layer_resolution - 1; ++texel)
	{
		// Convert texel index into depth radius.
		const double depth_radius = d_min_depth_layer_radius +
				(d_max_depth_layer_radius - d_min_depth_layer_radius) *
					(double(texel) / (depth_radius_to_layer_resolution - 1));

		// Find the two adjacent layers whose depth range contains the current texel's depth.
		while (layer_index + 1 < d_depth_layer_radii.size() &&
			depth_radius > d_depth_layer_radii[layer_index + 1])
		{
			++layer_index;
		}
		const double lower_depth_radius = d_depth_layer_radii[layer_index];
		const double upper_depth_radius = d_depth_layer_radii[layer_index + 1];

		// Linearly interpolate between the two adjacent layer depths.
		const double layer_fraction = (depth_radius - lower_depth_radius) / (upper_depth_radius - lower_depth_radius);
		const double layer = layer_index + layer_fraction;

		depth_layer_mapping.push_back(layer);
	}
	// Last texel is layer 'd_num_depth_layers - 1'.
	depth_layer_mapping.push_back(d_num_depth_layers - 1/*layer*/);

	// Upload the depth-radius-to-layer mapping data into the texture.
	d_depth_radius_to_layer_texture->gl_tex_sub_image_1D(
			renderer, GL_TEXTURE_1D, 0,
			0, // x offset
			depth_radius_to_layer_resolution, // width
			GL_RED, GL_FLOAT, &depth_layer_mapping[0]);
}


void
GPlatesOpenGL::GLScalarField3D::load_colour_palette_texture(
		GLRenderer &renderer)
{
	// The RGBA texels of the colour palette texture.
	std::vector<GLfloat> colour_palette_texels;

	// Number of texels in the colour palette texture.
	const unsigned int colour_palette_resolution = d_colour_palette_texture->get_width().get();

	// Iterate over texels.
	for (unsigned int texel = 0; texel < colour_palette_resolution; ++texel)
	{
		// Map input range [0,1] to colour.
		boost::optional<GPlatesGui::Colour> colour = d_current_colour_palette->get_colour(
				double(texel) / (colour_palette_resolution - 1));

		if (!colour)
		{
			// If we don't get a colour then it should only happen at the boundary of the colour range.
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					texel == 0 || texel == colour_palette_resolution - 1,
					GPLATES_ASSERTION_SOURCE);

			if (texel == 0)
			{
				// Lookup second texel instead...
				colour = d_current_colour_palette->get_colour(1.0/*texel*/ / (colour_palette_resolution - 1));
			}
			else // texel == colour_palette_resolution - 1
			{
				// Lookup second last texel...
				colour = d_current_colour_palette->get_colour(
						(colour_palette_resolution - 2)/*texel*/ / (colour_palette_resolution - 1));
			}

			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					colour,
					GPLATES_ASSERTION_SOURCE);
		}

		colour_palette_texels.push_back(colour->red());
		colour_palette_texels.push_back(colour->green());
		colour_palette_texels.push_back(colour->blue());
		colour_palette_texels.push_back(colour->alpha());
	}

	// Upload the colour palette data into the texture.
	d_colour_palette_texture->gl_tex_sub_image_1D(
			renderer, GL_TEXTURE_1D, 0,
			0, // x offset
			colour_palette_resolution, // width
			GL_RGBA, GL_FLOAT, &colour_palette_texels[0]);
}


#ifdef DEBUG_USING_SHADER_GENERATED_TEST_SCALAR_FIELD
void
GPlatesOpenGL::GLScalarField3D::load_shader_generated_test_scalar_field(
		GLRenderer &renderer,
		const QString &scalar_field_filename)
{
	// Make sure we leave the OpenGL state the way it was.
	GLRenderer::StateBlockScope save_restore_state(
			renderer,
			// We're rendering to a render target so reset to the default OpenGL state...
			true/*reset_to_default_state*/);


	// The depths of the layers - we'll make them equidistant.
	// And start with the minimum radius progressing to the maximum radius.
	for (unsigned int depth_layer_index = 0; depth_layer_index < d_num_depth_layers; ++depth_layer_index)
	{
		d_depth_layer_radii.push_back(
				d_min_depth_layer_radius +
					(d_max_depth_layer_radius - d_min_depth_layer_radius) *
							(double(depth_layer_index) / (d_num_depth_layers - 1)));
	}

	// Load the depth-radius-to-layer 1D texture mapping.
	load_depth_radius_to_layer_texture(renderer);


	// Field texture array should have been created.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_field_data_texture_array->get_width() &&
				d_field_data_texture_array->get_height() &&
				d_field_data_texture_array->get_depth(),
			GPLATES_ASSERTION_SOURCE);
	// Should have square tiles.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_field_data_texture_array->get_width() == d_field_data_texture_array->get_height(),
			GPLATES_ASSERTION_SOURCE);

	// Our cube map subdivision with a half-texel overlap at the border to avoid texture seams.
	GPlatesOpenGL::GLCubeSubdivision::non_null_ptr_to_const_type cube_subdivision =
			GPlatesOpenGL::GLCubeSubdivision::create(
					GPlatesOpenGL::GLCubeSubdivision::get_expand_frustum_ratio(
							d_field_data_texture_array->get_width().get(),
							0.5/* half a texel */));

	// Metadata needs to be power-of-two since we're created tiles in each cube face using a quad-tree.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			GPlatesUtils::Base2::is_power_of_two(d_tile_meta_data_resolution) &&
				// All tiles are active...
				d_num_active_tiles == 6/*six cube faces*/ * d_tile_meta_data_resolution * d_tile_meta_data_resolution,
			GPLATES_ASSERTION_SOURCE);

	// Used to draw a full-screen quad into render texture.
	const GLCompiledDrawState::non_null_ptr_to_const_type full_screen_quad_drawable =
			renderer.get_context().get_shared_state()->get_full_screen_2D_textured_quad(renderer);

	GLFrameBufferObject::shared_ptr_type framebuffer_object =
			renderer.get_context().get_non_shared_state()->acquire_frame_buffer_object(renderer);
	renderer.gl_bind_frame_buffer(framebuffer_object);

	// Largest buffer size needed for any texture array layer...
	const unsigned int buffer_size = d_tile_resolution * d_tile_resolution * 4 * sizeof(GLfloat);

	// A pixel buffer object to read the texture array data back to the CPU so can calculate
	// min/max/mean/std-dev of scalar field.
	GLBuffer::shared_ptr_type buffer = GLBuffer::create(renderer);
	buffer->gl_buffer_data(
			renderer,
			GLBuffer::TARGET_PIXEL_PACK_BUFFER,
			buffer_size,
			NULL, // Uninitialised memory.
			GLBuffer::USAGE_STREAM_READ);
	GLPixelBuffer::shared_ptr_type pixel_buffer = GLPixelBuffer::create(renderer, buffer);
	// Bind the pixel buffer so that all subsequent 'gl_read_pixels()' calls go into that buffer.
	pixel_buffer->gl_bind_pack(renderer);

	// Compile/link the shader program to render test scalar field.
	boost::optional<GLProgramObject::shared_ptr_type> render_test_scalar_field_program_object =
			GLShaderProgramUtils::compile_and_link_vertex_fragment_program(
					renderer,
					RENDER_TEST_SCALAR_FIELD_VERTEX_SHADER,
					RENDER_TEST_SCALAR_FIELD_FRAGMENT_SHADER);
	GPlatesGlobal::Assert<GPlatesGlobal::LogException>(
			render_test_scalar_field_program_object,
			GPLATES_ASSERTION_SOURCE,
			"GLScalarField3D: error compiling test scalar field shader program.");

	// Bind the shader program for rendering test scalar field.
	renderer.gl_bind_program_object(render_test_scalar_field_program_object.get());

	// Viewport for the texture slices in the field texture array.
	renderer.gl_viewport(0, 0,
			d_field_data_texture_array->get_width().get(),
			d_field_data_texture_array->get_height().get());

	// The tile metadata.
	std::vector<GPlatesFileIO::ScalarField3DFileFormat::TileMetaData> tile_meta_data_array;

	// The scalar field statistics.
	double scalar_min = (std::numeric_limits<double>::max)();
	double scalar_max = (std::numeric_limits<double>::min)();
	double scalar_sum = 0;
	double scalar_sum_squares = 0;
	double gradient_magnitude_min = (std::numeric_limits<double>::max)();
	double gradient_magnitude_max = (std::numeric_limits<double>::min)();
	double gradient_magnitude_sum = 0;
	double gradient_magnitude_sum_squares = 0;

	// The six faces of the cube.
	for (unsigned int face = 0; face < 6; ++face)
	{
		const GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face =
				static_cast<GPlatesMaths::CubeCoordinateFrame::CubeFaceType>(face);

		// The view matrix for the current cube face.
		const GLTransform::non_null_ptr_to_const_type view_transform =
				cube_subdivision->get_view_transform(cube_face);

		// Set the view matrix.
		renderer.gl_load_matrix(GL_MODELVIEW, view_transform->get_matrix());

		// Number of tiles across a cube face should be a power-of-two since using a quad-tree below.
		const unsigned int tile_cube_quad_tree_depth =
				GPlatesUtils::Base2::log2_power_of_two(d_tile_meta_data_resolution);

		// Iterate over the leaf nodes of the quad tree.
		// Each metadata pixel represents a tile which is also a leaf node (of quad tree).
		for (unsigned int y = 0; y < d_tile_meta_data_resolution; ++y)
		{
			for (unsigned int x = 0; x < d_tile_meta_data_resolution; ++x)
			{
				// Get the projection transforms of an entire cube face (the lowest resolution level-of-detail).
				const GLTransform::non_null_ptr_to_const_type projection_transform =
						cube_subdivision->get_projection_transform(
								tile_cube_quad_tree_depth/*level_of_detail*/,
								x/*tile_u_offset*/,
								y/*tile_v_offset*/);

				// The projection matrix.
				renderer.gl_load_matrix(GL_PROJECTION, projection_transform->get_matrix());

				// The current tile ID.
				const unsigned int tile_ID =
						// Skip past previous cube face tiles...
						face * d_tile_meta_data_resolution * d_tile_meta_data_resolution +
							// Skip past previous quad-tree row tiles in current face...
							y * d_tile_meta_data_resolution +
							// Skip past previous quad-tree tiles in current row in current face...
							x;

				// The min/max scalar of the current tile.
				double tile_scalar_min = (std::numeric_limits<double>::max)();
				double tile_scalar_max = (std::numeric_limits<double>::min)();

				// Iterate over the depth layers of the current tile.
				for (unsigned int depth_layer_index = 0;
					depth_layer_index < d_num_depth_layers;
					++depth_layer_index)
				{
					// Begin rendering to a 2D slice of the field texture array.
					framebuffer_object->gl_attach_texture_array(
							renderer,
							d_field_data_texture_array,
							0, // level
							tile_ID * d_num_depth_layers + depth_layer_index, // layer
							GL_COLOR_ATTACHMENT0_EXT);

					renderer.gl_clear_color(); // Clear colour to all zeros.
					renderer.gl_clear(GL_COLOR_BUFFER_BIT); // Clear only the colour buffer.

					// Set the depth at which to render the current layer.
					const double layer_depth_radius = d_depth_layer_radii[depth_layer_index];
					// Let the test shader know the current layer depth.
					render_test_scalar_field_program_object.get()->gl_uniform1f(
							renderer, "layer_depth_radius", layer_depth_radius);

					// Render the full-screen quad.
					renderer.apply_compiled_draw_state(*full_screen_quad_drawable);

					// Read back the data just rendered (to calculate scalar field statistics).
					// NOTE: We don't need to worry about changing the default GL_PACK_ALIGNMENT
					// (rows aligned to 4 bytes) since our data is floats (each float is already 4-byte aligned).
					pixel_buffer->gl_read_pixels(
							renderer,
							0,
							0,
							d_tile_resolution,
							d_tile_resolution,
							GL_RGBA,
							GL_FLOAT,
							0);

					// Map the pixel buffer to access its data.
					GLBuffer::MapBufferScope map_pixel_buffer_scope(
							renderer,
							*pixel_buffer->get_buffer(),
							GLBuffer::TARGET_PIXEL_PACK_BUFFER);

					// Map the pixel buffer data.
					void *field_data = map_pixel_buffer_scope.gl_map_buffer_static(GLBuffer::ACCESS_READ_ONLY);
					const GPlatesFileIO::ScalarField3DFileFormat::FieldDataSample *const field_data_pixels =
							static_cast<const GPlatesFileIO::ScalarField3DFileFormat::FieldDataSample *>(field_data);

					// Iterate over the pixels in the depth layer.
					for (unsigned int n = 0; n < d_tile_resolution * d_tile_resolution; ++n)
					{
						const double scalar = field_data_pixels[n].scalar;

						// Find tile minimum scalar.
						if (scalar < tile_scalar_min)
						{
							tile_scalar_min = scalar;
						}

						// Find tile maximum scalar.
						if (scalar > tile_scalar_max)
						{
							tile_scalar_max = scalar;
						}

						// Find global minimum scalar.
						if (scalar < scalar_min)
						{
							scalar_min = scalar;
						}

						// Find global maximum scalar.
						if (scalar > scalar_max)
						{
							scalar_max = scalar;
						}

						// To help find global mean scalar.
						scalar_sum += scalar;

						// To help find global std-dev scalar.
						scalar_sum_squares += scalar * scalar;

						const GPlatesMaths::Vector3D gradient(
								field_data_pixels[n].gradient[0],
								field_data_pixels[n].gradient[1],
								field_data_pixels[n].gradient[2]);
						const double gradient_magnitude = gradient.magnitude().dval();

						// Find global minimum gradient magnitude.
						if (gradient_magnitude < gradient_magnitude_min)
						{
							gradient_magnitude_min = gradient_magnitude;
						}

						// Find global maximum gradient magnitude.
						if (gradient_magnitude > gradient_magnitude_max)
						{
							gradient_magnitude_max = gradient_magnitude;
						}

						// To help find global mean gradient magnitude.
						gradient_magnitude_sum += gradient_magnitude;

						// To help find global std-dev gradient magnitude.
						gradient_magnitude_sum_squares += gradient_magnitude * gradient_magnitude;
					}
				}

				// Specify the current tile's metadata.
				GPlatesFileIO::ScalarField3DFileFormat::TileMetaData tile_meta_data;
				tile_meta_data.tile_ID = tile_ID;
				tile_meta_data.min_scalar_value = tile_scalar_min;
				tile_meta_data.max_scalar_value = tile_scalar_max;
				tile_meta_data_array.push_back(tile_meta_data);
			}
		}
	}

	// Scalar field statistics.
	//
	// mean = M = sum(Ci * Xi) / sum(Ci)
	// std_dev  = sqrt[sum(Ci * (Xi - M)^2) / sum(Ci)]
	//          = sqrt[(sum(Ci * Xi^2) - 2 * M * sum(Ci * Xi) + M^2 * sum(Ci)) / sum(Ci)]
	//          = sqrt[(sum(Ci * Xi^2) - 2 * M * M * sum(Ci) + M^2 * sum(Ci)) / sum(Ci)]
	//          = sqrt[(sum(Ci * Xi^2) - M^2 * sum(Ci)) / sum(Ci)]
	//          = sqrt[(sum(Ci * Xi^2) / sum(Ci) - M^2]
	//
	// For the test scalar field the coverage (or mask) values Ci are all 1.0.
	//
	// mean = M = sum(Xi) / N
	// std_dev  = sqrt[(sum(Xi^2) / N - M^2]
	//
	// ...where N is total number of scalar field samples.
	//

	const unsigned int num_field_samples = d_num_active_tiles * d_num_depth_layers * d_tile_resolution * d_tile_resolution;

	d_scalar_mean = scalar_sum / num_field_samples;
	d_gradient_magnitude_mean = gradient_magnitude_sum / num_field_samples;

	const double scalar_variance = scalar_sum_squares / num_field_samples - d_scalar_mean * d_scalar_mean;
	// Protect 'sqrt' in case variance is slightly negative due to numerical precision.
	d_scalar_standard_deviation = (scalar_variance > 0) ? std::sqrt(scalar_variance) : 0;

	const double gradient_magnitude_variance =
			gradient_magnitude_sum_squares / num_field_samples - d_gradient_magnitude_mean * d_gradient_magnitude_mean;
	// Protect 'sqrt' in case variance is slightly negative due to numerical precision.
	d_gradient_magnitude_standard_deviation =
			(gradient_magnitude_variance > 0) ? std::sqrt(gradient_magnitude_variance) : 0;

	d_scalar_min = scalar_min;
	d_gradient_magnitude_min = gradient_magnitude_min;

	d_scalar_max = scalar_max;
	d_gradient_magnitude_max = gradient_magnitude_max;

	//
	// Load the tile metadata.
	//
	// Upload the tile metadata into the texture array.
	d_tile_meta_data_texture_array->gl_tex_sub_image_3D(
			renderer, GL_TEXTURE_2D_ARRAY_EXT, 0,
			0, 0, 0, // x,y,z offsets
			d_tile_meta_data_resolution,
			d_tile_meta_data_resolution,
			6, // One layer per cube face.
			GL_RGB, GL_FLOAT, &tile_meta_data_array[0]);

	//
	// Load the mask data.
	//
	// Set the tile mask data to all ones.
	GPlatesFileIO::ScalarField3DFileFormat::MaskDataSample mask_data;
	mask_data.mask = 1.0f;
	const std::vector<GPlatesFileIO::ScalarField3DFileFormat::MaskDataSample> mask_data_array(
			d_tile_resolution * d_tile_resolution, mask_data);
	// Set all active tiles to mask values of one.
	for (unsigned int mask_tile_index = 0; mask_tile_index < d_num_active_tiles; ++mask_tile_index)
	{
		// Upload the mask data into the texture array.
		d_mask_data_texture_array->gl_tex_sub_image_3D(
				renderer, GL_TEXTURE_2D_ARRAY_EXT, 0,
				0, 0, // x,y offsets
				mask_tile_index, // z offset
				d_tile_resolution,
				d_tile_resolution,
				1, // depth
				GL_RED, GL_FLOAT, &mask_data_array[0]);
	}

	// Release attachment to our texture array before relinquishing acquired framebuffer object.
	// Not necessary but prevents framebuffer object keeping attached texture from being deleted if
	// acquired framebuffer object never used again (unlikely).
	framebuffer_object->gl_detach_all(renderer);

	// Write scalar field to file so can test loading code path.
	write_scalar_field_to_file(renderer, scalar_field_filename);
}


void
GPlatesOpenGL::GLScalarField3D::write_scalar_field_to_file(
		GLRenderer &renderer,
		const QString &scalar_field_filename)
{
	// Make sure we leave the OpenGL state the way it was.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// Open the file for writing.
	QFile file(scalar_field_filename);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		throw GPlatesFileIO::ErrorOpeningFileForWritingException(
				GPLATES_EXCEPTION_SOURCE, scalar_field_filename);
	}
	QDataStream out(&file);

	out.setVersion(GPlatesFileIO::ScalarField3DFileFormat::Q_DATA_STREAM_VERSION);
	out.setByteOrder(GPlatesFileIO::ScalarField3DFileFormat::Q_DATA_STREAM_BYTE_ORDER);

	// Write magic number/string.
	for (unsigned int n = 0; n < sizeof(GPlatesFileIO::ScalarField3DFileFormat::MAGIC_NUMBER); ++n)
	{
		out << static_cast<quint8>(GPlatesFileIO::ScalarField3DFileFormat::MAGIC_NUMBER[n]);
	}

	// Write the file size - write zero for now and come back later to fill it in.
	const qint64 file_size_offset = file.pos();
	qint64 total_output_file_size = 0;
	out << total_output_file_size;

	// Write version number.
	out << static_cast<quint32>(GPlatesFileIO::ScalarField3DFileFormat::VERSION_NUMBER);

	// Write tile metadata resolution.
	out << static_cast<quint32>(d_tile_meta_data_resolution);

	// Write tile resolution.
	out << static_cast<quint32>(d_tile_resolution);

	// Write number of active tiles.
	out << static_cast<quint32>(d_num_active_tiles);

	// Write number of depth layers.
	out << static_cast<quint32>(d_num_depth_layers);

	// Write the layer depth radii.
	for (unsigned int depth_layer_index = 0; depth_layer_index < d_num_depth_layers; ++depth_layer_index)
	{
		out << static_cast<float>(d_depth_layer_radii[depth_layer_index]);
	}

	// Write scalar minimum.
	out << static_cast<double>(d_scalar_min);

	// Write scalar maximum.
	out << static_cast<double>(d_scalar_max);

	// Write scalar mean.
	out << static_cast<double>(d_scalar_mean);

	// Write scalar standard deviation.
	out << static_cast<double>(d_scalar_standard_deviation);

	// Write gradient magnitude minimum.
	out << static_cast<double>(d_gradient_magnitude_min);

	// Write gradient magnitude maximum.
	out << static_cast<double>(d_gradient_magnitude_max);

	// Write gradient magnitude mean.
	out << static_cast<double>(d_gradient_magnitude_mean);

	// Write gradient magnitude standard deviation.
	out << static_cast<double>(d_gradient_magnitude_standard_deviation);

	// Largest buffer size needed for any texture array layer...
	const unsigned int buffer_size = d_tile_resolution * d_tile_resolution * 4 * sizeof(GLfloat);

	// A pixel buffer object to read the texture array data into (via framebuffer).
	GLBuffer::shared_ptr_type buffer = GLBuffer::create(renderer);
	buffer->gl_buffer_data(
			renderer,
			GLBuffer::TARGET_PIXEL_PACK_BUFFER,
			buffer_size,
			NULL, // Uninitialised memory.
			GLBuffer::USAGE_STREAM_READ);
	GLPixelBuffer::shared_ptr_type pixel_buffer = GLPixelBuffer::create(renderer, buffer);

	// Use framebuffer object to attach to a specific 2D layer of texture array.
	GLFrameBufferObject::shared_ptr_type framebuffer_object =
			renderer.get_context().get_non_shared_state()->acquire_frame_buffer_object(renderer);
	renderer.gl_bind_frame_buffer(framebuffer_object);

	// Bind the pixel buffer so that all subsequent 'gl_read_pixels()' calls go into that buffer.
	pixel_buffer->gl_bind_pack(renderer);

	//
	// Write the tile metadata layer-by-layer to the file.
	//
	const unsigned int num_meta_data_layers = 6;
	for (unsigned int meta_data_layer_index = 0; meta_data_layer_index < num_meta_data_layers; ++meta_data_layer_index)
	{
		// Attach to a 2D slice of the mask texture array.
		framebuffer_object->gl_attach_texture_array(
				renderer,
				d_tile_meta_data_texture_array,
				0, // level
				meta_data_layer_index, // layer
				GL_COLOR_ATTACHMENT0_EXT);

		// NOTE: We don't need to worry about changing the default GL_PACK_ALIGNMENT (rows aligned to 4 bytes)
		// since our data is floats (each float is already 4-byte aligned).
		pixel_buffer->gl_read_pixels(
				renderer,
				0,
				0,
				d_tile_meta_data_resolution,
				d_tile_meta_data_resolution,
				GL_RGB,
				GL_FLOAT,
				0);

		// Map the pixel buffer to access its data.
		GLBuffer::MapBufferScope map_pixel_buffer_scope(
				renderer,
				*pixel_buffer->get_buffer(),
				GLBuffer::TARGET_PIXEL_PACK_BUFFER);

		// Map the pixel buffer data.
		// Allow writes to mapped buffer in case need endian conversion.
		void *meta_data = map_pixel_buffer_scope.gl_map_buffer_static(GLBuffer::ACCESS_READ_WRITE);
		GPlatesFileIO::ScalarField3DFileFormat::TileMetaData *meta_data_pixels =
				static_cast<GPlatesFileIO::ScalarField3DFileFormat::TileMetaData *>(meta_data);

		// Convert from the runtime system endian to the endian required for the file (if necessary).
		GPlatesUtils::Endian::convert(
				meta_data_pixels,
				meta_data_pixels + d_tile_meta_data_resolution * d_tile_meta_data_resolution,
				static_cast<QSysInfo::Endian>(GPlatesFileIO::ScalarField3DFileFormat::Q_DATA_STREAM_BYTE_ORDER));

		// Write the tile metadata layer to the file.
		out.writeRawData(
				static_cast<const char *>(meta_data),
				d_tile_meta_data_resolution * d_tile_meta_data_resolution *
					GPlatesFileIO::ScalarField3DFileFormat::TileMetaData::STREAM_SIZE);
	}

	//
	// Write the field data layer-by-layer to the file.
	//
	const unsigned int num_field_layers = d_num_active_tiles * d_num_depth_layers;
	for (unsigned int field_layer_index = 0; field_layer_index < num_field_layers; ++field_layer_index)
	{
		// Attach to a 2D slice of the field texture array.
		framebuffer_object->gl_attach_texture_array(
				renderer,
				d_field_data_texture_array,
				0, // level
				field_layer_index, // layer
				GL_COLOR_ATTACHMENT0_EXT);

		// NOTE: We don't need to worry about changing the default GL_PACK_ALIGNMENT (rows aligned to 4 bytes)
		// since our data is floats (each float is already 4-byte aligned).
		pixel_buffer->gl_read_pixels(
				renderer,
				0,
				0,
				d_tile_resolution,
				d_tile_resolution,
				GL_RGBA,
				GL_FLOAT,
				0);

		// Map the pixel buffer to access its data.
		GLBuffer::MapBufferScope map_pixel_buffer_scope(
				renderer,
				*pixel_buffer->get_buffer(),
				GLBuffer::TARGET_PIXEL_PACK_BUFFER);

		// Map the pixel buffer data.
		// Allow writes to mapped buffer in case need endian conversion.
		void *field_data = map_pixel_buffer_scope.gl_map_buffer_static(GLBuffer::ACCESS_READ_WRITE);
		GPlatesFileIO::ScalarField3DFileFormat::FieldDataSample *field_data_pixels =
				static_cast<GPlatesFileIO::ScalarField3DFileFormat::FieldDataSample *>(field_data);

		// Convert from the runtime system endian to the endian required for the file (if necessary).
		GPlatesUtils::Endian::convert(
				field_data_pixels,
				field_data_pixels + d_tile_resolution * d_tile_resolution,
				static_cast<QSysInfo::Endian>(GPlatesFileIO::ScalarField3DFileFormat::Q_DATA_STREAM_BYTE_ORDER));

		// Write the field layer to the file.
		out.writeRawData(
				static_cast<const char *>(field_data),
				d_tile_resolution * d_tile_resolution *
					GPlatesFileIO::ScalarField3DFileFormat::FieldDataSample::STREAM_SIZE);
	}

	//
	// Write the mask data layer-by-layer to the file.
	//
	const unsigned int num_mask_layers = d_num_active_tiles;
	for (unsigned int mask_layer_index = 0; mask_layer_index < num_mask_layers; ++mask_layer_index)
	{
		// Attach to a 2D slice of the mask texture array.
		framebuffer_object->gl_attach_texture_array(
				renderer,
				d_mask_data_texture_array,
				0, // level
				mask_layer_index, // layer
				GL_COLOR_ATTACHMENT0_EXT);

		// NOTE: We don't need to worry about changing the default GL_PACK_ALIGNMENT (rows aligned to 4 bytes)
		// since our data is floats (each float is already 4-byte aligned).
		pixel_buffer->gl_read_pixels(
				renderer,
				0,
				0,
				d_tile_resolution,
				d_tile_resolution,
				GL_RED,
				GL_FLOAT,
				0);

		// Map the pixel buffer to access its data.
		GLBuffer::MapBufferScope map_pixel_buffer_scope(
				renderer,
				*pixel_buffer->get_buffer(),
				GLBuffer::TARGET_PIXEL_PACK_BUFFER);

		// Map the pixel buffer data.
		// Allow writes to mapped buffer in case need endian conversion.
		void *mask_data = map_pixel_buffer_scope.gl_map_buffer_static(GLBuffer::ACCESS_READ_WRITE);
		GPlatesFileIO::ScalarField3DFileFormat::MaskDataSample *mask_data_pixels =
				static_cast<GPlatesFileIO::ScalarField3DFileFormat::MaskDataSample *>(mask_data);

		// Convert from the runtime system endian to the endian required for the file (if necessary).
		GPlatesUtils::Endian::convert(
				mask_data_pixels,
				mask_data_pixels + d_tile_resolution * d_tile_resolution,
				static_cast<QSysInfo::Endian>(GPlatesFileIO::ScalarField3DFileFormat::Q_DATA_STREAM_BYTE_ORDER));

		// Write the mask layer to the file.
		out.writeRawData(
				static_cast<const char *>(mask_data),
				d_tile_resolution * d_tile_resolution *
					GPlatesFileIO::ScalarField3DFileFormat::MaskDataSample::STREAM_SIZE);
	}

	// Write the total size of the output file so the reader can verify that the
	// file was not partially written.
	file.seek(file_size_offset);
	total_output_file_size = file.size();
	out << total_output_file_size;

	file.close();

	// Release attachment to our texture array before relinquishing acquired framebuffer object.
	// Not necessary but prevents framebuffer object keeping attached texture from being deleted if
	// acquired framebuffer object never used again (unlikely).
	framebuffer_object->gl_detach_all(renderer);
}
#endif
