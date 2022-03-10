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

#include <opengl/OpenGL3.h>  // Should be included at TOP of ".cc" file.

#include <cmath>
#include <limits>
#include <set>
#include <vector>
#include <boost/cstdint.hpp>
#include <boost/scoped_array.hpp>
#include <QDataStream>
#include <QDebug>
#include <QFile>
#include <QImage>
#include <QString>

#include "GLScalarField3D.h"

#include "GL.h"
#include "GLBuffer.h"
#include "GLContext.h"
#include "GLShader.h"
#include "GLShaderSource.h"
#include "GLUtils.h"
#include "GLVertexUtils.h"
#include "GLViewProjection.h"
#include "OpenGLException.h"

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
		const QString SCALAR_FIELD_UTILS_SOURCE_FILE_NAME = ":/opengl/scalar_field_3d/utils.glsl";

		//
		// Shader source to render isosurface.
		//

		const char *ISO_SURFACE_VERTEX_SHADER_SOURCE =
			R"(
				layout (location = 0) in vec4 position;

				void main()
				{
					gl_Position = position;
				}
			)";


		const QString ISO_SURFACE_FRAGMENT_SHADER_SOURCE_FILE_NAME =
				":/opengl/scalar_field_3d/iso_surface_fragment_shader.glsl";

		//
		// Shader source to render vertical cross-sections of scalar field.
		//

		const char *CROSS_SECTION_VERTEX_SHADER_SOURCE =
			R"(
				// The (x,y,z) values contain the point location on globe surface.
				layout (location = 0) in vec3 surface_point;

				out VertexData
				{
					vec3 surface_point;
				} vs_out;

				void main()
				{
					vs_out.surface_point = surface_point;
				}
			)";

		const QString CROSS_SECTION_GEOMETRY_SHADER_SOURCE_FILE_NAME =
				":/opengl/scalar_field_3d/cross_section_geometry_shader.glsl";

		const QString CROSS_SECTION_FRAGMENT_SHADER_SOURCE_FILE_NAME =
				":/opengl/scalar_field_3d/cross_section_fragment_shader.glsl";

		//
		// Shader source to render surface fill mask.
		//

		const char *SURFACE_FILL_MASK_VERTEX_SHADER_SOURCE =
			R"(
				layout (location = 0) in vec4 surface_point;

				void main (void)
				{
					// Currently there's no model transform (Euler rotation) since rendering *reconstructed* geometries.
					// Just pass the vertex position on to the geometry shader.
					// Later, if there's a model transform, then we'll do it here to take the load
					// off the geometry shader (only vertex shader has a post-transform hardware cache).
					gl_Position = surface_point;
				}
			)";

		const char *SURFACE_FILL_MASK_GEOMETRY_SHADER_SOURCE =
			R"(
				//
				// Take each input triangle and create 6 output triangles, one per cube face,
				// where each cube face has its own view projection transform.
				//

				// The view projection matrices for rendering into each cube face.
				uniform mat4 cube_face_view_projection_matrices[6];

				layout (triangles) in;
				layout (triangle_strip, max_vertices = 18) out;

				void main (void)
				{
					// Iterate over the six cube faces.
					for (int cube_face = 0; cube_face < 6; cube_face++)
					{
						// The three vertices of the current triangle primitive.
						for (int vertex = 0; vertex < 3; vertex++)
						{
							// Transform each vertex using the current cube face view-projection matrix.
							gl_Position = cube_face_view_projection_matrices[cube_face] * gl_in[vertex].gl_Position;

							// Each cube face is represented by a separate layer in a texture array.
							gl_Layer = cube_face;

							EmitVertex();
						}
						EndPrimitive();
					}
				}
			)";

		const char *SURFACE_FILL_MASK_FRAGMENT_SHADER_SOURCE =
			R"(
				layout (location = 0) out vec4 colour;

				void main (void)
				{
					colour = vec4(1);
				}
			)";
		
		//
		// Shader source code to render volume fill boundary.
		//
		// Shader source code to render volume fill walls (vertically extruded quads).
		//
		//
		// Used for both depth range and wall normals.
		// Also previously (but no longer) used for both walls and spherical caps (for depth range).
		//

		const char *VOLUME_FILL_WALL_VERTEX_SHADER_SOURCE =
			R"(
				// The (x,y,z) values contain the point location on globe surface.
				layout (location = 0) in vec3 surface_point;

				out VertexData
				{
					vec3 surface_point;
				} vs_out;

				void main()
				{
					vs_out.surface_point = surface_point;
				}
			)";

		//! Geometry shader source code to render volume fill walls.
		const QString VOLUME_FILL_WALL_GEOMETRY_SHADER_SOURCE_FILE_NAME =
				":/opengl/scalar_field_3d/volume_fill_wall_geometry_shader.glsl";

		//! Fragment shader source code to render volume fill wall *surface normal and depth*.
		const QString VOLUME_FILL_WALL_SURFACE_NORMAL_FRAGMENT_SHADER_SOURCE_FILE_NAME =
				":/opengl/scalar_field_3d/volume_fill_wall_surface_normal_fragment_shader.glsl";

		//! Fragment shader source code to render volume fill wall *depth range*.
		const char *VOLUME_FILL_WALL_DEPTH_RANGE_FRAGMENT_SHADER_SOURCE =
			R"(
				layout (location = 0) out vec4 depth;

				void main (void)
				{
					// Write *screen-space* depth (ie, depth range [-1,1] and not [n,f]).
					// This is what's used in the ray-tracing shader since it uses inverse model-view-proj matrix on the depth
					// to get world space position and that requires normalised device coordinates not window coordinates).
					//
					// Convert window coordinates (z_w) to normalised device coordinates (z_ndc) which, for depth, is:
					//   [n,f] -> [-1,1]
					// ...where [n,f] is set by glDepthRange (default is [0,1]). The conversion is:
					//   z_w = z_ndc * (f-n)/2 + (n+f)/2
					// ...which is equivalent to:
					//   z_ndc = [2 * z_w - n - f)] / (f-n)
					//
					depth = vec4((2 * gl_FragCoord.z - gl_DepthRange.near - gl_DepthRange.far) / gl_DepthRange.diff);
				}
			)";

#if 0 // Not currently used...
		const char *VOLUME_FILL_SPHERICAL_CAP_VERTEX_SHADER_SOURCE =
			R"(
				layout (location = 0) in vec4 surface_point;
				layout (location = 1) in vec4 centroid_point;

				out vec4 polygon_centroid_point;

				void main (void)
				{
					gl_Position = surface_point;
					polygon_centroid_point = centroid_point;
				}
			)";

		//! Geometry shader source code to render volume fill spherical caps.
		const QString VOLUME_FILL_SPHERICAL_CAP_GEOMETRY_SHADER_SOURCE_FILE_NAME =
				":/opengl/scalar_field_3d/volume_fill_spherical_cap_geometry_shader.glsl";

		//! Fragment shader source code to render volume fill spherical caps.
		const QString VOLUME_FILL_SPHERICAL_CAP_FRAGMENT_SHADER_SOURCE_FILE_NAME =
				":/opengl/scalar_field_3d/volume_fill_spherical_cap_fragment_shader.glsl";
#endif


		//
		// Shader source for rendering the inner sphere as white (with lighting).
		//
		// We use it to render the white inner sphere when rendering cross-sections
		// (the ray-tracing isosurface rendering doesn't need it however).
		//

		const char *WHITE_INNER_SPHERE_VERTEX_SHADER_SOURCE =
			R"(
				layout (std140) uniform ViewProjection
				{
					// Combined view-projection transform, and inverse of view and projection transforms.
					mat4 view_projection;
					mat4 view_inverse;
					mat4 projection_inverse;

					// Viewport (used to convert window-space coordinates gl_FragCoord.xy to NDC space [-1,1]).
					uniform vec4 viewport;
				} view_projection_block;

				layout(std140) uniform Depth
				{
					// The actual minimum and maximum depth radii of the scalar field.
					vec2 min_max_depth_radius;

					// The depth range rendering is restricted to.
					// If depth range is not restricted then this is the same as 'min_max_depth_radius'.
					// Also the following conditions hold:
					//	min_max_depth_radius_restriction.x >= min_max_depth_radius.x
					//	min_max_depth_radius_restriction.y <= min_max_depth_radius.y
					// ...in other words the depth range for rendering is always within the actual depth range.
					vec2 min_max_depth_radius_restriction;

					// Number of depth layers in scalar field.
					int num_depth_layers;
				} depth_block;

				layout(location = 0) in vec4 position;

				out VertexData
				{
					// The world-space coordinates are interpolated across the geometry.
					vec3 world_space_position;
				} vs_out;

				void main (void)
				{
					vs_out.world_space_position = depth_block.min_max_depth_radius_restriction.x * position.xyz;
					gl_Position = view_projection_block.view_projection * vec4(vs_out.world_space_position, 1);
				}
			)";

		const char *WHITE_INNER_SPHERE_FRAGMENT_SHADER_SOURCE =
			R"(
				uniform vec4 sphere_colour;

				layout (std140) uniform Lighting
				{
					bool enabled;
					float ambient;
					vec3 world_space_light_direction;
				} lighting_block;

				in VertexData
				{
					// The world-space coordinates are interpolated across the geometry.
					vec3 world_space_position;
				} fs_in;
			
				layout(location = 0) out vec4 colour;

				void main (void)
				{
					colour = sphere_colour;

					if (lighting_block.enabled)
					{
						// Apply the Lambert diffuse lighting using the world-space position as the globe surface normal.
						// Note that neither the light direction nor the surface normal need be normalised.
						float diffuse_lighting = lambert_diffuse_lighting(
								lighting_block.world_space_light_direction,
								fs_in.world_space_position);

						// Blend between ambient and diffuse lighting.
						float lighting = mix_ambient_with_diffuse_lighting(diffuse_lighting, lighting_block.ambient);

						colour.rgb *= lighting;
					}
				}
			)";


		//
		// Shader source for rendering the inner sphere as screen-space depth.
		//

		const char *DEPTH_RANGE_INNER_SPHERE_VERTEX_SHADER_SOURCE =
			R"(
				layout (std140) uniform ViewProjection
				{
					// Combined view-projection transform, and inverse of view and projection transforms.
					mat4 view_projection;
					mat4 view_inverse;
					mat4 projection_inverse;

					// Viewport (used to convert window-space coordinates gl_FragCoord.xy to NDC space [-1,1]).
					uniform vec4 viewport;
				} view_projection_block;

				layout(std140) uniform Depth
				{
					// The actual minimum and maximum depth radii of the scalar field.
					vec2 min_max_depth_radius;

					// The depth range rendering is restricted to.
					// If depth range is not restricted then this is the same as 'min_max_depth_radius'.
					// Also the following conditions hold:
					//	min_max_depth_radius_restriction.x >= min_max_depth_radius.x
					//	min_max_depth_radius_restriction.y <= min_max_depth_radius.y
					// ...in other words the depth range for rendering is always within the actual depth range.
					vec2 min_max_depth_radius_restriction;

					// Number of depth layers in scalar field.
					int num_depth_layers;
				} depth_block;

				layout(location = 0) in vec4 position;

				void main (void)
				{
					gl_Position = view_projection_block.view_projection *
							vec4(depth_block.min_max_depth_radius_restriction.x * position.xyz, 1);
				}
			)";

		const char *DEPTH_RANGE_INNER_SPHERE_FRAGMENT_SHADER_SOURCE =
			R"(
				layout(location = 0) out vec4 depth;

				void main (void)
				{
					// Write *screen-space* depth (ie, depth range [-1,1] and not [n,f]).
					// This is what's used in the ray-tracing shader since it uses inverse model-view-proj matrix on the depth
					// to get world space position and that requires normalised device coordinates not window coordinates).
					//
					// Convert window coordinates (z_w) to normalised device coordinates (z_ndc) which, for depth, is:
					//   [n,f] -> [-1,1]
					// ...where [n,f] is set by glDepthRange (default is [0,1]). The conversion is:
					//   z_w = z_ndc * (f-n)/2 + (n+f)/2
					// ...which is equivalent to:
					//   z_ndc = [2 * z_w - n - f)] / (f-n)
					//
					depth = vec4((2 * gl_FragCoord.z - gl_DepthRange.near - gl_DepthRange.far) / gl_DepthRange.diff);
				}
			)";


		/**
		 * Useful when debugging a fixed-point texture array by saving each layer to an image file.
		 */
		void
		debug_fixed_point_texture_array(
				GL &gl,
				const GLTexture::shared_ptr_type &texture,
				const unsigned int texture_resolution,
				const unsigned int texture_depth,
				const QString &image_file_basename)
		{
			// Make sure we leave the OpenGL global state the way it was.
			GL::StateScope save_restore_state(gl);

			gl.BindTexture(GL_TEXTURE_2D_ARRAY, texture);

			// Allocate for all layers of texture array.
			boost::scoped_array<GPlatesGui::rgba8_t> rgba8_data(
					new GPlatesGui::rgba8_t[texture_resolution * texture_resolution * texture_depth]);

			// Read all layers of texture array.
			glGetTexImage(GL_TEXTURE_2D_ARRAY, 0/*lod*/, GL_RGBA, GL_UNSIGNED_BYTE, rgba8_data.get());

			// Iterate over the layers of texture array.
			for (unsigned int layer = 0; layer < texture_depth; ++layer)
			{
				// Pointer to data in the current layer.
				GPlatesGui::rgba8_t *rgba8_layer_data = rgba8_data.get() + layer * texture_resolution * texture_resolution;

				for (unsigned int y = 0; y < texture_resolution; ++y)
				{
					for (unsigned int x = 0; x < texture_resolution; ++x)
					{
						GPlatesGui::rgba8_t &result_pixel = rgba8_layer_data[y * texture_resolution + x];

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

						result_pixel = colour;
					}
				}

				boost::scoped_array<boost::uint32_t> argb32_layer_data(
						new boost::uint32_t[texture_resolution * texture_resolution]);

				// Convert to a format supported by QImage.
				GPlatesGui::convert_rgba8_to_argb32(
						rgba8_layer_data,
						argb32_layer_data.get(),
						texture_resolution * texture_resolution);

				QImage qimage(
						static_cast<const uchar *>(static_cast<const void *>(argb32_layer_data.get())),
						texture_resolution,
						texture_resolution,
						QImage::Format_ARGB32);

				// Save the image to a file.
				QString image_filename = QString("%1%2.png").arg(image_file_basename).arg(layer);
				qimage.save(image_filename, "PNG");
			}
		}
	}
}


GPlatesOpenGL::GLScalarField3D::non_null_ptr_type
GPlatesOpenGL::GLScalarField3D::create(
		GL &gl,
		const QString &scalar_field_filename,
		const GLLight::non_null_ptr_type &light)
{
	return non_null_ptr_type(new GLScalarField3D(gl, scalar_field_filename, light));
}


