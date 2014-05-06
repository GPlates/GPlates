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

#include <cmath>
#include <limits>
#include <set>
#include <boost/scoped_array.hpp>
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
#include <QImage>
#include <QString>

#include "GLScalarField3D.h"

#include "GLContext.h"
#include "GLPixelBuffer.h"
#include "GLRenderer.h"
#include "GLShaderProgramUtils.h"
#include "GLShaderSource.h"
#include "GLUtils.h"

#include "file-io/ErrorOpeningFileForWritingException.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/LogException.h"
#include "global/PreconditionViolationError.h"

#include "gui/Colour.h"

#include "maths/GreatCircleArc.h"
#include "maths/MathsUtils.h"
#include "maths/Vector3D.h"

#include "utils/Base2Utils.h"
#include "utils/Endian.h"
#include "utils/Profile.h"


namespace GPlatesOpenGL
{
	namespace
	{
		/**
		 * We will tessellate a great circle arc, when rendering 2D cross-section geometries,
		 * if the two endpoints are far enough apart.
		 */
		const double GREAT_CIRCLE_ARC_ANGULAR_THRESHOLD = GPlatesMaths::convert_deg_to_rad(5);
		const double COSINE_GREAT_CIRCLE_ARC_ANGULAR_THRESHOLD = std::cos(GREAT_CIRCLE_ARC_ANGULAR_THRESHOLD);

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

		//! Vertex shader source code to render surface fill mask.
		const QString SURFACE_FILL_MASK_VERTEX_SHADER_SOURCE_FILE_NAME =
				":/opengl/scalar_field_3d/surface_fill_mask_vertex_shader.glsl";

		//! Geometry shader source code to render surface fill mask.
		const QString SURFACE_FILL_MASK_GEOMETRY_SHADER_SOURCE_FILE_NAME =
				":/opengl/scalar_field_3d/surface_fill_mask_geometry_shader.glsl";

		//! Fragment shader source code to render surface fill mask.
		const QString SURFACE_FILL_MASK_FRAGMENT_SHADER_SOURCE_FILE_NAME =
				":/opengl/scalar_field_3d/surface_fill_mask_fragment_shader.glsl";

		/**
		 * Vertex shader source code to render volume fill boundary.
		 *
		 * Used for both depth range and wall normals.
		 * Also used for both walls and spherical caps (for depth range).
		 */
		const QString VOLUME_FILL_VERTEX_SHADER_SOURCE_FILE_NAME =
				":/opengl/scalar_field_3d/volume_fill_vertex_shader.glsl";

#if 0 // Not currently used...
		//! Geometry shader source code to render volume fill spherical caps.
		const QString VOLUME_FILL_SPHERICAL_CAP_GEOMETRY_SHADER_SOURCE_FILE_NAME =
				":/opengl/scalar_field_3d/volume_fill_spherical_cap_geometry_shader.glsl";
#endif

		//! Geometry shader source code to render volume fill walls.
		const QString VOLUME_FILL_WALL_GEOMETRY_SHADER_SOURCE_FILE_NAME =
				":/opengl/scalar_field_3d/volume_fill_wall_geometry_shader.glsl";

#if 0 // Not currently used...
		//! Fragment shader source code to render volume fill spherical caps.
		const QString VOLUME_FILL_SPHERICAL_CAP_FRAGMENT_SHADER_SOURCE_FILE_NAME =
				":/opengl/scalar_field_3d/volume_fill_spherical_cap_fragment_shader.glsl";
#endif

		//! Fragment shader source code to render volume fill wal depth range.
		const QString VOLUME_FILL_WALL_FRAGMENT_SHADER_SOURCE_FILE_NAME =
				":/opengl/scalar_field_3d/volume_fill_wall_fragment_shader.glsl";

		/**
		 * Vertex shader source code to render coloured (white) sphere with lighting.
		 *
		 * We use it to render the white inner sphere when rendering cross-sections
		 * (the ray-tracing isosurface rendering doesn't need it however).
		 */
		const QString SPHERE_VERTEX_SHADER = ":/opengl/scalar_field_3d/sphere_vertex_shader.glsl";

		/**
		 * Fragment shader source code to render coloured (white) sphere with lighting.
		 *
		 * We use it to render the white inner sphere when rendering cross-sections
		 * (the ray-tracing isosurface rendering doesn't need it however).
		 */
		const QString SPHERE_FRAGMENT_SHADER = ":/opengl/scalar_field_3d/sphere_fragment_shader.glsl";

		/**
		 * Useful when debugging a fixed-point texture array by saving each layer to an image file.
		 */
		void
		debug_fixed_point_texture_array(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_to_const_type &texture,
				const QString &image_file_basename)
		{
			// Make sure we leave the OpenGL state the way it was.
			GLRenderer::StateBlockScope save_restore_state(renderer);

			// Classify our frame buffer object according to texture format/dimensions.
			GLFrameBufferObject::Classification framebuffer_object_classification;
			framebuffer_object_classification.set_dimensions(
					renderer,
					texture->get_width().get(),
					texture->get_height().get());
			framebuffer_object_classification.set_attached_texture_array_layer(
					renderer,
					texture->get_internal_format().get());

			// Acquire and bind a frame buffer object.
			GLFrameBufferObject::shared_ptr_type framebuffer_object =
					renderer.get_context().get_non_shared_state()->acquire_frame_buffer_object(
							renderer,
							framebuffer_object_classification);
			renderer.gl_bind_frame_buffer(framebuffer_object);

			// Buffer size needed for a texture array layer.
			const unsigned int buffer_size = texture->get_width().get() * texture->get_height().get() * 4;

			// A pixel buffer object to read the texture array.
			GLBuffer::shared_ptr_type buffer = GLBuffer::create(renderer, GLBuffer::BUFFER_TYPE_PIXEL);
			buffer->gl_buffer_data(
					renderer,
					GLBuffer::TARGET_PIXEL_PACK_BUFFER,
					buffer_size,
					NULL, // Uninitialised memory.
					GLBuffer::USAGE_STREAM_READ);
			GLPixelBuffer::shared_ptr_type pixel_buffer = GLPixelBuffer::create(renderer, buffer);
			// Bind the pixel buffer so that all subsequent 'gl_read_pixels()' calls go into that buffer.
			pixel_buffer->gl_bind_pack(renderer);

			for (unsigned int layer = 0; layer < texture->get_depth().get(); ++layer)
			{
				framebuffer_object->gl_attach_texture_array_layer(
						renderer,
						texture,
						0, // level
						layer, // layer
						GL_COLOR_ATTACHMENT0_EXT);

				pixel_buffer->gl_read_pixels(
						renderer,
						0,
						0,
						texture->get_width().get(),
						texture->get_height().get(),
						GL_RGBA,
						GL_UNSIGNED_BYTE,
						0);

				// Map the pixel buffer to access its data.
				GLBuffer::MapBufferScope map_pixel_buffer_scope(
						renderer,
						*pixel_buffer->get_buffer(),
						GLBuffer::TARGET_PIXEL_PACK_BUFFER);

				// Map the pixel buffer data.
				const void *result_data = map_pixel_buffer_scope.gl_map_buffer_static(GLBuffer::ACCESS_READ_ONLY);
				const GPlatesGui::rgba8_t *result_rgba8_data = static_cast<const GPlatesGui::rgba8_t *>(result_data);

				boost::scoped_array<GPlatesGui::rgba8_t> rgba8_data(
						new GPlatesGui::rgba8_t[texture->get_width().get() * texture->get_height().get()]);

				for (unsigned int y = 0; y < texture->get_height().get(); ++y)
				{
					for (unsigned int x = 0; x < texture->get_width().get(); ++x)
					{
						const GPlatesGui::rgba8_t &result_pixel = result_rgba8_data[y * texture->get_width().get() + x];

						GPlatesGui::rgba8_t colour(0, 0, 0, 255);
						if (result_pixel.red == 255 && result_pixel.green == 255 && result_pixel.blue == 255)
						{
							colour.red = colour.green = colour.blue = 255;
						}
						else if (result_pixel.red == 0 && result_pixel.green == 0 && result_pixel.blue == 0)
						{
							colour.red = colour.green = colour.blue = 0;
						}
						else
						{
							colour.red = 255;
						}

						rgba8_data.get()[y * texture->get_width().get() + x] = colour;
					}
				}

				map_pixel_buffer_scope.gl_unmap_buffer();

				boost::scoped_array<boost::uint32_t> argb32_data(
						new boost::uint32_t[texture->get_width().get() * texture->get_height().get()]);

				// Convert to a format supported by QImage.
				GPlatesGui::convert_rgba8_to_argb32(
						rgba8_data.get(),
						argb32_data.get(),
						texture->get_width().get() * texture->get_height().get());

				QImage qimage(
						static_cast<const uchar *>(static_cast<const void *>(argb32_data.get())),
						texture->get_width().get(),
						texture->get_height().get(),
						QImage::Format_ARGB32);

				// Save the image to a file.
				QString image_filename = QString("%1%2.png").arg(image_file_basename).arg(layer);
				qimage.save(image_filename, "PNG");
			}

			// Detach from the framebuffer object before we return it to the framebuffer object cache.
			framebuffer_object->gl_detach_all(renderer);
		}
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

		const GLCapabilities &capabilities = renderer.get_capabilities();

		// We essentially need graphics hardware supporting OpenGL 3.0.
		//
		// Instead of testing for GLEW_VERSION_3_0 we test for GL_EXT_texture_array
		// (which was introduced in OpenGL 3.0) - and is the main requirement for the ray-tracing shader.
		// This is done because OpenGL 3.0 is not officially supported on MacOS Snow Leopard - in that
		// it supports OpenGL 3.0 extensions but not the specific OpenGL 3.0 functions (according to
		// http://www.cultofmac.com/26065/snow-leopard-10-6-3-update-significantly-improves-opengl-3-0-support/).
		//
		// All the other requirement should be supported by OpenGL 3.0 hardware.
		if (!capabilities.texture.gl_EXT_texture_array ||
			// Shader relies on hardware bilinear filtering of floating-point textures...
			!capabilities.texture.gl_supports_floating_point_filtering_and_blending ||
			// Using floating-point textures...
			!capabilities.texture.gl_ARB_texture_float ||
			// We want floating-point RG texture format...
			!capabilities.texture.gl_ARB_texture_rg ||
			!capabilities.texture.gl_ARB_texture_non_power_of_two ||
			!capabilities.shader.gl_ARB_vertex_shader ||
			// Use geometry shader to render surface geometries to all six textures of texture array at once...
			!capabilities.shader.gl_EXT_geometry_shader4 ||
			!capabilities.shader.gl_ARB_fragment_shader ||
			// We use multiple render targets to output colour and depth to textures...
			!capabilities.framebuffer.gl_ARB_draw_buffers ||
			// Need to render to surface fill mask...
			!capabilities.framebuffer.gl_EXT_framebuffer_object ||
			// Separate alpha blend for RGB and Alpha...
			!capabilities.framebuffer.gl_EXT_blend_equation_separate ||
			!capabilities.framebuffer.gl_EXT_blend_func_separate ||
			// Min/max alpha-blending...
			!capabilities.framebuffer.gl_EXT_blend_minmax)
		{
			qWarning() << "3D scalar fields NOT supported by this graphics hardware - requires hardware supporting OpenGL 3.0.";
			return false;
		}

		// Make sure we have enough texture image units for the shader programs that uses the most
		// texture units at once.
		if (MAX_TEXTURE_IMAGE_UNITS_USED > capabilities.texture.gl_max_texture_image_units)
		{
			qWarning() << "3D scalar fields NOT supported by this graphics hardware - insufficient texture image units.";
			return false;
		}

		// Make sure our geometry shaders don't output more vertices than allowed.
		if (SURFACE_FILL_MASK_GEOMETRY_SHADER_MAX_OUTPUT_VERTICES > capabilities.shader.gl_max_geometry_output_vertices
			|| VOLUME_FILL_WALL_GEOMETRY_SHADER_MAX_OUTPUT_VERTICES > capabilities.shader.gl_max_geometry_output_vertices
#if 0 // Not currently used...
			|| VOLUME_FILL_SPHERICAL_CAP_GEOMETRY_SHADER_MAX_OUTPUT_VERTICES > capabilities.shader.gl_max_geometry_output_vertices
#endif
			)
		{
			qWarning() << "3D scalar fields NOT supported by this graphics hardware - too many vertices output by geometry shaders.";
			return false;
		}

		// Need to be able to render using a framebuffer object with an attached depth buffer.
		if (!GLScreenRenderTarget::is_supported(renderer, GL_RGBA32F_ARB, true, false) ||
			!GLScreenRenderTarget::is_supported(renderer, GL_RGBA8, true, false))
		{
			qWarning() << "3D scalar fields NOT supported by this graphics hardware - unsupported FBO/depth-buffer combination.";
			return false;
		}

		//
		// Try to compile our most complex ray-tracing shader program.
		//
		// If this fails then it could be exceeding some resource limit on the runtime system.
		//

		GLShaderSource iso_surface_fragment_shader_source(SHADER_VERSION);
		iso_surface_fragment_shader_source.add_code_segment_from_file(GLShaderProgramUtils::UTILS_SHADER_SOURCE_FILE_NAME);
		iso_surface_fragment_shader_source.add_code_segment_from_file(SCALAR_FIELD_UTILS_SOURCE_FILE_NAME);
		iso_surface_fragment_shader_source.add_code_segment_from_file(ISO_SURFACE_FRAGMENT_SHADER_SOURCE_FILE_NAME);
		
		GLShaderSource iso_surface_vertex_shader_source(SHADER_VERSION);
		iso_surface_vertex_shader_source.add_code_segment_from_file(GLShaderProgramUtils::UTILS_SHADER_SOURCE_FILE_NAME);
		iso_surface_vertex_shader_source.add_code_segment_from_file(SCALAR_FIELD_UTILS_SOURCE_FILE_NAME);
		iso_surface_vertex_shader_source.add_code_segment_from_file(ISO_SURFACE_VERTEX_SHADER_SOURCE_FILE_NAME);

		// Attempt to create the test shader program.
		if (!GLShaderProgramUtils::compile_and_link_vertex_fragment_program(
				renderer, iso_surface_vertex_shader_source, iso_surface_fragment_shader_source))
		{
			qWarning() << "3D scalar fields NOT supported by this graphics hardware - failed to compile isosurface shader program.";
			return false;
		}

		GLShaderSource surface_fill_mask_vertex_shader_source(SHADER_VERSION);
		surface_fill_mask_vertex_shader_source.add_code_segment_from_file(GLShaderProgramUtils::UTILS_SHADER_SOURCE_FILE_NAME);
		surface_fill_mask_vertex_shader_source.add_code_segment_from_file(SCALAR_FIELD_UTILS_SOURCE_FILE_NAME);
		surface_fill_mask_vertex_shader_source.add_code_segment_from_file(SURFACE_FILL_MASK_VERTEX_SHADER_SOURCE_FILE_NAME);

		GLShaderSource surface_fill_mask_geometry_shader_source(SHADER_VERSION);
		surface_fill_mask_geometry_shader_source.add_code_segment_from_file(GLShaderProgramUtils::UTILS_SHADER_SOURCE_FILE_NAME);
		surface_fill_mask_geometry_shader_source.add_code_segment_from_file(SCALAR_FIELD_UTILS_SOURCE_FILE_NAME);
		surface_fill_mask_geometry_shader_source.add_code_segment_from_file(SURFACE_FILL_MASK_GEOMETRY_SHADER_SOURCE_FILE_NAME);

		GLShaderSource surface_fill_mask_fragment_shader_source(SHADER_VERSION);
		surface_fill_mask_fragment_shader_source.add_code_segment_from_file(GLShaderProgramUtils::UTILS_SHADER_SOURCE_FILE_NAME);
		surface_fill_mask_fragment_shader_source.add_code_segment_from_file(SCALAR_FIELD_UTILS_SOURCE_FILE_NAME);
		surface_fill_mask_fragment_shader_source.add_code_segment_from_file(SURFACE_FILL_MASK_FRAGMENT_SHADER_SOURCE_FILE_NAME);

		// Attempt to create the test shader program.
		if (!GLShaderProgramUtils::compile_and_link_vertex_geometry_fragment_program(
				renderer,
				surface_fill_mask_vertex_shader_source,
				surface_fill_mask_geometry_shader_source,
				surface_fill_mask_fragment_shader_source,
				GLShaderProgramUtils::GeometryShaderProgramParameters(
						SURFACE_FILL_MASK_GEOMETRY_SHADER_MAX_OUTPUT_VERTICES)))
		{
			qWarning() << "3D scalar fields NOT supported by this graphics hardware - failed to compile surface fill mask shader program.";
			return false;
		}

