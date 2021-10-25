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

#include <algorithm>
#include <cmath>
#include <limits>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
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

#include "GLScalarField3DGenerator.h"

#include "GLBuffer.h"
#include "GLCompiledDrawState.h"
#include "GLContext.h"
#include "GLCubeSubdivision.h"
#include "GLFrameBufferObject.h"
#include "GLMultiResolutionRaster.h"
#include "GLPixelBuffer.h"
#include "GLRenderer.h"
#include "GLUtils.h"

#include "file-io/ErrorOpeningFileForWritingException.h"
#include "file-io/RasterReader.h"
#include "file-io/ReadErrorAccumulation.h"
#include "file-io/ReadErrorOccurrence.h"
#include "file-io/ScalarField3DFileFormat.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/LogException.h"
#include "global/PreconditionViolationError.h"

#include "maths/CubeCoordinateFrame.h"

#include "property-values/RawRaster.h"

#include "utils/Endian.h"
#include "utils/Profile.h"

// Uses a shader-generated test scalar field instead of importing from depth layer rasters.
// WARNING: Turning this on will overwrite the scalar field file if it exists.
//#define DEBUG_USING_SHADER_GENERATED_TEST_SCALAR_FIELD

//#define SCALAR_FIELD_VERSION_ZERO

namespace GPlatesOpenGL
{
	namespace
	{
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
GPlatesOpenGL::GLScalarField3DGenerator::is_supported(
		GLRenderer &renderer)
{
	static bool supported = false;

	// Only test for support the first time we're called.
	static bool tested_for_support = false;
	if (!tested_for_support)
	{
		tested_for_support = true;

		// Need support for GLScalarFieldDepthLayersSource.
		if (!GLMultiResolutionRaster::supports_scalar_field_depth_layers_source(renderer))
		{
			qWarning() << "Generation of 3D scalar fields NOT supported by this OpenGL system.";
			return false;
		}

		const GLCapabilities &capabilities = renderer.get_capabilities();

		// Test for OpenGL features used to generate scalar fields.
		if (// Using floating-point textures...
			!capabilities.texture.gl_ARB_texture_float ||
			!capabilities.texture.gl_ARB_texture_non_power_of_two ||
			!capabilities.shader.gl_ARB_vertex_shader ||
			!capabilities.shader.gl_ARB_fragment_shader ||
			// Need to render to textures using FBO...
			!capabilities.framebuffer.gl_EXT_framebuffer_object)
		{
			qWarning() << "Generation of 3D scalar fields NOT supported by this OpenGL system.";
			return false;
		}

		// If we get this far then we have support.
		supported = true;
	}

	return supported;
}


GPlatesOpenGL::GLScalarField3DGenerator::non_null_ptr_type
GPlatesOpenGL::GLScalarField3DGenerator::create(
		GLRenderer &renderer,
		const QString &scalar_field_filename,
		const GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type &georeferencing,
		unsigned int depth_layer_width,
		unsigned int depth_layer_height,
		const depth_layer_seq_type &depth_layers,
		GPlatesFileIO::ReadErrorAccumulation *read_errors)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			is_supported(renderer),
			GPLATES_ASSERTION_SOURCE);

	return non_null_ptr_type(
			new GLScalarField3DGenerator(
					renderer,
					scalar_field_filename,
					georeferencing,
					depth_layer_width,
					depth_layer_height,
					depth_layers,
					read_errors));
}