GPlatesOpenGL::GLScalarField3D::GLScalarField3D(
		GL &gl,
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
	d_tile_meta_data_texture_array(GLTexture::create(gl)),
	d_field_data_texture_array(GLTexture::create(gl)),
	d_mask_data_texture_array(GLTexture::create(gl)),
	d_depth_radius_to_layer_texture(GLTexture::create(gl)),
	d_depth_radius_to_layer_resolution(
			// Can't be larger than the maximum texture dimension for the current system...
			(DEPTH_RADIUS_TO_LAYER_RESOLUTION > gl.get_capabilities().gl_max_texture_size)
			? gl.get_capabilities().gl_max_texture_size
			: DEPTH_RADIUS_TO_LAYER_RESOLUTION),
	d_colour_palette_texture(GLTexture::create(gl)),
	d_colour_palette_resolution(
			// Can't be larger than the maximum texture dimension for the current system...
			(COLOUR_PALETTE_RESOLUTION > gl.get_capabilities().gl_max_texture_size)
			? gl.get_capabilities().gl_max_texture_size
			: COLOUR_PALETTE_RESOLUTION),
	d_uniform_buffer(GLBuffer::create(gl)),
	d_uniform_buffer_ranges{},  // zero-initialization of each range - we'll properly initialize later
	d_cross_section_vertex_array(GLVertexArray::create(gl)),
	d_surface_fill_mask_vertex_array(GLVertexArray::create(gl)),
	d_surface_fill_mask_texture(GLTexture::create(gl)),
	d_surface_fill_mask_framebuffer(GLFramebuffer::create(gl)),
	d_surface_fill_mask_resolution(
			// Can't be larger than the maximum texture dimension for the current system...
			(SURFACE_FILL_MASK_RESOLUTION > gl.get_capabilities().gl_max_texture_size)
			? gl.get_capabilities().gl_max_texture_size
			: SURFACE_FILL_MASK_RESOLUTION),
	d_volume_fill_boundary_vertex_array(GLVertexArray::create(gl)),
	d_volume_fill_texture_sampler(GLSampler::create(gl)),
	d_volume_fill_texture(GLTexture::create(gl)),
	d_volume_fill_depth_buffer(GLRenderbuffer::create(gl)),
	d_volume_fill_framebuffer(GLFramebuffer::create(gl)),
	d_volume_fill_screen_width(0),
	d_volume_fill_screen_height(0),
	d_inner_sphere_vertex_array(GLVertexArray::create(gl)),
	d_inner_sphere_vertex_buffer(GLBuffer::create(gl)),
	d_inner_sphere_vertex_element_buffer(GLBuffer::create(gl)),
	d_inner_sphere_num_vertex_indices(0),
	d_full_screen_quad(gl.get_context().get_shared_state()->get_full_screen_quad(gl)),
	d_streaming_vertex_element_buffer(
			GLStreamBuffer::create(
					GLBuffer::create(gl),
					NUM_BYTES_IN_STREAMING_VERTEX_ELEMENT_BUFFER)),
	d_streaming_vertex_buffer(
			GLStreamBuffer::create(
					GLBuffer::create(gl),
					NUM_BYTES_IN_STREAMING_VERTEX_BUFFER))
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
				gl.get_capabilities().gl_max_texture_array_layers,
			GPLATES_ASSERTION_SOURCE,
			"GLScalarField3D: number texture layers in scalar field file exceeded GPU limit.");

	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(gl);

	// Create the textures (allocate their memory and set their filtering modes).
	create_tile_meta_data_texture_array(gl);
	create_field_data_texture_array(gl);
	create_mask_data_texture_array(gl);
	create_depth_radius_to_layer_texture(gl);
	create_colour_palette_texture(gl);
	create_surface_fill_mask_texture(gl);
	create_surface_fill_mask_framebuffer(gl); // Call after 'create_surface_fill_mask_texture()'.
	create_volume_fill_texture_sampler(gl);

	// Initialise the uniform buffer containing named uniform blocks for our shader programs.
	initialise_uniform_buffer(gl);

	// An inner sphere needs to be explicitly rendered when drawing cross-sections.
	// It's also used when rendering depth range of volume fill walls.
	// However it's rendered implicitly by ray-tracing when rendering iso-surface.
	initialise_inner_sphere(gl);

	// Initialise the shader program and vertex arrays for rendering cross-section geometry.
	initialise_cross_section_rendering(gl);

	// Initialise the shader program for rendering isosurface.
	initialise_iso_surface_rendering(gl);

	// Initialise the shader program for rendering surface fill mask.
	initialise_surface_fill_mask_rendering(gl);

	// Initialise the shader program for rendering volume fill boundary.
	initialise_volume_fill_boundary_rendering(gl);

	// Load the scalar field from the file.
	load_scalar_field(gl, scalar_field_reader);

	// The colour palette texture will get loaded when the client calls 'set_colour_palette()'.
}


void
GPlatesOpenGL::GLScalarField3D::set_colour_palette(
		GL &gl,
		const GPlatesGui::ColourPalette<double>::non_null_ptr_to_const_type &colour_palette,
		const std::pair<double, double> &colour_palette_value_range)
{
	d_colour_palette_value_range = colour_palette_value_range;
	load_colour_palette_texture(gl, colour_palette, colour_palette_value_range);
}


bool
GPlatesOpenGL::GLScalarField3D::change_scalar_field(
		GL &gl,
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
	load_scalar_field(gl, scalar_field_reader);

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
		GL &gl,
		const GLViewProjection &view_projection,
		cache_handle_type &cache_handle,
		GPlatesViewOperations::ScalarField3DRenderParameters::IsosurfaceDeviationWindowMode deviation_window_mode,
		GPlatesViewOperations::ScalarField3DRenderParameters::IsosurfaceColourMode colour_mode,
		const GPlatesViewOperations::ScalarField3DRenderParameters::IsovalueParameters &isovalue_parameters,
		const GPlatesViewOperations::ScalarField3DRenderParameters::DeviationWindowRenderOptions &deviation_window_render_options,
		const GPlatesViewOperations::ScalarField3DRenderParameters::DepthRestriction &depth_restriction,
		const GPlatesViewOperations::ScalarField3DRenderParameters::QualityPerformance &quality_performance,
		const std::vector<float> &test_variables,
		boost::optional<SurfaceFillMask> surface_fill_mask,
		boost::optional<GLTexture::shared_ptr_type> depth_read_texture)
{
	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(gl);

	// Update the uniform buffer containing named uniform blocks and bind its ranges to the block binding points.
	update_uniform_variables_for_iso_surface(
			gl, view_projection, deviation_window_mode, colour_mode, isovalue_parameters, deviation_window_render_options,
			depth_restriction, quality_performance, test_variables, surface_fill_mask, depth_read_texture);

	if (depth_read_texture)
	{
		// Bind the depth texture to its texture unit.
		gl.ActiveTexture(GL_TEXTURE0 + Samplers::DEPTH_TEXTURE_SAMPLER);
		gl.BindTexture(GL_TEXTURE_2D, depth_read_texture.get());
	}

	if (surface_fill_mask)
	{
		// Render to the surface fill mask texture array (6 cube faces) using the surface geometries.
		render_surface_fill_mask(
				gl,
				surface_fill_mask->surface_polygons_mask,
				surface_fill_mask->treat_polylines_as_polygons);

		// Bind the surface fill mask texture.
		gl.ActiveTexture(GL_TEXTURE0 + Samplers::SURFACE_FILL_MASK_SAMPLER);
		gl.BindTexture(GL_TEXTURE_2D, d_surface_fill_mask_texture);

		if (surface_fill_mask->show_walls)
		{
			// Render to the screen-size normal/depth texture.
			render_volume_fill_walls(
					gl,
					view_projection,
					surface_fill_mask->surface_polygons_mask,
					surface_fill_mask->treat_polylines_as_polygons,
					surface_fill_mask->show_walls->only_boundary_walls);
		}
		else
		{
			// Render to the screen-size depth range texture.
			//
			// We are not drawing the volume fill walls (surface normal and depth), so instead
			// generate the min/max depth range of the volume fill walls.
			// This makes the isosurface shader more efficient by reducing the length along
			// each ray that is sampled/traversed - note that the walls are not visible though.
			// We don't need this if the walls are going to be drawn because there are already good
			// optimisations in place to limit ray sampling based on the fact that the walls are opaque.
			render_volume_fill_depth_range(
					gl,
					view_projection,
					surface_fill_mask->surface_polygons_mask,
					surface_fill_mask->treat_polylines_as_polygons);
		}

		// Bind the volume fill texture (and sampler object).
		//
		// Note: The same volume fill texture contains either the walls (surface normals + depth) or
		//       depth range (depending on the path taken above).
		gl.ActiveTexture(GL_TEXTURE0 + Samplers::VOLUME_FILL_SAMPLER);
		gl.BindTexture(GL_TEXTURE_2D, d_volume_fill_texture);
		gl.BindSampler(Samplers::VOLUME_FILL_SAMPLER, d_volume_fill_texture_sampler);
	}

	// Bind the tile metadata texture sampler.
	gl.ActiveTexture(GL_TEXTURE0 + Samplers::TILE_META_DATA_SAMPLER);
	gl.BindTexture(GL_TEXTURE_2D_ARRAY, d_tile_meta_data_texture_array);

	// Bind the field data texture sampler.
	gl.ActiveTexture(GL_TEXTURE0 + Samplers::FIELD_DATA_SAMPLER);
	gl.BindTexture(GL_TEXTURE_2D_ARRAY, d_field_data_texture_array);

	// Bind the mask data texture sampler.
	gl.ActiveTexture(GL_TEXTURE0 + Samplers::MASK_DATA_SAMPLER);
	gl.BindTexture(GL_TEXTURE_2D_ARRAY, d_mask_data_texture_array);

	// Bind the 1D depth radius to layer texture sampler.
	gl.ActiveTexture(GL_TEXTURE0 + Samplers::DEPTH_RADIUS_TO_LAYER_SAMPLER);
	gl.BindTexture(GL_TEXTURE_1D, d_depth_radius_to_layer_texture);

	// Bind the 1D colour palette texture sampler.
	gl.ActiveTexture(GL_TEXTURE0 + Samplers::COLOUR_PALETTE_SAMPLER);
	gl.BindTexture(GL_TEXTURE_1D, d_colour_palette_texture);

	// Bind the shader program for rendering iso-surface.
	gl.UseProgram(d_iso_surface_program);

	// The isosurface shader program outputs colours that have premultiplied alpha.
	// So, since RGB has been premultiplied with alpha we want its source factor to be one (instead of alpha):
	//
	//   RGB =     1 * RGB_src + (1-A_src) * RGB_dst
	//
	// And for Alpha we want its source factor to be one (as usual):
	//
	//     A =     1 *   A_src + (1-A_src) *   A_dst
	//
	// ...this enables the destination to be a texture that is subsequently blended into the final scene.
	// In this case the destination alpha must be correct in order to properly blend the texture into the final scene.
	//
	gl.Enable(GL_BLEND);
	gl.BlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	// Draw the full screen quad.
	gl.BindVertexArray(d_full_screen_quad);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}


void
GPlatesOpenGL::GLScalarField3D::render_surface_fill_mask(
		GL &gl,
		const surface_polygons_mask_seq_type &surface_polygons_mask,
		bool include_polylines)
{
	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(
			gl,
			// We're rendering to a render target so reset to the default OpenGL state...
			true/*reset_to_default_state*/);

	// Bind our framebuffer object for rendering to the surface fill mask texture array.
	// This directs rendering to the tile texture at the first colour attachment, and
	// its associated depth/stencil renderbuffer at the depth/stencil attachment.
	gl.BindFramebuffer(GL_FRAMEBUFFER, d_surface_fill_mask_framebuffer);

	// Viewport for the textures in the field texture array.
	gl.Viewport(0, 0, d_surface_fill_mask_resolution, d_surface_fill_mask_resolution);

	// Clear all layers of texture array.
	gl.ClearColor(); // Clear colour to all zeros.
	glClear(GL_COLOR_BUFFER_BIT); // Clear only the colour buffer.

	// Set up separate alpha-blending factors for the RGB and Alpha channels.
	// Doing this means we can minimise OpenGL state changes and simply switch between
	// masking the RGB channels and masking the Alpha channel to switch between generating
	// a fill for a single polygon and accumulating that fill in the render target.
	gl.Enable(GL_BLEND);
	// The RGB channel factors copy over destination alpha to destination RGB in order to accumulate
	// multiple filled polygons into the render target.
	// The alpha channel factors are what actually generate a (concave) polygon fill.
	gl.BlendFuncSeparate(
			// Accumulate destination alpha into destination RGB...
			GL_DST_ALPHA, GL_ONE,
			// Invert destination alpha every time a pixel is rendered (this means we get 1 where a
			// pixel is covered by an odd number of triangles and 0 by an even number of triangles)...
			GL_ONE_MINUS_DST_ALPHA, GL_ZERO);

	// Bind the shader program for rendering the surface fill mask.
	gl.UseProgram(d_surface_fill_mask_program);

	// Bind the surface fill mask vertex array.
	gl.BindVertexArray(d_surface_fill_mask_vertex_array);

	// Visitor to render surface fill mask geometries.
	SurfaceFillMaskGeometryOnSphereVisitor surface_fill_mask_visitor(
			gl,
			d_streaming_vertex_element_buffer,
			d_streaming_vertex_buffer,
			include_polylines);

	// Render the surface fill mask polygons (and optionally polylines).
	for (surface_polygons_mask_seq_type::const_iterator surface_geoms_iter = surface_polygons_mask.begin();
		surface_geoms_iter != surface_polygons_mask.end();
		++surface_geoms_iter)
	{
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &surface_geometry = *surface_geoms_iter;

		surface_geometry->accept_visitor(surface_fill_mask_visitor);
	}
}


void
GPlatesOpenGL::GLScalarField3D::render_volume_fill_depth_range(
		GL &gl,
		const GLViewProjection &view_projection,
		const surface_polygons_mask_seq_type &surface_polygons_mask,
		bool include_polylines)
{
	GL::StateScope save_restore_state(
			gl,
			// We're rendering to a render target so reset to the default OpenGL state...
			true/*reset_to_default_state*/);

	// The viewport of the screen we're rendering to.
	const GLViewport &screen_viewport = view_projection.get_viewport();

	// (Re)allocate the volume fill texture and depth buffer if the screen has been resized.
	if (d_volume_fill_screen_width != screen_viewport.width() ||
		d_volume_fill_screen_height != screen_viewport.height())
	{
		d_volume_fill_screen_width = screen_viewport.width();
		d_volume_fill_screen_height = screen_viewport.height();

		update_volume_fill_framebuffer(gl);
	}

	// Begin rendering to the volume fill texture.
	gl.BindFramebuffer(GL_FRAMEBUFFER, d_volume_fill_framebuffer);

	// Enable alpha-blending and set the RGB blend equation to GL_MIN and Alpha to GL_MAX.
	gl.Enable(GL_BLEND);
	gl.BlendEquationSeparate(GL_MIN/*modeRGB*/, GL_MAX/*modeAlpha*/);

	// Disable depth testing and depth writes because depth-testing would interfere with max blending.
	gl.Disable(GL_DEPTH_TEST);
	gl.DepthMask(GL_FALSE);

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
	gl.ClearColor(2.0f, 2.0f, 1.0f, -1.0f);
	// Also clear the depth buffer (even though we're not using it).
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// First render the inner sphere.
	render_inner_sphere_depth_range(gl);

	// Bind the shader program for rendering the volume fill depth range.
	gl.UseProgram(d_volume_fill_wall_depth_range_program);

	// Bind the volume fill boundary vertex array.
	gl.BindVertexArray(d_volume_fill_boundary_vertex_array);

	// Visitor to render the depth ranges of the volume fill region.
	VolumeFillBoundaryGeometryOnSphereVisitor volume_fill_boundary_visitor(
			gl,
			d_streaming_vertex_element_buffer,
			d_streaming_vertex_buffer,
			include_polylines);

	//
	// Render the wall depth ranges.
	//

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
}


void
GPlatesOpenGL::GLScalarField3D::render_volume_fill_walls(
		GL &gl,
		const GLViewProjection &view_projection,
		const surface_polygons_mask_seq_type &surface_polygons_mask,
		bool include_polylines,
		bool only_show_boundary_walls)
{
	GL::StateScope save_restore_state(
			gl,
			// We're rendering to a render target so reset to the default OpenGL state...
			true/*reset_to_default_state*/);

	// The viewport of the screen we're rendering to.
	const GLViewport &screen_viewport = view_projection.get_viewport();

	// (Re)allocate the volume fill texture and depth buffer if the screen has been resized.
	if (d_volume_fill_screen_width != screen_viewport.width() ||
		d_volume_fill_screen_height != screen_viewport.height())
	{
		d_volume_fill_screen_width = screen_viewport.width();
		d_volume_fill_screen_height = screen_viewport.height();

		update_volume_fill_framebuffer(gl);
	}

	// Begin rendering to the volume fill texture.
	gl.BindFramebuffer(GL_FRAMEBUFFER, d_volume_fill_framebuffer);

	// NOTE: We don't need alpha-blending because we're rendering surface normals (xyz) and depth (w),
	//       so there's no alpha.

	// Enable depth testing and depth writes.
	// NOTE: Depth writes must also be enabled for depth clears to work (same for colour buffers).
	gl.Enable(GL_DEPTH_TEST);
	gl.DepthMask(GL_TRUE);

	// Clear colour and depth buffers in render target.
	//
	// The colour buffer stores normals as (signed) floating-point.
	// An alpha value of 1.0 signifies (to the isosurface shader) that a wall is not present.
	// This is the screen-space (normalised device coordinates) depth (not window coordinates)
	// in the range [-1, 1] and 1 corresponds to the far clip plane.
	gl.ClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	gl.ClearDepth(); // Clear depth to 1.0
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Disable colour writes of the inner white sphere.
	// We just want to write the inner sphere depth values into the depth buffer so that
	// walls behind the inner sphere do not overwrite our default colour buffer values.
	gl.ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	// First render the inner sphere.
	render_white_inner_sphere(gl);

	// Re-enable colour writes.
	gl.ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	// Bind the surface fill mask texture.
	gl.ActiveTexture(GL_TEXTURE0 + Samplers::SURFACE_FILL_MASK_SAMPLER);
	gl.BindTexture(GL_TEXTURE_2D, d_surface_fill_mask_texture);

	// Bind the shader program for rendering the volume fill walls.
	gl.UseProgram(d_volume_fill_wall_surface_normal_program);

	// Bind the volume fill boundary vertex array.
	gl.BindVertexArray(d_volume_fill_boundary_vertex_array);

	// Visitor to render the walls of the volume fill region.
	VolumeFillBoundaryGeometryOnSphereVisitor volume_fill_walls_visitor(
			gl,
			d_streaming_vertex_element_buffer,
			d_streaming_vertex_buffer,
			include_polylines);

	//
	// Render the walls.
	//

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
}