		// TODO: Add framebuffer check status for rendering to layered texture array (surface fill mask).

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
	d_surface_fill_mask_resolution(SURFACE_FILL_MASK_RESOLUTION),
	d_streaming_vertex_element_buffer(
			GLVertexElementBuffer::create(renderer, GLBuffer::create(renderer, GLBuffer::BUFFER_TYPE_VERTEX))),
	d_streaming_vertex_buffer(
			GLVertexBuffer::create(renderer, GLBuffer::create(renderer, GLBuffer::BUFFER_TYPE_VERTEX))),
	d_cross_section_vertex_array(GLVertexArray::create(renderer)),
	d_surface_fill_mask_vertex_array(GLVertexArray::create(renderer)),
	d_volume_fill_boundary_vertex_array(GLVertexArray::create(renderer)),
	d_white_inner_sphere_vertex_array(GLVertexArray::create(renderer))
{
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

	// Check that the number of texture array layers does not exceed the maximum supported by
	// the GPU on the runtime system.
	// TODO: For now we'll just report an error but later we'll need to adapt somehow.
	GPlatesGlobal::Assert<GPlatesGlobal::LogException>(
			d_num_active_tiles * d_num_depth_layers <=
				renderer.get_capabilities().texture.gl_max_texture_array_layers,
			GPLATES_ASSERTION_SOURCE,
			"GLScalarField3D: number texture layers in scalar field file exceeded GPU limit.");

	// An inner sphere needs to be explicitly rendered when drawing cross-sections.
	// It's also used when rendering depth range of volume fill walls.
	// However it's rendered implicitly by ray-tracing when rendering iso-surface.
	initialise_inner_sphere(renderer);

	// Allocate memory for the vertex buffers used to render cross-section geometry and
	// surface geometry for surface fill mask texture array.
	allocate_streaming_vertex_buffers(renderer);

	// Initialise the shader program and vertex arrays for rendering cross-section geometry.
	initialise_cross_section_rendering(renderer);

	// Initialise the shader program for rendering isosurface.
	initialise_iso_surface_rendering(renderer);

	// Initialise the shader program for rendering surface fill mask.
	initialise_surface_fill_mask_rendering(renderer);

	// Initialise the shader program for rendering volume fill boundary.
	initialise_volume_fill_boundary_rendering(renderer);

	create_tile_meta_data_texture_array(renderer);
	create_field_data_texture_array(renderer);
	create_mask_data_texture_array(renderer);
	create_depth_radius_to_layer_texture(renderer);
	create_colour_palette_texture(renderer);

	// Load the scalar field from the file.
	load_scalar_field(renderer, scalar_field_reader);

	// The colour palette texture will get loaded when the client calls 'set_colour_palette()'.
}


void
GPlatesOpenGL::GLScalarField3D::set_colour_palette(
		GLRenderer &renderer,
		const GPlatesGui::ColourPalette<double>::non_null_ptr_to_const_type &colour_palette,
		const std::pair<double, double> &colour_palette_value_range)
{
	d_colour_palette_value_range = colour_palette_value_range;
	load_colour_palette_texture(renderer, colour_palette, colour_palette_value_range);
}


bool
GPlatesOpenGL::GLScalarField3D::change_scalar_field(
		GLRenderer &renderer,
		const QString &scalar_field_filename)
{
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
GPlatesOpenGL::GLScalarField3D::render_iso_surface(
		GLRenderer &renderer,
		cache_handle_type &cache_handle,
		GPlatesViewOperations::ScalarField3DRenderParameters::IsosurfaceDeviationWindowMode deviation_window_mode,
		GPlatesViewOperations::ScalarField3DRenderParameters::IsosurfaceColourMode colour_mode,
		const GPlatesViewOperations::ScalarField3DRenderParameters::IsovalueParameters &isovalue_parameters,
		const GPlatesViewOperations::ScalarField3DRenderParameters::DeviationWindowRenderOptions &deviation_window_render_options,
		const GPlatesViewOperations::ScalarField3DRenderParameters::DepthRestriction &depth_restriction,
		const GPlatesViewOperations::ScalarField3DRenderParameters::QualityPerformance &quality_performance,
		const std::vector<float> &test_variables,
		boost::optional<SurfaceFillMask> surface_fill_mask,
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

	// Bind the shader program for rendering iso-surface.
	renderer.gl_bind_program_object(d_render_iso_surface_program_object.get());

	unsigned int current_texture_unit = 0;

	// Set shader variables common to all shaders (currently iso-surface and cross-sections).
	set_iso_surface_and_cross_sections_shader_common_variables(
			renderer,
			d_render_iso_surface_program_object.get(),
			current_texture_unit,
			depth_restriction,
			test_variables,
			surface_occlusion_texture);

	// Currently always using orthographic projection.
	// TODO: Add support for perspective projection.
	d_render_iso_surface_program_object.get()->gl_uniform1i(
			renderer,
			"using_ortho_projection",
			true);

	// Specify the colour mode.
	d_render_iso_surface_program_object.get()->gl_uniform1i(
			renderer,
			"colour_mode_depth",
			colour_mode == GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_COLOUR_MODE_DEPTH);
	d_render_iso_surface_program_object.get()->gl_uniform1i(
			renderer,
			"colour_mode_isovalue",
			colour_mode == GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_COLOUR_MODE_SCALAR);
	d_render_iso_surface_program_object.get()->gl_uniform1i(
			renderer,
			"colour_mode_gradient",
			colour_mode == GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_COLOUR_MODE_GRADIENT);

	// Set the min/max range of values used to map to colour whether that mapping is a look up
	// of the colour palette (eg, colouring by scalar value or gradient magnitude) or by using
	// a hard-wired mapping in the shader code.
	GLfloat min_colour_mapping_range = 0;
	GLfloat max_colour_mapping_range = 0;
	switch (colour_mode)
	{
	case GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_COLOUR_MODE_DEPTH:
		// Colour mapping range not used in shader code.
		break;
	case GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_COLOUR_MODE_SCALAR:
		min_colour_mapping_range = d_colour_palette_value_range.first/*min*/;
		max_colour_mapping_range = d_colour_palette_value_range.second/*max*/;
		break;
	case GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_COLOUR_MODE_GRADIENT:
		min_colour_mapping_range = d_colour_palette_value_range.first/*min*/;
		max_colour_mapping_range = d_colour_palette_value_range.second/*max*/;
		break;
	default:
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
		break;
	}
	d_render_iso_surface_program_object.get()->gl_uniform2f(
			renderer,
			"min_max_colour_mapping_range",
			min_colour_mapping_range,
			max_colour_mapping_range);

	//
	// Set the depth read texture.
	//

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
		// Unbind the depth texture sampler from current texture unit.
		renderer.gl_unbind_texture(GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_2D);
		// NOTE: Set the shader sampler to the current texture unit instead of a used texture unit
		// like unit 0. This avoids shader program validation failure when active shader samplers of
		// different types reference the same texture unit. Currently happens on MacOS - probably
		// because shader compiler does not detect that the sampler is not used and keeps it active.
		d_render_iso_surface_program_object.get()->gl_uniform1i(
				renderer,
				"depth_texture_sampler",
				current_texture_unit);
		// Move to the next texture unit.
		++current_texture_unit;

		// Disable reads from depth texture.
		d_render_iso_surface_program_object.get()->gl_uniform1i(
				renderer,
				"read_from_depth_texture",
				false);
	}

	//
	// Set the surface fill mask options.
	//

	// Note: These are declared in the same scope as the isosurface rendering to ensure that these
	// screen render targets (used for the depth range and wall surface normals of the
	// volume fill region) are not released before their internal textures are used when rendering isosurface.
	// If this was not done then another client might acquire (from GLContext) the same screen
	// render target and draw something else into the internal texture before we've used it.
	boost::optional<GLScreenRenderTarget::shared_ptr_type> volume_fill_wall_depth_range_screen_render_target;
	boost::optional<GLScreenRenderTarget::shared_ptr_type> volume_fill_wall_surface_normal_and_depth_screen_render_target;

	// Surface fill mask texture defining surface fill area on surface of globe.
	GLTexture::shared_ptr_to_const_type surface_fill_mask_texture;
	// First generate the surface fill mask from the surface geometries if requested.
	// The returned texture array was temporarily acquired (from GLContext) and will be returned
	// when GLRenderer has finished using it, ie, when it is no longer bound to a texture slot
	// ('gl_bind_texture()' keeps the binding until it's unbound or bound to another texture).
	if (surface_fill_mask &&
		render_surface_fill_mask(
				renderer,
				surface_fill_mask->surface_polygons_mask,
				surface_fill_mask->treat_polylines_as_polygons,
				surface_fill_mask_texture))
	{
		// If we have a surface fill mask, but we are not drawing the volume fill walls
		// (surface normal and depth), then generate the min/max depth range of the volume fill walls.
		// This makes the isosurface shader more efficient by reducing the length along
		// each ray that is sampled/traversed - note that the walls are not visible though.
		// We don't need this if the walls are going to be drawn because there are already good
		// optimisations in place to limit ray sampling based on the fact that the walls are opaque.
		if (!surface_fill_mask->show_walls &&
			render_volume_fill_wall_depth_range(
					renderer,
					surface_fill_mask->surface_polygons_mask,
					surface_fill_mask->treat_polylines_as_polygons,
					surface_fill_mask_texture,
					depth_restriction,
					volume_fill_wall_depth_range_screen_render_target))
		{
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					volume_fill_wall_depth_range_screen_render_target,
					GPLATES_ASSERTION_SOURCE);

			// Set volume fill wall depth range sampler to current texture unit.
			renderer.gl_bind_texture(
					volume_fill_wall_depth_range_screen_render_target.get()->get_texture(),
					GL_TEXTURE0 + current_texture_unit,
					GL_TEXTURE_2D);
			d_render_iso_surface_program_object.get()->gl_uniform1i(
					renderer,
					"volume_fill_wall_depth_range_sampler",
					current_texture_unit);
			// Move to the next texture unit.
			++current_texture_unit;

			// Enable rendering using the volume fill wall depth range.
			d_render_iso_surface_program_object.get()->gl_uniform1i(
					renderer,
					"using_volume_fill_wall_depth_range",
					true);
		}
		else
		{
			// Unbind the volume fill wall depth range sampler from current texture unit.
			renderer.gl_unbind_texture(GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_2D);
			// NOTE: Set the shader sampler to the current texture unit instead of a used texture unit
			// like unit 0. This avoids shader program validation failure when active shader samplers of
			// different types reference the same texture unit. Currently happens on MacOS - probably
			// because shader compiler does not detect that the sampler is not used and keeps it active.
			d_render_iso_surface_program_object.get()->gl_uniform1i(
					renderer,
					"volume_fill_wall_depth_range_sampler",
					current_texture_unit);
			// Move to the next texture unit.
			++current_texture_unit;

			// Disable rendering using the volume fill wall depth range.
			d_render_iso_surface_program_object.get()->gl_uniform1i(
					renderer,
					"using_volume_fill_wall_depth_range",
					false);
		}

		// If we've been requested to render the walls of the volume fill region then
		// render the screen-size normal/depth texture.
		if (surface_fill_mask->show_walls &&
			render_volume_fill_wall_surface_normal_and_depth(
					renderer,
					surface_fill_mask->surface_polygons_mask,
					surface_fill_mask->treat_polylines_as_polygons,
					surface_fill_mask->show_walls->only_boundary_walls,
					surface_fill_mask_texture,
					depth_restriction,
					volume_fill_wall_surface_normal_and_depth_screen_render_target))
		{
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					volume_fill_wall_surface_normal_and_depth_screen_render_target,
					GPLATES_ASSERTION_SOURCE);

			// Set volume fill walls sampler to current texture unit.
			renderer.gl_bind_texture(
					volume_fill_wall_surface_normal_and_depth_screen_render_target.get()->get_texture(),
					GL_TEXTURE0 + current_texture_unit,
					GL_TEXTURE_2D);
			d_render_iso_surface_program_object.get()->gl_uniform1i(
					renderer,
					"volume_fill_wall_surface_normal_and_depth_sampler",
					current_texture_unit);
			// Move to the next texture unit.
			++current_texture_unit;

			// Enable rendering of the volume fill walls.
			d_render_iso_surface_program_object.get()->gl_uniform1i(
					renderer,
					"show_volume_fill_walls",
					true);
		}
		else
		{
			// Unbind the volume fill walls sampler from current texture unit.
			renderer.gl_unbind_texture(GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_2D);
			// NOTE: Set the shader sampler to the current texture unit instead of a used texture unit
			// like unit 0. This avoids shader program validation failure when active shader samplers of
			// different types reference the same texture unit. Currently happens on MacOS - probably
			// because shader compiler does not detect that the sampler is not used and keeps it active.
			d_render_iso_surface_program_object.get()->gl_uniform1i(
					renderer,
					"volume_fill_wall_surface_normal_and_depth_sampler",
					current_texture_unit);
			// Move to the next texture unit.
			++current_texture_unit;

			// Disable rendering of the volume fill walls.
			d_render_iso_surface_program_object.get()->gl_uniform1i(
					renderer,
					"show_volume_fill_walls",
					false);
		}

		// Set surface fill mask sampler to current texture unit.
		renderer.gl_bind_texture(surface_fill_mask_texture, GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_2D_ARRAY_EXT);
		d_render_iso_surface_program_object.get()->gl_uniform1i(
				renderer,
				"surface_fill_mask_sampler",
				current_texture_unit);
		// Move to the next texture unit.
		++current_texture_unit;

		// Set the surface fill mask (square) texture resolution.
		// This is a texture array containing square textures (width == height).
		d_render_iso_surface_program_object.get()->gl_uniform1i(
				renderer,
				"surface_fill_mask_resolution",
				surface_fill_mask_texture->get_width().get());

		// Enable reads from surface fill mask.
		d_render_iso_surface_program_object.get()->gl_uniform1i(
				renderer,
				"using_surface_fill_mask",
				true);
	}
	else
	{
		// Unbind the surface fill mask sampler from current texture unit.
		renderer.gl_unbind_texture(GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_2D_ARRAY_EXT);
		// NOTE: Set the shader sampler to the current texture unit instead of a used texture unit
		// like unit 0. This avoids shader program validation failure when active shader samplers of
		// different types reference the same texture unit. Currently happens on MacOS - probably
		// because shader compiler does not detect that the sampler is not used and keeps it active.
		d_render_iso_surface_program_object.get()->gl_uniform1i(
				renderer,
				"surface_fill_mask_sampler",
				current_texture_unit);
		// Move to the next texture unit.
		++current_texture_unit;

		// Disable reads from surface fill mask.
		d_render_iso_surface_program_object.get()->gl_uniform1i(
				renderer,
				"using_surface_fill_mask",
				false);

		// Unbind the volume fill wall depth range sampler from current texture unit.
		renderer.gl_unbind_texture(GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_2D);
		// NOTE: Set the shader sampler to the current texture unit instead of a used texture unit
		// like unit 0. This avoids shader program validation failure when active shader samplers of
		// different types reference the same texture unit. Currently happens on MacOS - probably
		// because shader compiler does not detect that the sampler is not used and keeps it active.
		d_render_iso_surface_program_object.get()->gl_uniform1i(
				renderer,
				"volume_fill_wall_depth_range_sampler",
				current_texture_unit);
		// Move to the next texture unit.
		++current_texture_unit;

		// Disable rendering using the volume fill wall depth range.
		d_render_iso_surface_program_object.get()->gl_uniform1i(
				renderer,
				"using_volume_fill_wall_depth_range",
				false);

		// Unbind the volume fill walls sampler from current texture unit.
		renderer.gl_unbind_texture(GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_2D);
		// NOTE: Set the shader sampler to the current texture unit instead of a used texture unit
		// like unit 0. This avoids shader program validation failure when active shader samplers of
		// different types reference the same texture unit. Currently happens on MacOS - probably
		// because shader compiler does not detect that the sampler is not used and keeps it active.
		d_render_iso_surface_program_object.get()->gl_uniform1i(
				renderer,
				"volume_fill_wall_surface_normal_and_depth_sampler",
				current_texture_unit);
		// Move to the next texture unit.
		++current_texture_unit;

		// Disable rendering of the volume fill walls.
		d_render_iso_surface_program_object.get()->gl_uniform1i(
				renderer,
				"show_volume_fill_walls",
				false);
	}

	// Note that 'colour_mode_depth', etc, is set in
	// 'set_iso_surface_and_cross_sections_shader_common_variables()'.

	//
	// Set the isovalue parameters.
	//

	// Instead of setting boolean variables, according to the render mode, for the shader program
	// to branch the shader program currently just draws a double deviation surface in all cases.
	// So we need to set the isovalue (and deviation) parameters to emulate a single deviation window
	// or just regular isosurface rendering.
	//
	// These are the current rules...
	//
	// Each isovalue is a vec3 with (x,y,z) components that are:
	//   (isovalue, lower deviation, upper deviation).
	//
	// If deviation parameters are symmetric then lower and upper deviation will have same value.
	//
	// If rendering a single isosurface then:
	//   isovalue1 = vec3(<isovalue 1>, 0, 0);
	//   isovalue2 = vec3(<isovalue 1>, 0, 0);
	// If rendering a single deviation window then:
	//   isovalue1 = vec3(<isovalue 1>, <lower deviation 1>, 0);
	//   isovalue2 = vec3(<isovalue 1>, 0, <upper deviation 1>);
	// If rendering a double deviation window then:
	//   isovalue1 = vec3(<isovalue 1>, <lower deviation 1>, <upper deviation 1>);
	//   isovalue2 = vec3(<isovalue 2>, <lower deviation 2>, <upper deviation 2>);
	GPlatesViewOperations::ScalarField3DRenderParameters::IsovalueParameters emulated_isovalue_parameters;
	if (deviation_window_mode == GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_DEVIATION_WINDOW_MODE_NONE)
	{
		emulated_isovalue_parameters.isovalue1 = isovalue_parameters.isovalue1;
		emulated_isovalue_parameters.isovalue2 = isovalue_parameters.isovalue1;
		emulated_isovalue_parameters.lower_deviation1 = 0;
		emulated_isovalue_parameters.upper_deviation1 = 0;
		emulated_isovalue_parameters.lower_deviation2 = 0;
		emulated_isovalue_parameters.upper_deviation2 = 0;
	}
	else if (deviation_window_mode == GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_DEVIATION_WINDOW_MODE_SINGLE)
	{
		emulated_isovalue_parameters.isovalue1 = isovalue_parameters.isovalue1;
		emulated_isovalue_parameters.isovalue2 = isovalue_parameters.isovalue1;
		emulated_isovalue_parameters.lower_deviation1 = isovalue_parameters.lower_deviation1;
		emulated_isovalue_parameters.upper_deviation1 = 0;
		emulated_isovalue_parameters.lower_deviation2 = 0;
		emulated_isovalue_parameters.upper_deviation2 = isovalue_parameters.upper_deviation1;
	}
	else if (deviation_window_mode == GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_DEVIATION_WINDOW_MODE_DOUBLE)
	{
		emulated_isovalue_parameters.isovalue1 = isovalue_parameters.isovalue1;
		emulated_isovalue_parameters.isovalue2 = isovalue_parameters.isovalue2;
		emulated_isovalue_parameters.lower_deviation1 = isovalue_parameters.lower_deviation1;
		emulated_isovalue_parameters.upper_deviation1 = isovalue_parameters.upper_deviation1;
		emulated_isovalue_parameters.lower_deviation2 = isovalue_parameters.lower_deviation2;
		emulated_isovalue_parameters.upper_deviation2 = isovalue_parameters.upper_deviation2;
	}

	// Set the parameters associated with isovalue 1.
	d_render_iso_surface_program_object.get()->gl_uniform3f(
			renderer,
			"isovalue1",
			emulated_isovalue_parameters.isovalue1,
			emulated_isovalue_parameters.isovalue1 - emulated_isovalue_parameters.lower_deviation1,
			emulated_isovalue_parameters.isovalue1 + emulated_isovalue_parameters.upper_deviation1);
// 	qDebug() << "isovalue1: "
// 		<< emulated_isovalue_parameters.isovalue1 << ", "
// 		<< emulated_isovalue_parameters.lower_deviation1 << ", "
// 		<< emulated_isovalue_parameters.upper_deviation1;

	// Set the parameters associated with isovalue 2.
	d_render_iso_surface_program_object.get()->gl_uniform3f(
			renderer,
			"isovalue2",
			emulated_isovalue_parameters.isovalue2,
			emulated_isovalue_parameters.isovalue2 - emulated_isovalue_parameters.lower_deviation2,
			emulated_isovalue_parameters.isovalue2 + emulated_isovalue_parameters.upper_deviation2);
// 	qDebug() << "isovalue2: "
// 		<< emulated_isovalue_parameters.isovalue2 << ", "
// 		<< emulated_isovalue_parameters.lower_deviation2 << ", "
// 		<< emulated_isovalue_parameters.upper_deviation2;