GPlatesOpenGL::GLScalarField3DGenerator::GLScalarField3DGenerator(
		GLRenderer &renderer,
		const QString &scalar_field_filename,
		const GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type &georeferencing,
		unsigned int depth_layer_width,
		unsigned int depth_layer_height,
		const depth_layer_seq_type &depth_layers,
		GPlatesFileIO::ReadErrorAccumulation *read_errors) :
	d_scalar_field_filename(scalar_field_filename),
	d_georeferencing(georeferencing),
	d_depth_layers(depth_layers),
	d_cube_face_dimension(0)
{
	// Make sure we leave the OpenGL state the way it was.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// Should have at least two depth layers.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_depth_layers.size() >= 2,
			GPLATES_ASSERTION_SOURCE);

	// Sort the depth layers from low to high radius.
	//
	// NOTE: GLScalarField assumes that the depth layer radii increase in radius through the depth layer sequence.
	std::sort(d_depth_layers.begin(), d_depth_layers.end(),
			boost::bind(&DepthLayer::depth_radius, _1) <
			boost::bind(&DepthLayer::depth_radius, _2));

	// Create a single multi-resolution raster that will be used to render all depth layers
	// into the cube map.
	if (!initialise_multi_resolution_raster(renderer, read_errors))
	{
		return;
	}

	initialise_cube_face_dimension(renderer);
}