void
GPlatesOpenGL::GLScalarField3D::render_cross_sections(
		GL &gl,
		const GLViewProjection &view_projection,
		cache_handle_type &cache_handle,
		const cross_sections_seq_type &cross_sections,
		GPlatesViewOperations::ScalarField3DRenderParameters::CrossSectionColourMode colour_mode,
		const GPlatesViewOperations::ScalarField3DRenderParameters::DepthRestriction &depth_restriction,
		const std::vector<float> &test_variables,
		boost::optional<SurfaceFillMask> surface_fill_mask)
{
	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(gl);

	// Update the uniform buffer containing named uniform blocks and bind its ranges to the block binding points.
	update_uniform_variables_for_cross_sections(
			gl, view_projection, colour_mode, depth_restriction, test_variables, surface_fill_mask);

	// First render the white inner sphere.
	// This is not needed when rendering an isosurface because the isosurface ray-tracer does that.
	render_white_inner_sphere(gl);

	if (surface_fill_mask)
	{
		// Render to the surface fill mask texture array (6 cube faces) using the surface geometries.
		render_surface_fill_mask(
				gl,
				surface_fill_mask->surface_polygons_mask,
				surface_fill_mask->treat_polylines_as_polygons);

		// Bind the surface fill mask texture.
		gl.ActiveTexture(GL_TEXTURE0 + Samplers::SURFACE_FILL_MASK_SAMPLER);
		gl.BindTexture(GL_TEXTURE_2D, d_surface_fill_mask_texture);
	}

	// Bind the tile metadata texture sampler.
	gl.ActiveTexture(GL_TEXTURE0 + Samplers::TILE_META_DATA_SAMPLER);
	gl.BindTexture(GL_TEXTURE_2D_ARRAY, d_tile_meta_data_texture_array);

	// Bind the field data texture sampler.
	gl.ActiveTexture(GL_TEXTURE0 + Samplers::FIELD_DATA_SAMPLER);
	gl.BindTexture(GL_TEXTURE_2D_ARRAY, d_field_data_texture_array);

	// Bind the mask data texture sampler.
	gl.ActiveTexture(GL_TEXTURE0 + Samplers::MASK_DATA_SAMPLER);
	gl.BindTexture(GL_TEXTURE_2D_ARRAY, d_mask_data_texture_array);

	// Bind the 1D depth radius to layer texture sampler.
	gl.ActiveTexture(GL_TEXTURE0 + Samplers::DEPTH_RADIUS_TO_LAYER_SAMPLER);
	gl.BindTexture(GL_TEXTURE_1D, d_depth_radius_to_layer_texture);

	// Bind the 1D colour palette texture sampler.
	gl.ActiveTexture(GL_TEXTURE0 + Samplers::COLOUR_PALETTE_SAMPLER);
	gl.BindTexture(GL_TEXTURE_1D, d_colour_palette_texture);

	// Bind the cross-section vertex array.
	gl.BindVertexArray(d_cross_section_vertex_array);

	// Line anti-aliasing shouldn't be on, but turn it off to be sure.
	gl.Disable(GL_LINE_SMOOTH);
	// Ensure lines are single-pixel wide.
	gl.LineWidth(1);

	// NOTE: We don't need alpha-blending because the cross-section shader program does alpha cutouts
	//       where alpha [0,0.5] -> 0.0 (and is discarded) and alpha [0.5,1] -> 1.0 (and rgb is
	//       un-premultiplied from the premultiplied alpha value sampled from 1D colour texture).

	// Surface points/multi-points are vertically extruded to create 1D lines.
	render_cross_sections_1d(
			gl,
			d_streaming_vertex_element_buffer,
			d_streaming_vertex_buffer,
			cross_sections);

	// Surface polylines/polygons are vertically extruded to create 2D triangular meshes.
	render_cross_sections_2d(
			gl,
			d_streaming_vertex_element_buffer,
			d_streaming_vertex_buffer,
			cross_sections);
}


void
GPlatesOpenGL::GLScalarField3D::render_cross_sections_1d(
		GL &gl,
		const GLStreamBuffer::shared_ptr_type &streaming_vertex_element_buffer,
		const GLStreamBuffer::shared_ptr_type &streaming_vertex_buffer,
		const cross_sections_seq_type &cross_sections)
{
	// Bind the shader program for rendering 1D cross-sections.
	gl.UseProgram(d_cross_section_1d_program);

	// Visitor to render 1D cross-section geometries.
	CrossSection1DGeometryOnSphereVisitor cross_section_1d_visitor(
			gl,
			streaming_vertex_element_buffer,
			streaming_vertex_buffer);

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
		GL &gl,
		const GLStreamBuffer::shared_ptr_type &streaming_vertex_element_buffer,
		const GLStreamBuffer::shared_ptr_type &streaming_vertex_buffer,
		const cross_sections_seq_type &cross_sections)
{
	// Bind the shader program for rendering 2D cross-sections.
	gl.UseProgram(d_cross_section_2d_program);

	// Visitor to render 2D cross-section geometries.
	CrossSection2DGeometryOnSphereVisitor cross_section_2d_visitor(
			gl,
			streaming_vertex_element_buffer,
			streaming_vertex_buffer);

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
GPlatesOpenGL::GLScalarField3D::render_white_inner_sphere(
		GL &gl)
{
	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(gl);

	gl.UseProgram(d_white_inner_sphere_program);

	// Bind vertex array object.
	gl.BindVertexArray(d_inner_sphere_vertex_array);

	// Render the inner sphere.
	glDrawElements(
			GL_TRIANGLES,
			d_inner_sphere_num_vertex_indices/*count*/,
			GLVertexUtils::ElementTraits<GLuint>::type,
			nullptr/*indices_offset*/);
}


void
GPlatesOpenGL::GLScalarField3D::render_inner_sphere_depth_range(
		GL &gl)
{
	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(gl);

	gl.UseProgram(d_depth_range_inner_sphere_program);

	// Bind vertex array object.
	gl.BindVertexArray(d_inner_sphere_vertex_array);

	// Render the inner sphere.
	glDrawElements(
			GL_TRIANGLES,
			d_inner_sphere_num_vertex_indices/*count*/,
			GLVertexUtils::ElementTraits<GLuint>::type,
			nullptr/*indices_offset*/);
}


void
GPlatesOpenGL::GLScalarField3D::initialise_uniform_buffer(
		GL &gl)
{
	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(gl);

	const GLCapabilities &capabilities = gl.get_capabilities();

	GLintptr bind_range_offset = 0;
	GLsizeiptr bind_range_size;

	//
	// Uniform block "CubeFaceCoordinateFrames" (only in "scalar_field_3d/utils.glsl").
	//
	// struct CubeFaceCoordinateFrame
	// {
	//     vec3 x_axis;
	//     vec3 y_axis;
	//     vec3 z_axis;
	// };
	// 
	// layout(std140) uniform CubeFaceCoordinateFrames
	// {
	//     CubeFaceCoordinateFrame cube_face_coordinate_frames[6];
	// };
	//

	struct CubeFaceCoordinateFrame
	{
		// Each vec3 uses 4 floats to keep them 16-byte aligned as required by "std140" layout - 4th float is unused.
		GLfloat xyz_axes[3][4];
	} uniform_buffer_data_cube_face_coordinate_frames[6];

	// Set the local coordinate frames of each cube face.
	// The order of faces is the same as the GPlatesMaths::CubeCoordinateFrame::CubeFaceType enumeration.
	//
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

			uniform_buffer_data_cube_face_coordinate_frames[face].xyz_axes[axis][0] = static_cast<GLfloat>(cube_face_axis.x().dval());
			uniform_buffer_data_cube_face_coordinate_frames[face].xyz_axes[axis][1] = static_cast<GLfloat>(cube_face_axis.y().dval());
			uniform_buffer_data_cube_face_coordinate_frames[face].xyz_axes[axis][2] = static_cast<GLfloat>(cube_face_axis.z().dval());
		}
	}

	bind_range_size = sizeof(uniform_buffer_data_cube_face_coordinate_frames);

	// Record the offset and size in the uniform buffer.
	d_uniform_buffer_ranges[UniformBlockBinding::CUBE_FACE_COORDINATE_FRAMES] = {bind_range_offset, bind_range_size};

	// Increment offset for the next range.
	bind_range_offset += bind_range_size;
	// Make sure it's aligned correctly.
	if (bind_range_offset % capabilities.gl_uniform_buffer_offset_alignment)
	{
		bind_range_offset += capabilities.gl_uniform_buffer_offset_alignment -
				(bind_range_offset % capabilities.gl_uniform_buffer_offset_alignment);
	}

	//
	// Uniform block "ViewProjection".
	//
	// layout(std140) uniform ViewProjection
	// {
	//     // Combined view-projection transform, and inverse of view and projection transforms.
	//     mat4 view_projection;
	//     mat4 view_inverse;
	//     mat4 projection_inverse;
	//     
	//     // Viewport (used to convert window-space coordinates gl_FragCoord.xy to NDC space [-1,1]).
	//     vec4 viewport;
	// };
	//

	struct ViewProjection
	{
		GLfloat view_projection[16];
		GLfloat view_inverse[16];
		GLfloat projection_inverse[16];
		GLfloat viewport[4];
	};

	bind_range_size = sizeof(ViewProjection);

	// Record the offset and size in the uniform buffer.
	d_uniform_buffer_ranges[UniformBlockBinding::VIEW_PROJECTION] = { bind_range_offset, bind_range_size };

	// Increment offset for the next range.
	bind_range_offset += bind_range_size;
	// Make sure it's aligned correctly.
	if (bind_range_offset % capabilities.gl_uniform_buffer_offset_alignment)
	{
		bind_range_offset += capabilities.gl_uniform_buffer_offset_alignment -
				(bind_range_offset % capabilities.gl_uniform_buffer_offset_alignment);
	}

	//
	// Uniform block "ScalarField".
	//
	// layout(std140) uniform ScalarField
	// {
	// 	   // min/max scalar value across the entire scalar field
	// 	   vec2 min_max_scalar_value;
	// 	   
	// 	   // min/max gradient magnitude across the entire scalar field
	// 	   vec2 min_max_gradient_magnitude;
	// };
	//

	struct ScalarField
	{
		GLfloat min_max_scalar_value[2];
		GLfloat min_max_gradient_magnitude[2];
	};

	bind_range_size = sizeof(ScalarField);

	// Record the offset and size in the uniform buffer.
	d_uniform_buffer_ranges[UniformBlockBinding::SCALAR_FIELD] = { bind_range_offset, bind_range_size };

	// Increment offset for the next range.
	bind_range_offset += bind_range_size;
	// Make sure it's aligned correctly.
	if (bind_range_offset % capabilities.gl_uniform_buffer_offset_alignment)
	{
		bind_range_offset += capabilities.gl_uniform_buffer_offset_alignment -
				(bind_range_offset % capabilities.gl_uniform_buffer_offset_alignment);
	}

	//
	// Uniform block "Depth".
	//
	// layout(std140) uniform Depth
	// {
	//     // The actual minimum and maximum depth radii of the scalar field.
	//     vec2 min_max_depth_radius;
	//
	//     // The depth range rendering is restricted to.
	//     // If depth range is not restricted then this is the same as 'min_max_depth_radius'.
	//     // Also the following conditions hold:
	//     //	render_min_max_depth_radius_restriction.x >= min_max_depth_radius.x
	//     //	render_min_max_depth_radius_restriction.y <= min_max_depth_radius.y
	//     // ...in other words the depth range for rendering is always within the actual depth range.
	//     vec2 min_max_depth_radius_restriction;
	//
	//     // Number of depth layers in scalar field.
	//     int num_depth_layers;
	// };
	//

	struct Depth
	{
		GLfloat min_max_depth_radius[2];
		GLfloat min_max_depth_radius_restriction[2];
		boost::uint32_t num_depth_layers;
	};

	bind_range_size = sizeof(Depth);

	// Record the offset and size in the uniform buffer.
	d_uniform_buffer_ranges[UniformBlockBinding::DEPTH] = { bind_range_offset, bind_range_size };

	// Increment offset for the next range.
	bind_range_offset += bind_range_size;
	// Make sure it's aligned correctly.
	if (bind_range_offset % capabilities.gl_uniform_buffer_offset_alignment)
	{
		bind_range_offset += capabilities.gl_uniform_buffer_offset_alignment -
				(bind_range_offset % capabilities.gl_uniform_buffer_offset_alignment);
	}

	//
	// Uniform block "SurfaceFill".
	//     
	// layout(std140) uniform SurfaceFill
	// {
	//     // If using a surface mask to limit regions of scalar field to render
	//     // (e.g. Africa, see Static Polygons connected with Scalar Field).
	//     bool using_surface_fill_mask;
	//     
	//     // If this is *true* (and 'using_surface_fill_mask' is true) then we're rendering the walls of the extruded fill volume.
	//     // The volume fill (screen-size) texture contains the surface normal and depth of the walls.
	//     //
	//     // If this is *false* (and 'using_surface_fill_mask' is true) then we're using the min/max depth range of walls
	//     // of extruded fill volume to limit raytracing traversal (but we're not rendering the walls as visible).
	//     // The volume fill (screen-size) texture will contain the min/max depth range of the walls.
	//     // This enables us to reduce the length along each ray that is sampled/traversed.
	//     // We don't need this if the walls are going to be drawn because there are already good
	//     // optimisations in place to limit ray sampling based on the fact that the walls are opaque.
	//     bool show_volume_fill_walls;
	//     
	//     // Is true if we should only render walls extruded from the *boundary* of the 2D surface fill mask.
	//     bool only_show_boundary_volume_fill_walls;
	// };
	//

	struct SurfaceFill
	{
		boost::uint32_t using_surface_fill_mask;
		boost::uint32_t show_volume_fill_walls;
		boost::uint32_t only_show_boundary_volume_fill_walls;
	};

	bind_range_size = sizeof(SurfaceFill);

	// Record the offset and size in the uniform buffer.
	d_uniform_buffer_ranges[UniformBlockBinding::SURFACE_FILL] = { bind_range_offset, bind_range_size };

	// Increment offset for the next range.
	bind_range_offset += bind_range_size;
	// Make sure it's aligned correctly.
	if (bind_range_offset % capabilities.gl_uniform_buffer_offset_alignment)
	{
		bind_range_offset += capabilities.gl_uniform_buffer_offset_alignment -
			(bind_range_offset % capabilities.gl_uniform_buffer_offset_alignment);
	}

	//
	// Uniform block "Lighting".
	//
	// layout(std140) uniform Lighting
	// {
	//     bool enabled;
	//     float ambient;
	//     vec3 world_space_light_direction;
	// };
	//

	struct Lighting
	{
		boost::uint32_t enabled;
		GLfloat ambient;
		// "std140" layout requires the following vec3 to be aligned to 16 bytes...
		boost::uint32_t padding[2];
		GLfloat world_space_light_direction[4]; // last element unused
	};

	bind_range_size = sizeof(Lighting);

	// Record the offset and size in the uniform buffer.
	d_uniform_buffer_ranges[UniformBlockBinding::LIGHTING] = { bind_range_offset, bind_range_size };

	// Increment offset for the next range.
	bind_range_offset += bind_range_size;
	// Make sure it's aligned correctly.
	if (bind_range_offset % capabilities.gl_uniform_buffer_offset_alignment)
	{
		bind_range_offset += capabilities.gl_uniform_buffer_offset_alignment -
				(bind_range_offset % capabilities.gl_uniform_buffer_offset_alignment);
	}

	//
	// Uniform block "Test".
	//
	// layout(std140) uniform Test
	// {
	//     float variable[16];
	// };

	struct Test
	{
		// "std140" layout has a minimum alignment of vec4 for each scalar in an array of scalars.
		GLfloat variable[16][4];
	};

	bind_range_size = sizeof(Test);

	// Record the offset and size in the uniform buffer.
	d_uniform_buffer_ranges[UniformBlockBinding::TEST] = { bind_range_offset, bind_range_size };

	// Increment offset for the next range.
	bind_range_offset += bind_range_size;
	// Make sure it's aligned correctly.
	if (bind_range_offset % capabilities.gl_uniform_buffer_offset_alignment)
	{
		bind_range_offset += capabilities.gl_uniform_buffer_offset_alignment -
				(bind_range_offset % capabilities.gl_uniform_buffer_offset_alignment);
	}


	//
	// Allocate the uniform buffer (now that we know the full buffer size).
	//

	gl.BindBuffer(GL_UNIFORM_BUFFER, d_uniform_buffer);
	const GLsizeiptr uniform_buffer_size = bind_range_offset;
	glBufferData(GL_UNIFORM_BUFFER, uniform_buffer_size, nullptr, GL_DYNAMIC_DRAW);

	// Write the "CubeFaceCoordinateFrames" data (it never changes).
	auto bind_range_cube_face_coordinate_frames = d_uniform_buffer_ranges[UniformBlockBinding::CUBE_FACE_COORDINATE_FRAMES];
	glBufferSubData(
			GL_UNIFORM_BUFFER,
			bind_range_cube_face_coordinate_frames.first/*offset*/,
			bind_range_cube_face_coordinate_frames.second/*size*/,
			&uniform_buffer_data_cube_face_coordinate_frames[0]);
}