	//
	// Set the render options.
	//
	// NOTE: For regular isosurface rendering (ie, not single or double deviation window) these
	// parameters are set to the following defaults:
	//
	//   opacity_deviation_surfaces = 1.0
	//   deviation_window_volume_rendering = false
	//   opacity_deviation_window_volume_rendering = 1.0
	//   surface_deviation_window = false
	//   surface_deviation_isoline_frequency = 0
	//
	// ...this enables the shader program, as with the isovalue parameters, to render/emulate all
	// isosurface modes as a double deviation window with differences expressed as the parameters.
	GPlatesViewOperations::ScalarField3DRenderParameters::DeviationWindowRenderOptions
			emulated_deviation_window_render_options(deviation_window_render_options);
	if (deviation_window_mode == GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_DEVIATION_WINDOW_MODE_NONE)
	{
		emulated_deviation_window_render_options.opacity_deviation_surfaces = 1;
		emulated_deviation_window_render_options.deviation_window_volume_rendering = false;
		emulated_deviation_window_render_options.opacity_deviation_window_volume_rendering = 1;
		emulated_deviation_window_render_options.surface_deviation_window = false;
		emulated_deviation_window_render_options.surface_deviation_window_isoline_frequency = 0;
	}
	// ...else single or double deviation window.

	d_render_iso_surface_program_object.get()->gl_uniform1f(
			renderer,
			"opacity_deviation_surfaces",
			emulated_deviation_window_render_options.opacity_deviation_surfaces);
// 	qDebug() << "opacity_deviation_surfaces: " << emulated_deviation_window_render_options.opacity_deviation_surfaces;
	d_render_iso_surface_program_object.get()->gl_uniform1i(
			renderer,
			"deviation_window_volume_rendering",
			emulated_deviation_window_render_options.deviation_window_volume_rendering);
// 	qDebug() << "deviation_window_volume_rendering: " << emulated_deviation_window_render_options.deviation_window_volume_rendering;
	d_render_iso_surface_program_object.get()->gl_uniform1f(
			renderer,
			"opacity_deviation_window_volume_rendering",
			emulated_deviation_window_render_options.opacity_deviation_window_volume_rendering);
// 	qDebug() << "opacity_deviation_window_volume_rendering: " << emulated_deviation_window_render_options.opacity_deviation_window_volume_rendering;
	d_render_iso_surface_program_object.get()->gl_uniform1i(
			renderer,
			"surface_deviation_window",
			emulated_deviation_window_render_options.surface_deviation_window);
// 	qDebug() << "surface_deviation_window: " << emulated_deviation_window_render_options.surface_deviation_window;
	d_render_iso_surface_program_object.get()->gl_uniform1f(
			renderer,
			"surface_deviation_isoline_frequency",
			emulated_deviation_window_render_options.surface_deviation_window_isoline_frequency);
// 	qDebug() << "surface_deviation_isoline_frequency: " << emulated_deviation_window_render_options.surface_deviation_window_isoline_frequency;

	// Note that 'render_min_max_depth_radius_restriction' is set in
	// 'set_iso_surface_and_cross_sections_shader_common_variables()'.

	//
	// Set the quality/performance options.
	//

	d_render_iso_surface_program_object.get()->gl_uniform2f(
			renderer,
			"sampling_rate",
			// Distance between samples - and 2.0 is diameter of the globe...
			2.0f / quality_performance.sampling_rate,
			quality_performance.sampling_rate / 2.0f);
// 	qDebug() << "sampling_rate: "
// 		<< 2.0f / quality_performance.sampling_rate << ", "
// 		<< quality_performance.sampling_rate / 2.0f;
	d_render_iso_surface_program_object.get()->gl_uniform1i(
			renderer,
			"bisection_iterations",
			quality_performance.bisection_iterations);
// 	qDebug() << "bisection_iterations: " << quality_performance.bisection_iterations;

	// Used to draw a full-screen quad.
	const GLCompiledDrawState::non_null_ptr_to_const_type full_screen_quad_drawable =
			renderer.get_context().get_shared_state()->get_full_screen_2D_textured_quad(renderer);

	// Render the full-screen quad.
	renderer.apply_compiled_draw_state(*full_screen_quad_drawable);
}


bool
GPlatesOpenGL::GLScalarField3D::render_surface_fill_mask(
		GLRenderer &renderer,
		const surface_polygons_mask_seq_type &surface_polygons_mask,
		bool include_polylines,
		GLTexture::shared_ptr_to_const_type &surface_fill_mask_texture)
{
	// Make sure we leave the OpenGL state the way it was.
	GLRenderer::StateBlockScope save_restore_state(
			renderer,
			// We're rendering to a render target so reset to the default OpenGL state...
			true/*reset_to_default_state*/);

	// We should always have a valid shader program but test just in case.
	if (!d_render_surface_fill_mask_program_object)
	{
		return false;
	}

	// Bind the shader program for rendering the surface fill mask.
	renderer.gl_bind_program_object(d_render_surface_fill_mask_program_object.get());

	// Temporarily acquire a texture array to render the surface fill mask into.
	surface_fill_mask_texture = acquire_surface_fill_mask_texture(renderer);

	// Classify our frame buffer object according to texture format/dimensions.
	GLFrameBufferObject::Classification framebuffer_object_classification;
	framebuffer_object_classification.set_dimensions(
			renderer,
			surface_fill_mask_texture->get_width().get(),
			surface_fill_mask_texture->get_height().get());
	framebuffer_object_classification.set_attached_texture_array(
			renderer,
			surface_fill_mask_texture->get_internal_format().get());

	// Acquire and bind a frame buffer object.
	GLFrameBufferObject::shared_ptr_type framebuffer_object =
			renderer.get_context().get_non_shared_state()->acquire_frame_buffer_object(
					renderer,
					framebuffer_object_classification);
	renderer.gl_bind_frame_buffer(framebuffer_object);

	// Begin rendering to the entire texture array (layered texture rendering).
	// We will be using a geometry shader to direct each filled primitive to all six layers of the texture array.
	framebuffer_object->gl_attach_texture_array(
			renderer,
			surface_fill_mask_texture,
			0, // level - note that this is mipmap level and not the layer number
			GL_COLOR_ATTACHMENT0_EXT);

	// Check for framebuffer completeness (after attaching to texture array).
	// It seems some hardware fails even though we checked OpenGL capabilities in 'is_supported()'
	// such as 'gl_EXT_geometry_shader4' and we are using nice power-of-two texture dimensions, etc.
	// Note that the expensive completeness check is cached so it shouldn't slow us down.
	if (!renderer.get_context().get_non_shared_state()->check_framebuffer_object_completeness(
			renderer,
			framebuffer_object,
			framebuffer_object_classification))
	{
		// Only output warning once for each framebuffer object classification.
		static std::set<GLFrameBufferObject::Classification::tuple_type> warning_map;
		if (warning_map.find(framebuffer_object_classification.get_tuple()) == warning_map.end())
		{
			qWarning() << "Scalar field surface polygons mask failed framebuffer completeness check.";

			// Flag warning has been output.
			warning_map.insert(framebuffer_object_classification.get_tuple());
		}

		// Detach from the framebuffer object before it gets returned to the framebuffer object cache.
		framebuffer_object->gl_detach_all(renderer);

		return false;
	}

	// Clear all layers of texture array.
	renderer.gl_clear_color(); // Clear colour to all zeros.
	renderer.gl_clear(GL_COLOR_BUFFER_BIT); // Clear only the colour buffer.

	// Bind the surface fill mask vertex array.
	d_surface_fill_mask_vertex_array->gl_bind(renderer);

	// Viewport for the textures in the field texture array.
	renderer.gl_viewport(0, 0,
			surface_fill_mask_texture->get_width().get(),
			surface_fill_mask_texture->get_height().get());

	// Set up separate alpha-blending factors for the RGB and Alpha channels.
	// Doing this means we can minimise OpenGL state changes and simply switch between
	// masking the RGB channels and masking the Alpha channel to switch between generating
	// a fill for a single polygon and accumulating that fill in the render target.
	renderer.gl_enable(GL_BLEND);
	// The RGB channel factors copy over destination alpha to destination RGB in order to accumulate
	// multiple filled polygons into the render target.
	// The alpha channel factors are what actually generate a (concave) polygon fill.
	renderer.gl_blend_func_separate(
			// Accumulate destination alpha into destination RGB...
			GL_DST_ALPHA, GL_ONE,
			// Invert destination alpha every time a pixel is rendered (this means we get 1 where a
			// pixel is covered by an odd number of triangles and 0 by an even number of triangles)...
			GL_ONE_MINUS_DST_ALPHA, GL_ZERO);

	// Visitor to render surface fill mask geometries.
	SurfaceFillMaskGeometryOnSphereVisitor surface_fill_mask_visitor(
			renderer,
			d_streaming_vertex_element_buffer,
			d_streaming_vertex_buffer,
			d_surface_fill_mask_vertex_array,
			include_polylines);

	// Render the surface fill mask polygons (and optionally polylines).
	for (surface_polygons_mask_seq_type::const_iterator surface_geoms_iter = surface_polygons_mask.begin();
		surface_geoms_iter != surface_polygons_mask.end();
		++surface_geoms_iter)
	{
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &surface_geometry = *surface_geoms_iter;

		surface_geometry->accept_visitor(surface_fill_mask_visitor);
	}

	// Detach from the framebuffer object before we return it to the framebuffer object cache.
	framebuffer_object->gl_detach_all(renderer);

	return true;
}


bool
GPlatesOpenGL::GLScalarField3D::render_volume_fill_wall_depth_range(
		GLRenderer &renderer,
		const surface_polygons_mask_seq_type &surface_polygons_mask,
		bool include_polylines,
		const GLTexture::shared_ptr_to_const_type &surface_fill_mask_texture,
		const GPlatesViewOperations::ScalarField3DRenderParameters::DepthRestriction &depth_restriction,
		boost::optional<GLScreenRenderTarget::shared_ptr_type> &volume_fill_depth_range_screen_render_target)
{
	// Make sure we leave the OpenGL state the way it was.
	// NOTE: We're not reseting to the default OpenGL state because we want to use the current
	// GL_MODELVIEW and GL_PROJECTION matrices as well as the current viewport.
	GLRenderer::StateBlockScope save_restore_state(renderer);

#if 0 // Not currently used...
	// We should always have valid shader programs but test just in case.
	if (!d_render_volume_fill_spherical_cap_depth_range_program_object)
	{
		return false;
	}

	// Set the depth restricted minimum and maximum depth radius of the scalar field.
	d_render_volume_fill_spherical_cap_depth_range_program_object.get()->gl_uniform2f(
			renderer,
			"render_min_max_depth_radius_restriction",
			depth_restriction.min_depth_radius_restriction,
			depth_restriction.max_depth_radius_restriction);

	// Set the tessellate parameters to ensure the polygon fan triangles of the spherical cap
	// roughly follow the curved spherical surface rather than cut into it too much.
	d_render_volume_fill_spherical_cap_depth_range_program_object.get()->gl_uniform2f(
			renderer,
			"tessellate_params",
			COSINE_GREAT_CIRCLE_ARC_ANGULAR_THRESHOLD,
			SPHERICAL_CAP_NUM_SUBDIVISIONS);

	// Set surface fill mask sampler to texture unit 0.
	// Note: This is only needed when rendering spherical caps.
	renderer.gl_bind_texture(surface_fill_mask_texture, GL_TEXTURE0, GL_TEXTURE_2D_ARRAY_EXT);
	d_render_volume_fill_spherical_cap_depth_range_program_object.get()->gl_uniform1i(
			renderer,
			"surface_fill_mask_sampler",
			0);

	// Set the surface fill mask (square) texture resolution.
	// This is a texture array containing square textures (width == height).
	// Note: This is only needed when rendering spherical caps.
	d_render_volume_fill_spherical_cap_depth_range_program_object.get()->gl_uniform1i(
			renderer,
			"surface_fill_mask_resolution",
			surface_fill_mask_texture->get_width().get());
#endif

	// We should always have valid shader programs but test just in case.
	if (!d_render_volume_fill_wall_depth_range_program_object)
	{
		return false;
	}

	// Set the depth restricted minimum and maximum depth radius of the scalar field.
	//
	// NOTE: We artificially reduce the min depth to avoid artifacts due to discarded iso-surface rays
	// when a wall is perpendicular to the ray - in this case the finite tessellation of the
	// inner sphere leaves thin cracks of pixels adjacent to the wall where no depth range is
	// recorded - and at these pixels the ray's min and max depth can become equal.
	// By setting reducing the min depth we extrude the wall further in order to cover these cracks.
	d_render_volume_fill_wall_depth_range_program_object.get()->gl_uniform2f(
			renderer,
			"render_min_max_depth_radius_restriction",
			0.9f * depth_restriction.min_depth_radius_restriction,
			depth_restriction.max_depth_radius_restriction);


	// We don't need a depth buffer when rendering the min/max depths using min/max alpha-blending.
	// In fact a depth buffer (with depth-testing enabled) would interfere with max blending.
	// We're also using a four-channel RGBA floating-point texture.
	// Would be nicer to use two-channel RG but alpha-blending min/max can only have separate
	// blend equations for RGB and Alpha (not R and G).
	// Two channels contain min/max depth and one channel contains flag indicating volume intersection.
	volume_fill_depth_range_screen_render_target =
			renderer.get_context().get_non_shared_state()->acquire_screen_render_target(
					renderer,
					GL_RGBA32F_ARB/*texture_internalformat*/,
					false/*include_depth_buffer*/,
					false/*include_stencil_buffer*/);

	// We've already checked for screen render target support in 'is_supported()' so this shouldn't fail.
	// If it does then return false to ignore request to render boundary of volume fill region.
	if (!volume_fill_depth_range_screen_render_target)
	{
		return false;
	}

	// Bind the volume fill boundary vertex array.
	d_volume_fill_boundary_vertex_array->gl_bind(renderer);

	// The viewport of the screen we're rendering to.
	const GLViewport screen_viewport = renderer.gl_get_viewport();

	// Begin rendering to the depth range render target.
	GLScreenRenderTarget::RenderScope volume_fill_depth_range_screen_render_target_scope(
			*volume_fill_depth_range_screen_render_target.get(),
			renderer,
			screen_viewport.width(),
			screen_viewport.height());

	// Set the new viewport in case the current viewport has non-zero x and y offsets which happens
	// when the main scene is rendered as overlapping tiles (for rendering very large images).
	// It's also important that, later when accessing the screen render texture, the NDC
	// coordinates (-1,-1) and (1,1) map to the corners of the screen render texture.
	renderer.gl_viewport(0, 0, screen_viewport.width(), screen_viewport.height());
	// Also change the scissor rectangle in case scissoring is enabled.
	renderer.gl_scissor(0, 0, screen_viewport.width(), screen_viewport.height());

	// Enable alpha-blending and set the RGB blend equation to GL_MIN and Alpha to GL_MAX.
	renderer.gl_enable(GL_BLEND, GL_TRUE);
	renderer.gl_blend_equation_separate(GL_MIN_EXT/*modeRGB*/, GL_MAX_EXT/*modeAlpha*/);
	// Disable alpha-testing.
	renderer.gl_enable(GL_ALPHA_TEST, GL_FALSE);

	// Disable depth testing and depth writes - we don't have a depth buffer - because
	// a depth buffer (with depth-testing enabled) would interfere with max blending.
	renderer.gl_enable(GL_DEPTH_TEST, GL_FALSE);
	renderer.gl_depth_mask(GL_FALSE);

	// Clear render target to a particular value.
	// The blue and alpha channels store the minimum and maximum of the screen-space depth [-1,1]
	// (ie, in normalised device coordinates, not window coordinates) so they must start out as the
	// maximum and minimum possible values respectively (ie, +1 and -1).
	// The alpha-blending has been set to GL_MIN for RGB and GL_MAX for Alpha.
	// The red/green channel is a flag to indicate if a screen pixel intersects the volume fill region.
	// It will remain at 2.0f unless rendered to by the volume fill boundary geometry in which case
	// it will be in the range [-1,1] due to the GL_MIN blending of the RGB channels.
	// This is used in the isosurface ray-tracing shader to ignore pixels outside the
	// volume fill region by comparing with 2.0.
	renderer.gl_clear_color(2.0f, 2.0f, 1.0f, -1.0f);
	renderer.gl_clear(GL_COLOR_BUFFER_BIT); // There's only a colour buffer (no depth buffer).

	// First render the inner sphere.
	render_inner_sphere_depth_range(renderer, depth_restriction);

	// Visitor to render the depth ranges of the volume fill region.
	VolumeFillBoundaryGeometryOnSphereVisitor volume_fill_boundary_visitor(
			renderer,
			d_streaming_vertex_element_buffer,
			d_streaming_vertex_buffer,
			d_volume_fill_boundary_vertex_array,
			include_polylines);

	//
	// Render the wall depth ranges.
	//

	renderer.gl_bind_program_object(d_render_volume_fill_wall_depth_range_program_object.get());

	// Start rendering *wall* geometries.
	volume_fill_boundary_visitor.begin_rendering();

	// Render the surface geometries (polylines/polygons) from which the volume fill boundary is generated.
	for (surface_polygons_mask_seq_type::const_iterator surface_geoms_iter = surface_polygons_mask.begin();
		surface_geoms_iter != surface_polygons_mask.end();
		++surface_geoms_iter)
	{
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &surface_geometry = *surface_geoms_iter;

		surface_geometry->accept_visitor(volume_fill_boundary_visitor);
	}

	// Finish rendering wall geometries.
	volume_fill_boundary_visitor.end_rendering();

#if 0 // Not currently used...
	//
	// Second, render the *spherical cap* depth ranges.
	//

	renderer.gl_bind_program_object(d_render_volume_fill_spherical_cap_depth_range_program_object.get());

	// Start rendering *spherical cap* geometries.
	volume_fill_boundary_visitor.begin_rendering();

	// Render the surface geometries (polylines/polygons) from which the volume fill boundary is generated.
	for (surface_geometry_seq_type::const_iterator surface_geoms_iter = surface_polygons_mask.begin();
		surface_geoms_iter != surface_polygons_mask.end();
		++surface_geoms_iter)
	{
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &surface_geometry = *surface_geoms_iter;

		surface_geometry->accept_visitor(volume_fill_boundary_visitor);
	}

	// Finish rendering *spherical cap* geometries.
	volume_fill_boundary_visitor.end_rendering();
#endif

	// Finished rendering to the screen render target.
	volume_fill_depth_range_screen_render_target_scope.end_render();

	return true;
}