bool
GPlatesOpenGL::GLScalarField3DGenerator::generate_scalar_field(
		GLRenderer &renderer,
		GPlatesFileIO::ReadErrorAccumulation *read_errors)
{
	PROFILE_FUNC();

	// Make sure we leave the OpenGL state the way it was.
	GLRenderer::StateBlockScope save_restore_state(
			renderer,
			// We're rendering to a render target so reset to the default OpenGL state...
			true/*reset_to_default_state*/);

	// If we were unable to create the multi-resolution raster in the constructor.
	if (!d_depth_layers_source ||
		!d_multi_resolution_raster)
	{
		return false;
	}

	// For now we write only global data which does not require partitioning of each cube face.
	// TODO: Once regional scalar fields are supported this will change.
	const unsigned int tile_meta_data_resolution = 1;
	const unsigned int tile_resolution = d_cube_face_dimension;
	const unsigned int num_active_tiles = 6;

	// Open the scalar field file for writing.
	QFile file(d_scalar_field_filename);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		throw GPlatesFileIO::ErrorOpeningFileForWritingException(
				GPLATES_EXCEPTION_SOURCE, d_scalar_field_filename);
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
#ifdef SCALAR_FIELD_VERSION_ZERO
	out << static_cast<quint32>(0);
#else
	out << static_cast<quint32>(GPlatesFileIO::ScalarField3DFileFormat::VERSION_NUMBER);
#endif

	// Write tile metadata resolution.
	out << static_cast<quint32>(tile_meta_data_resolution);

	// Write tile resolution.
	out << static_cast<quint32>(tile_resolution);

	// Write number of active tiles.
	out << static_cast<quint32>(num_active_tiles);

	// Write number of depth layers.
	out << static_cast<quint32>(d_depth_layers.size());

	// Write the layer depth radii.
	for (unsigned int depth_layer_index = 0; depth_layer_index < d_depth_layers.size(); ++depth_layer_index)
	{
		out << static_cast<float>(d_depth_layers[depth_layer_index].depth_radius);
	}

	//
	// Write the scalar/gradient statistics.
	// NOTE: This won't be available until we've rendered all the cube map scalar/gradient data
	// and written it to the file so we'll have to come back and write these out after that.
	//
	const qint64 scalar_gradient_statistics_file_offset = file.pos();
	// Write scalar minimum.
	out << static_cast<double>(0);
	// Write scalar maximum.
	out << static_cast<double>(0);
	// Write scalar mean.
	out << static_cast<double>(0);
	// Write scalar standard deviation.
	out << static_cast<double>(0);
	// Write gradient magnitude minimum.
	out << static_cast<double>(0);
	// Write gradient magnitude maximum.
	out << static_cast<double>(0);
	// Write gradient magnitude mean.
	out << static_cast<double>(0);
	// Write gradient magnitude standard deviation.
	out << static_cast<double>(0);

#ifdef SCALAR_FIELD_VERSION_ZERO
	const qint64 meta_data_file_offset = file.pos();

	// Write the tile metadata to the file.
	float tile_meta_data_array_tmp[6 * 3];
	for (unsigned int t = 0; t < 6 * 3; ++t)
	{
		tile_meta_data_array_tmp[t] = 0;
	}
	void *tile_meta_data_array_tmp_void = &tile_meta_data_array_tmp[0];
	out.writeRawData(static_cast<const char *>(tile_meta_data_array_tmp_void), 6 * 3 * sizeof(float));
#endif

	// Our cube map subdivision with a half-texel overlap at the border to avoid texture seams.
	GLCubeSubdivision::non_null_ptr_to_const_type cube_subdivision =
			GLCubeSubdivision::create(
					GLCubeSubdivision::get_expand_frustum_ratio(
							d_cube_face_dimension,
							0.5/* half a texel */));

	// Create a texture for rendering the cube map tiles to.
	GLTexture::shared_ptr_type cube_tile_texture = create_cube_tile_texture(renderer, tile_resolution);

	// Classify our frame buffer object according to texture format/dimensions.
	GLFrameBufferObject::Classification framebuffer_object_classification;
	framebuffer_object_classification.set_dimensions(
			cube_tile_texture->get_width().get(),
			cube_tile_texture->get_height().get());
	framebuffer_object_classification.set_texture_internal_format(
			cube_tile_texture->get_internal_format().get());

	// Acquire and bind a frame buffer object.
	// Framebuffer used to render to cube tile texture.
	GLFrameBufferObject::shared_ptr_type framebuffer_object =
			renderer.get_context().get_non_shared_state()->acquire_frame_buffer_object(
					renderer,
					framebuffer_object_classification);
	renderer.gl_bind_frame_buffer(framebuffer_object);

	// All rendering is directed to the cube tile texture.
	framebuffer_object->gl_attach_texture_2D(
			renderer,
			GL_TEXTURE_2D,
			cube_tile_texture,
			0, // level
			GL_COLOR_ATTACHMENT0_EXT);

	// Buffer size needed for a single layer of cube tile.
	// Each floating-point RGBA pixel contains scalar value and field gradient.
	const unsigned int buffer_size = tile_resolution * tile_resolution * 4 * sizeof(GLfloat);

	// A pixel buffer object to read the cube map scalar field data.
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

	// Viewport for the cube tile render target.
	renderer.gl_viewport(0, 0, tile_resolution, tile_resolution);

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

		// Get the projection transforms of an entire cube face (the lowest resolution level-of-detail).
		const GLTransform::non_null_ptr_to_const_type projection_transform =
				cube_subdivision->get_projection_transform(
						0/*level_of_detail*/,
						0/*tile_u_offset*/,
						0/*tile_v_offset*/);

		// The projection matrix.
		renderer.gl_load_matrix(GL_PROJECTION, projection_transform->get_matrix());

		// Get the source multi-resolution tiles that are visible in the current cube face view frustum.
		// These tiles are the same for all depth layers since each layer has the same georeferencing.
		std::vector<GLMultiResolutionRaster::tile_handle_type> source_raster_tile_handles;
		d_multi_resolution_raster.get()->get_visible_tiles(
				source_raster_tile_handles,
				view_transform->get_matrix(),
				projection_transform->get_matrix(),
				0/*tile_level_of_detail*/);

		// The current tile ID.
		// TODO: This only applies to global data.
		const unsigned int tile_ID = face;

		// The min/max scalar of the current tile.
		double tile_scalar_min = (std::numeric_limits<double>::max)();
		double tile_scalar_max = (std::numeric_limits<double>::min)();

		// Iterate over the depth layers of the current tile.
		for (unsigned int depth_layer_index = 0;
			depth_layer_index < d_depth_layers.size();
			++depth_layer_index)
		{
			renderer.gl_clear_color(); // Clear colour to all zeros.
			renderer.gl_clear(GL_COLOR_BUFFER_BIT); // Clear only the colour buffer.

			// Set the depth at which to render the current layer.
			d_depth_layers_source.get()->set_depth_layer(renderer, depth_layer_index);

			// Render the multi-resolution raster.
			// We don't need to keep the cache handle alive because we've asked for no caching in
			// the multi-resolution raster.
			GLMultiResolutionRaster::cache_handle_type multi_resolution_raster_cache_handle;
			d_multi_resolution_raster.get()->render(
					renderer,
					source_raster_tile_handles,
					multi_resolution_raster_cache_handle);

			// Read back the data just rendered.
			// NOTE: We don't need to worry about changing the default GL_PACK_ALIGNMENT
			// (rows aligned to 4 bytes) since our data is floats (each float is already 4-byte aligned).
			pixel_buffer->gl_read_pixels(
					renderer, 0, 0, tile_resolution, tile_resolution, GL_RGBA, GL_FLOAT, 0);

			// Map the pixel buffer to access its data.
			GLBuffer::MapBufferScope map_pixel_buffer_scope(
					renderer,
					*pixel_buffer->get_buffer(),
					GLBuffer::TARGET_PIXEL_PACK_BUFFER);

			// Map the pixel buffer data.
			void *field_data = map_pixel_buffer_scope.gl_map_buffer_static(GLBuffer::ACCESS_READ_ONLY);
			GPlatesFileIO::ScalarField3DFileFormat::FieldDataSample *const field_data_pixels =
					static_cast<GPlatesFileIO::ScalarField3DFileFormat::FieldDataSample *>(field_data);

			// Iterate over the pixels in the depth layer.
			for (unsigned int n = 0; n < tile_resolution * tile_resolution; ++n)
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

			// Convert from the runtime system endian to the endian required for the file (if necessary).
			GPlatesUtils::Endian::convert(
					field_data_pixels,
					field_data_pixels + tile_resolution * tile_resolution,
					static_cast<QSysInfo::Endian>(GPlatesFileIO::ScalarField3DFileFormat::Q_DATA_STREAM_BYTE_ORDER));

			// Write the field layer to the file.
			out.writeRawData(
					static_cast<const char *>(field_data),
					tile_resolution * tile_resolution *
						GPlatesFileIO::ScalarField3DFileFormat::FieldDataSample::STREAM_SIZE);
		}

		// Specify the current tile's metadata.
		GPlatesFileIO::ScalarField3DFileFormat::TileMetaData tile_meta_data;
		tile_meta_data.tile_ID = tile_ID;
		tile_meta_data.min_scalar_value = tile_scalar_min;
		tile_meta_data.max_scalar_value = tile_scalar_max;
		tile_meta_data_array.push_back(tile_meta_data);
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

	const unsigned int num_field_samples =
			num_active_tiles * d_depth_layers.size() * tile_resolution * tile_resolution;

	const double scalar_mean = scalar_sum / num_field_samples;
	const double gradient_magnitude_mean = gradient_magnitude_sum / num_field_samples;

	const double scalar_variance = scalar_sum_squares / num_field_samples - scalar_mean * scalar_mean;
	// Protect 'sqrt' in case variance is slightly negative due to numerical precision.
	const double scalar_standard_deviation = (scalar_variance > 0) ? std::sqrt(scalar_variance) : 0;

	const double gradient_magnitude_variance =
			gradient_magnitude_sum_squares / num_field_samples - gradient_magnitude_mean * gradient_magnitude_mean;
	// Protect 'sqrt' in case variance is slightly negative due to numerical precision.
	const double gradient_magnitude_standard_deviation =
			(gradient_magnitude_variance > 0) ? std::sqrt(gradient_magnitude_variance) : 0;

	//
	// Go back to the file offset for scalar/gradient statistics and write them out.
	//
	const qint64 mask_data_file_offset = file.pos();
	file.seek(scalar_gradient_statistics_file_offset);
	// Write scalar minimum.
	out << static_cast<double>(scalar_min);
	// Write scalar maximum.
	out << static_cast<double>(scalar_max);
	// Write scalar mean.
	out << static_cast<double>(scalar_mean);
	// Write scalar standard deviation.
	out << static_cast<double>(scalar_standard_deviation);
	// Write gradient magnitude minimum.
	out << static_cast<double>(gradient_magnitude_min);
	// Write gradient magnitude maximum.
	out << static_cast<double>(gradient_magnitude_max);
	// Write gradient magnitude mean.
	out << static_cast<double>(gradient_magnitude_mean);
	// Write gradient magnitude standard deviation.
	out << static_cast<double>(gradient_magnitude_standard_deviation);

	// Seek back to the current file offset where the mask data starts.
	file.seek(mask_data_file_offset);

	//
	// Write the mask data layer-by-layer to the file.
	//
	// Set the tile mask data to all ones for now since only supporting global scalar fields.
	// TODO: Add support for regional scalar fields.
	GPlatesFileIO::ScalarField3DFileFormat::MaskDataSample mask_data_sample;
	mask_data_sample.mask = 1.0f;
	std::vector<GPlatesFileIO::ScalarField3DFileFormat::MaskDataSample> mask_data_array(
			tile_resolution * tile_resolution, mask_data_sample);
	// Dodgy endian conversion hack inside a std::vector.
	// TODO: Fix this when adding support for regional scalar fields.
	void *mask_data = &mask_data_array[0];
	GPlatesFileIO::ScalarField3DFileFormat::MaskDataSample *mask_data_pixels =
			static_cast<GPlatesFileIO::ScalarField3DFileFormat::MaskDataSample *>(mask_data);
	// Set all active tiles to mask values of one.
	for (unsigned int mask_tile_index = 0; mask_tile_index < num_active_tiles; ++mask_tile_index)
	{
		// Convert from the runtime system endian to the endian required for the file (if necessary).
		GPlatesUtils::Endian::convert(
				mask_data_pixels,
				mask_data_pixels + tile_resolution * tile_resolution,
				static_cast<QSysInfo::Endian>(GPlatesFileIO::ScalarField3DFileFormat::Q_DATA_STREAM_BYTE_ORDER));

		// Write the mask layer to the file.
		out.writeRawData(
				static_cast<const char *>(mask_data),
				tile_resolution * tile_resolution *
					GPlatesFileIO::ScalarField3DFileFormat::MaskDataSample::STREAM_SIZE);
	}

#ifdef SCALAR_FIELD_VERSION_ZERO
	file.seek(meta_data_file_offset);
#endif

	//
	// Write the tile metadata layer-by-layer to the file.
	//
	// Dodgy endian conversion hack inside a std::vector.
	// TODO: Fix this when adding support for regional scalar fields.
	void *meta_data = &tile_meta_data_array[0];
	GPlatesFileIO::ScalarField3DFileFormat::TileMetaData *meta_data_pixels =
			static_cast<GPlatesFileIO::ScalarField3DFileFormat::TileMetaData *>(meta_data);
	const unsigned int num_meta_data_layers = 6;

	// Convert from the runtime system endian to the endian required for the file (if necessary).
	GPlatesUtils::Endian::convert(
			meta_data_pixels,
			meta_data_pixels + num_meta_data_layers * tile_meta_data_resolution * tile_meta_data_resolution,
			static_cast<QSysInfo::Endian>(GPlatesFileIO::ScalarField3DFileFormat::Q_DATA_STREAM_BYTE_ORDER));

	// Write the tile metadata layer to the file.
	out.writeRawData(
			static_cast<const char *>(meta_data),
			num_meta_data_layers * tile_meta_data_resolution * tile_meta_data_resolution *
				GPlatesFileIO::ScalarField3DFileFormat::TileMetaData::STREAM_SIZE);

	// Write the total size of the output file so the reader can verify that the
	// file was not partially written.
	file.seek(file_size_offset);
	total_output_file_size = file.size();
	out << total_output_file_size;

	file.close();

	// Release attachment to our cube tile texture before relinquishing acquired framebuffer object
	// since the texture only exists in the current scope.
	framebuffer_object->gl_detach_all(renderer);

	return true;
}