void
GPlatesOpenGL::GLScalarField3D::update_shared_uniform_buffer_blocks(
		GL &gl,
		const GLViewProjection &view_projection,
		const GPlatesViewOperations::ScalarField3DRenderParameters::DepthRestriction &depth_restriction,
		const std::vector<float> &test_variables,
		boost::optional<SurfaceFillMask> surface_fill_mask)
{
	//
	// Uniform block "CubeFaceCoordinateFrames" (only in "scalar_field_3d/utils.glsl").
	//

	// Bind the range in the uniform buffer.
	//
	// Note: The data has already been uploaded during initialisation (and it never changes).
	auto bind_cube_face_coordinate_frames_range = d_uniform_buffer_ranges[UniformBlockBinding::CUBE_FACE_COORDINATE_FRAMES];
	gl.BindBufferRange(
			GL_UNIFORM_BUFFER,
			UniformBlockBinding::CUBE_FACE_COORDINATE_FRAMES,
			d_uniform_buffer,
			bind_cube_face_coordinate_frames_range.first/*offset*/,
			bind_cube_face_coordinate_frames_range.second/*size*/);

	//
	// Uniform block "ViewProjection".
	//
	// layout(std140) uniform ViewProjection
	// {
	//     // Combined view-projection transform, and inverse of view and projection transforms.
	//     mat4 view_projection;
	//     mat4 view_inverse;
	//     mat4 projection_inverse;
	//     
	//     // Viewport (used to convert window-space coordinates gl_FragCoord.xy to NDC space [-1,1]).
	//     vec4 viewport;
	// } view_projection_block;
	//

	struct ViewProjection
	{
		GLfloat view_projection[16];
		GLfloat view_inverse[16];
		GLfloat projection_inverse[16];
		GLfloat viewport[4];
	} view_projection_block;

	// If the view and projection matrices are invertible (ie, not singular).
	if (view_projection.get_inverse_view_transform() &&
		view_projection.get_inverse_projection_transform())
	{
		// View-projection transform.
		view_projection.get_view_projection_transform().get_float_matrix(view_projection_block.view_projection);

		// View-projection transform.
		view_projection.get_inverse_view_transform()->get_float_matrix(view_projection_block.view_inverse);

		// View-projection transform.
		view_projection.get_inverse_projection_transform()->get_float_matrix(view_projection_block.projection_inverse);
	}
	else // view/projection matrices *not* invertible (this shouldn't happen with typical view/projection matrices) ...
	{
		// Log a warning (only once) and just use identity matrices (which will render incorrectly, but this should never happen).
		static bool warned = false;
		if (!warned)
		{
			qWarning() << "View or projection transform not invertible. Scalar field will not render correctly.";
			warned = true;
		}

		// View-projection transform.
		GLMatrix::IDENTITY.get_float_matrix(view_projection_block.view_projection);

		// View-projection transform.
		GLMatrix::IDENTITY.get_float_matrix(view_projection_block.view_inverse);

		// View-projection transform.
		GLMatrix::IDENTITY.get_float_matrix(view_projection_block.projection_inverse);
	}

	// Viewport.
	const GLViewport &viewport = view_projection.get_viewport();
	view_projection_block.viewport[0] = viewport.x();
	view_projection_block.viewport[1] = viewport.y();
	view_projection_block.viewport[2] = viewport.width();
	view_projection_block.viewport[3] = viewport.height();

	// Bind the range in the uniform buffer.
	auto bind_view_projection_block_range = d_uniform_buffer_ranges[UniformBlockBinding::VIEW_PROJECTION];
	gl.BindBufferRange(
			GL_UNIFORM_BUFFER,
			UniformBlockBinding::VIEW_PROJECTION,
			d_uniform_buffer,
			bind_view_projection_block_range.first/*offset*/,
			bind_view_projection_block_range.second/*size*/);

	// Update the data in the range in the uniform buffer.
	glBufferSubData(
			GL_UNIFORM_BUFFER,
			bind_view_projection_block_range.first/*offset*/,
			bind_view_projection_block_range.second/*size*/,
			&view_projection_block);

	//
	// Uniform block "ScalarField".
	//
	// layout(std140) uniform ScalarField
	// {
	// 	   // min/max scalar value across the entire scalar field
	// 	   vec2 min_max_scalar_value;
	// 	   
	// 	   // min/max gradient magnitude across the entire scalar field
	// 	   vec2 min_max_gradient_magnitude;
	// } scalar_field_block;
	//

	struct ScalarField
	{
		GLfloat min_max_scalar_value[2];
		GLfloat min_max_gradient_magnitude[2];
	} scalar_field_block;

	scalar_field_block.min_max_scalar_value[0] = d_scalar_min;
	scalar_field_block.min_max_scalar_value[1] = d_scalar_max;

	scalar_field_block.min_max_gradient_magnitude[0] = d_gradient_magnitude_min;
	scalar_field_block.min_max_gradient_magnitude[1] = d_gradient_magnitude_max;

	// Bind the range in the uniform buffer.
	auto bind_scalar_field_block_range = d_uniform_buffer_ranges[UniformBlockBinding::SCALAR_FIELD];
	gl.BindBufferRange(
			GL_UNIFORM_BUFFER,
			UniformBlockBinding::SCALAR_FIELD,
			d_uniform_buffer,
			bind_scalar_field_block_range.first/*offset*/,
			bind_scalar_field_block_range.second/*size*/);

	// Update the data in the range in the uniform buffer.
	glBufferSubData(
			GL_UNIFORM_BUFFER,
			bind_scalar_field_block_range.first/*offset*/,
			bind_scalar_field_block_range.second/*size*/,
			&scalar_field_block);

	//
	// Uniform block "Depth".
	//
	// layout(std140) uniform Depth
	// {
	//     // The actual minimum and maximum depth radii of the scalar field.
	//     vec2 min_max_depth_radius;
	//
	//     // The depth range rendering is restricted to.
	//     // If depth range is not restricted then this is the same as 'min_max_depth_radius'.
	//     // Also the following conditions hold:
	//     //	render_min_max_depth_radius_restriction.x >= min_max_depth_radius.x
	//     //	render_min_max_depth_radius_restriction.y <= min_max_depth_radius.y
	//     // ...in other words the depth range for rendering is always within the actual depth range.
	//     vec2 min_max_depth_radius_restriction;
	//
	//     // Number of depth layers in scalar field.
	//     int num_depth_layers;
	// } depth_block;
	//

	struct Depth
	{
		GLfloat min_max_depth_radius[2];
		GLfloat min_max_depth_radius_restriction[2];
		boost::uint32_t num_depth_layers;
	} depth_block;

	depth_block.min_max_depth_radius[0] = d_min_depth_layer_radius;
	depth_block.min_max_depth_radius[1] = d_max_depth_layer_radius;

	depth_block.min_max_depth_radius_restriction[0] = depth_restriction.min_depth_radius_restriction;
	depth_block.min_max_depth_radius_restriction[1] = depth_restriction.max_depth_radius_restriction;

	depth_block.num_depth_layers = d_num_depth_layers;

	// Bind the range in the uniform buffer.
	auto bind_depth_block_range = d_uniform_buffer_ranges[UniformBlockBinding::DEPTH];
	gl.BindBufferRange(
			GL_UNIFORM_BUFFER,
			UniformBlockBinding::DEPTH,
			d_uniform_buffer,
			bind_depth_block_range.first/*offset*/,
			bind_depth_block_range.second/*size*/);

	// Update the data in the range in the uniform buffer.
	glBufferSubData(
			GL_UNIFORM_BUFFER,
			bind_depth_block_range.first/*offset*/,
			bind_depth_block_range.second/*size*/,
			&depth_block);

	//
	// Uniform block "SurfaceFill".
	//     
	// layout(std140) uniform SurfaceFill
	// {
	//     // If using a surface mask to limit regions of scalar field to render
	//     // (e.g. Africa, see Static Polygons connected with Scalar Field).
	//     bool using_surface_fill_mask;
	//     
	//     // If this is *true* (and 'using_surface_fill_mask' is true) then we're rendering the walls of the extruded fill volume.
	//     // The volume fill (screen-size) texture contains the surface normal and depth of the walls.
	//     //
	//     // If this is *false* (and 'using_surface_fill_mask' is true) then we're using the min/max depth range of walls
	//     // of extruded fill volume to limit raytracing traversal (but we're not rendering the walls as visible).
	//     // The volume fill (screen-size) texture will contain the min/max depth range of the walls.
	//     // This enables us to reduce the length along each ray that is sampled/traversed.
	//     // We don't need this if the walls are going to be drawn because there are already good
	//     // optimisations in place to limit ray sampling based on the fact that the walls are opaque.
	//     bool show_volume_fill_walls;
	//     
	//     // Is true if we should only render walls extruded from the *boundary* of the 2D surface fill mask.
	//     bool only_show_boundary_volume_fill_walls;
	// } surface_fill_block;
	//

	struct SurfaceFill
	{
		boost::uint32_t using_surface_fill_mask;
		boost::uint32_t show_volume_fill_walls;
		boost::uint32_t only_show_boundary_volume_fill_walls;
	} surface_fill_block = { false, false, false };

	surface_fill_block.using_surface_fill_mask = bool(surface_fill_mask);
	if (surface_fill_mask)
	{
		surface_fill_block.show_volume_fill_walls = bool(surface_fill_mask->show_walls);
		if (surface_fill_mask->show_walls)
		{
			surface_fill_block.only_show_boundary_volume_fill_walls =
					surface_fill_mask->show_walls->only_boundary_walls;
		}
	}

	// Bind the range in the uniform buffer.
	auto bind_surface_fill_block_range = d_uniform_buffer_ranges[UniformBlockBinding::SURFACE_FILL];
	gl.BindBufferRange(
			GL_UNIFORM_BUFFER,
			UniformBlockBinding::SURFACE_FILL,
			d_uniform_buffer,
			bind_surface_fill_block_range.first/*offset*/,
			bind_surface_fill_block_range.second/*size*/);

	// Update the data in the range in the uniform buffer.
	glBufferSubData(
			GL_UNIFORM_BUFFER,
			bind_surface_fill_block_range.first/*offset*/,
			bind_surface_fill_block_range.second/*size*/,
			&surface_fill_block);

	//
	// Uniform block "Lighting".
	//
	// layout(std140) uniform Lighting
	// {
	//     bool enabled;
	//     float ambient;
	//     vec3 world_space_light_direction;
	// } lighting_block;
	//

	struct Lighting
	{
		boost::uint32_t enabled;
		GLfloat ambient;
		// "std140" layout requires the following vec3 to be aligned to 16 bytes...
		boost::uint32_t padding[2];
		GLfloat world_space_light_direction[4]; // last element unused
	} lighting_block;

	lighting_block.enabled = d_light->get_scene_lighting_parameters().is_lighting_enabled(
			GPlatesGui::SceneLightingParameters::LIGHTING_SCALAR_FIELD);

	if (lighting_block.enabled)
	{
		// Set the light ambient contribution.
		lighting_block.ambient = static_cast<GLfloat>(
				d_light->get_scene_lighting_parameters().get_ambient_light_contribution());

		// Set the world-space light direction.
		const GPlatesMaths::UnitVector3D &world_space_light_direction = d_light->get_globe_view_light_direction();
		lighting_block.world_space_light_direction[0] = static_cast<GLfloat>(world_space_light_direction.x().dval());
		lighting_block.world_space_light_direction[1] = static_cast<GLfloat>(world_space_light_direction.y().dval());
		lighting_block.world_space_light_direction[2] = static_cast<GLfloat>(world_space_light_direction.z().dval());
	}

	// Bind the range in the uniform buffer.
	auto bind_lighting_block_range = d_uniform_buffer_ranges[UniformBlockBinding::LIGHTING];
	gl.BindBufferRange(
			GL_UNIFORM_BUFFER,
			UniformBlockBinding::LIGHTING,
			d_uniform_buffer,
			bind_lighting_block_range.first/*offset*/,
			bind_lighting_block_range.second/*size*/);

	// Update the data in the range in the uniform buffer.
	glBufferSubData(
			GL_UNIFORM_BUFFER,
			bind_lighting_block_range.first/*offset*/,
			bind_lighting_block_range.second/*size*/,
			&lighting_block);

	//
	// Uniform block "Test".
	//
	// layout(std140) uniform Test
	// {
	//     float variable[16];
	// } test_block;

	struct Test
	{
		// "std140" layout has a minimum alignment of vec4 for each scalar in an array of scalars.
		GLfloat variable[16][4];
	} test_block;

	for (unsigned int variable_index = 0; variable_index < 16; ++variable_index)
	{
		if (variable_index < test_variables.size())
		{
			test_block.variable[variable_index][0] = test_variables[variable_index];
		}
		else
		{
			test_block.variable[variable_index][0] = 0;
		}

		// The first element of vec4 alignment is test variable, the rest is unused padding.
		test_block.variable[variable_index][1] = 0;
		test_block.variable[variable_index][2] = 0;
		test_block.variable[variable_index][3] = 0;
	}

	// Bind the range in the uniform buffer.
	auto bind_test_block_range = d_uniform_buffer_ranges[UniformBlockBinding::TEST];
	gl.BindBufferRange(
			GL_UNIFORM_BUFFER,
			UniformBlockBinding::TEST,
			d_uniform_buffer,
			bind_test_block_range.first/*offset*/,
			bind_test_block_range.second/*size*/);

	// Update the data in the range in the uniform buffer.
	glBufferSubData(
			GL_UNIFORM_BUFFER,
			bind_test_block_range.first/*offset*/,
			bind_test_block_range.second/*size*/,
			&test_block);
}