bool
GPlatesOpenGL::GLScalarField3D::render_volume_fill_wall_surface_normal_and_depth(
		GLRenderer &renderer,
		const surface_polygons_mask_seq_type &surface_polygons_mask,
		bool include_polylines,
		bool only_show_boundary_walls,
		const GLTexture::shared_ptr_to_const_type &surface_fill_mask_texture,
		const GPlatesViewOperations::ScalarField3DRenderParameters::DepthRestriction &depth_restriction,
		boost::optional<GLScreenRenderTarget::shared_ptr_type> &volume_fill_walls_screen_render_target)
{
	// Make sure we leave the OpenGL state the way it was.
	// NOTE: We're not reseting to the default OpenGL state because we want to use the current
	// GL_MODELVIEW and GL_PROJECTION matrices as well as the current viewport.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// We should always have valid shader programs but test just in case.
	if (!d_render_volume_fill_wall_surface_normals_program_object)
	{
		return false;
	}

	// Set the depth restricted minimum and maximum depth radius of the scalar field.
	d_render_volume_fill_wall_surface_normals_program_object.get()->gl_uniform2f(
			renderer,
			"render_min_max_depth_radius_restriction",
			depth_restriction.min_depth_radius_restriction,
			depth_restriction.max_depth_radius_restriction);

	// Set surface fill mask sampler to texture unit 0.
	renderer.gl_bind_texture(surface_fill_mask_texture, GL_TEXTURE0, GL_TEXTURE_2D_ARRAY_EXT);
	d_render_volume_fill_wall_surface_normals_program_object.get()->gl_uniform1i(
			renderer,
			"surface_fill_mask_sampler",
			0);

	// Set the surface fill mask (square) texture resolution.
	// This is a texture array containing square textures (width == height).
	d_render_volume_fill_wall_surface_normals_program_object.get()->gl_uniform1i(
			renderer,
			"surface_fill_mask_resolution",
			surface_fill_mask_texture->get_width().get());

	// Set flag to show only boundary walls.
	d_render_volume_fill_wall_surface_normals_program_object.get()->gl_uniform1i(
			renderer,
			"only_show_boundary_walls",
			only_show_boundary_walls);

	// We need a depth buffer when rendering the walls, otherwise we are not
	// guaranteed to get the closest wall at each screen pixel.
	// Normally an 8-bit texture is enough to store surface normals.
	// However we're also storing screen-space depth in the alpha channel and that requires
	// more precision so we'll make the entire RGBA floating-point.
	volume_fill_walls_screen_render_target =
			renderer.get_context().get_non_shared_state()->acquire_screen_render_target(
					renderer,
					GL_RGBA32F_ARB/*texture_internalformat*/,
					true/*include_depth_buffer*/,
					false/*include_stencil_buffer*/);

	// We've already checked for screen render target support in 'is_supported()' so this shouldn't fail.
	// If it does then return false to ignore request to render walls of volume fill region.
	if (!volume_fill_walls_screen_render_target)
	{
		return false;
	}

	// Bind the volume fill boundary vertex array.
	d_volume_fill_boundary_vertex_array->gl_bind(renderer);

	// The viewport of the screen we're rendering to.
	const GLViewport screen_viewport = renderer.gl_get_viewport();

	// Begin rendering to the walls render target.
	GLScreenRenderTarget::RenderScope volume_fill_walls_screen_render_target_scope(
			*volume_fill_walls_screen_render_target.get(),
			renderer,
			screen_viewport.width(),
			screen_viewport.height());

	// Set the new viewport in case the current viewport has non-zero x and y offsets which happens
	// when the main scene is rendered as overlapping tiles (for rendering very large images).
	// It's also important that, later when accessing the screen render texture, the NDC
	// coordinates (-1,-1) and (1,1) map to the corners of the screen render texture.
	renderer.gl_viewport(0, 0, screen_viewport.width(), screen_viewport.height());
	// Also change the scissor rectangle in case scissoring is enabled.
	renderer.gl_scissor(0, 0, screen_viewport.width(), screen_viewport.height());

	// Disable alpha-blending/testing.
	renderer.gl_enable(GL_BLEND, GL_FALSE);
	renderer.gl_enable(GL_ALPHA_TEST, GL_FALSE);

	// Enable depth testing and depth writes.
	// NOTE: Depth writes must also be enabled for depth clears to work (same for colour buffers).
	renderer.gl_enable(GL_DEPTH_TEST);
	renderer.gl_depth_mask(GL_TRUE);

	// Clear colour and depth buffers in render target.
	//
	// We also clear the stencil buffer in case it is used - also it's usually interleaved
	// with depth so it's more efficient to clear both depth and stencil.
	//
	// Note that this clears the depth render buffer attached to the framebuffer object
	// in the GLScreenRenderTarget (not the main framebuffer).
	// The colour buffer stores normals as (signed) floating-point.
	// An alpha value of 1.0 signifies (to the isosurface shader) that a wall is not present.
	// This is the screen-space (normalised device coordinates) depth (not window coordinates)
	// in the range [-1, 1] and 1 corresponds to the far clip plane.
	renderer.gl_clear_color(0.0f, 0.0f, 0.0f, 1.0f);
	renderer.gl_clear_depth(); // Clear depth to 1.0
	renderer.gl_clear_stencil();
	renderer.gl_clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	// Disable colour writes of the inner white sphere.
	// We just want to write the inner sphere depth values into the depth buffer so that
	// walls behind the inner sphere do not overwrite our default colour buffer values.
	renderer.gl_color_mask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	// First render the inner sphere.
	render_white_inner_sphere(renderer, depth_restriction);

	// Re-enable colour writes.
	renderer.gl_color_mask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	// Visitor to render the walls of the volume fill region.
	VolumeFillBoundaryGeometryOnSphereVisitor volume_fill_walls_visitor(
			renderer,
			d_streaming_vertex_element_buffer,
			d_streaming_vertex_buffer,
			d_volume_fill_boundary_vertex_array,
			include_polylines);

	//
	// Render the walls.
	//

	renderer.gl_bind_program_object(d_render_volume_fill_wall_surface_normals_program_object.get());

	// Start rendering *wall* geometries.
	volume_fill_walls_visitor.begin_rendering();

	// Render the surface geometries (polylines/polygons) from which the volume fill boundary is generated.
	for (surface_polygons_mask_seq_type::const_iterator surface_geoms_iter = surface_polygons_mask.begin();
		surface_geoms_iter != surface_polygons_mask.end();
		++surface_geoms_iter)
	{
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &surface_geometry = *surface_geoms_iter;

		surface_geometry->accept_visitor(volume_fill_walls_visitor);
	}

	// Finish rendering *wall* geometries.
	volume_fill_walls_visitor.end_rendering();

	// Finished rendering to the screen render target.
	volume_fill_walls_screen_render_target_scope.end_render();

	return true;
}


void
GPlatesOpenGL::GLScalarField3D::render_cross_sections(
		GLRenderer &renderer,
		cache_handle_type &cache_handle,
		const cross_sections_seq_type &cross_sections,
		GPlatesViewOperations::ScalarField3DRenderParameters::CrossSectionColourMode colour_mode,
		const GPlatesViewOperations::ScalarField3DRenderParameters::DepthRestriction &depth_restriction,
		const std::vector<float> &test_variables,
		boost::optional<SurfaceFillMask> surface_fill_mask,
		boost::optional<GLTexture::shared_ptr_to_const_type> surface_occlusion_texture)
{
	// Make sure we leave the OpenGL state the way it was.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// First render the white inner sphere.
	// This is not needed when rendering an isosurface because the isosurface ray-tracer does that.
	render_white_inner_sphere(renderer, depth_restriction);

	// We should always have a valid shader program but test just in case.
	if (!d_render_cross_section_program_object)
	{
		return;
	}

	// Bind the shader program for rendering cross-sections.
	renderer.gl_bind_program_object(d_render_cross_section_program_object.get());

	unsigned int current_texture_unit = 0;

	// Set shader variables common to all shaders (currently iso-surface and cross-sections).
	set_iso_surface_and_cross_sections_shader_common_variables(
			renderer,
			d_render_cross_section_program_object.get(),
			current_texture_unit,
			depth_restriction,
			test_variables,
			surface_occlusion_texture);

	// Specify the colour mode.
	d_render_cross_section_program_object.get()->gl_uniform1i(
			renderer,
			"colour_mode_scalar",
			colour_mode == GPlatesViewOperations::ScalarField3DRenderParameters::CROSS_SECTION_COLOUR_MODE_SCALAR);
	d_render_cross_section_program_object.get()->gl_uniform1i(
			renderer,
			"colour_mode_gradient",
			colour_mode == GPlatesViewOperations::ScalarField3DRenderParameters::CROSS_SECTION_COLOUR_MODE_GRADIENT);

	// Set the min/max range of values used to map to colour whether that mapping is a look up
	// of the colour palette (eg, colouring by scalar value or gradient magnitude) or by using
	// a hard-wired mapping in the shader code.
	// Currently there's only palette look ups for cross sections.
	GLfloat min_colour_mapping_range = 0;
	GLfloat max_colour_mapping_range = 0;
	switch (colour_mode)
	{
	case GPlatesViewOperations::ScalarField3DRenderParameters::CROSS_SECTION_COLOUR_MODE_SCALAR:
		min_colour_mapping_range = d_colour_palette_value_range.first/*min*/;
		max_colour_mapping_range = d_colour_palette_value_range.second/*max*/;
		break;
	case GPlatesViewOperations::ScalarField3DRenderParameters::CROSS_SECTION_COLOUR_MODE_GRADIENT:
		min_colour_mapping_range = d_colour_palette_value_range.first/*min*/;
		max_colour_mapping_range = d_colour_palette_value_range.second/*max*/;
		break;
	default:
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
		break;
	}
	d_render_cross_section_program_object.get()->gl_uniform2f(
			renderer,
			"min_max_colour_mapping_range",
			min_colour_mapping_range,
			max_colour_mapping_range);

	// Surface fill mask texture defining surface fill area on surface of globe.
	GLTexture::shared_ptr_to_const_type surface_fill_mask_texture;
	// First generate the surface fill mask from the surface geometries if requested.
	// The returned texture array was temporarily acquired (from GLContext) and will be returned
	// when GLRenderer has finished using it, ie, when it is no longer bound to a texture slot
	// ('gl_bind_texture()' keeps the binding until it's unbound or bound to another texture).
	if (surface_fill_mask &&
		render_surface_fill_mask(
				renderer,
				surface_fill_mask->surface_polygons_mask,
				surface_fill_mask->treat_polylines_as_polygons,
				surface_fill_mask_texture))
	{
		// Set surface fill mask sampler to current texture unit.
		renderer.gl_bind_texture(surface_fill_mask_texture, GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_2D_ARRAY_EXT);
		d_render_cross_section_program_object.get()->gl_uniform1i(
				renderer,
				"surface_fill_mask_sampler",
				current_texture_unit);
		// Move to the next texture unit.
		++current_texture_unit;

		// Set the surface fill mask (square) texture resolution.
		// This is a texture array containing square textures (width == height).
		d_render_cross_section_program_object.get()->gl_uniform1i(
				renderer,
				"surface_fill_mask_resolution",
				surface_fill_mask_texture->get_width().get());

		// Enable reads from surface fill mask.
		d_render_cross_section_program_object.get()->gl_uniform1i(
				renderer,
				"using_surface_fill_mask",
				true);
	}
	else
	{
		// Unbind the surface fill mask sampler from current texture unit.
		renderer.gl_unbind_texture(GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_2D_ARRAY_EXT);
		// NOTE: Set the shader sampler to the current texture unit instead of a used texture unit
		// like unit 0. This avoids shader program validation failure when active shader samplers of
		// different types reference the same texture unit. Currently happens on MacOS - probably
		// because shader compiler does not detect that the sampler is not used and keeps it active.
		d_render_cross_section_program_object.get()->gl_uniform1i(
				renderer,
				"surface_fill_mask_sampler",
				current_texture_unit);
		// Move to the next texture unit.
		++current_texture_unit;

		// Disable reads from surface fill mask.
		d_render_cross_section_program_object.get()->gl_uniform1i(
				renderer,
				"using_surface_fill_mask",
				false);
	}

	// Bind the cross-section vertex array.
	d_cross_section_vertex_array->gl_bind(renderer);

	// Line anti-aliasing shouldn't be on, but turn it off to be sure.
	renderer.gl_enable(GL_LINE_SMOOTH, false);
	// Ensure lines are single-pixel wide.
	renderer.gl_line_width(1);

	// Surface points/multi-points are vertically extruded to create 1D lines.
	render_cross_sections_1d(
			renderer,
			d_streaming_vertex_element_buffer,
			d_streaming_vertex_buffer,
			d_cross_section_vertex_array,
			cross_sections);

	// Surface polylines/polygons are vertically extruded to create 2D triangular meshes.
	render_cross_sections_2d(
			renderer,
			d_streaming_vertex_element_buffer,
			d_streaming_vertex_buffer,
			d_cross_section_vertex_array,
			cross_sections);
}


void
GPlatesOpenGL::GLScalarField3D::render_cross_sections_1d(
		GLRenderer &renderer,
		const GLVertexElementBuffer::shared_ptr_type &streaming_vertex_element_buffer,
		const GLVertexBuffer::shared_ptr_type &streaming_vertex_buffer,
		const GLVertexArray::shared_ptr_type &cross_section_vertex_array,
		const cross_sections_seq_type &cross_sections)
{
	// Visitor to render 1D cross-section geometries.
	CrossSection1DGeometryOnSphereVisitor cross_section_1d_visitor(
			renderer,
			streaming_vertex_element_buffer,
			streaming_vertex_buffer,
			cross_section_vertex_array);

	// Start rendering.
	cross_section_1d_visitor.begin_rendering();

	// Render the surface geometries (points/multi-points) that form 1D cross-sections.
	for (cross_sections_seq_type::const_iterator cross_sections_iter = cross_sections.begin();
		cross_sections_iter != cross_sections.end();
		++cross_sections_iter)
	{
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &cross_section = *cross_sections_iter;

		cross_section->accept_visitor(cross_section_1d_visitor);
	}

	// Finish rendering.
	cross_section_1d_visitor.end_rendering();
}


void
GPlatesOpenGL::GLScalarField3D::render_cross_sections_2d(
		GLRenderer &renderer,
		const GLVertexElementBuffer::shared_ptr_type &streaming_vertex_element_buffer,
		const GLVertexBuffer::shared_ptr_type &streaming_vertex_buffer,
		const GLVertexArray::shared_ptr_type &cross_section_vertex_array,
		const cross_sections_seq_type &cross_sections)
{
	// Visitor to render 2D cross-section geometries.
	CrossSection2DGeometryOnSphereVisitor cross_section_2d_visitor(
			renderer,
			streaming_vertex_element_buffer,
			streaming_vertex_buffer,
			cross_section_vertex_array);

	// Start rendering.
	cross_section_2d_visitor.begin_rendering();

	// Render the surface geometries (polylines/polygons) that form 2D cross-sections.
	for (cross_sections_seq_type::const_iterator cross_sections_iter = cross_sections.begin();
		cross_sections_iter != cross_sections.end();
		++cross_sections_iter)
	{
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &cross_section = *cross_sections_iter;

		cross_section->accept_visitor(cross_section_2d_visitor);
	}

	// Finish rendering.
	cross_section_2d_visitor.end_rendering();
}


void
GPlatesOpenGL::GLScalarField3D::initialise_inner_sphere(
		GLRenderer &renderer)
{
	//
	// Created a compiled draw state that renders the white inner sphere.
	//

	// We'll stream vertices/indices into a std::vector.
	std::vector<GLColourVertex> vertices;
	std::vector<GLuint> vertex_elements;

	// Build the mesh vertices/indices.
	const unsigned int recursion_depth_to_generate_mesh = 4;
	SphereMeshBuilder sphere_mesh_builder(
			vertices,
			vertex_elements,
			GPlatesGui::Colour::to_rgba8(GPlatesGui::Colour::get_white()),
			recursion_depth_to_generate_mesh);
	GPlatesMaths::HierarchicalTriangularMeshTraversal htm;
	const unsigned int current_recursion_depth = 0;
	htm.visit(sphere_mesh_builder, current_recursion_depth);

	// All streamed triangle primitives end up as indexed triangles.
	d_white_inner_sphere_compiled_draw_state =
			compile_vertex_array_draw_state(
					renderer,
					*d_white_inner_sphere_vertex_array,
					vertices,
					vertex_elements,
					GL_TRIANGLES);

	d_render_white_inner_sphere_program_object =
			create_shader_program(
					renderer,
					SPHERE_VERTEX_SHADER,
					SPHERE_FRAGMENT_SHADER,
					boost::none,
					"#define WHITE_WITH_LIGHTING\n");

	// Note: If failed to create shader program then we just won't render the white inner sphere.

	d_render_depth_range_inner_sphere_program_object =
			create_shader_program(
					renderer,
					SPHERE_VERTEX_SHADER,
					SPHERE_FRAGMENT_SHADER,
					boost::none,
					"#define DEPTH_RANGE\n");

	// Note: If failed to create shader program then we just won't render the inner sphere depth range.
}


void
GPlatesOpenGL::GLScalarField3D::render_white_inner_sphere(
		GLRenderer &renderer,
		const GPlatesViewOperations::ScalarField3DRenderParameters::DepthRestriction &depth_restriction)
{
	// Make sure we leave the OpenGL state the way it was.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// We should always have a valid shader program and compiled draw state, but test just in case.
	if (!d_render_white_inner_sphere_program_object ||
		!d_white_inner_sphere_compiled_draw_state)
	{
		return;
	}

	renderer.gl_bind_program_object(d_render_white_inner_sphere_program_object.get());

	// Set depth radius of sphere to the minimum depth restricted radius.
	d_render_white_inner_sphere_program_object.get()->gl_uniform1f(
			renderer,
			"depth_radius",
			depth_restriction.min_depth_radius_restriction);

	// Set boolean flag if lighting is enabled.
	d_render_white_inner_sphere_program_object.get()->gl_uniform1i(
			renderer,
			"lighting_enabled",
			d_light->get_scene_lighting_parameters().is_lighting_enabled(
					GPlatesGui::SceneLightingParameters::LIGHTING_SCALAR_FIELD));

	// Set the world-space light direction.
	d_render_white_inner_sphere_program_object.get()->gl_uniform3f(
			renderer,
			"world_space_light_direction",
			d_light->get_globe_view_light_direction(renderer));

	// Set the light ambient contribution.
	d_render_white_inner_sphere_program_object.get()->gl_uniform1f(
			renderer,
			"light_ambient_contribution",
			d_light->get_scene_lighting_parameters().get_ambient_light_contribution());

	// This binds and renders the vertex array.
	renderer.apply_compiled_draw_state(*d_white_inner_sphere_compiled_draw_state.get());
}


void
GPlatesOpenGL::GLScalarField3D::render_inner_sphere_depth_range(
		GLRenderer &renderer,
		const GPlatesViewOperations::ScalarField3DRenderParameters::DepthRestriction &depth_restriction)
{
	// Make sure we leave the OpenGL state the way it was.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// We should always have a valid shader program and compiled draw state, but test just in case.
	if (!d_render_depth_range_inner_sphere_program_object ||
		!d_white_inner_sphere_compiled_draw_state)
	{
		return;
	}

	renderer.gl_bind_program_object(d_render_depth_range_inner_sphere_program_object.get());

	// Set depth radius of sphere to the minimum depth restricted radius.
	d_render_depth_range_inner_sphere_program_object.get()->gl_uniform1f(
			renderer,
			"depth_radius",
			depth_restriction.min_depth_radius_restriction);

	// This binds and renders the vertex array.
	renderer.apply_compiled_draw_state(*d_white_inner_sphere_compiled_draw_state.get());
}