GPlatesOpenGL::GLTexture::shared_ptr_type
GPlatesOpenGL::GLScalarField3DGenerator::create_cube_tile_texture(
		GLRenderer &renderer,
		unsigned int tile_resolution)
{
	const GLCapabilities &capabilities = renderer.get_capabilities();

	// Create a texture for rendering the cube map tiles to.
	GLTexture::shared_ptr_type cube_tile_texture = GLTexture::create(renderer);

	// Nearest filtering is fine.
	// We're not actually going to use the texture - instead we download data from it to the CPU.
	cube_tile_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	cube_tile_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Clamp texture coordinates to centre of edge texels -
	// it's easier for hardware to implement - and doesn't affect our calculations.
	if (capabilities.texture.gl_EXT_texture_edge_clamp ||
		capabilities.texture.gl_SGIS_texture_edge_clamp)
	{
		cube_tile_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		cube_tile_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	else
	{
		cube_tile_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		cube_tile_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	}

	// Create the texture but don't load any data into it.
	//
	// NOTE: Since the image data is NULL it doesn't really matter what 'format' and 'type' are -
	// just use values that are compatible with all internal formats to avoid a possible error.
	cube_tile_texture->gl_tex_image_2D(
			renderer, GL_TEXTURE_2D, 0, GL_RGBA32F_ARB,
			tile_resolution, tile_resolution, 0, GL_RGBA, GL_FLOAT, NULL);

	// Check there are no OpenGL errors.
	GLUtils::assert_no_gl_errors(GPLATES_ASSERTION_SOURCE);

	return cube_tile_texture;
}


bool
GPlatesOpenGL::GLScalarField3DGenerator::initialise_multi_resolution_raster(
		GLRenderer &renderer,
		GPlatesFileIO::ReadErrorAccumulation *read_errors)
{
	GLScalarFieldDepthLayersSource::depth_layer_seq_type depth_layers_source_sequence;

	// Create a proxied raster for each depth layer in the sequence.
	BOOST_FOREACH(const DepthLayer &depth_layer, d_depth_layers)
	{
		// Create a raster reader for the current depth layer.
		GPlatesFileIO::RasterReader::non_null_ptr_type reader =
				GPlatesFileIO::RasterReader::create(depth_layer.depth_raster_filename, read_errors);
		if (!reader->can_read())
		{
			return false;
		}

		// Create a proxied RawRaster for the first band in the raster file.
		// Band numbers start at 1.
		// TODO: Allow user to select other bands to import from.
		boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type> proxied_raw_raster =
				reader->get_proxied_raw_raster(1, read_errors);
		if (!proxied_raw_raster)
		{
			return false;
		}

		depth_layers_source_sequence.push_back(
				GLScalarFieldDepthLayersSource::DepthLayer(
						proxied_raw_raster.get(),
						depth_layer.depth_radius));
	}

	// Create a data source for the multi-resolution raster that can switch between the various depth layers.
	d_depth_layers_source = GLScalarFieldDepthLayersSource::create(renderer, depth_layers_source_sequence);
	if (!d_depth_layers_source)
	{
		report_failure_to_begin(read_errors, GPlatesFileIO::ReadErrors::DepthLayerRasterIsNotNumerical);
		return false;
	}

	// Create the multi-resolution raster.
	d_multi_resolution_raster =
			GLMultiResolutionRaster::create(
					renderer,
					d_georeferencing,
					d_depth_layers_source.get(),
					GLMultiResolutionRaster::DEFAULT_FIXED_POINT_TEXTURE_FILTER,
					// No need to cache tiles...
					GLMultiResolutionRaster::CACHE_TILE_TEXTURES_NONE);

	return true;
}


void
GPlatesOpenGL::GLScalarField3DGenerator::initialise_cube_face_dimension(
		GLRenderer &renderer)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_multi_resolution_raster,
			GPLATES_ASSERTION_SOURCE);

	// We don't worry about half-texel expansion of the projection frustums here because
	// we just need to determine viewport dimensions. There will be a slight error by neglecting
	// the half texel but it's already an approximation anyway.
	// Besides, the half texel depends on the tile texel dimension and we're going to change that below.
	GLCubeSubdivision::non_null_ptr_to_const_type cube_subdivision = GLCubeSubdivision::create();

	// Get the projection transforms of an entire cube face (the lowest resolution level-of-detail).
	const GLTransform::non_null_ptr_to_const_type projection_transform =
			cube_subdivision->get_projection_transform(
					0/*level_of_detail*/, 0/*tile_u_offset*/, 0/*tile_v_offset*/);

	// Get the view transform - it doesn't matter which cube face we choose because, although
	// the view transform are different, it won't matter to us since we're projecting onto
	// a spherical globe from its centre and all faces project the same way.
	const GLTransform::non_null_ptr_to_const_type view_transform =
			cube_subdivision->get_view_transform(
					GPlatesMaths::CubeCoordinateFrame::POSITIVE_X);

	// Start off with a fixed-size viewport - we'll adjust its width and height shortly.
	// It doesn't matter the initial value since it'll be adjusted to the same end value anyway.
	d_cube_face_dimension = 256;

	// Determine the scale factor for our viewport dimensions required to capture the resolution
	// of the highest level of detail (level 0) of the source raster into an entire cube face.
	double viewport_dimension_scale = d_multi_resolution_raster.get()->get_viewport_dimension_scale(
			view_transform->get_matrix(),
			projection_transform->get_matrix(),
			GLViewport(0, 0, d_cube_face_dimension, d_cube_face_dimension),
			0/*level_of_detail*/);

	// The source raster level-of-detail (and hence viewport dimension scale) is determined such
	// that a pixel on the globe covers no more than one pixel in the cube map. However the
	// variation in cube map projection from face centre to face corner is approximately
	// a factor of two (or one level-of-detail difference). This means two pixels on the globe
	// can fit into one pixel in the cube map at a face centre. By increasing the viewport
	// dimension by approximately a factor of two we get more detail in the scalar field.
	// The factor is sqrt(3) * (1 / cos(A)); where sin(A) = (1 / sqrt(3)).
	// This is the same as 3 / sqrt(2).
	// The sqrt(3) is length of cube half-diagonal (divided by unit-length globe radius).
	// The cos(A) is a 35 degree angle between the cube face and globe tangent plane at cube corner
	// (globe tangent calculated at position on globe that cube corner projects onto).
	// This factor is how much a pixel on the globe expands in size when projected to a pixel on the
	// cube face at its corner (and is close to a factor of two).
	viewport_dimension_scale *= 3.0 / std::sqrt(2.0);

	// Adjust the dimension (either reduce or enlarge).
	d_cube_face_dimension *= viewport_dimension_scale;

	qDebug() << "initial cube_face_dimension: " << d_cube_face_dimension;

	// For now just limit the cube face dimension to something reasonable
	// to avoid excessive memory usage.
	if (d_cube_face_dimension > 128)
	{
		d_cube_face_dimension = 128;
	}
	// Also limit to max texture size if exceeds.
	if (d_cube_face_dimension > renderer.get_capabilities().texture.gl_max_texture_size)
	{
		d_cube_face_dimension = renderer.get_capabilities().texture.gl_max_texture_size;
	}
}


void
GPlatesOpenGL::GLScalarField3DGenerator::report_recoverable_error(
		GPlatesFileIO::ReadErrorAccumulation *read_errors,
		GPlatesFileIO::ReadErrors::Description description)
{
	if (read_errors)
	{
		read_errors->d_recoverable_errors.push_back(
				GPlatesFileIO::make_read_error_occurrence(
					d_scalar_field_filename,
					GPlatesFileIO::DataFormats::ScalarField3D,
					0,
					description,
					GPlatesFileIO::ReadErrors::FileNotImported));
	}
}


void
GPlatesOpenGL::GLScalarField3DGenerator::report_failure_to_begin(
		GPlatesFileIO::ReadErrorAccumulation *read_errors,
		GPlatesFileIO::ReadErrors::Description description)
{
	if (read_errors)
	{
		read_errors->d_failures_to_begin.push_back(
				GPlatesFileIO::make_read_error_occurrence(
					d_scalar_field_filename,
					GPlatesFileIO::DataFormats::ScalarField3D,
					0,
					description,
					GPlatesFileIO::ReadErrors::FileNotImported));
	}
}