void
GPlatesOpenGL::GLScalarField3D::update_uniform_variables_for_iso_surface(
		GL &gl,
		const GLViewProjection &view_projection,
		GPlatesViewOperations::ScalarField3DRenderParameters::IsosurfaceDeviationWindowMode deviation_window_mode,
		GPlatesViewOperations::ScalarField3DRenderParameters::IsosurfaceColourMode colour_mode,
		const GPlatesViewOperations::ScalarField3DRenderParameters::IsovalueParameters &isovalue_parameters,
		const GPlatesViewOperations::ScalarField3DRenderParameters::DeviationWindowRenderOptions &deviation_window_render_options,
		const GPlatesViewOperations::ScalarField3DRenderParameters::DepthRestriction &depth_restriction,
		const GPlatesViewOperations::ScalarField3DRenderParameters::QualityPerformance &quality_performance,
		const std::vector<float> &test_variables,
		boost::optional<SurfaceFillMask> surface_fill_mask,
		boost::optional<GLTexture::shared_ptr_type> depth_read_texture)
{
	// All shader programs (not just the iso-surface shader program) share uniform variables in named uniform blocks
	// backed by a single uniform buffer.
	update_shared_uniform_buffer_blocks(gl, view_projection, depth_restriction, test_variables, surface_fill_mask);

	// Bind the iso-surface shader program so that subsequent glUniform* calls target it.
	gl.UseProgram(d_iso_surface_program);

	// Enable or disable reads from depth texture.
	glUniform1i(
			d_iso_surface_program->get_uniform_location("read_from_depth_texture"),
			bool(depth_read_texture));

	// Specify the colour mode.
	glUniform1i(
			d_iso_surface_program->get_uniform_location("colour_mode_depth"),
			colour_mode == GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_COLOUR_MODE_DEPTH);
	glUniform1i(
			d_iso_surface_program->get_uniform_location("colour_mode_isovalue"),
			colour_mode == GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_COLOUR_MODE_SCALAR);
	glUniform1i(
			d_iso_surface_program->get_uniform_location("colour_mode_gradient"),
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
	glUniform2f(
			d_iso_surface_program->get_uniform_location("min_max_colour_mapping_range"),
			min_colour_mapping_range,
			max_colour_mapping_range);

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
	glUniform3f(
			d_iso_surface_program->get_uniform_location("isovalue1"),
			emulated_isovalue_parameters.isovalue1,
			emulated_isovalue_parameters.isovalue1 - emulated_isovalue_parameters.lower_deviation1,
			emulated_isovalue_parameters.isovalue1 + emulated_isovalue_parameters.upper_deviation1);
// 	qDebug() << "isovalue1: "
// 		<< emulated_isovalue_parameters.isovalue1 << ", "
// 		<< emulated_isovalue_parameters.lower_deviation1 << ", "
// 		<< emulated_isovalue_parameters.upper_deviation1;

	// Set the parameters associated with isovalue 2.
	glUniform3f(
			d_iso_surface_program->get_uniform_location("isovalue2"),
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

	glUniform1f(
			d_iso_surface_program->get_uniform_location("opacity_deviation_surfaces"),
			emulated_deviation_window_render_options.opacity_deviation_surfaces);
// 	qDebug() << "opacity_deviation_surfaces: " << emulated_deviation_window_render_options.opacity_deviation_surfaces;
	glUniform1i(
			d_iso_surface_program->get_uniform_location("deviation_window_volume_rendering"),
			emulated_deviation_window_render_options.deviation_window_volume_rendering);
// 	qDebug() << "deviation_window_volume_rendering: " << emulated_deviation_window_render_options.deviation_window_volume_rendering;
	glUniform1f(
			d_iso_surface_program->get_uniform_location("opacity_deviation_window_volume_rendering"),
			emulated_deviation_window_render_options.opacity_deviation_window_volume_rendering);
// 	qDebug() << "opacity_deviation_window_volume_rendering: " << emulated_deviation_window_render_options.opacity_deviation_window_volume_rendering;
	glUniform1i(
			d_iso_surface_program->get_uniform_location("surface_deviation_window"),
			emulated_deviation_window_render_options.surface_deviation_window);
// 	qDebug() << "surface_deviation_window: " << emulated_deviation_window_render_options.surface_deviation_window;
	glUniform1f(
			d_iso_surface_program->get_uniform_location("surface_deviation_isoline_frequency"),
			emulated_deviation_window_render_options.surface_deviation_window_isoline_frequency);
// 	qDebug() << "surface_deviation_isoline_frequency: " << emulated_deviation_window_render_options.surface_deviation_window_isoline_frequency;

	//
	// Set the quality/performance options.
	//

	glUniform2f(
			d_iso_surface_program->get_uniform_location("sampling_rate"),
			// Distance between samples - and 2.0 is diameter of the globe...
			2.0f / quality_performance.sampling_rate,
			quality_performance.sampling_rate / 2.0f);
// 	qDebug() << "sampling_rate: "
// 		<< 2.0f / quality_performance.sampling_rate << ", "
// 		<< quality_performance.sampling_rate / 2.0f;
	glUniform1i(
			d_iso_surface_program->get_uniform_location("bisection_iterations"),
			quality_performance.bisection_iterations);
// 	qDebug() << "bisection_iterations: " << quality_performance.bisection_iterations;
}


void
GPlatesOpenGL::GLScalarField3D::update_uniform_variables_for_cross_sections(
		GL &gl,
		const GLViewProjection &view_projection,
		GPlatesViewOperations::ScalarField3DRenderParameters::CrossSectionColourMode colour_mode,
		const GPlatesViewOperations::ScalarField3DRenderParameters::DepthRestriction &depth_restriction,
		const std::vector<float> &test_variables,
		boost::optional<SurfaceFillMask> surface_fill_mask)
{
	// All shader programs (not just the iso-surface shader program) share uniform variables in named uniform blocks
	// backed by a single uniform buffer.
	update_shared_uniform_buffer_blocks(gl, view_projection, depth_restriction, test_variables, surface_fill_mask);

	// Get the min/max range of values used to map to colour whether that mapping is a look up
	// of the colour palette (eg, colouring by scalar value or gradient magnitude) or by using
	// a hard-wired mapping in the shader code. Currently there's only palette look ups for cross sections.
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

	// Need to update both the 1D and 2D cross section programs.
	for (auto cross_section_program : {d_cross_section_1d_program, d_cross_section_2d_program})
	{
		// Bind the cross-section shader program so that subsequent glUniform* calls target it.
		gl.UseProgram(cross_section_program);

		glUniform2f(
				cross_section_program->get_uniform_location("min_max_colour_mapping_range"),
				min_colour_mapping_range,
				max_colour_mapping_range);
		// Specify the colour mode.
		glUniform1i(
				cross_section_program->get_uniform_location("colour_mode_scalar"),
				colour_mode == GPlatesViewOperations::ScalarField3DRenderParameters::CROSS_SECTION_COLOUR_MODE_SCALAR);
		glUniform1i(
				cross_section_program->get_uniform_location("colour_mode_gradient"),
				colour_mode == GPlatesViewOperations::ScalarField3DRenderParameters::CROSS_SECTION_COLOUR_MODE_GRADIENT);
	}
}


void
GPlatesOpenGL::GLScalarField3D::bind_uniform_buffer_blocks_in_program(
		GL &gl,
		const GLProgram::shared_ptr_type &program)
{
	//
	// Bind each named uniform block that's active in the shader program.
	//

	if (program->is_active_uniform_block("CubeFaceCoordinateFrames"))
	{
		glUniformBlockBinding(
				program->get_resource_handle(),
				program->get_uniform_block_index("CubeFaceCoordinateFrames"),
				UniformBlockBinding::CUBE_FACE_COORDINATE_FRAMES);
	}

	if (program->is_active_uniform_block("ViewProjection"))
	{
		glUniformBlockBinding(
				program->get_resource_handle(),
				program->get_uniform_block_index("ViewProjection"),
				UniformBlockBinding::VIEW_PROJECTION);
	}

	if (program->is_active_uniform_block("ScalarField"))
	{
		glUniformBlockBinding(
				program->get_resource_handle(),
				program->get_uniform_block_index("ScalarField"),
				UniformBlockBinding::SCALAR_FIELD);
	}

	if (program->is_active_uniform_block("Depth"))
	{
		glUniformBlockBinding(
				program->get_resource_handle(),
				program->get_uniform_block_index("Depth"),
				UniformBlockBinding::DEPTH);
	}

	if (program->is_active_uniform_block("SurfaceFill"))
	{
		glUniformBlockBinding(
				program->get_resource_handle(),
				program->get_uniform_block_index("SurfaceFill"),
				UniformBlockBinding::SURFACE_FILL);
	}

	if (program->is_active_uniform_block("Lighting"))
	{
		glUniformBlockBinding(
				program->get_resource_handle(),
				program->get_uniform_block_index("Lighting"),
				UniformBlockBinding::LIGHTING);
	}

	if (program->is_active_uniform_block("Test"))
	{
		glUniformBlockBinding(
				program->get_resource_handle(),
				program->get_uniform_block_index("Test"),
				UniformBlockBinding::TEST);
	}
}


void
GPlatesOpenGL::GLScalarField3D::initialise_samplers_in_program(
		GL &gl,
		const GLProgram::shared_ptr_type &program)
{
	//
	// Specify texture unit of each sampler that's active in the shader program.
	//
	// Each sampler gets its own texture unit but not all samplers are active in every shader program.
	//

	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(gl);

	gl.UseProgram(program);

	const GLint tile_meta_data_sampler_location = program->get_uniform_location("tile_meta_data_sampler");
	if (tile_meta_data_sampler_location >= 0)
	{
		glUniform1i(tile_meta_data_sampler_location, Samplers::TILE_META_DATA_SAMPLER);
	}

	const GLint field_data_sampler_location = program->get_uniform_location("field_data_sampler");
	if (field_data_sampler_location >= 0)
	{
		glUniform1i(field_data_sampler_location, Samplers::FIELD_DATA_SAMPLER);
	}

	const GLint mask_data_sampler_location = program->get_uniform_location("mask_data_sampler");
	if (mask_data_sampler_location >= 0)
	{
		glUniform1i(mask_data_sampler_location, Samplers::MASK_DATA_SAMPLER);
	}

	const GLint depth_radius_to_layer_sampler_location = program->get_uniform_location("depth_radius_to_layer_sampler");
	if (depth_radius_to_layer_sampler_location >= 0)
	{
		glUniform1i(depth_radius_to_layer_sampler_location, Samplers::DEPTH_RADIUS_TO_LAYER_SAMPLER);
	}

	const GLint colour_palette_sampler_location = program->get_uniform_location("colour_palette_sampler");
	if (colour_palette_sampler_location >= 0)
	{
		glUniform1i(colour_palette_sampler_location, Samplers::COLOUR_PALETTE_SAMPLER);
	}

	const GLint depth_texture_sampler_location = program->get_uniform_location("depth_texture_sampler");
	if (depth_texture_sampler_location >= 0)
	{
		glUniform1i(depth_texture_sampler_location, Samplers::DEPTH_TEXTURE_SAMPLER);
	}

	const GLint surface_fill_mask_sampler_location = program->get_uniform_location("surface_fill_mask_sampler");
	if (surface_fill_mask_sampler_location >= 0)
	{
		glUniform1i(surface_fill_mask_sampler_location, Samplers::SURFACE_FILL_MASK_SAMPLER);
	}

	const GLint volume_fill_sampler_location = program->get_uniform_location("volume_fill_sampler");
	if (volume_fill_sampler_location >= 0)
	{
		glUniform1i(volume_fill_sampler_location, Samplers::VOLUME_FILL_SAMPLER);
	}
}


void
GPlatesOpenGL::GLScalarField3D::initialise_inner_sphere(
		GL &gl)
{
	//
	// Compile/link shader programs to render inner sphere as white and as screen-space depth.
	//

	d_white_inner_sphere_program =
			create_shader_program(
					gl,
					WHITE_INNER_SPHERE_VERTEX_SHADER_SOURCE,
					WHITE_INNER_SPHERE_FRAGMENT_SHADER_SOURCE);

	d_depth_range_inner_sphere_program =
			create_shader_program(
					gl,
					DEPTH_RANGE_INNER_SPHERE_VERTEX_SHADER_SOURCE,
					DEPTH_RANGE_INNER_SPHERE_FRAGMENT_SHADER_SOURCE);

	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(gl);

	//
	// Initialise some shader program uniform variables that don't change.
	//

	gl.UseProgram(d_white_inner_sphere_program);

	// Set the inner sphere colour (it's the same across the whole sphere).
	const GPlatesGui::Colour white = GPlatesGui::Colour::get_white();
	glUniform4f(
			d_white_inner_sphere_program->get_uniform_location("sphere_colour"),
			white.red(), white.green(), white.blue(), white.alpha());

	//
	// Create vertices that render the inner sphere.
	//

	// We'll stream vertices/indices into a std::vector.
	std::vector<GLVertexUtils::Vertex> vertices;
	std::vector<GLuint> vertex_elements;

	// Build the mesh vertices/indices.
	const unsigned int recursion_depth_to_generate_mesh = 4;
	SphereMeshBuilder sphere_mesh_builder(
			vertices,
			vertex_elements,
			recursion_depth_to_generate_mesh);
	GPlatesMaths::SphericalSubdivision::HierarchicalTriangularMeshTraversal htm;
	const unsigned int current_recursion_depth = 0;
	htm.visit(sphere_mesh_builder, current_recursion_depth);

	//
	// Load vertices into vertex array.
	//

	// Bind vertex array object.
	gl.BindVertexArray(d_inner_sphere_vertex_array);

	// Bind vertex element buffer object to currently bound vertex array object.
	gl.BindBuffer(GL_ELEMENT_ARRAY_BUFFER, d_inner_sphere_vertex_element_buffer);

	// Transfer vertex element data to currently bound vertex element buffer object.
	glBufferData(
			GL_ELEMENT_ARRAY_BUFFER,
			vertex_elements.size() * sizeof(vertex_elements[0]),
			vertex_elements.data(),
			GL_STATIC_DRAW);
	d_inner_sphere_num_vertex_indices = vertex_elements.size();

	// Bind vertex buffer object (used by vertex attribute arrays, not vertex array object).
	gl.BindBuffer(GL_ARRAY_BUFFER, d_inner_sphere_vertex_buffer);

	// Transfer vertex data to currently bound vertex buffer object.
	glBufferData(
			GL_ARRAY_BUFFER,
			vertices.size() * sizeof(vertices[0]),
			vertices.data(),
			GL_STATIC_DRAW);

	// Specify vertex attributes (position) in currently bound vertex buffer object.
	// This transfers each vertex attribute array (parameters + currently bound vertex buffer object)
	// to currently bound vertex array object.
	gl.EnableVertexAttribArray(0);
	gl.VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
			sizeof(GLVertexUtils::Vertex), BUFFER_OFFSET(GLVertexUtils::Vertex, x));
}


void
GPlatesOpenGL::GLScalarField3D::initialise_cross_section_rendering(
		GL &gl)
{
	//
	// Create the shader programs.
	//

	d_cross_section_1d_program =
			create_shader_program(
					gl,
					CROSS_SECTION_VERTEX_SHADER_SOURCE,
					CROSS_SECTION_FRAGMENT_SHADER_SOURCE_FILE_NAME,
					CROSS_SECTION_GEOMETRY_SHADER_SOURCE_FILE_NAME/*geometry shader*/,
					"#define CROSS_SECTION_1D\n");

	d_cross_section_2d_program =
			create_shader_program(
					gl,
					CROSS_SECTION_VERTEX_SHADER_SOURCE,
					CROSS_SECTION_FRAGMENT_SHADER_SOURCE_FILE_NAME,
					CROSS_SECTION_GEOMETRY_SHADER_SOURCE_FILE_NAME/*geometry shader*/,
					"#define CROSS_SECTION_2D\n");

	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(gl);

	//
	// Initialise the vertex array for rendering cross-section geometry.
	//

	// Bind vertex array object.
	gl.BindVertexArray(d_cross_section_vertex_array);

	// Bind vertex element buffer object to currently bound vertex array object.
	gl.BindBuffer(GL_ELEMENT_ARRAY_BUFFER, d_streaming_vertex_element_buffer->get_buffer());

	// Bind vertex buffer object (used by vertex attribute arrays, not vertex array object).
	gl.BindBuffer(GL_ARRAY_BUFFER, d_streaming_vertex_buffer->get_buffer());

	// Specify vertex attributes in currently bound vertex buffer object.
	// This transfers each vertex attribute array (parameters + currently bound vertex buffer object)
	// to currently bound vertex array object.
	//
	// The "surface_point" attribute data is index 0, and a 'vec3' vertex attribute.
	gl.EnableVertexAttribArray(0);
	gl.VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
			sizeof(CrossSectionVertex), BUFFER_OFFSET(CrossSectionVertex, surface_point[0]));
}


void
GPlatesOpenGL::GLScalarField3D::initialise_iso_surface_rendering(
		GL &gl)
{
	//
	// Create the shader program.
	//

	d_iso_surface_program =
			create_shader_program(
					gl,
					ISO_SURFACE_VERTEX_SHADER_SOURCE,
					ISO_SURFACE_FRAGMENT_SHADER_SOURCE_FILE_NAME);
}


void
GPlatesOpenGL::GLScalarField3D::initialise_surface_fill_mask_rendering(
		GL &gl)
{
	const GLCapabilities &capabilities = gl.get_capabilities();

	//
	// Create the shader program.
	//

	d_surface_fill_mask_program =
			create_shader_program(
					gl,
					SURFACE_FILL_MASK_VERTEX_SHADER_SOURCE,
					SURFACE_FILL_MASK_FRAGMENT_SHADER_SOURCE,
					QString(SURFACE_FILL_MASK_GEOMETRY_SHADER_SOURCE));

	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(gl);

	//
	// Initialise some shader program uniform variables for rendering surface fill mask that never change.
	//

	gl.UseProgram(d_surface_fill_mask_program);

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
		GLMatrix cube_face_view_projection_matrix = projection_transform->get_matrix();
		cube_face_view_projection_matrix.gl_mult_matrix(view_transform->get_matrix());

		// Set the view-projection matrix in the shader program.
		// It never changes so we just set it once here.

		GLfloat cube_face_view_projection_float_matrix[16];
		cube_face_view_projection_matrix.get_float_matrix(cube_face_view_projection_float_matrix);
		glUniformMatrix4fv(
				d_surface_fill_mask_program->get_uniform_location(
						QString("cube_face_view_projection_matrices[%1]").arg(face).toLatin1().constData()),
				1, GL_FALSE/*transpose*/, cube_face_view_projection_float_matrix);
	}

	//
	// Initialise the vertex array for rendering surface fill mask.
	//

	// Bind vertex array object.
	gl.BindVertexArray(d_surface_fill_mask_vertex_array);

	// Bind vertex element buffer object to currently bound vertex array object.
	gl.BindBuffer(GL_ELEMENT_ARRAY_BUFFER, d_streaming_vertex_element_buffer->get_buffer());

	// Bind vertex buffer object (used by vertex attribute arrays, not vertex array object).
	gl.BindBuffer(GL_ARRAY_BUFFER, d_streaming_vertex_buffer->get_buffer());

	// Specify vertex attributes in currently bound vertex buffer object.
	// This transfers each vertex attribute array (parameters + currently bound vertex buffer object)
	// to currently bound vertex array object.
	//
	// The "surface_point" attribute data is index 0, and is the surface point packed in the
	// (x,y,z) components of 'vec4' vertex attribute.
	gl.EnableVertexAttribArray(0);
	gl.VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
			sizeof(SurfaceFillMaskVertex), BUFFER_OFFSET(SurfaceFillMaskVertex, surface_point[0]));
}