void
GPlatesOpenGL::GLScalarField3D::set_iso_surface_and_cross_sections_shader_common_variables(
		GLRenderer &renderer,
		const GLProgramObject::shared_ptr_type &program_object,
		unsigned int &current_texture_unit,
		const GPlatesViewOperations::ScalarField3DRenderParameters::DepthRestriction &depth_restriction,
		const std::vector<float> &test_variables,
		boost::optional<GLTexture::shared_ptr_to_const_type> surface_occlusion_texture)
{
	// Set the test variables.
	set_shader_test_variables(renderer, program_object, test_variables);

	// Set tile metadata texture sampler to current texture unit.
	renderer.gl_bind_texture(d_tile_meta_data_texture_array, GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_2D_ARRAY_EXT);
	program_object->gl_uniform1i(
			renderer,
			"tile_meta_data_sampler",
			current_texture_unit);
	// Move to the next texture unit.
	++current_texture_unit;

	// Set field data texture sampler to current texture unit.
	renderer.gl_bind_texture(d_field_data_texture_array, GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_2D_ARRAY_EXT);
	program_object->gl_uniform1i(
			renderer,
			"field_data_sampler",
			current_texture_unit);
	// Move to the next texture unit.
	++current_texture_unit;

	// Set mask data texture sampler to current texture unit.
	renderer.gl_bind_texture(d_mask_data_texture_array, GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_2D_ARRAY_EXT);
	program_object->gl_uniform1i(
			renderer,
			"mask_data_sampler",
			current_texture_unit);
	// Move to the next texture unit.
	++current_texture_unit;

	// Set 1D depth radius to layer texture sampler to current texture unit.
	renderer.gl_bind_texture(d_depth_radius_to_layer_texture, GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_1D);
	program_object->gl_uniform1i(
			renderer,
			"depth_radius_to_layer_sampler",
			current_texture_unit);
	// Move to the next texture unit.
	++current_texture_unit;

	// Set 1D depth radius to layer texture sampler to current texture unit.
	renderer.gl_bind_texture(d_colour_palette_texture, GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_1D);
	program_object->gl_uniform1i(
			renderer,
			"colour_palette_sampler",
			current_texture_unit);
	// Move to the next texture unit.
	++current_texture_unit;

	if (surface_occlusion_texture)
	{
		// Set surface occlusion texture sampler to current texture unit.
		renderer.gl_bind_texture(surface_occlusion_texture.get(), GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_2D);
		program_object->gl_uniform1i(
				renderer,
				"surface_occlusion_texture_sampler",
				current_texture_unit);
		// Move to the next texture unit.
		++current_texture_unit;

		// Enable reads from surface occlusion texture.
		program_object->gl_uniform1i(
				renderer,
				"read_from_surface_occlusion_texture",
				true);
	}
	else
	{
		// Unbind the surface occlusion texture sampler from current texture unit.
		renderer.gl_unbind_texture(GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_2D);
		// NOTE: Set the shader sampler to the current texture unit instead of a used texture unit
		// like unit 0. This avoids shader program validation failure when active shader samplers of
		// different types reference the same texture unit. Currently happens on MacOS - probably
		// because shader compiler does not detect that the sampler is not used and keeps it active.
		d_render_iso_surface_program_object.get()->gl_uniform1i(
				renderer,
				"surface_occlusion_texture_sampler",
				current_texture_unit);
		// Move to the next texture unit.
		++current_texture_unit;

		// Disable reads from surface occlusion texture.
		program_object->gl_uniform1i(
				renderer,
				"read_from_surface_occlusion_texture",
				false);
	}

	// Set the tile metadata resolution.
	program_object->gl_uniform1i(
			renderer,
			"tile_meta_data_resolution",
			d_tile_meta_data_resolution);

	// Set the tile resolution.
	program_object->gl_uniform1i(
			renderer,
			"tile_resolution",
			d_tile_resolution);

	// Set the 1D texture depth-radius-to-layer resolution.
	program_object->gl_uniform1i(
			renderer,
			"depth_radius_to_layer_resolution",
			d_depth_radius_to_layer_texture->get_width().get());

	// Set the 1D texture colour palette resolution.
	program_object->gl_uniform1i(
			renderer,
			"colour_palette_resolution",
			d_colour_palette_texture->get_width().get());

	// Set the scalar field min/max depth radius.
	program_object->gl_uniform2f(
			renderer,
			"min_max_depth_radius",
			d_min_depth_layer_radius,
			d_max_depth_layer_radius);

	// Set the depth restricted min/max depth radius.
	program_object->gl_uniform2f(
			renderer,
			"render_min_max_depth_radius_restriction",
			depth_restriction.min_depth_radius_restriction,
			depth_restriction.max_depth_radius_restriction);
// 	qDebug() << "render_min_max_depth_radius_restriction: "
// 		<< depth_restriction.min_depth_radius_restriction << ", "
// 		<< depth_restriction.max_depth_radius_restriction;

	// Set the number of depth layers.
	program_object->gl_uniform1i(
			renderer,
			"num_depth_layers",
			d_num_depth_layers);

	// Set the min/max scalar value.
	// Note: It might not currently be used so only set if active in program object to avoid warning.
	if (program_object->is_active_uniform("min_max_scalar_value"))
	{
		program_object->gl_uniform2f(
				renderer,
				"min_max_scalar_value",
				d_scalar_min,
				d_scalar_max);
	}

	// Set the min/max gradient magnitude.
	// Note: It might not currently be used so only set if active in program object to avoid warning.
	if (program_object->is_active_uniform("min_max_gradient_magnitude"))
	{
		program_object->gl_uniform2f(
				renderer,
				"min_max_gradient_magnitude",
				d_gradient_magnitude_min,
				d_gradient_magnitude_max);
	}

	// Set boolean flag if lighting is enabled.
	program_object->gl_uniform1i(
			renderer,
			"lighting_enabled",
			d_light->get_scene_lighting_parameters().is_lighting_enabled(
					GPlatesGui::SceneLightingParameters::LIGHTING_SCALAR_FIELD));

	// Set the world-space light direction.
	program_object->gl_uniform3f(
			renderer,
			"world_space_light_direction",
			d_light->get_globe_view_light_direction(renderer));

	// Set the light ambient contribution.
	program_object->gl_uniform1f(
			renderer,
			"light_ambient_contribution",
			d_light->get_scene_lighting_parameters().get_ambient_light_contribution());
}


void
GPlatesOpenGL::GLScalarField3D::set_shader_test_variables(
		GLRenderer &renderer,
		const GLProgramObject::shared_ptr_type &program_object,
		const std::vector<float> &test_variables)
{
	for (unsigned int variable_index = 0; variable_index < test_variables.size(); ++variable_index)
	{
		const QString variable_name = QString("test_variable_%1").arg(variable_index);

		// Set the shader test variable.
		// If the variable doesn't exist in the shader program or is not used then
		// a warning is emitted (but can be ignored).
		//
		// Not all test variables are necessarily used by the shader program.
		if (program_object->is_active_uniform(variable_name.toAscii().constData()))
		{
			program_object->gl_uniform1f(
					renderer,
					variable_name.toAscii().constData(),
					test_variables[variable_index]);
		}
	}
}



void
GPlatesOpenGL::GLScalarField3D::allocate_streaming_vertex_buffers(
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
}


void
GPlatesOpenGL::GLScalarField3D::initialise_cross_section_rendering(
		GLRenderer &renderer)
{
	d_render_cross_section_program_object =
			create_shader_program(
					renderer,
					CROSS_SECTION_VERTEX_SHADER_SOURCE_FILE_NAME,
					CROSS_SECTION_FRAGMENT_SHADER_SOURCE_FILE_NAME);

	//
	// Initialise the vertex array for rendering cross-section geometry.
	//
	// WARNING - For these vertex bindings to take effect the shader program must be re-linked *afterwards*.
	//

	if (d_render_cross_section_program_object)
	{
		// Attach vertex element buffer to the vertex array.
		d_cross_section_vertex_array->set_vertex_element_buffer(
				renderer,
				d_streaming_vertex_element_buffer);

		// The "surface_point_xyz_depth_weight_w" attribute data is the surface point
		// packed in (x,y,z) and the depth weight into 'w' of a 'vec4' vertex attribute.
		d_render_cross_section_program_object.get()->gl_bind_attrib_location(
				"surface_point_xyz_depth_weight_w", 0/*attribute_index*/);
		d_cross_section_vertex_array->set_enable_vertex_attrib_array(
				renderer, 0/*attribute_index*/, true/*enable*/);
		d_cross_section_vertex_array->set_vertex_attrib_pointer(
				renderer,
				d_streaming_vertex_buffer,
				0/*attribute_index*/,
				4,
				GL_FLOAT,
				GL_FALSE/*normalized*/,
				sizeof(CrossSectionVertex),
				0/*offset*/);

		// The "neighbour_surface_point_xyz_normal_weight_w" attribute data is the neighbour surface point
		// packed in (x,y,z) and the normal weight into 'w' of a 'vec4' vertex attribute.
		d_render_cross_section_program_object.get()->gl_bind_attrib_location(
				"neighbour_surface_point_xyz_normal_weight_w", 1/*attribute_index*/);
		d_cross_section_vertex_array->set_enable_vertex_attrib_array(
				renderer, 1/*attribute_index*/, true/*enable*/);
		d_cross_section_vertex_array->set_vertex_attrib_pointer(
				renderer,
				d_streaming_vertex_buffer,
				1/*attribute_index*/,
				4,
				GL_FLOAT,
				GL_FALSE/*normalized*/,
				sizeof(CrossSectionVertex),
				4 * sizeof(GLfloat)/*offset*/);
	}

	//
	// Re-link the cross-section shader program.
	//
	// WARNING: Vertex array bindings and program parameters should be set *before* this and
	// program uniform variables should be set *after*.
	//

	if (d_render_cross_section_program_object)
	{
		// Now that we've changed the attribute bindings in the program object we need to
		// re-link it in order for them to take effect.
		if (!d_render_cross_section_program_object.get()->gl_link_program(renderer))
		{
			d_render_cross_section_program_object = boost::none;
		}
	}

	//
	// Initialise shader uniform variables (for the common scalar field utils shader) that don't change.
	//
	// WARNING - If the shader program is subsequently re-linked then the uniform variables will need updating.
	//

	if (d_render_cross_section_program_object)
	{
		initialise_shader_utils(renderer, d_render_cross_section_program_object.get());
	}
}


void
GPlatesOpenGL::GLScalarField3D::initialise_iso_surface_rendering(
		GLRenderer &renderer)
{
	//
	// Create the shader programs.
	//

	d_render_iso_surface_program_object =
			create_shader_program(
					renderer,
					ISO_SURFACE_VERTEX_SHADER_SOURCE_FILE_NAME,
					ISO_SURFACE_FRAGMENT_SHADER_SOURCE_FILE_NAME);

	//
	// Initialise shader uniform variables (for the common scalar field utils shader) that don't change.
	//
	// WARNING - If the shader program is subsequently re-linked then the uniform variables will need updating.
	//

	if (d_render_iso_surface_program_object)
	{
		initialise_shader_utils(renderer, d_render_iso_surface_program_object.get());
	}
}


void
GPlatesOpenGL::GLScalarField3D::initialise_surface_fill_mask_rendering(
		GLRenderer &renderer)
{
	const GLCapabilities &capabilities = renderer.get_capabilities();

	//
	// Initialise the surface fill mask texture resolution.
	//

	d_surface_fill_mask_resolution = SURFACE_FILL_MASK_RESOLUTION;

	// It can't be larger than the maximum texture dimension for the current system.
	if (d_surface_fill_mask_resolution > capabilities.texture.gl_max_texture_size)
	{
		d_surface_fill_mask_resolution = capabilities.texture.gl_max_texture_size;
	}

	//
	// Create the shader programs.
	//

	d_render_surface_fill_mask_program_object =
			create_shader_program(
					renderer,
					SURFACE_FILL_MASK_VERTEX_SHADER_SOURCE_FILE_NAME,
					SURFACE_FILL_MASK_FRAGMENT_SHADER_SOURCE_FILE_NAME,
					std::make_pair(
							SURFACE_FILL_MASK_GEOMETRY_SHADER_SOURCE_FILE_NAME,
							GLShaderProgramUtils::GeometryShaderProgramParameters(
									SURFACE_FILL_MASK_GEOMETRY_SHADER_MAX_OUTPUT_VERTICES,
									GL_TRIANGLES,
									GL_TRIANGLE_STRIP)));

	//
	// Initialise the vertex array for rendering surface fill mask.
	//
	// WARNING - For these vertex bindings to take effect the shader program must be re-linked *afterwards*.
	//

	if (d_render_surface_fill_mask_program_object)
	{
		// Attach vertex element buffer to the vertex array.
		d_surface_fill_mask_vertex_array->set_vertex_element_buffer(
				renderer,
				d_streaming_vertex_element_buffer);

		// The "surface_point" attribute data is the surface point packed in (x,y,z) components of 'vec4' vertex attribute.
		d_render_surface_fill_mask_program_object.get()->gl_bind_attrib_location(
				"surface_point", 0/*attribute_index*/);
		d_surface_fill_mask_vertex_array->set_enable_vertex_attrib_array(
				renderer, 0/*attribute_index*/, true/*enable*/);
		d_surface_fill_mask_vertex_array->set_vertex_attrib_pointer(
				renderer,
				d_streaming_vertex_buffer,
				0/*attribute_index*/,
				3,
				GL_FLOAT,
				GL_FALSE/*normalized*/,
				sizeof(SurfaceFillMaskVertex),
				0/*offset*/);
	}

	//
	// Re-link the surface fill mask shader program.
	//
	// WARNING: Vertex array bindings and program parameters should be set *before* this and
	// program uniform variables should be set *after*.
	//

	if (d_render_surface_fill_mask_program_object)
	{
		// Now that we've changed the attribute bindings in the program object we need to
		// re-link it in order for them to take effect.
		if (!d_render_surface_fill_mask_program_object.get()->gl_link_program(renderer))
		{
			d_render_surface_fill_mask_program_object = boost::none;
		}
	}

	//
	// Initialise some shader program uniform variables for rendering surface fill mask.
	//
	// WARNING - If the shader program is subsequently re-linked then the uniform variables will need updating.
	//

	if (d_render_surface_fill_mask_program_object)
	{
		std::vector<GLMatrix> cube_face_view_projection_matrices;

		// Our cube map subdivision with a (one-and-a)-half-texel overlap at the border to avoid texture seams.
		//
		// NOTE: We expand by 1.5 texels instead of the normal 0.5 texels.
		// This is because we want the centre of the next-to-border texels to map to the edge
		// of a cube face frustum (instead of the centre of border texels).
		// This enables the iso-surface shader program to sample in a 3x3 texel pattern and not
		// have any sample texture coordinates get clamped (which could cause issues at cube face
		// edges). The 3x3 sample pattern is used to emulate a pre-processing dilation of the texture.
		GLCubeSubdivision::non_null_ptr_to_const_type cube_subdivision =
				GLCubeSubdivision::create(
						GLCubeSubdivision::get_expand_frustum_ratio(
								d_surface_fill_mask_resolution,
								1.5/* half a texel */));

		// Set up the view-projection matrices for rendering into the six faces of the cube.
		for (unsigned int face = 0; face < 6; ++face)
		{
			const GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face =
					static_cast<GPlatesMaths::CubeCoordinateFrame::CubeFaceType>(face);

			// The view matrix for the current cube face.
			const GLTransform::non_null_ptr_to_const_type view_transform =
					cube_subdivision->get_view_transform(cube_face);

			// Get the projection transforms of an entire cube face.
			// We use the lowest resolution level-of-detail since we're rendering to the entire cube face.
			const GLTransform::non_null_ptr_to_const_type projection_transform =
					cube_subdivision->get_projection_transform(
							0/*level_of_detail*/,
							0/*tile_u_offset*/,
							0/*tile_v_offset*/);

			// Multiply the view and projection matrices.
			GLMatrix view_projection_matrix = projection_transform->get_matrix();
			view_projection_matrix.gl_mult_matrix(view_transform->get_matrix());

			cube_face_view_projection_matrices.push_back(view_projection_matrix);
		}

		// Set the view-projection matrices in the shader program.
		// They never change so we just set them once here.
		//
		// WARNING - If the shader program is subsequently re-linked then the uniform variables will
		// need updating.
		d_render_surface_fill_mask_program_object.get()->gl_uniform_matrix4x4f(
				renderer,
				"cube_face_view_projection_matrices",
				cube_face_view_projection_matrices);
	}
}