void
GPlatesOpenGL::GLScalarField3D::initialise_volume_fill_boundary_rendering(
		GL &gl)
{
	//
	// Create the shader programs.
	//

	d_volume_fill_wall_depth_range_program =
			create_shader_program(
					gl,
					VOLUME_FILL_WALL_VERTEX_SHADER_SOURCE,
					VOLUME_FILL_WALL_DEPTH_RANGE_FRAGMENT_SHADER_SOURCE,
					VOLUME_FILL_WALL_GEOMETRY_SHADER_SOURCE_FILE_NAME,
					"#define DEPTH_RANGE\n");

	d_volume_fill_wall_surface_normal_program =
			create_shader_program(
					gl,
					VOLUME_FILL_WALL_VERTEX_SHADER_SOURCE,
					VOLUME_FILL_WALL_SURFACE_NORMAL_FRAGMENT_SHADER_SOURCE_FILE_NAME,
					VOLUME_FILL_WALL_GEOMETRY_SHADER_SOURCE_FILE_NAME,
					"#define SURFACE_NORMAL_AND_DEPTH\n");

	//
	// Initialise the vertex array for rendering volume fill boundary.
	//

	// Bind vertex array object.
	gl.BindVertexArray(d_volume_fill_boundary_vertex_array);

	// Bind vertex element buffer object to currently bound vertex array object.
	gl.BindBuffer(GL_ELEMENT_ARRAY_BUFFER, d_streaming_vertex_element_buffer->get_buffer());

	// Bind vertex buffer object (used by vertex attribute arrays, not vertex array object).
	gl.BindBuffer(GL_ARRAY_BUFFER, d_streaming_vertex_buffer->get_buffer());

	// Specify vertex attributes in currently bound vertex buffer object.
	// This transfers each vertex attribute array (parameters + currently bound vertex buffer object)
	// to currently bound vertex array object.
	//
	// The "surface_point" attribute data is index 0, and is the surface point packed in the
	// (x,y,z) components of 'vec4' vertex attribute.
	gl.EnableVertexAttribArray(0);
	gl.VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
			sizeof(VolumeFillBoundaryVertex), BUFFER_OFFSET(VolumeFillBoundaryVertex, surface_point[0]));
}


GPlatesOpenGL::GLProgram::shared_ptr_type
GPlatesOpenGL::GLScalarField3D::create_shader_program(
		GL &gl,
		const QString &vertex_shader_source_string_or_file_name,
		const QString &fragment_shader_source_string_or_file_name,
		boost::optional<QString> geometry_shader_source_string_or_file_name,
		boost::optional<const char *> shader_defines)
{
	// Add this scope to the call stack trace printed if exception thrown in this scope (eg, failure to compile/link shader).
	TRACK_CALL_STACK();

	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(gl);

	// Program object.
	GLProgram::shared_ptr_type program = GLProgram::create(gl);

	//
	// Vertex shader
	//

	// Vertex shader source.
	GLShaderSource vertex_shader_source;
	// Add the '#define' statements first.
	if (shader_defines)
	{
		vertex_shader_source.add_code_segment(shader_defines.get());
	}
	// Add the general utilities.
	vertex_shader_source.add_code_segment_from_file(GLShaderSource::UTILS_FILE_NAME);
	// Add the scalar field utilities.
	vertex_shader_source.add_code_segment_from_file(SCALAR_FIELD_UTILS_SOURCE_FILE_NAME);
	//  Add the actual source code either from an embedded resource file (starts with ":/") or directly from string.
	if (vertex_shader_source_string_or_file_name.startsWith(":/"))
	{
		vertex_shader_source.add_code_segment_from_file(vertex_shader_source_string_or_file_name);
	}
	else
	{
		vertex_shader_source.add_code_segment(vertex_shader_source_string_or_file_name.toUtf8());
	}

	// Vertex shader.
	GLShader::shared_ptr_type vertex_shader = GLShader::create(gl, GL_VERTEX_SHADER);
	vertex_shader->shader_source(vertex_shader_source);
	vertex_shader->compile_shader();

	// Attach shader to program.
	program->attach_shader(vertex_shader);

	//
	// Geometry shader (optional)
	//

	if (geometry_shader_source_string_or_file_name)
	{
		// Geometry shader source.
		GLShaderSource geometry_shader_source;
		// Add the '#define' statements first.
		if (shader_defines)
		{
			geometry_shader_source.add_code_segment(shader_defines.get());
		}
		// Add the general utilities.
		geometry_shader_source.add_code_segment_from_file(GLShaderSource::UTILS_FILE_NAME);
		// Add the scalar field utilities.
		geometry_shader_source.add_code_segment_from_file(SCALAR_FIELD_UTILS_SOURCE_FILE_NAME);
		//  Add the actual source code either from an embedded resource file (starts with ":/") or directly from string.
		if (geometry_shader_source_string_or_file_name->startsWith(":/"))
		{
			geometry_shader_source.add_code_segment_from_file(geometry_shader_source_string_or_file_name.get());
		}
		else
		{
			geometry_shader_source.add_code_segment(geometry_shader_source_string_or_file_name->toUtf8());
		}

		// Geometry shader.
		GLShader::shared_ptr_type geometry_shader = GLShader::create(gl, GL_GEOMETRY_SHADER);
		geometry_shader->shader_source(geometry_shader_source);
		geometry_shader->compile_shader();

		// Attach shader to program.
		program->attach_shader(geometry_shader);
	}

	//
	// Fragment shader
	//

	// Fragment shader source.
	GLShaderSource fragment_shader_source;
	// Add the '#define' statements first.
	if (shader_defines)
	{
		fragment_shader_source.add_code_segment(shader_defines.get());
	}
	// Add the general utilities.
	fragment_shader_source.add_code_segment_from_file(GLShaderSource::UTILS_FILE_NAME);
	// Add the scalar field utilities.
	fragment_shader_source.add_code_segment_from_file(SCALAR_FIELD_UTILS_SOURCE_FILE_NAME);
	//  Add the actual source code either from an embedded resource file (starts with ":/") or directly from string.
	if (fragment_shader_source_string_or_file_name.startsWith(":/"))
	{
		fragment_shader_source.add_code_segment_from_file(fragment_shader_source_string_or_file_name);
	}
	else
	{
		fragment_shader_source.add_code_segment(fragment_shader_source_string_or_file_name.toUtf8());
	}

	// Fragment shader.
	GLShader::shared_ptr_type fragment_shader = GLShader::create(gl, GL_FRAGMENT_SHADER);
	fragment_shader->shader_source(fragment_shader_source);
	fragment_shader->compile_shader();

	// Attach shader to program.
	program->attach_shader(fragment_shader);

	// Finally link the program.
	program->link_program();

	// Specify texture unit of each sampler that's active in the program.
	initialise_samplers_in_program(gl, program);

	// Bind any named uniform blocks used in the program to their uniform buffer binding points.
	bind_uniform_buffer_blocks_in_program(gl, program);

	return program;
}


void
GPlatesOpenGL::GLScalarField3D::create_tile_meta_data_texture_array(
		GL &gl)
{
	const GLCapabilities &capabilities = gl.get_capabilities();

	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(gl);

	gl.BindTexture(GL_TEXTURE_2D_ARRAY, d_tile_meta_data_texture_array);

	// Using nearest-neighbour filtering since don't want to filter pixel metadata.
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Clamp texture coordinates to centre of edge texels.
	// Not strictly necessary for nearest-neighbour filtering.
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Create the texture but don't load any data into it.
	//
	// NOTE: Since the image data is NULL it doesn't really matter what 'format' and 'type' are -
	// just use values that are compatible with all internal formats to avoid a possible error.

	glTexImage3D(
			GL_TEXTURE_2D_ARRAY, 0, GL_RGB32F,
			d_tile_meta_data_resolution,
			d_tile_meta_data_resolution,
			6, // One layer per cube face.
			0, GL_RGB, GL_FLOAT, nullptr);

	// Check there are no OpenGL errors.
	GLUtils::check_gl_errors(GPLATES_ASSERTION_SOURCE);
}


void
GPlatesOpenGL::GLScalarField3D::create_field_data_texture_array(
		GL &gl)
{
	const GLCapabilities &capabilities = gl.get_capabilities();

	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(gl);

	gl.BindTexture(GL_TEXTURE_2D_ARRAY, d_field_data_texture_array);

	// Using linear filtering.
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Clamp texture coordinates to centre of edge texels.
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Create the texture but don't load any data into it.
	//
	// NOTE: Since the image data is nullptr it doesn't really matter what 'format' and 'type' are -
	// just use values that are compatible with all internal formats to avoid a possible error.

	glTexImage3D(
			GL_TEXTURE_2D_ARRAY, 0, GL_RGBA32F,
			d_tile_resolution,
			d_tile_resolution,
			d_num_active_tiles * d_num_depth_layers,
			0, GL_RGBA, GL_FLOAT, nullptr);

	// Check there are no OpenGL errors.
	GLUtils::check_gl_errors(GPLATES_ASSERTION_SOURCE);
}


void
GPlatesOpenGL::GLScalarField3D::create_mask_data_texture_array(
		GL &gl)
{
	const GLCapabilities &capabilities = gl.get_capabilities();

	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(gl);

	gl.BindTexture(GL_TEXTURE_2D_ARRAY, d_mask_data_texture_array);

	// Using linear filtering.
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Clamp texture coordinates to centre of edge texels.
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Create the texture but don't load any data into it.
	//
	// NOTE: Since the image data is NULL it doesn't really matter what 'format' and 'type' are -
	// just use values that are compatible with all internal formats to avoid a possible error.

	glTexImage3D(
			GL_TEXTURE_2D_ARRAY, 0, GL_R32F,
			d_tile_resolution,
			d_tile_resolution,
			d_num_active_tiles,
			0, GL_RED, GL_FLOAT, nullptr);

	// Check there are no OpenGL errors.
	GLUtils::check_gl_errors(GPLATES_ASSERTION_SOURCE);
}


void
GPlatesOpenGL::GLScalarField3D::create_depth_radius_to_layer_texture(
		GL &gl)
{
	const GLCapabilities &capabilities = gl.get_capabilities();

	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(gl);

	gl.BindTexture(GL_TEXTURE_1D, d_depth_radius_to_layer_texture);

	// Using linear filtering.
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Clamp texture coordinates to centre of edge texels.
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

	// Create the texture but don't load any data into it.
	//
	// NOTE: Since the image data is nullptr it doesn't really matter what 'format' and 'type' are -
	// just use values that are compatible with all internal formats to avoid a possible error.

	glTexImage1D(
			GL_TEXTURE_1D, 0, GL_R32F,
			d_depth_radius_to_layer_resolution,
			0, GL_RED, GL_FLOAT, nullptr);

	// Check there are no OpenGL errors.
	GLUtils::check_gl_errors(GPLATES_ASSERTION_SOURCE);
}


void
GPlatesOpenGL::GLScalarField3D::create_colour_palette_texture(
		GL &gl)
{
	const GLCapabilities &capabilities = gl.get_capabilities();

	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(gl);

	gl.BindTexture(GL_TEXTURE_1D, d_colour_palette_texture);

	// Using linear filtering.
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Clamp texture coordinates to centre of edge texels.
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

	// Create the texture but don't load any data into it.
	//
	// NOTE: Since the image data is NULL it doesn't really matter what 'format' and 'type' are -
	// just use values that are compatible with all internal formats to avoid a possible error.

	glTexImage1D(
			GL_TEXTURE_1D, 0, GL_RGBA32F,
			d_colour_palette_resolution,
			0, GL_RGBA, GL_FLOAT, nullptr);

	// Check there are no OpenGL errors.
	GLUtils::check_gl_errors(GPLATES_ASSERTION_SOURCE);
}


void
GPlatesOpenGL::GLScalarField3D::create_surface_fill_mask_texture(
		GL &gl)
{
	const GLCapabilities &capabilities = gl.get_capabilities();

	const unsigned int texture_depth = 6;

	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(gl);

	gl.BindTexture(GL_TEXTURE_2D_ARRAY, d_surface_fill_mask_texture);

	// Linear filtering.
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Clamp texture coordinates to centre of edge texels.
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Create the texture but don't load any data into it.
	//
	// NOTE: Since the image data is NULL it doesn't really matter what 'format' and 'type' are -
	// just use values that are compatible with all internal formats to avoid a possible error.

	glTexImage3D(
			// Note: We use a fixed-point internal format (which clamps to [0,1]) instead of
			//       floating-point (which is un-clamped) such that our rendered surface mask texture
			//       will have an accumulated value of 1.0 where two polygons overlap (instead of 2.0).
			//       And that means we can use a value of 0.5 to separate areas covered by the surface
			//       mask from areas not covered (noting that surface mask is bilinearly filtered).
			GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8,
			d_surface_fill_mask_resolution,
			d_surface_fill_mask_resolution,
			6,  // texture depth is 6 cube faces
			0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	// Check there are no OpenGL errors.
	GLUtils::check_gl_errors(GPLATES_ASSERTION_SOURCE);
}


void
GPlatesOpenGL::GLScalarField3D::create_surface_fill_mask_framebuffer(
		GL &gl)
{
	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(gl);

	gl.BindFramebuffer(GL_FRAMEBUFFER, d_surface_fill_mask_framebuffer);

	// Bind surface fill mask texture to framebuffer's first colour attachment for rendering
	// to the entire texture array (layered texture rendering).
	// We will be using a geometry shader to direct each filled primitive to all six layers of the texture array.
	//
	// Note that *level* 0 is the base mipmap level of each 2D texture in array (not the layer).
	gl.FramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, d_surface_fill_mask_texture, 0/*level*/);

	const GLenum completeness = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	GPlatesGlobal::Assert<OpenGLException>(
			completeness == GL_FRAMEBUFFER_COMPLETE,
			GPLATES_ASSERTION_SOURCE,
			"Framebuffer not complete for rendering to surface fill mask texture.");
}


void
GPlatesOpenGL::GLScalarField3D::create_volume_fill_texture_sampler(
		GL &gl)
{
	const GLuint volume_fill_texture_sampler = d_volume_fill_texture_sampler->get_resource_handle();

	// Use nearest filtering since this a full-screen texture (full-screen usually point sampled).
	glSamplerParameteri(volume_fill_texture_sampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glSamplerParameteri(volume_fill_texture_sampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Clamp texture coordinates to centre of edge texels -
	// it's easier for hardware to implement - and doesn't affect our calculations.
	glSamplerParameteri(volume_fill_texture_sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(volume_fill_texture_sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}


void
GPlatesOpenGL::GLScalarField3D::update_volume_fill_framebuffer(
		GL &gl)
{
	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(gl);

	// (Re)allocate the texture to the current screen dimensions.
	//
	// We use the same texture for rendering volume fill walls (surface normals) and volume fill depth range.
	// To satisfy both we use an RGBA floating-point texture.
	//
	// For rendering volume fill walls (surface normals)...
	// Normally an 8-bit texture is enough to store surface normals.
	// However we're also storing screen-space depth in the alpha channel and that requires
	// more precision so we'll make the entire RGBA floating-point.
	//
	// For rendering volume fill depth range...
	// We only really need a two-channel RG texture, but alpha-blending min/max can only have separate
	// blend equations for RGB and Alpha (not R and G).
	// So two channels contain min/max depth and one channel contains flag indicating volume intersection.
	gl.BindTexture(GL_TEXTURE_2D, d_volume_fill_texture);
	glTexImage2D(GL_TEXTURE_2D, 0/*level*/, GL_RGBA32F,
			d_volume_fill_screen_width, d_volume_fill_screen_height,
			0, GL_RGBA, GL_FLOAT, nullptr);

	// (Re)allocate the depth buffer to the current screen dimensions.
	//
	// For rendering volume fill walls (surface normals)...
	// We need a depth buffer when rendering the walls (surface normals), otherwise we are not
	// guaranteed to get the closest wall at each screen pixel.
	//
	// For rendering volume fill depth range...
	// We don't need a depth buffer when rendering the min/max depths using min/max alpha-blending.
	// In fact a depth buffer (with depth-testing enabled) would interfere with max blending.
	// But it's needed for rendering volume fill walls (surface normals).
	gl.BindRenderbuffer(GL_RENDERBUFFER, d_volume_fill_depth_buffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24/*a required format*/,
			d_volume_fill_screen_width, d_volume_fill_screen_height);

	// Bind the volume fill texture and depth buffer to the framebuffer object.
	gl.BindFramebuffer(GL_FRAMEBUFFER, d_volume_fill_framebuffer);
	gl.FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, d_volume_fill_depth_buffer);
	gl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, d_volume_fill_texture, 0/*level*/);

	const GLenum completeness = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	GPlatesGlobal::Assert<OpenGLException>(
			completeness == GL_FRAMEBUFFER_COMPLETE,
			GPLATES_ASSERTION_SOURCE,
			"Framebuffer not complete for rendering to volume fill texture.");
}


void
GPlatesOpenGL::GLScalarField3D::load_scalar_field(
		GL &gl,
		const GPlatesFileIO::ScalarField3DFileFormat::Reader &scalar_field_reader)
{
	// Load the depth-radius-to-layer 1D texture mapping.
	load_depth_radius_to_layer_texture(gl);

	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(gl);

	//
	// Read the tile metadata from the file.
	//

	gl.BindTexture(GL_TEXTURE_2D_ARRAY, d_tile_meta_data_texture_array);

	// This is a relatively small amount of data so we don't need to worry about memory usage.
	boost::shared_array<GPlatesFileIO::ScalarField3DFileFormat::TileMetaData>
			tile_meta_data = scalar_field_reader.read_tile_meta_data();

	// Upload the tile metadata into the texture array.
	glTexSubImage3D(
			GL_TEXTURE_2D_ARRAY, 0,
			0, 0, 0, // x,y,z offsets
			scalar_field_reader.get_tile_meta_data_resolution(),
			scalar_field_reader.get_tile_meta_data_resolution(),
			6, // One layer per cube face.
			GL_RGB, GL_FLOAT, tile_meta_data.get());

	//
	// Read the field data from the file.
	//

	gl.BindTexture(GL_TEXTURE_2D_ARRAY, d_field_data_texture_array);

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
		glTexSubImage3D(
				GL_TEXTURE_2D_ARRAY, 0,
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

	gl.BindTexture(GL_TEXTURE_2D_ARRAY, d_mask_data_texture_array);

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
		glTexSubImage3D(
				GL_TEXTURE_2D_ARRAY, 0,
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
		GL &gl)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_depth_layer_radii.size() == d_num_depth_layers,
			GPLATES_ASSERTION_SOURCE);

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
	for (unsigned int texel = 1; texel < d_depth_radius_to_layer_resolution - 1; ++texel)
	{
		// Convert texel index into depth radius.
		const double depth_radius = d_min_depth_layer_radius +
				(d_max_depth_layer_radius - d_min_depth_layer_radius) *
					(double(texel) / (d_depth_radius_to_layer_resolution - 1));

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

	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(gl);

	gl.BindTexture(GL_TEXTURE_1D, d_depth_radius_to_layer_texture);

	// Upload the depth-radius-to-layer mapping data into the texture.
	glTexSubImage1D(
			GL_TEXTURE_1D, 0,
			0, // x offset
			d_depth_radius_to_layer_resolution, // width
			GL_RED, GL_FLOAT, &depth_layer_mapping[0]);
}


void
GPlatesOpenGL::GLScalarField3D::load_colour_palette_texture(
		GL &gl,
		const GPlatesGui::ColourPalette<double>::non_null_ptr_to_const_type &colour_palette,
		const std::pair<double, double> &colour_palette_value_range)
{
	// The colours for the colour palette texture.
	std::vector<GPlatesGui::Colour> colour_palette_texels;

	// Iterate over texels.
	unsigned int texel;
	for (texel = 0; texel < d_colour_palette_resolution; ++texel)
	{
		// Map the current texel to the colour palette input value range.
		const double colour_palette_value = colour_palette_value_range.first +
				(colour_palette_value_range.second - colour_palette_value_range.first) *
					double(texel) / (d_colour_palette_resolution - 1);

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
		}

		colour_palette_texels.push_back(colour.get());
	}

	// The RGBA texels of the colour palette data.
	std::vector<GLfloat> colour_palette_data;
	colour_palette_data.reserve(4/*RGBA*/ * colour_palette_texels.size());

	// Convert colours to RGBA float data.
	for (texel = 0; texel < d_colour_palette_resolution; ++texel)
	{
		const GPlatesGui::Colour &texel_colour = colour_palette_texels[texel];

		// Premultiply alpha so that we don't have the issue of black RGB values (at transparent texels)
		// corrupting the linear texture filtering (when texture is accessed).
		colour_palette_data.push_back(texel_colour.red() * texel_colour.alpha());
		colour_palette_data.push_back(texel_colour.green() * texel_colour.alpha());
		colour_palette_data.push_back(texel_colour.blue() * texel_colour.alpha());

		colour_palette_data.push_back(texel_colour.alpha());
	}

	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(gl);

	gl.BindTexture(GL_TEXTURE_1D, d_colour_palette_texture);

	// Upload the colour palette data into the texture.
	glTexSubImage1D(
			GL_TEXTURE_1D, 0,
			0, // x offset
			d_colour_palette_resolution, // width
			GL_RGBA, GL_FLOAT, &colour_palette_data[0]);
}


GPlatesOpenGL::GLScalarField3D::CrossSection1DGeometryOnSphereVisitor::CrossSection1DGeometryOnSphereVisitor(
		GL &gl,
		const GLStreamBuffer::shared_ptr_type &streaming_vertex_element_buffer,
		const GLStreamBuffer::shared_ptr_type &streaming_vertex_buffer) :
	d_gl(gl),
	d_map_stream_buffer_scope(
			d_stream,
			*streaming_vertex_element_buffer,
			*streaming_vertex_buffer,
			MINIMUM_BYTES_TO_STREAM_IN_VERTEX_ELEMENT_BUFFER,
			MINIMUM_BYTES_TO_STREAM_IN_VERTEX_BUFFER),
	d_stream_primitives(d_stream)
{
	// Need to bind vertex buffer before streaming into it.
	//
	// Note that we don't bind the vertex element buffer since binding the vertex array does that
	// (however it does not bind the GL_ARRAY_BUFFER, only records vertex attribute buffers).
	gl.BindBuffer(GL_ARRAY_BUFFER, streaming_vertex_buffer->get_buffer());
}


void
GPlatesOpenGL::GLScalarField3D::CrossSection1DGeometryOnSphereVisitor::begin_rendering()
{
	// Start streaming cross-section 1D geometries.
	d_map_stream_buffer_scope.start_streaming();
}


void
GPlatesOpenGL::GLScalarField3D::CrossSection1DGeometryOnSphereVisitor::end_rendering()
{
	// Stop streaming cross-section 1D geometries so we can render the last batch.
	if (d_map_stream_buffer_scope.stop_streaming())
	{	// Only render if buffer contents are not undefined...
		// Render the last batch of streamed cross-section 1D geometries (if any).
		render_stream();
	}
}


void
GPlatesOpenGL::GLScalarField3D::CrossSection1DGeometryOnSphereVisitor::visit_multi_point_on_sphere(
		GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere)
{
	for (const GPlatesMaths::PointOnSphere &point : *multi_point_on_sphere)
	{
		render_cross_section_1d(point.position_vector());
	}
}


void
GPlatesOpenGL::GLScalarField3D::CrossSection1DGeometryOnSphereVisitor::visit_point_on_sphere(
		GPlatesMaths::PointGeometryOnSphere::non_null_ptr_to_const_type point_on_sphere)
{
	render_cross_section_1d(point_on_sphere->position().position_vector());
}


void
GPlatesOpenGL::GLScalarField3D::CrossSection1DGeometryOnSphereVisitor::render_cross_section_1d(
		const GPlatesMaths::UnitVector3D &surface_point)
{
	// There is one vertex for the current point.
	// Each surface point is vertically extruded to form a line.
	if (!d_stream_primitives.begin_primitive(1/*max_num_vertices*/, 1/*max_num_vertex_elements*/))
	{
		// There's not enough vertices or indices so render what we have so far and obtain new stream buffers.
		if (d_map_stream_buffer_scope.stop_streaming())
		{	// Only render if buffer contents are not undefined...
			render_stream();
		}
		d_map_stream_buffer_scope.start_streaming();

		d_stream_primitives.begin_primitive(1/*max_num_vertices*/, 1/*max_num_vertex_elements*/);
	}

	CrossSectionVertex vertex;

	vertex.surface_point[0] = surface_point.x().dval();
	vertex.surface_point[1] = surface_point.y().dval();
	vertex.surface_point[2] = surface_point.z().dval();

	d_stream_primitives.add_vertex(vertex);
	d_stream_primitives.add_vertex_element(0);

	d_stream_primitives.end_primitive();
}


void
GPlatesOpenGL::GLScalarField3D::CrossSection1DGeometryOnSphereVisitor::render_stream()
{
	// Only render if we've got some data to render.
	if (d_map_stream_buffer_scope.get_num_streamed_vertex_elements() > 0)
	{
		glDrawRangeElements(
				GL_POINTS,
				d_map_stream_buffer_scope.get_start_streaming_vertex_count()/*start*/,
				d_map_stream_buffer_scope.get_start_streaming_vertex_count() +
						d_map_stream_buffer_scope.get_num_streamed_vertices() - 1/*end*/,
				d_map_stream_buffer_scope.get_num_streamed_vertex_elements()/*count*/,
				GLVertexUtils::ElementTraits<streaming_vertex_element_type>::type,
				GLVertexUtils::buffer_offset(
						d_map_stream_buffer_scope.get_start_streaming_vertex_element_count() *
								sizeof(streaming_vertex_element_type)/*indices_offset*/));
	}
}


GPlatesOpenGL::GLScalarField3D::CrossSection2DGeometryOnSphereVisitor::CrossSection2DGeometryOnSphereVisitor(
		GL &gl,
		const GLStreamBuffer::shared_ptr_type &streaming_vertex_element_buffer,
		const GLStreamBuffer::shared_ptr_type &streaming_vertex_buffer) :
	d_gl(gl),
	d_map_stream_buffer_scope(
			d_stream,
			*streaming_vertex_element_buffer,
			*streaming_vertex_buffer,
			MINIMUM_BYTES_TO_STREAM_IN_VERTEX_ELEMENT_BUFFER,
			MINIMUM_BYTES_TO_STREAM_IN_VERTEX_BUFFER),
	d_stream_primitives(d_stream)
{
	// Need to bind vertex buffer before streaming into it.
	//
	// Note that we don't bind the vertex element buffer since binding the vertex array does that
	// (however it does not bind the GL_ARRAY_BUFFER, only records vertex attribute buffers).
	gl.BindBuffer(GL_ARRAY_BUFFER, streaming_vertex_buffer->get_buffer());
}


void
GPlatesOpenGL::GLScalarField3D::CrossSection2DGeometryOnSphereVisitor::begin_rendering()
{
	// Start streaming cross-section 2D geometries.
	d_map_stream_buffer_scope.start_streaming();
}


void
GPlatesOpenGL::GLScalarField3D::CrossSection2DGeometryOnSphereVisitor::end_rendering()
{
	// Stop streaming cross-section 2D geometries so we can render the last batch.
	if (d_map_stream_buffer_scope.stop_streaming())
	{	// Only render if buffer contents are not undefined...
		// Render the last batch of streamed cross-section 1D geometries (if any).
		render_stream();
	}
}


void
GPlatesOpenGL::GLScalarField3D::CrossSection2DGeometryOnSphereVisitor::visit_polygon_on_sphere(
		GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
{
	// Exterior ring.
	render_cross_sections_2d(
			polygon_on_sphere->exterior_ring_begin(),
			polygon_on_sphere->exterior_ring_end());

	// Interior rings.
 	const unsigned int num_interior_rings = polygon_on_sphere->number_of_interior_rings();
 	for (unsigned int interior_ring_index = 0; interior_ring_index < num_interior_rings; ++interior_ring_index)
 	{
 		render_cross_sections_2d(
 				polygon_on_sphere->interior_ring_begin(interior_ring_index),
 				polygon_on_sphere->interior_ring_end(interior_ring_index));
 	}
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
	// There are two vertices for each line segment (great circle arc).
	// Each surface great circle arc is vertically extruded in geometry shader to form a 2D quad (two triangles).
	if (!d_stream_primitives.begin_primitive(2/*max_num_vertices*/, 2/*max_num_vertex_elements*/))
	{
		// There's not enough vertices or indices so render what we have so far and obtain new stream buffers.
		if (d_map_stream_buffer_scope.stop_streaming())
		{	// Only render if buffer contents are not undefined...
			render_stream();
		}
		d_map_stream_buffer_scope.start_streaming();

		d_stream_primitives.begin_primitive(2/*max_num_vertices*/, 2/*max_num_vertex_elements*/);
	}

	CrossSectionVertex vertex;

	// Line start point.
	vertex.surface_point[0] = start_surface_point.x().dval();
	vertex.surface_point[1] = start_surface_point.y().dval();
	vertex.surface_point[2] = start_surface_point.z().dval();

	d_stream_primitives.add_vertex(vertex);

	// Line end point.
	vertex.surface_point[0] = end_surface_point.x().dval();
	vertex.surface_point[1] = end_surface_point.y().dval();
	vertex.surface_point[2] = end_surface_point.z().dval();

	d_stream_primitives.add_vertex(vertex);

	// Line segment.
	d_stream_primitives.add_vertex_element(0);
	d_stream_primitives.add_vertex_element(1);

	d_stream_primitives.end_primitive();
}


void
GPlatesOpenGL::GLScalarField3D::CrossSection2DGeometryOnSphereVisitor::render_stream()
{
	// Only render if we've got some data to render.
	if (d_map_stream_buffer_scope.get_num_streamed_vertex_elements() > 0)
	{
		glDrawRangeElements(
				GL_LINES,
				d_map_stream_buffer_scope.get_start_streaming_vertex_count()/*start*/,
				d_map_stream_buffer_scope.get_start_streaming_vertex_count() +
						d_map_stream_buffer_scope.get_num_streamed_vertices() - 1/*end*/,
				d_map_stream_buffer_scope.get_num_streamed_vertex_elements()/*count*/,
				GLVertexUtils::ElementTraits<streaming_vertex_element_type>::type,
				GLVertexUtils::buffer_offset(
						d_map_stream_buffer_scope.get_start_streaming_vertex_element_count() *
								sizeof(streaming_vertex_element_type)/*indices_offset*/));
	}
}


GPlatesOpenGL::GLScalarField3D::SurfaceFillMaskGeometryOnSphereVisitor::SurfaceFillMaskGeometryOnSphereVisitor(
		GL &gl,
		const GLStreamBuffer::shared_ptr_type &streaming_vertex_element_buffer,
		const GLStreamBuffer::shared_ptr_type &streaming_vertex_buffer,
		bool include_polylines) :
	d_gl(gl),
	d_map_stream_buffer_scope(
			d_stream,
			*streaming_vertex_element_buffer,
			*streaming_vertex_buffer,
			MINIMUM_BYTES_TO_STREAM_IN_VERTEX_ELEMENT_BUFFER,
			MINIMUM_BYTES_TO_STREAM_IN_VERTEX_BUFFER),
	d_stream_primitives(d_stream),
	d_include_polylines(include_polylines)
{
	// Need to bind vertex buffer before streaming into it.
	//
	// Note that we don't bind the vertex element buffer since binding the vertex array does that
	// (however it does not bind the GL_ARRAY_BUFFER, only records vertex attribute buffers).
	gl.BindBuffer(GL_ARRAY_BUFFER, streaming_vertex_buffer->get_buffer());
}


void
GPlatesOpenGL::GLScalarField3D::SurfaceFillMaskGeometryOnSphereVisitor::visit_polygon_on_sphere(
		GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
{
	render_surface_fill_mask<GPlatesMaths::PolygonOnSphere>(polygon_on_sphere);
}


void
GPlatesOpenGL::GLScalarField3D::SurfaceFillMaskGeometryOnSphereVisitor::visit_polyline_on_sphere(
		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
{
	if (d_include_polylines)
	{
		render_surface_fill_mask<GPlatesMaths::PolylineOnSphere>(polyline_on_sphere);
	}
}


template <typename LineGeometryType>
void
GPlatesOpenGL::GLScalarField3D::SurfaceFillMaskGeometryOnSphereVisitor::render_surface_fill_mask(
		const typename LineGeometryType::non_null_ptr_to_const_type &line_geometry)
{
	// This is an optimisation whereby if the entire geometry fits within the stream buffer
	// (which is usually the case) then the geometry does not need to be re-streamed for each
	// subsequent rendering and only a draw call needs to be issued.
	bool entire_geometry_is_in_stream_target = false;

	// First render the fill geometry with disabled color writes to the RGB channels.
	// This leaves the alpha-blending factors for the alpha channel to generate the (concave)
	// polygon fill mask in the alpha channel.
	d_gl.ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);

	render_surface_fill_mask_geometry<LineGeometryType>(line_geometry, entire_geometry_is_in_stream_target);

	// Second render the fill geometry with disabled color writes to the Alpha channel.
	// This leaves the alpha-blending factors for the RGB channels to accumulate the
	// polygon fill mask (just rendered) from the alpha channel into the RGB channels.
	d_gl.ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);

	render_surface_fill_mask_geometry<LineGeometryType>(line_geometry, entire_geometry_is_in_stream_target);

	// Third render the fill geometry with disabled color writes to the RGB channels again.
	// This effectively clears the alpha channel of the current polygon fill mask in preparation
	// for the next polygon fill mask.
	// The reason this clears is because the alpha-channel is set up to give 1 where a pixel is
	// covered by an odd number of triangles and 0 by an even number of triangles.
	// This second rendering results in all pixels being covered by an even number of triangles
	// (two times an odd or even number is an even number) resulting in 0 for all pixels (in alpha channel).
	d_gl.ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);

	render_surface_fill_mask_geometry<LineGeometryType>(line_geometry, entire_geometry_is_in_stream_target);
}


template <typename LineGeometryType>
void
GPlatesOpenGL::GLScalarField3D::SurfaceFillMaskGeometryOnSphereVisitor::render_surface_fill_mask_geometry(
		const typename LineGeometryType::non_null_ptr_to_const_type &line_geometry,
		bool &entire_geometry_is_in_stream_target)
{
	// If the entire geometry is already in the stream then we only need to issue a draw call.
	if (entire_geometry_is_in_stream_target)
	{
		render_stream();

		// Entire geometry is still in the stream buffer.
		return;
	}

	// Start streaming the current surface fill mask geometry.
	d_map_stream_buffer_scope.start_streaming();

	stream_surface_fill_mask_geometry(line_geometry, entire_geometry_is_in_stream_target);

	// Stop streaming the current surface fill mask geometry.
	if (d_map_stream_buffer_scope.stop_streaming())
	{	// Only render if buffer contents are not undefined...
		// Render the current surface fill mask geometry.
		render_stream();
	}
}


void
GPlatesOpenGL::GLScalarField3D::SurfaceFillMaskGeometryOnSphereVisitor::stream_surface_fill_mask_geometry(
		const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type &polyline,
		bool &entire_geometry_is_in_stream_target)
{
	// Polyline is rendered as a triangle fan which has an extra vertex at centroid and to close off ring.
	const unsigned int num_vertices = polyline->number_of_vertices() + 2;
	const unsigned int num_vertex_elements = 3 * polyline->number_of_vertices();

	// See if there's enough space remaining in the streaming buffers to stream the entire polyline.
	if (d_stream_primitives.begin_primitive(
			num_vertices/*max_num_vertices*/,
			num_vertex_elements/*max_num_vertex_elements*/))
	{
		// Vertex element relative to the beginning of the primitive (not beginning of buffer).
		streaming_vertex_element_type vertex_index = 0;

		stream_surface_fill_mask_ring_as_primitive(
				polyline->vertex_begin(),
				polyline->number_of_vertices(),
				polyline->get_centroid(),
				vertex_index);

		d_stream_primitives.end_primitive();

		// The entire polyline is now in the stream buffer.
		entire_geometry_is_in_stream_target = true;
	}
	else // Not enough space remaining in streaming buffer for the entire polyline...
	{
		stream_surface_fill_mask_ring_as_triangle_fan(
				polyline->vertex_begin(),
				polyline->number_of_vertices(),
				polyline->get_centroid());
	}
}


void
GPlatesOpenGL::GLScalarField3D::SurfaceFillMaskGeometryOnSphereVisitor::stream_surface_fill_mask_geometry(
		const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type &polygon,
		bool &entire_geometry_is_in_stream_target)
{
	const unsigned int num_vertices_in_all_rings = polygon->number_of_vertices();
	const unsigned int num_interior_rings = polygon->number_of_interior_rings();

	// Each ring in the polygon is rendered as a triangle fan which has an extra vertex at centroid
	// and to close off ring (although the ring is already closed, but we are using the same code
	// used for a polyline which does require an extra vertex to close the ring).
	const unsigned int num_rings = 1 + num_interior_rings;
	const unsigned int num_vertices = num_vertices_in_all_rings + 2 * num_rings;
	const unsigned int num_vertex_elements = 3 * num_vertices_in_all_rings;

	// See if there's enough space remaining in the streaming buffers to stream the entire polygon.
	if (d_stream_primitives.begin_primitive(
			num_vertices/*max_num_vertices*/,
			num_vertex_elements/*max_num_vertex_elements*/))
	{
		// Vertex element relative to the beginning of the primitive (not beginning of buffer).
		streaming_vertex_element_type vertex_index = 0;

		// Exterior ring.
		stream_surface_fill_mask_ring_as_primitive(
				polygon->exterior_ring_vertex_begin(),
				polygon->number_of_vertices_in_exterior_ring(),
				polygon->get_boundary_centroid(),
				vertex_index);

		// Interior rings.
		//
		// Note that the interior rings make holes in the exterior ring's surface mask.
		for (unsigned int interior_ring_index = 0; interior_ring_index < num_interior_rings; ++interior_ring_index)
		{
			stream_surface_fill_mask_ring_as_primitive(
					polygon->interior_ring_vertex_begin(interior_ring_index),
					polygon->number_of_vertices_in_interior_ring(interior_ring_index),
					polygon->get_boundary_centroid(),
					vertex_index);
		}

		d_stream_primitives.end_primitive();

		// The entire polygon is now in the stream buffer.
		entire_geometry_is_in_stream_target = true;
	}
	else // Not enough space remaining in streaming buffer for the entire polygon...
	{
		// Exterior ring.
		stream_surface_fill_mask_ring_as_triangle_fan(
				polygon->exterior_ring_vertex_begin(),
				polygon->number_of_vertices_in_exterior_ring(),
				polygon->get_boundary_centroid());

		// Interior rings.
		//
		// Note that the interior rings make holes in the exterior ring's surface mask.
		for (unsigned int interior_ring_index = 0; interior_ring_index < num_interior_rings; ++interior_ring_index)
		{
			stream_surface_fill_mask_ring_as_triangle_fan(
					polygon->interior_ring_vertex_begin(interior_ring_index),
					polygon->number_of_vertices_in_interior_ring(interior_ring_index),
					polygon->get_boundary_centroid());
		}
	}
}


template <typename PointOnSphereForwardIter>
void
GPlatesOpenGL::GLScalarField3D::SurfaceFillMaskGeometryOnSphereVisitor::stream_surface_fill_mask_ring_as_primitive(
		const PointOnSphereForwardIter begin_points,
		const unsigned int num_points,
		const GPlatesMaths::UnitVector3D &centroid,
		streaming_vertex_element_type &vertex_index)
{
	//
	// Here we use the more efficient path of generating the triangle fan mesh ourselves.
	// The price we pay is having to be more explicit in how we submit the triangle fan.
	//

	SurfaceFillMaskVertex vertex;

	// The first vertex is the polygon centroid.
	const streaming_vertex_element_type centroid_vertex_index = vertex_index;
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

		d_stream_primitives.add_vertex_element(centroid_vertex_index); // Centroid.
		d_stream_primitives.add_vertex_element(vertex_index); // Current boundary point.
		d_stream_primitives.add_vertex_element(vertex_index + 1); // Next boundary point.
	}

	// Wraparound back to the first boundary vertex to close off the ring.
	const GPlatesMaths::UnitVector3D &first_point_position = begin_points->position_vector();
	vertex.surface_point[0] = first_point_position.x().dval();
	vertex.surface_point[1] = first_point_position.y().dval();
	vertex.surface_point[2] = first_point_position.z().dval();
	d_stream_primitives.add_vertex(vertex);
	++vertex_index;
}


template <typename PointOnSphereForwardIter>
void
GPlatesOpenGL::GLScalarField3D::SurfaceFillMaskGeometryOnSphereVisitor::stream_surface_fill_mask_ring_as_triangle_fan(
		const PointOnSphereForwardIter begin_points,
		const unsigned int num_points,
		const GPlatesMaths::UnitVector3D &centroid)
{
	//
	// Here we use the less efficient path of rendering a triangle fan in order to have the
	// stream take care of copying the fan apex vertex whenever the stream fills up mid-triangle-fan.
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
		// There's not enough vertices or indices so render what we have so far and obtain new stream buffers.
		if (d_map_stream_buffer_scope.stop_streaming())
		{	// Only render if buffer contents are not undefined...
			render_stream();
		}
		d_map_stream_buffer_scope.start_streaming();

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
			// There's not enough vertices or indices so render what we have so far and obtain new stream buffers.
			if (d_map_stream_buffer_scope.stop_streaming())
			{	// Only render if buffer contents are not undefined...
				render_stream();
			}
			d_map_stream_buffer_scope.start_streaming();

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
		// There's not enough vertices or indices so render what we have so far and obtain new stream buffers.
		if (d_map_stream_buffer_scope.stop_streaming())
		{	// Only render if buffer contents are not undefined...
			render_stream();
		}
		d_map_stream_buffer_scope.start_streaming();

		fill_stream_triangle_fans.add_vertex(vertex);
	}

	fill_stream_triangle_fans.end_triangle_fan();
}