void
GPlatesOpenGL::GLScalarField3D::initialise_volume_fill_boundary_rendering(
		GLRenderer &renderer)
{
	//
	// Create the shader programs.
	//

#if 0 // Not currently used...
	d_render_volume_fill_spherical_cap_depth_range_program_object =
			create_shader_program(
					renderer,
					VOLUME_FILL_VERTEX_SHADER_SOURCE_FILE_NAME,
					VOLUME_FILL_SPHERICAL_CAP_FRAGMENT_SHADER_SOURCE_FILE_NAME,
					std::make_pair(
							VOLUME_FILL_SPHERICAL_CAP_GEOMETRY_SHADER_SOURCE_FILE_NAME,
							GLShaderProgramUtils::GeometryShaderProgramParameters(
									VOLUME_FILL_SPHERICAL_CAP_GEOMETRY_SHADER_MAX_OUTPUT_VERTICES,
									GL_LINES,
									GL_TRIANGLE_STRIP)),
					"#define DEPTH_RANGE\n");
#endif

	d_render_volume_fill_wall_depth_range_program_object =
			create_shader_program(
					renderer,
					VOLUME_FILL_VERTEX_SHADER_SOURCE_FILE_NAME,
					VOLUME_FILL_WALL_FRAGMENT_SHADER_SOURCE_FILE_NAME,
					std::make_pair(
							VOLUME_FILL_WALL_GEOMETRY_SHADER_SOURCE_FILE_NAME,
							GLShaderProgramUtils::GeometryShaderProgramParameters(
									VOLUME_FILL_WALL_GEOMETRY_SHADER_MAX_OUTPUT_VERTICES,
									GL_LINES,
									GL_TRIANGLE_STRIP)),
					"#define DEPTH_RANGE\n");

	d_render_volume_fill_wall_surface_normals_program_object =
			create_shader_program(
					renderer,
					VOLUME_FILL_VERTEX_SHADER_SOURCE_FILE_NAME,
					VOLUME_FILL_WALL_FRAGMENT_SHADER_SOURCE_FILE_NAME,
					std::make_pair(
							VOLUME_FILL_WALL_GEOMETRY_SHADER_SOURCE_FILE_NAME,
							GLShaderProgramUtils::GeometryShaderProgramParameters(
									VOLUME_FILL_WALL_GEOMETRY_SHADER_MAX_OUTPUT_VERTICES,
									GL_LINES,
									GL_TRIANGLE_STRIP)),
					"#define SURFACE_NORMALS_AND_DEPTH\n");

	//
	// Initialise the vertex array for rendering volume fill boundary.
	//
	// WARNING - For these vertex bindings to take effect the shader program must be re-linked *afterwards*.
	//

	// Attach vertex element buffer to the vertex array.
	d_volume_fill_boundary_vertex_array->set_vertex_element_buffer(
			renderer,
			d_streaming_vertex_element_buffer);

	// The "surface_point" attribute data is the surface point packed in (x,y,z) components of 'vec4' vertex attribute.
	d_volume_fill_boundary_vertex_array->set_enable_vertex_attrib_array(
			renderer, 0/*attribute_index*/, true/*enable*/);
	d_volume_fill_boundary_vertex_array->set_vertex_attrib_pointer(
			renderer,
			d_streaming_vertex_buffer,
			0/*attribute_index*/,
			3,
			GL_FLOAT,
			GL_FALSE/*normalized*/,
			sizeof(VolumeFillBoundaryVertex),
			0/*offset*/);

	// The "centroid_point" attribute data is a surface point packed in (x,y,z) components of 'vec4' vertex attribute.
	d_volume_fill_boundary_vertex_array->set_enable_vertex_attrib_array(
			renderer, 1/*attribute_index*/, true/*enable*/);
	d_volume_fill_boundary_vertex_array->set_vertex_attrib_pointer(
			renderer,
			d_streaming_vertex_buffer,
			1/*attribute_index*/,
			3,
			GL_FLOAT,
			GL_FALSE/*normalized*/,
			sizeof(VolumeFillBoundaryVertex),
			3 * sizeof(GLfloat)/*offset*/);

#if 0 // Not currently used...
	if (d_render_volume_fill_spherical_cap_depth_range_program_object)
	{
		d_render_volume_fill_spherical_cap_depth_range_program_object.get()->gl_bind_attrib_location(
				"surface_point", 0/*attribute_index*/);
		d_render_volume_fill_spherical_cap_depth_range_program_object.get()->gl_bind_attrib_location(
				"centroid_point", 1/*attribute_index*/);
	}
#endif
	if (d_render_volume_fill_wall_depth_range_program_object)
	{
		d_render_volume_fill_wall_depth_range_program_object.get()->gl_bind_attrib_location(
				"surface_point", 0/*attribute_index*/);
		d_render_volume_fill_wall_depth_range_program_object.get()->gl_bind_attrib_location(
				"centroid_point", 1/*attribute_index*/);
	}
	if (d_render_volume_fill_wall_surface_normals_program_object)
	{
		d_render_volume_fill_wall_surface_normals_program_object.get()->gl_bind_attrib_location(
				"surface_point", 0/*attribute_index*/);
		d_render_volume_fill_wall_surface_normals_program_object.get()->gl_bind_attrib_location(
				"centroid_point", 1/*attribute_index*/);
	}

	//
	// Re-link the volume fill boundary shader programs.
	//
	// WARNING: Vertex array bindings and program parameters should be set *before* this and
	// program uniform variables should be set *after*.
	//

#if 0 // Not currently used...
	if (d_render_volume_fill_spherical_cap_depth_range_program_object)
	{
		// Now that we've changed the attribute bindings in the program object we need to
		// re-link it in order for them to take effect.
		if (!d_render_volume_fill_spherical_cap_depth_range_program_object.get()->gl_link_program(renderer))
		{
			d_render_volume_fill_spherical_cap_depth_range_program_object = boost::none;
		}
	}
#endif
	if (d_render_volume_fill_wall_depth_range_program_object)
	{
		// Now that we've changed the attribute bindings in the program object we need to
		// re-link it in order for them to take effect.
		if (!d_render_volume_fill_wall_depth_range_program_object.get()->gl_link_program(renderer))
		{
			d_render_volume_fill_wall_depth_range_program_object = boost::none;
		}
	}
	if (d_render_volume_fill_wall_surface_normals_program_object)
	{
		// Now that we've changed the attribute bindings in the program object we need to
		// re-link it in order for them to take effect.
		if (!d_render_volume_fill_wall_surface_normals_program_object.get()->gl_link_program(renderer))
		{
			d_render_volume_fill_wall_surface_normals_program_object = boost::none;
		}
	}

	//
	// Initialise shader uniform variables (for the common scalar field utils shader) that don't change.
	//
	// WARNING - If the shader program is subsequently re-linked then the uniform variables will need updating.
	//

#if 0 // Not currently used...
	if (d_render_volume_fill_spherical_cap_depth_range_program_object)
	{
		initialise_shader_utils(renderer, d_render_volume_fill_spherical_cap_depth_range_program_object.get());
	}
#endif
	if (d_render_volume_fill_wall_depth_range_program_object)
	{
		initialise_shader_utils(renderer, d_render_volume_fill_wall_depth_range_program_object.get());
	}
	if (d_render_volume_fill_wall_surface_normals_program_object)
	{
		initialise_shader_utils(renderer, d_render_volume_fill_wall_surface_normals_program_object.get());
	}
}


void
GPlatesOpenGL::GLScalarField3D::initialise_shader_utils(
		GLRenderer &renderer,
		const GLProgramObject::shared_ptr_type &program_object)
{
	// Set the local coordinate frames of each cube face.
	// The order of faces is the same as the GPlatesMaths::CubeCoordinateFrame::CubeFaceType enumeration.
	//
	// WARNING - If the shader program is subsequently re-linked then the uniform variables will
	// need updating.

	// The six faces of the cube.
	for (unsigned int face = 0; face < 6; ++face)
	{
		const GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face =
				static_cast<GPlatesMaths::CubeCoordinateFrame::CubeFaceType>(face);

		// The three coordinate axes.
		for (unsigned int axis = 0; axis < 3; ++axis)
		{
			const GPlatesMaths::CubeCoordinateFrame::CubeFaceCoordinateFrameAxis cube_axis =
					static_cast<GPlatesMaths::CubeCoordinateFrame::CubeFaceCoordinateFrameAxis>(axis);

			const GPlatesMaths::UnitVector3D &cube_face_axis =
					GPlatesMaths::CubeCoordinateFrame::get_cube_face_coordinate_frame_axis(
							cube_face, cube_axis);

			const QString uniform_name = QString("cube_face_coordinate_frames[%1].%2_axis")
					.arg(face)
					.arg(axis == 0 ? 'x' : (axis == 1 ? 'y' : 'z'));

			// Not all shader programs use the cube coordinate frames.
			if (program_object->is_active_uniform(uniform_name.toAscii().constData()))
			{
				program_object->gl_uniform3f(
						renderer,
						uniform_name.toAscii().constData(),
						cube_face_axis);
			}
		}
	}
}


boost::optional<GPlatesOpenGL::GLProgramObject::shared_ptr_type>
GPlatesOpenGL::GLScalarField3D::create_shader_program(
		GLRenderer &renderer,
		const QString &vertex_shader_source_file_name,
		const QString &fragment_shader_source_file_name,
		boost::optional< std::pair<QString, GLShaderProgramUtils::GeometryShaderProgramParameters> > geometry_shader,
		const char *shader_defines)
{
	// Vertex shader source.
	GLShaderSource vertex_shader_source(SHADER_VERSION);
	// Add the '#define' statements first.
	vertex_shader_source.add_code_segment(shader_defines);
	// Then add the general utilities.
	vertex_shader_source.add_code_segment_from_file(GLShaderProgramUtils::UTILS_SHADER_SOURCE_FILE_NAME);
	// Then add the scalar field utilities.
	vertex_shader_source.add_code_segment_from_file(SCALAR_FIELD_UTILS_SOURCE_FILE_NAME);
	// Then add the GLSL 'main()' function.
	vertex_shader_source.add_code_segment_from_file(vertex_shader_source_file_name);

	GLShaderSource fragment_shader_source(SHADER_VERSION);
	// Add the '#define' statements first.
	fragment_shader_source.add_code_segment(shader_defines);
	// Then add the general utilities.
	fragment_shader_source.add_code_segment_from_file(GLShaderProgramUtils::UTILS_SHADER_SOURCE_FILE_NAME);
	// Then add the scalar field utilities.
	fragment_shader_source.add_code_segment_from_file(SCALAR_FIELD_UTILS_SOURCE_FILE_NAME);
	// Then add the GLSL 'main()' function.
	fragment_shader_source.add_code_segment_from_file(fragment_shader_source_file_name);

	boost::optional<GLProgramObject::shared_ptr_type> program_object;

	if (geometry_shader)
	{
		const QString geometry_shader_source_file_name = geometry_shader->first;
		const GLShaderProgramUtils::GeometryShaderProgramParameters &
				geometry_shader_program_parameters = geometry_shader->second;

		// Geometry shader source.
		GLShaderSource geometry_shader_source(SHADER_VERSION);
		// Add the '#define' statements first.
		geometry_shader_source.add_code_segment(shader_defines);
		// Then add the general utilities.
		geometry_shader_source.add_code_segment_from_file(GLShaderProgramUtils::UTILS_SHADER_SOURCE_FILE_NAME);
		// Then add the scalar field utilities.
		geometry_shader_source.add_code_segment_from_file(SCALAR_FIELD_UTILS_SOURCE_FILE_NAME);
		// Then add the GLSL 'main()' function.
		geometry_shader_source.add_code_segment_from_file(geometry_shader_source_file_name);

		// Compile and link the vertex/geometry/fragment shader program.
		program_object = GLShaderProgramUtils::compile_and_link_vertex_geometry_fragment_program(
				renderer,
				vertex_shader_source,
				geometry_shader_source,
				fragment_shader_source,
				geometry_shader_program_parameters);
	}
	else
	{
		// Compile and link the vertex/fragment shader program.
		program_object = GLShaderProgramUtils::compile_and_link_vertex_fragment_program(
				renderer,
				vertex_shader_source,
				fragment_shader_source);
	}

	return program_object;
}


void
GPlatesOpenGL::GLScalarField3D::create_tile_meta_data_texture_array(
		GLRenderer &renderer)
{
	const GLCapabilities &capabilities = renderer.get_capabilities();

	// Using nearest-neighbour filtering since don't want to filter pixel metadata.
	d_tile_meta_data_texture_array->gl_tex_parameteri(
			renderer, GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	d_tile_meta_data_texture_array->gl_tex_parameteri(
			renderer, GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Clamp texture coordinates to centre of edge texels.
	// Not strictly necessary for nearest-neighbour filtering.
	if (capabilities.texture.gl_EXT_texture_edge_clamp ||
		capabilities.texture.gl_SGIS_texture_edge_clamp)
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
	const GLCapabilities &capabilities = renderer.get_capabilities();

	// Using linear filtering.
	// We've tested for support for filtering of floating-point textures in 'is_supported()'.
	d_field_data_texture_array->gl_tex_parameteri(
			renderer, GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	d_field_data_texture_array->gl_tex_parameteri(
			renderer, GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Clamp texture coordinates to centre of edge texels.
	if (capabilities.texture.gl_EXT_texture_edge_clamp ||
		capabilities.texture.gl_SGIS_texture_edge_clamp)
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
	const GLCapabilities &capabilities = renderer.get_capabilities();

	// Using linear filtering.
	// We've tested for support for filtering of floating-point textures in 'is_supported()'.
	d_mask_data_texture_array->gl_tex_parameteri(
			renderer, GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	d_mask_data_texture_array->gl_tex_parameteri(
			renderer, GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Clamp texture coordinates to centre of edge texels.
	if (capabilities.texture.gl_EXT_texture_edge_clamp ||
		capabilities.texture.gl_SGIS_texture_edge_clamp)
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
	const GLCapabilities &capabilities = renderer.get_capabilities();

	// Using linear filtering.
	// We've tested for support for filtering of floating-point textures in 'is_supported()'.
	d_depth_radius_to_layer_texture->gl_tex_parameteri(
			renderer, GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	d_depth_radius_to_layer_texture->gl_tex_parameteri(
			renderer, GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Clamp texture coordinates to centre of edge texels.
	if (capabilities.texture.gl_EXT_texture_edge_clamp ||
		capabilities.texture.gl_SGIS_texture_edge_clamp)
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
	if (depth_radius_to_layer_resolution > capabilities.texture.gl_max_texture_size)
	{
		depth_radius_to_layer_resolution = capabilities.texture.gl_max_texture_size;
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
	const GLCapabilities &capabilities = renderer.get_capabilities();

	// Using linear filtering.
	// We've tested for support for filtering of floating-point textures in 'is_supported()'.
	d_colour_palette_texture->gl_tex_parameteri(
			renderer, GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	d_colour_palette_texture->gl_tex_parameteri(
			renderer, GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Clamp texture coordinates to centre of edge texels.
	if (capabilities.texture.gl_EXT_texture_edge_clamp ||
		capabilities.texture.gl_SGIS_texture_edge_clamp)
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
	if (colour_palette_resolution > capabilities.texture.gl_max_texture_size)
	{
		colour_palette_resolution = capabilities.texture.gl_max_texture_size;
	}

	d_colour_palette_texture->gl_tex_image_1D(
			renderer, GL_TEXTURE_1D, 0, GL_RGBA32F_ARB,
			colour_palette_resolution,
			0, GL_RGBA, GL_FLOAT, NULL);

	// Check there are no OpenGL errors.
	GLUtils::assert_no_gl_errors(GPLATES_ASSERTION_SOURCE);
}


GPlatesOpenGL::GLTexture::shared_ptr_to_const_type
GPlatesOpenGL::GLScalarField3D::acquire_surface_fill_mask_texture(
		GLRenderer &renderer)
{
	const GLCapabilities &capabilities = renderer.get_capabilities();

	const unsigned int texture_depth = 6;

	// Acquire an RGBA8 texture.
	const GLTexture::shared_ptr_type surface_fill_mask_texture =
			renderer.get_context().get_shared_state()->acquire_texture(
					renderer,
					GL_TEXTURE_2D_ARRAY_EXT,
					GL_RGBA8,
					d_surface_fill_mask_resolution,
					d_surface_fill_mask_resolution,
					texture_depth);

	// 'acquire_texture' initialises the texture memory (to empty) but does not set the filtering
	// state when it creates a new texture.
	// Also it might have been used by another client that specified different filtering settings for it.
	// So we set the filtering settings each time we acquire.

	// Linear filtering.
	surface_fill_mask_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	surface_fill_mask_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// Turn off any anisotropic filtering - we don't need it.
	// Besides, it anisotropic filtering needs explicit gradients in GLSL code for texture accesses in non-uniform flow.
	if (capabilities.texture.gl_EXT_texture_filter_anisotropic)
	{
		surface_fill_mask_texture->gl_tex_parameterf(renderer, GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);
	}


	// Clamp texture coordinates to centre of edge texels -
	// it's easier for hardware to implement - and doesn't affect our calculations.
	if (capabilities.texture.gl_EXT_texture_edge_clamp ||
		capabilities.texture.gl_SGIS_texture_edge_clamp)
	{
		surface_fill_mask_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		surface_fill_mask_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	else
	{
		surface_fill_mask_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_WRAP_S, GL_CLAMP);
		surface_fill_mask_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_WRAP_T, GL_CLAMP);
	}

	return surface_fill_mask_texture;
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

	//
	// NOTE: We assume that the depth layer radii increase in radius through the depth layer sequence.
	//

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
		GLRenderer &renderer,
		const GPlatesGui::ColourPalette<double>::non_null_ptr_to_const_type &colour_palette,
		const std::pair<double, double> &colour_palette_value_range)
{
	// The colours for the colour palette texture.
	std::vector<GPlatesGui::Colour> colour_palette_texels;

	// Flags to indicate which texels, if any, are fully transparent.
	std::vector<bool> transparent_texels;
	bool any_transparent_texels = false;

	// Number of texels in the colour palette texture.
	const unsigned int colour_palette_resolution = d_colour_palette_texture->get_width().get();

	// Iterate over texels.
	unsigned int texel;
	for (texel = 0; texel < colour_palette_resolution; ++texel)
	{
		// Map the current texel to the colour palette input value range.
		const double colour_palette_value = colour_palette_value_range.first +
				(colour_palette_value_range.second - colour_palette_value_range.first) *
					double(texel) / (colour_palette_resolution - 1);

		// Map the colour palette input value range to the texture.
		boost::optional<GPlatesGui::Colour> colour = colour_palette->get_colour(colour_palette_value);

		// The colour palette should normally return a valid colour for any input value since
		// it's usually either:
		//   (1) Read from a CPT file which should have 'B', 'F' and 'N' colour entries for
		//       background, foreground and NaN colours (where background is used for values below
		//       the minimum, foreground for above and NaN for any gaps between the colour slices), or
		//   (2) Generated from a default set of colours (with background, foreground and NaN set).
		//
		// If any of background, foreground or NaN are not set then it's possible to have no colour.
		// In this situation we'll use transparency (alpha = 0) to avoid drawing those values.
		if (!colour)
		{
			colour = GPlatesGui::Colour(0, 0, 0, 0);
			transparent_texels.push_back(true);
			any_transparent_texels = true;
		}
		else
		{
			transparent_texels.push_back(false);
		}

		colour_palette_texels.push_back(colour.get());
	}

	// If there are any transparent texels then we need to avoid linear blending artifacts
	// between a transparent texel and its neighbouring opaque texel in the 1D texture.
	// This artifact manifests as a darkening of the pixel colour due to blending with the
	// transparent texel's black RGB colour.
	// In vertical cross-sections this is visible as a darkening at the boundaries of opaque regions.
	//
	// For example, if sampling halfway between transparent and opaque texel then the RGB colour
	// would be:
	//   RGB = 0.5 * OpaqueRGB + 0.5 * TransparentRGB = 0.5 * OpaqueRGB
	// ...combined with the linearly interpolated alpha of:
	//   Alpha = 0.5 * OpaqueAlpha + 0.5 * TransparentAlpha = 0.5 * OpaqueAlpha
	// ...we get 0.25 * OpaqueAlpha * OpaqueRGB when instead we want 0.5 * OpaqueAlpha * OpaqueRGB.
	//
	// This is achieved by dilating the texture by one texel - that is each transparent texel that
	// is next to an opaque texel will use the RGB colour of that opaque texel (but not its alpha).
	// So now we get:
	//   RGB = 0.5 * OpaqueRGB + 0.5 * OpaqueRGB = OpaqueRGB
	// ...combined with the linearly interpolated alpha of:
	//   Alpha = 0.5 * OpaqueAlpha + 0.5 * TransparentAlpha = 0.5 * OpaqueAlpha
	// ...we get 0.5 * OpaqueAlpha * OpaqueRGB.
	if (any_transparent_texels)
	{
		for (texel = 0; texel < colour_palette_resolution; ++texel)
		{
			// Skip opaque texels.
			if (!transparent_texels[texel])
			{
				continue;
			}

			// Get the previous opaque texel if, any.
			boost::optional<GPlatesGui::Colour> prev_opaque_texel;
			if (texel > 0 &&
				!transparent_texels[texel - 1])
			{
				prev_opaque_texel = colour_palette_texels[texel - 1];
			}

			// Get the next opaque texel if, any.
			boost::optional<GPlatesGui::Colour> next_opaque_texel;
			if (texel < colour_palette_resolution - 1 &&
				!transparent_texels[texel + 1])
			{
				next_opaque_texel = colour_palette_texels[texel + 1];
			}

			// If the current transparent texel has no opaque neighbours then it will not cause
			// linear interpolation artifacts and hence can be left with a black RGB.
			if (!prev_opaque_texel && !next_opaque_texel)
			{
				continue;
			}

			boost::optional<GPlatesGui::Colour> opaque_texel;
			if (prev_opaque_texel && next_opaque_texel)
			{
				// Both neighbouring texels are opaque so just average their colours.
				opaque_texel =
						GPlatesGui::Colour::linearly_interpolate(
								prev_opaque_texel.get(),
								next_opaque_texel.get(), 0.5);
			}
			else if (prev_opaque_texel)
			{
				opaque_texel = prev_opaque_texel.get();
			}
			else // next_opaque_texel ...
			{
				opaque_texel = next_opaque_texel.get();
			}

			// Use the neighbouring opaque texel(s) RGB colour but keep this texel transparent.
			colour_palette_texels[texel] = GPlatesGui::Colour(
					opaque_texel->red(),
					opaque_texel->green(),
					opaque_texel->blue(),
					0/*alpha*/);
		}
	}

	// The RGBA texels of the colour palette data.
	std::vector<GLfloat> colour_palette_data;
	colour_palette_data.reserve(4/*RGBA*/ * colour_palette_texels.size());

	// Convert colours to RGBA float data.
	for (texel = 0; texel < colour_palette_resolution; ++texel)
	{
		const GPlatesGui::Colour &texel_colour = colour_palette_texels[texel];

		colour_palette_data.push_back(texel_colour.red());
		colour_palette_data.push_back(texel_colour.green());
		colour_palette_data.push_back(texel_colour.blue());
		colour_palette_data.push_back(texel_colour.alpha());
	}

	// Upload the colour palette data into the texture.
	d_colour_palette_texture->gl_tex_sub_image_1D(
			renderer, GL_TEXTURE_1D, 0,
			0, // x offset
			colour_palette_resolution, // width
			GL_RGBA, GL_FLOAT, &colour_palette_data[0]);
}


GPlatesOpenGL::GLScalarField3D::SurfaceFillMask::SurfaceFillMask(
		const surface_polygons_mask_seq_type &surface_polygons_mask_,
		bool treat_polylines_as_polygons_,
		boost::optional<ShowWalls> show_walls_) :
	surface_polygons_mask(surface_polygons_mask_),
	treat_polylines_as_polygons(treat_polylines_as_polygons_),
	show_walls(show_walls_)
{
}


GPlatesOpenGL::GLScalarField3D::CrossSection1DGeometryOnSphereVisitor::CrossSection1DGeometryOnSphereVisitor(
		GLRenderer &renderer,
		const GLVertexElementBuffer::shared_ptr_type &streaming_vertex_element_buffer,
		const GLVertexBuffer::shared_ptr_type &streaming_vertex_buffer,
		const GLVertexArray::shared_ptr_type &vertex_array) :
	d_renderer(renderer),
	d_vertex_array(vertex_array),
	d_map_vertex_element_buffer_scope(
			renderer,
			*streaming_vertex_element_buffer->get_buffer(),
			GLBuffer::TARGET_ELEMENT_ARRAY_BUFFER),
	d_map_vertex_buffer_scope(
			renderer,
			*streaming_vertex_buffer->get_buffer(),
			GLBuffer::TARGET_ARRAY_BUFFER),
	d_stream_target(d_stream),
	d_stream_primitives(d_stream)
{
}


void
GPlatesOpenGL::GLScalarField3D::CrossSection1DGeometryOnSphereVisitor::begin_rendering()
{
	// Start streaming cross-section 1D geometries.
	begin_vertex_array_streaming<CrossSectionVertex, streaming_vertex_element_type>(
			d_renderer,
			d_stream_target,
			d_map_vertex_element_buffer_scope,
			MINIMUM_BYTES_TO_STREAM_IN_VERTEX_ELEMENT_BUFFER,
			d_map_vertex_buffer_scope,
			MINIMUM_BYTES_TO_STREAM_IN_VERTEX_BUFFER);
}


void
GPlatesOpenGL::GLScalarField3D::CrossSection1DGeometryOnSphereVisitor::end_rendering()
{
	// Stop streaming cross-section 1D geometries so we can render the last batch.
	end_vertex_array_streaming<CrossSectionVertex, streaming_vertex_element_type>(
			d_renderer,
			d_stream_target,
			d_map_vertex_element_buffer_scope,
			d_map_vertex_buffer_scope);

	// Render the last batch of streamed cross-section 1D geometries (if any).
	render_vertex_array_stream<CrossSectionVertex, streaming_vertex_element_type>(
			d_renderer,
			d_stream_target,
			d_vertex_array,
			GL_LINES);
}


void
GPlatesOpenGL::GLScalarField3D::CrossSection1DGeometryOnSphereVisitor::visit_multi_point_on_sphere(
		GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere)
{
	GPlatesMaths::MultiPointOnSphere::const_iterator point_iter = multi_point_on_sphere->begin();
	GPlatesMaths::MultiPointOnSphere::const_iterator point_end = multi_point_on_sphere->end();
	for ( ; point_iter != point_end; ++point_iter)
	{
		render_cross_section_1d(point_iter->position_vector());
	}
}


void
GPlatesOpenGL::GLScalarField3D::CrossSection1DGeometryOnSphereVisitor::visit_point_on_sphere(
		GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere)
{
	render_cross_section_1d(point_on_sphere->position_vector());
}


void
GPlatesOpenGL::GLScalarField3D::CrossSection1DGeometryOnSphereVisitor::render_cross_section_1d(
		const GPlatesMaths::UnitVector3D &surface_point)
{
	// There are two vertices for the current line.
	// Each surface point is vertically extruded to form a line.
	if (!d_stream_primitives.begin_primitive(2/*max_num_vertices*/, 2/*max_num_vertex_elements*/))
	{
		// There's not enough vertices or indices so render what we have so far and
		// obtain new stream buffers.
		suspend_render_resume_vertex_array_streaming<CrossSectionVertex, streaming_vertex_element_type>(
				d_renderer,
				d_stream_target,
				d_map_vertex_element_buffer_scope,
				MINIMUM_BYTES_TO_STREAM_IN_VERTEX_ELEMENT_BUFFER,
				d_map_vertex_buffer_scope,
				MINIMUM_BYTES_TO_STREAM_IN_VERTEX_BUFFER,
				d_vertex_array,
				GL_LINES);

		d_stream_primitives.begin_primitive(2/*max_num_vertices*/, 2/*max_num_vertex_elements*/);
	}

	CrossSectionVertex vertex;

	// Cross-section normal is not calculated for 1D cross-sections.
	vertex.normal_weight = 0;
	vertex.neighbour_surface_point[0] = 0;
	vertex.neighbour_surface_point[1] = 0;
	vertex.neighbour_surface_point[2] = 0;

	// Both minimum and maximum depth radius vertices have the same surface position.
	vertex.surface_point[0] = surface_point.x().dval();
	vertex.surface_point[1] = surface_point.y().dval();
	vertex.surface_point[2] = surface_point.z().dval();

	// The minimum depth radius vertex.
	vertex.depth_weight = 0;

	d_stream_primitives.add_vertex(vertex);
	d_stream_primitives.add_vertex_element(0);

	// The maximum depth radius vertex.
	vertex.depth_weight = 1;

	d_stream_primitives.add_vertex(vertex);
	d_stream_primitives.add_vertex_element(1);

	d_stream_primitives.end_primitive();
}


GPlatesOpenGL::GLScalarField3D::CrossSection2DGeometryOnSphereVisitor::CrossSection2DGeometryOnSphereVisitor(
		GLRenderer &renderer,
		const GLVertexElementBuffer::shared_ptr_type &streaming_vertex_element_buffer,
		const GLVertexBuffer::shared_ptr_type &streaming_vertex_buffer,
		const GLVertexArray::shared_ptr_type &vertex_array) :
	d_renderer(renderer),
	d_vertex_array(vertex_array),
	d_map_vertex_element_buffer_scope(
			renderer,
			*streaming_vertex_element_buffer->get_buffer(),
			GLBuffer::TARGET_ELEMENT_ARRAY_BUFFER),
	d_map_vertex_buffer_scope(
			renderer,
			*streaming_vertex_buffer->get_buffer(),
			GLBuffer::TARGET_ARRAY_BUFFER),
	d_stream_target(d_stream),
	d_stream_primitives(d_stream)
{
}


void
GPlatesOpenGL::GLScalarField3D::CrossSection2DGeometryOnSphereVisitor::begin_rendering()
{
	// Start streaming cross-section 2D geometries.
	begin_vertex_array_streaming<CrossSectionVertex, streaming_vertex_element_type>(
			d_renderer,
			d_stream_target,
			d_map_vertex_element_buffer_scope,
			MINIMUM_BYTES_TO_STREAM_IN_VERTEX_ELEMENT_BUFFER,
			d_map_vertex_buffer_scope,
			MINIMUM_BYTES_TO_STREAM_IN_VERTEX_BUFFER);
}


void
GPlatesOpenGL::GLScalarField3D::CrossSection2DGeometryOnSphereVisitor::end_rendering()
{
	// Stop streaming cross-section 2D geometries so we can render the last batch.
	end_vertex_array_streaming<CrossSectionVertex, streaming_vertex_element_type>(
			d_renderer,
			d_stream_target,
			d_map_vertex_element_buffer_scope,
			d_map_vertex_buffer_scope);

	// Render the last batch of streamed cross-section 2D geometries (if any).
	render_vertex_array_stream<CrossSectionVertex, streaming_vertex_element_type>(
			d_renderer,
			d_stream_target,
			d_vertex_array,
			GL_TRIANGLES/*rendering each quad as two triangles*/);
}


void
GPlatesOpenGL::GLScalarField3D::CrossSection2DGeometryOnSphereVisitor::visit_polygon_on_sphere(
		GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
{
	render_cross_sections_2d(polygon_on_sphere->begin(), polygon_on_sphere->end());
}


void
GPlatesOpenGL::GLScalarField3D::CrossSection2DGeometryOnSphereVisitor::visit_polyline_on_sphere(
		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
{
	render_cross_sections_2d(polyline_on_sphere->begin(), polyline_on_sphere->end());
}


template <typename GreatCircleArcForwardIter>
void
GPlatesOpenGL::GLScalarField3D::CrossSection2DGeometryOnSphereVisitor::render_cross_sections_2d(
		GreatCircleArcForwardIter begin_arcs,
		GreatCircleArcForwardIter end_arcs)
{
	// Iterate over the great circle arcs and output a quad (two tris) per great circle arc.
	for (GreatCircleArcForwardIter gca_iter = begin_arcs ; gca_iter != end_arcs; ++gca_iter)
	{
		const GPlatesMaths::GreatCircleArc &gca = *gca_iter;

		// Tessellate the current arc if its two endpoints are far enough apart.
		if (gca.dot_of_endpoints() < COSINE_GREAT_CIRCLE_ARC_ANGULAR_THRESHOLD)
		{
			// Tessellate the current great circle arc.
			std::vector<GPlatesMaths::PointOnSphere> surface_points;
			tessellate(surface_points, gca, GREAT_CIRCLE_ARC_ANGULAR_THRESHOLD);

			// Add the tessellated sub-arcs.
			for (unsigned int n = 1; n < surface_points.size(); ++n)
			{
				const GPlatesMaths::UnitVector3D &start_surface_point = surface_points[n-1].position_vector();
				const GPlatesMaths::UnitVector3D &end_surface_point = surface_points[n].position_vector();

				render_cross_section_2d(start_surface_point, end_surface_point);
			}
		}
		else // no need to tessellate great circle arc...
		{
			const GPlatesMaths::UnitVector3D &start_surface_point = gca.start_point().position_vector();
			const GPlatesMaths::UnitVector3D &end_surface_point = gca.end_point().position_vector();

			render_cross_section_2d(start_surface_point, end_surface_point);
		}
	}
}


void
GPlatesOpenGL::GLScalarField3D::CrossSection2DGeometryOnSphereVisitor::render_cross_section_2d(
		const GPlatesMaths::UnitVector3D &start_surface_point,
		const GPlatesMaths::UnitVector3D &end_surface_point)
{
	// There are four vertices and two triangles (six indices) per great circle arc.
	// Each surface great circle arc is vertically extruded to form a 2D quad (two triangles).
	if (!d_stream_primitives.begin_primitive(4/*max_num_vertices*/, 6/*max_num_vertex_elements*/))
	{
		// There's not enough vertices or indices so render what we have so far and
		// obtain new stream buffers.
		suspend_render_resume_vertex_array_streaming<CrossSectionVertex, streaming_vertex_element_type>(
				d_renderer,
				d_stream_target,
				d_map_vertex_element_buffer_scope,
				MINIMUM_BYTES_TO_STREAM_IN_VERTEX_ELEMENT_BUFFER,
				d_map_vertex_buffer_scope,
				MINIMUM_BYTES_TO_STREAM_IN_VERTEX_BUFFER,
				d_vertex_array,
				GL_TRIANGLES/*rendering each quad as two triangles*/);

		d_stream_primitives.begin_primitive(4/*max_num_vertices*/, 6/*max_num_vertex_elements*/);
	}

	// The default for front-facing primitives is counter-clockwise - see glFrontFace (GLRenderer::gl_front_face).
	// So arrange the two quad triangles such that the surface normal calculated for the front face
	// (calculated in vertex shader) is the correct orientation (ie, not the negative normal of back face).

	CrossSectionVertex vertex;

	// Both minimum and maximum depth radius vertices have the same surface positions.
	vertex.surface_point[0] = start_surface_point.x().dval();
	vertex.surface_point[1] = start_surface_point.y().dval();
	vertex.surface_point[2] = start_surface_point.z().dval();
	vertex.neighbour_surface_point[0] = end_surface_point.x().dval();
	vertex.neighbour_surface_point[1] = end_surface_point.y().dval();
	vertex.neighbour_surface_point[2] = end_surface_point.z().dval();

	// Normal, calculated in vertex shader, is...
	//   cross(surface_point, neighbour_surface_point) =
	//   cross(start_surface_point, end_surface_point)
	// ...which faces backward so need to invert to face forward.
	vertex.normal_weight = -1.0;

	// The minimum depth radius vertex.
	vertex.depth_weight = 0;
	d_stream_primitives.add_vertex(vertex);
	// The maximum depth radius vertex.
	vertex.depth_weight = 1;
	d_stream_primitives.add_vertex(vertex);

	// Both minimum and maximum depth radius vertices have the same surface positions.
	vertex.surface_point[0] = end_surface_point.x().dval();
	vertex.surface_point[1] = end_surface_point.y().dval();
	vertex.surface_point[2] = end_surface_point.z().dval();
	vertex.neighbour_surface_point[0] = start_surface_point.x().dval();
	vertex.neighbour_surface_point[1] = start_surface_point.y().dval();
	vertex.neighbour_surface_point[2] = start_surface_point.z().dval();

	// Normal, calculated in vertex shader, is...
	//   cross(surface_point, neighbour_surface_point) =
	//   cross(end_surface_point, start_surface_point)
	// ...which faces forward so no need to invert it.
	vertex.normal_weight = 1.0;

	// The minimum depth radius vertex.
	vertex.depth_weight = 0;
	d_stream_primitives.add_vertex(vertex);
	// The maximum depth radius vertex.
	vertex.depth_weight = 1;
	d_stream_primitives.add_vertex(vertex);

	// Arrange triangles such that they have a counter-clockwise vertex ordering (ie, front face)
	// when the view position is on the positive side of face plane, ie, the half-space that
	// the surface normal, which is cross(start_surface_point, end_surface_point), is pointing to.

	// First triangle of quad.
	d_stream_primitives.add_vertex_element(2);
	d_stream_primitives.add_vertex_element(1);
	d_stream_primitives.add_vertex_element(0);
	// Second triangle of quad.
	d_stream_primitives.add_vertex_element(1);
	d_stream_primitives.add_vertex_element(2);
	d_stream_primitives.add_vertex_element(3);

	d_stream_primitives.end_primitive();
}


GPlatesOpenGL::GLScalarField3D::SurfaceFillMaskGeometryOnSphereVisitor::SurfaceFillMaskGeometryOnSphereVisitor(
		GLRenderer &renderer,
		const GLVertexElementBuffer::shared_ptr_type &streaming_vertex_element_buffer,
		const GLVertexBuffer::shared_ptr_type &streaming_vertex_buffer,
		const GLVertexArray::shared_ptr_type &vertex_array,
		bool include_polylines) :
	d_renderer(renderer),
	d_vertex_array(vertex_array),
	d_map_vertex_element_buffer_scope(
			renderer,
			*streaming_vertex_element_buffer->get_buffer(),
			GLBuffer::TARGET_ELEMENT_ARRAY_BUFFER),
	d_map_vertex_buffer_scope(
			renderer,
			*streaming_vertex_buffer->get_buffer(),
			GLBuffer::TARGET_ARRAY_BUFFER),
	d_stream_target(d_stream),
	d_stream_primitives(d_stream),
	d_include_polylines(include_polylines)
{
}


void
GPlatesOpenGL::GLScalarField3D::SurfaceFillMaskGeometryOnSphereVisitor::visit_polygon_on_sphere(
		GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
{
	render_surface_fill_mask(
			polygon_on_sphere->vertex_begin(),
			polygon_on_sphere->number_of_vertices(),
			polygon_on_sphere->get_centroid());
}


void
GPlatesOpenGL::GLScalarField3D::SurfaceFillMaskGeometryOnSphereVisitor::visit_polyline_on_sphere(
		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
{
	if (d_include_polylines)
	{
		render_surface_fill_mask(
				polyline_on_sphere->vertex_begin(),
				polyline_on_sphere->number_of_vertices(),
				polyline_on_sphere->get_centroid());
	}
}


template <typename PointOnSphereForwardIter>
void
GPlatesOpenGL::GLScalarField3D::SurfaceFillMaskGeometryOnSphereVisitor::render_surface_fill_mask(
		const PointOnSphereForwardIter begin_points,
		const unsigned int num_points,
		const GPlatesMaths::UnitVector3D &centroid)
{
	// This is an optimisation whereby if the entire geometry fits within the stream buffer
	// (which is usually the case) then the geometry does not need to be re-streamed for each
	// subsequent rendering and only a draw call needs to be issued.
	bool entire_geometry_is_in_stream_target = false;

	// First render the fill geometry with disabled color writes to the RGB channels.
	// This leaves the alpha-blending factors for the alpha channel to generate the (concave)
	// polygon fill mask in the alpha channel.
	d_renderer.gl_color_mask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);

	render_surface_fill_mask_geometry(
			begin_points,
			num_points,
			centroid,
			entire_geometry_is_in_stream_target);

	// Second render the fill geometry with disabled color writes to the Alpha channel.
	// This leaves the alpha-blending factors for the RGB channels to accumulate the
	// polygon fill mask (just rendered) from the alpha channel into the RGB channels.
	d_renderer.gl_color_mask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);

	render_surface_fill_mask_geometry(
			begin_points,
			num_points,
			centroid,
			entire_geometry_is_in_stream_target);

	// Third render the fill geometry with disabled color writes to the RGB channels again.
	// This effectively clears the alpha channel of the current polygon fill mask in preparation
	// for the next polygon fill mask.
	// The reason this clears is because the alpha-channel is set up to give 1 where a pixel is
	// covered by an odd number of triangles and 0 by an even number of triangles.
	// This second rendering results in all pixels being covered by an even number of triangles
	// (two times an odd or even number is an even number) resulting in 0 for all pixels (in alpha channel).
	d_renderer.gl_color_mask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);

	render_surface_fill_mask_geometry(
			begin_points,
			num_points,
			centroid,
			entire_geometry_is_in_stream_target);
}


template <typename PointOnSphereForwardIter>
void
GPlatesOpenGL::GLScalarField3D::SurfaceFillMaskGeometryOnSphereVisitor::render_surface_fill_mask_geometry(
		const PointOnSphereForwardIter begin_points,
		const unsigned int num_points,
		const GPlatesMaths::UnitVector3D &centroid,
		bool &entire_geometry_is_in_stream_target)
{
	// If the entire geometry is already in the stream then we only need to issue a draw call.
	if (entire_geometry_is_in_stream_target)
	{
		render_vertex_array_stream<SurfaceFillMaskVertex, streaming_vertex_element_type>(
				d_renderer,
				d_stream_target,
				d_vertex_array,
				GL_TRIANGLES);

		// Entire geometry is still in the stream buffer.
		return;
	}

	// Start streaming the current surface fill mask geometry.
	begin_vertex_array_streaming<SurfaceFillMaskVertex, streaming_vertex_element_type>(
			d_renderer,
			d_stream_target,
			d_map_vertex_element_buffer_scope,
			MINIMUM_BYTES_TO_STREAM_IN_VERTEX_ELEMENT_BUFFER,
			d_map_vertex_buffer_scope,
			MINIMUM_BYTES_TO_STREAM_IN_VERTEX_BUFFER);

	// See if there's enough space remaining in the streaming buffers to stream the entire geometry.
	if (d_stream_primitives.begin_primitive(
			num_points + 2/*max_num_vertices*/,
			3 * num_points/*max_num_vertex_elements*/))
	{
		//
		// Here we use the more efficient path of generating the triangle fan mesh ourselves.
		// The price we pay is having to be more explicit in how we submit the triangle fan.
		//

		// Vertex element relative to the beginning of the primitive (not beginning of buffer).
		streaming_vertex_element_type vertex_index = 0;

		SurfaceFillMaskVertex vertex;

		// The first vertex is the polygon centroid.
		vertex.surface_point[0] = centroid.x().dval();
		vertex.surface_point[1] = centroid.y().dval();
		vertex.surface_point[2] = centroid.z().dval();
		d_stream_primitives.add_vertex(vertex);
		++vertex_index;

		// The remaining vertices form the boundary.
		PointOnSphereForwardIter points_iter = begin_points;
		for (unsigned int n = 0; n < num_points; ++n, ++vertex_index, ++points_iter)
		{
			const GPlatesMaths::UnitVector3D &point_position = points_iter->position_vector();

			vertex.surface_point[0] = point_position.x().dval();
			vertex.surface_point[1] = point_position.y().dval();
			vertex.surface_point[2] = point_position.z().dval();
			d_stream_primitives.add_vertex(vertex);

			d_stream_primitives.add_vertex_element(0); // Centroid.
			d_stream_primitives.add_vertex_element(vertex_index); // Current boundary point.
			d_stream_primitives.add_vertex_element(vertex_index + 1); // Next boundary point.
		}

		// Wraparound back to the first boundary vertex to close off the polygon.
		const GPlatesMaths::UnitVector3D &first_point_position = begin_points->position_vector();
		vertex.surface_point[0] = first_point_position.x().dval();
		vertex.surface_point[1] = first_point_position.y().dval();
		vertex.surface_point[2] = first_point_position.z().dval();
		d_stream_primitives.add_vertex(vertex);

		d_stream_primitives.end_primitive();

		// The entire geometry is now in the stream buffer.
		entire_geometry_is_in_stream_target = true;
	}
	else // Not enough space remaining in streaming buffer for the entire geometry...
	{
		//
		// Here we use the less efficient path of rendering a triangle fan in order to have the
		// stream take of copying the fan apex vertex whenever the stream fills up mid-triangle-fan.
		// It also makes things easier by allowing us to simply add vertices.
		//

		// Render each polygon as a triangle fan with the fan apex being the polygon centroid.
		surface_fill_mask_stream_primitives_type::TriangleFans fill_stream_triangle_fans(d_stream);

		fill_stream_triangle_fans.begin_triangle_fan();

		SurfaceFillMaskVertex vertex;

		// The first vertex is the polygon centroid.
		vertex.surface_point[0] = centroid.x().dval();
		vertex.surface_point[1] = centroid.y().dval();
		vertex.surface_point[2] = centroid.z().dval();
		if (!fill_stream_triangle_fans.add_vertex(vertex))
		{
			suspend_render_resume_vertex_array_streaming<SurfaceFillMaskVertex, streaming_vertex_element_type>(
					d_renderer,
					d_stream_target,
					d_map_vertex_element_buffer_scope,
					MINIMUM_BYTES_TO_STREAM_IN_VERTEX_ELEMENT_BUFFER,
					d_map_vertex_buffer_scope,
					MINIMUM_BYTES_TO_STREAM_IN_VERTEX_BUFFER,
					d_vertex_array,
					GL_TRIANGLES);
			fill_stream_triangle_fans.add_vertex(vertex);
		}

		// The remaining vertices form the boundary.
		PointOnSphereForwardIter points_iter = begin_points;
		for (unsigned int n = 0; n < num_points; ++n, ++points_iter)
		{
			const GPlatesMaths::UnitVector3D &point_position = points_iter->position_vector();

			vertex.surface_point[0] = point_position.x().dval();
			vertex.surface_point[1] = point_position.y().dval();
			vertex.surface_point[2] = point_position.z().dval();
			if (!fill_stream_triangle_fans.add_vertex(vertex))
			{
				suspend_render_resume_vertex_array_streaming<SurfaceFillMaskVertex, streaming_vertex_element_type>(
						d_renderer,
						d_stream_target,
						d_map_vertex_element_buffer_scope,
						MINIMUM_BYTES_TO_STREAM_IN_VERTEX_ELEMENT_BUFFER,
						d_map_vertex_buffer_scope,
						MINIMUM_BYTES_TO_STREAM_IN_VERTEX_BUFFER,
						d_vertex_array,
						GL_TRIANGLES);
				fill_stream_triangle_fans.add_vertex(vertex);
			}
		}

		// Wraparound back to the first polygon vertex to close off the polygon.
		const GPlatesMaths::UnitVector3D &first_point_position = begin_points->position_vector();
		vertex.surface_point[0] = first_point_position.x().dval();
		vertex.surface_point[1] = first_point_position.y().dval();
		vertex.surface_point[2] = first_point_position.z().dval();
		if (!fill_stream_triangle_fans.add_vertex(vertex))
		{
			suspend_render_resume_vertex_array_streaming<SurfaceFillMaskVertex, streaming_vertex_element_type>(
					d_renderer,
					d_stream_target,
					d_map_vertex_element_buffer_scope,
					MINIMUM_BYTES_TO_STREAM_IN_VERTEX_ELEMENT_BUFFER,
					d_map_vertex_buffer_scope,
					MINIMUM_BYTES_TO_STREAM_IN_VERTEX_BUFFER,
					d_vertex_array,
					GL_TRIANGLES);
			fill_stream_triangle_fans.add_vertex(vertex);
		}

		fill_stream_triangle_fans.end_triangle_fan();
	}

	// Stop streaming the current surface fill mask geometry.
	end_vertex_array_streaming<SurfaceFillMaskVertex, streaming_vertex_element_type>(
			d_renderer,
			d_stream_target,
			d_map_vertex_element_buffer_scope,
			d_map_vertex_buffer_scope);

	// Render the current surface fill mask geometry.
	render_vertex_array_stream<SurfaceFillMaskVertex, streaming_vertex_element_type>(
			d_renderer,
			d_stream_target,
			d_vertex_array,
			GL_TRIANGLES);
}


GPlatesOpenGL::GLScalarField3D::VolumeFillBoundaryGeometryOnSphereVisitor::VolumeFillBoundaryGeometryOnSphereVisitor(
		GLRenderer &renderer,
		const GLVertexElementBuffer::shared_ptr_type &streaming_vertex_element_buffer,
		const GLVertexBuffer::shared_ptr_type &streaming_vertex_buffer,
		const GLVertexArray::shared_ptr_type &vertex_array,
		bool include_polylines) :
	d_renderer(renderer),
	d_vertex_array(vertex_array),
	d_map_vertex_element_buffer_scope(
			renderer,
			*streaming_vertex_element_buffer->get_buffer(),
			GLBuffer::TARGET_ELEMENT_ARRAY_BUFFER),
	d_map_vertex_buffer_scope(
			renderer,
			*streaming_vertex_buffer->get_buffer(),
			GLBuffer::TARGET_ARRAY_BUFFER),
	d_stream_target(d_stream),
	d_stream_primitives(d_stream),
	d_include_polylines(include_polylines)
{
}


void
GPlatesOpenGL::GLScalarField3D::VolumeFillBoundaryGeometryOnSphereVisitor::begin_rendering()
{
	// Start streaming volume fill boundary geometries.
	begin_vertex_array_streaming<VolumeFillBoundaryVertex, streaming_vertex_element_type>(
			d_renderer,
			d_stream_target,
			d_map_vertex_element_buffer_scope,
			MINIMUM_BYTES_TO_STREAM_IN_VERTEX_ELEMENT_BUFFER,
			d_map_vertex_buffer_scope,
			MINIMUM_BYTES_TO_STREAM_IN_VERTEX_BUFFER);
}


void
GPlatesOpenGL::GLScalarField3D::VolumeFillBoundaryGeometryOnSphereVisitor::end_rendering()
{
	// Stop streaming volume fill boundary geometries so we can render the last batch.
	end_vertex_array_streaming<VolumeFillBoundaryVertex, streaming_vertex_element_type>(
			d_renderer,
			d_stream_target,
			d_map_vertex_element_buffer_scope,
			d_map_vertex_buffer_scope);

	// Render the current contents of the stream.
	render_stream();
}


void
GPlatesOpenGL::GLScalarField3D::VolumeFillBoundaryGeometryOnSphereVisitor::render_stream()
{
	// Render a batch of streamed volume fill boundary geometries (if any).
	render_vertex_array_stream<VolumeFillBoundaryVertex, streaming_vertex_element_type>(
			d_renderer,
			d_stream_target,
			d_vertex_array,
			GL_LINES/*geometry shader converts lines to triangles*/);
}


void
GPlatesOpenGL::GLScalarField3D::VolumeFillBoundaryGeometryOnSphereVisitor::visit_polygon_on_sphere(
		GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
{
	render_volume_fill_boundary(
			polygon_on_sphere->begin(),
			polygon_on_sphere->end(),
			polygon_on_sphere->get_centroid());
}


void
GPlatesOpenGL::GLScalarField3D::VolumeFillBoundaryGeometryOnSphereVisitor::visit_polyline_on_sphere(
		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
{
	if (d_include_polylines)
	{
		render_volume_fill_boundary(
				polyline_on_sphere->begin(),
				polyline_on_sphere->end(),
				polyline_on_sphere->get_centroid());

		// Close off the polygon boundary using the last and first polyline points.
		const GPlatesMaths::GreatCircleArc last_to_first_gca[1] =
		{
			GPlatesMaths::GreatCircleArc::create(
					polyline_on_sphere->end_point(),
					polyline_on_sphere->start_point())
		};

		// Render the single great circle arc (as a sequence of arcs).
		render_volume_fill_boundary(
				last_to_first_gca,
				last_to_first_gca + 1,
				polyline_on_sphere->get_centroid());
	}
}


template <typename GreatCircleArcForwardIter>
void
GPlatesOpenGL::GLScalarField3D::VolumeFillBoundaryGeometryOnSphereVisitor::render_volume_fill_boundary(
		GreatCircleArcForwardIter begin_arcs,
		GreatCircleArcForwardIter end_arcs,
		const GPlatesMaths::UnitVector3D &centroid)
{
	// All vertices have the same centroid in common.
	// This is sent along with each vertex and used by the geometry shader to generate the spherical surface.
	VolumeFillBoundaryVertex vertex;
	vertex.centroid_point[0] = centroid.x().dval();
	vertex.centroid_point[1] = centroid.y().dval();
	vertex.centroid_point[2] = centroid.z().dval();

	// Iterate over the great circle arcs and output a quad (two tris) per great circle arc.
	for (GreatCircleArcForwardIter gca_iter = begin_arcs ; gca_iter != end_arcs; ++gca_iter)
	{
		const GPlatesMaths::GreatCircleArc &gca = *gca_iter;

		// Tessellate the current arc if its two endpoints are far enough apart.
		if (gca.dot_of_endpoints() < COSINE_GREAT_CIRCLE_ARC_ANGULAR_THRESHOLD)
		{
			// Tessellate the current great circle arc.
			std::vector<GPlatesMaths::PointOnSphere> surface_points;
			tessellate(surface_points, gca, GREAT_CIRCLE_ARC_ANGULAR_THRESHOLD);

			// Add the tessellated sub-arcs.
			for (unsigned int n = 1; n < surface_points.size(); ++n)
			{
				const GPlatesMaths::UnitVector3D &start_surface_point = surface_points[n-1].position_vector();
				const GPlatesMaths::UnitVector3D &end_surface_point = surface_points[n].position_vector();

				render_volume_fill_boundary_segment(start_surface_point, end_surface_point, vertex);
			}
		}
		else // no need to tessellate great circle arc...
		{
			const GPlatesMaths::UnitVector3D &start_surface_point = gca.start_point().position_vector();
			const GPlatesMaths::UnitVector3D &end_surface_point = gca.end_point().position_vector();

			render_volume_fill_boundary_segment(start_surface_point, end_surface_point, vertex);
		}
	}
}


void
GPlatesOpenGL::GLScalarField3D::VolumeFillBoundaryGeometryOnSphereVisitor::render_volume_fill_boundary_segment(
		const GPlatesMaths::UnitVector3D &start_surface_point,
		const GPlatesMaths::UnitVector3D &end_surface_point,
		VolumeFillBoundaryVertex &vertex)
{
	// There are two vertices and two indices per great circle arc.
	// Each great circle arc is sent as a line.
	// The geometry shader converts lines to triangles when it generates the
	// wall and spherical cap boundary surfaces.
	if (!d_stream_primitives.begin_primitive(2/*max_num_vertices*/, 2/*max_num_vertex_elements*/))
	{
		// There's not enough vertices or indices so render what we have so far and obtain new stream buffers.

		// Stop streaming volume fill boundary geometries so we can render the last batch.
		end_vertex_array_streaming<VolumeFillBoundaryVertex, streaming_vertex_element_type>(
				d_renderer,
				d_stream_target,
				d_map_vertex_element_buffer_scope,
				d_map_vertex_buffer_scope);

		// Render current contents of the stream.
		render_stream();

		// Start streaming volume fill boundary geometries.
		begin_vertex_array_streaming<VolumeFillBoundaryVertex, streaming_vertex_element_type>(
				d_renderer,
				d_stream_target,
				d_map_vertex_element_buffer_scope,
				MINIMUM_BYTES_TO_STREAM_IN_VERTEX_ELEMENT_BUFFER,
				d_map_vertex_buffer_scope,
				MINIMUM_BYTES_TO_STREAM_IN_VERTEX_BUFFER);

		d_stream_primitives.begin_primitive(2/*max_num_vertices*/, 2/*max_num_vertex_elements*/);
	}

	// NOTE: The centroid position has already been set in 'vertex'.

	// Line segment start vertex.
	vertex.surface_point[0] = start_surface_point.x().dval();
	vertex.surface_point[1] = start_surface_point.y().dval();
	vertex.surface_point[2] = start_surface_point.z().dval();
	d_stream_primitives.add_vertex(vertex);

	// Line segment end vertex.
	vertex.surface_point[0] = end_surface_point.x().dval();
	vertex.surface_point[1] = end_surface_point.y().dval();
	vertex.surface_point[2] = end_surface_point.z().dval();
	d_stream_primitives.add_vertex(vertex);

	// Line segment.
	d_stream_primitives.add_vertex_element(0);
	d_stream_primitives.add_vertex_element(1);

	d_stream_primitives.end_primitive();
}


GPlatesOpenGL::GLScalarField3D::SphereMeshBuilder::SphereMeshBuilder(
		std::vector<GLColourVertex> &vertices,
		std::vector<GLuint> &vertex_elements,
		const GPlatesGui::rgba8_t &colour,
		unsigned int recursion_depth_to_generate_mesh) :
	d_vertices(vertices),
	d_vertex_elements(vertex_elements),
	d_colour(colour),
	d_recursion_depth_to_generate_mesh(recursion_depth_to_generate_mesh)
{
}


void
GPlatesOpenGL::GLScalarField3D::SphereMeshBuilder::visit(
		const GPlatesMaths::HierarchicalTriangularMeshTraversal::Triangle &triangle,
		const unsigned int &recursion_depth)
{
	// If we're at the correct depth then add the triangle to our mesh.
	if (recursion_depth == d_recursion_depth_to_generate_mesh)
	{
		const unsigned int base_vertex_index = d_vertices.size();

		d_vertices.push_back(GLColourVertex(triangle.vertex0, d_colour));
		d_vertices.push_back(GLColourVertex(triangle.vertex1, d_colour));
		d_vertices.push_back(GLColourVertex(triangle.vertex2, d_colour));

		d_vertex_elements.push_back(base_vertex_index);
		d_vertex_elements.push_back(base_vertex_index + 1);
		d_vertex_elements.push_back(base_vertex_index + 2);

		return;
	}

	// Recurse into the child triangles.
	const unsigned int child_recursion_depth = recursion_depth + 1;
	triangle.visit_children(*this, child_recursion_depth);
}