void
GPlatesOpenGL::GLScalarField3D::SurfaceFillMaskGeometryOnSphereVisitor::render_stream()
{
	// Only render if we've got some data to render.
	if (d_map_stream_buffer_scope.get_num_streamed_vertex_elements() > 0)
	{
		glDrawRangeElements(
				GL_TRIANGLES,
				d_map_stream_buffer_scope.get_start_streaming_vertex_count()/*start*/,
				d_map_stream_buffer_scope.get_start_streaming_vertex_count() +
						d_map_stream_buffer_scope.get_num_streamed_vertices() - 1/*end*/,
				d_map_stream_buffer_scope.get_num_streamed_vertex_elements()/*count*/,
				GLVertexUtils::ElementTraits<streaming_vertex_element_type>::type,
				GLVertexUtils::buffer_offset(
						d_map_stream_buffer_scope.get_start_streaming_vertex_element_count() *
								sizeof(streaming_vertex_element_type)/*indices_offset*/));
	}
}


GPlatesOpenGL::GLScalarField3D::VolumeFillBoundaryGeometryOnSphereVisitor::VolumeFillBoundaryGeometryOnSphereVisitor(
		GL &gl,
		const GLStreamBuffer::shared_ptr_type &streaming_vertex_element_buffer,
		const GLStreamBuffer::shared_ptr_type &streaming_vertex_buffer,
		bool include_polylines) :
	d_gl(gl),
	d_map_stream_buffer_scope(
			d_stream,
			*streaming_vertex_element_buffer,
			*streaming_vertex_buffer,
			MINIMUM_BYTES_TO_STREAM_IN_VERTEX_ELEMENT_BUFFER,
			MINIMUM_BYTES_TO_STREAM_IN_VERTEX_BUFFER),
	d_stream_primitives(d_stream),
	d_include_polylines(include_polylines)
{
	// Need to bind vertex buffer before streaming into it.
	//
	// Note that we don't bind the vertex element buffer since binding the vertex array does that
	// (however it does not bind the GL_ARRAY_BUFFER, only records vertex attribute buffers).
	gl.BindBuffer(GL_ARRAY_BUFFER, streaming_vertex_buffer->get_buffer());
}


void
GPlatesOpenGL::GLScalarField3D::VolumeFillBoundaryGeometryOnSphereVisitor::begin_rendering()
{
	// Start streaming volume fill boundary geometries.
	d_map_stream_buffer_scope.start_streaming();
}


void
GPlatesOpenGL::GLScalarField3D::VolumeFillBoundaryGeometryOnSphereVisitor::end_rendering()
{
	// Stop streaming volume fill boundary geometries so we can render the last batch.
	if (d_map_stream_buffer_scope.stop_streaming())
	{	// Only render if buffer contents are not undefined...
		// Render the current contents of the stream.
		render_stream();
	}
}


void
GPlatesOpenGL::GLScalarField3D::VolumeFillBoundaryGeometryOnSphereVisitor::visit_polygon_on_sphere(
		GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
{
	// Exterior ring.
	render_volume_fill_boundary(
			polygon_on_sphere->exterior_ring_begin(),
			polygon_on_sphere->exterior_ring_end(),
			polygon_on_sphere->get_boundary_centroid());

	// Interior rings.
 	const unsigned int num_interior_rings = polygon_on_sphere->number_of_interior_rings();
 	for (unsigned int interior_ring_index = 0; interior_ring_index < num_interior_rings; ++interior_ring_index)
 	{
		render_volume_fill_boundary(
				polygon_on_sphere->interior_ring_begin(interior_ring_index),
				polygon_on_sphere->interior_ring_end(interior_ring_index),
				polygon_on_sphere->get_boundary_centroid());
 	}
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

				render_volume_fill_boundary_segment(start_surface_point, end_surface_point);
			}
		}
		else // no need to tessellate great circle arc...
		{
			const GPlatesMaths::UnitVector3D &start_surface_point = gca.start_point().position_vector();
			const GPlatesMaths::UnitVector3D &end_surface_point = gca.end_point().position_vector();

			render_volume_fill_boundary_segment(start_surface_point, end_surface_point);
		}
	}
}


void
GPlatesOpenGL::GLScalarField3D::VolumeFillBoundaryGeometryOnSphereVisitor::render_volume_fill_boundary_segment(
		const GPlatesMaths::UnitVector3D &start_surface_point,
		const GPlatesMaths::UnitVector3D &end_surface_point)
{
	// There are two vertices and two indices per great circle arc.
	// Each great circle arc is sent as a line.
	// The geometry shader converts lines to triangles when it generates the
	// wall and spherical cap boundary surfaces.
	if (!d_stream_primitives.begin_primitive(2/*max_num_vertices*/, 2/*max_num_vertex_elements*/))
	{
		// There's not enough vertices or indices so render what we have so far and obtain new stream buffers.

		// Stop streaming volume fill boundary geometries so we can render the last batch.
		if (d_map_stream_buffer_scope.stop_streaming())
		{	// Only render if buffer contents are not undefined...
			// Render current contents of the stream.
			render_stream();
		}

		// Start streaming volume fill boundary geometries.
		d_map_stream_buffer_scope.start_streaming();

		d_stream_primitives.begin_primitive(2/*max_num_vertices*/, 2/*max_num_vertex_elements*/);
	}

	VolumeFillBoundaryVertex vertex;

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


void
GPlatesOpenGL::GLScalarField3D::VolumeFillBoundaryGeometryOnSphereVisitor::render_stream()
{
	// Render a batch of streamed volume fill boundary geometries (if any).
	// Only render if we've got some data to render.
	if (d_map_stream_buffer_scope.get_num_streamed_vertex_elements() > 0)
	{
		glDrawRangeElements(
				GL_LINES/*geometry shader converts lines to triangles*/,
				d_map_stream_buffer_scope.get_start_streaming_vertex_count()/*start*/,
				d_map_stream_buffer_scope.get_start_streaming_vertex_count() +
						d_map_stream_buffer_scope.get_num_streamed_vertices() - 1/*end*/,
				d_map_stream_buffer_scope.get_num_streamed_vertex_elements()/*count*/,
				GLVertexUtils::ElementTraits<streaming_vertex_element_type>::type,
				GLVertexUtils::buffer_offset(
						d_map_stream_buffer_scope.get_start_streaming_vertex_element_count() *
								sizeof(streaming_vertex_element_type)/*indices_offset*/));
	}
}


GPlatesOpenGL::GLScalarField3D::SphereMeshBuilder::SphereMeshBuilder(
		std::vector<GLVertexUtils::Vertex> &vertices,
		std::vector<GLuint> &vertex_elements,
		unsigned int recursion_depth_to_generate_mesh) :
	d_vertices(vertices),
	d_vertex_elements(vertex_elements),
	d_recursion_depth_to_generate_mesh(recursion_depth_to_generate_mesh)
{
}


void
GPlatesOpenGL::GLScalarField3D::SphereMeshBuilder::visit(
		const GPlatesMaths::SphericalSubdivision::HierarchicalTriangularMeshTraversal::Triangle &triangle,
		const unsigned int &recursion_depth)
{
	// If we're at the correct depth then add the triangle to our mesh.
	if (recursion_depth == d_recursion_depth_to_generate_mesh)
	{
		const unsigned int base_vertex_index = d_vertices.size();

		d_vertices.push_back(GLVertexUtils::Vertex(triangle.vertex0));
		d_vertices.push_back(GLVertexUtils::Vertex(triangle.vertex1));
		d_vertices.push_back(GLVertexUtils::Vertex(triangle.vertex2));

		d_vertex_elements.push_back(base_vertex_index);
		d_vertex_elements.push_back(base_vertex_index + 1);
		d_vertex_elements.push_back(base_vertex_index + 2);

		return;
	}

	// Recurse into the child triangles.
	const unsigned int child_recursion_depth = recursion_depth + 1;
	triangle.visit_children(*this, child_recursion_depth);
}
