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
#include <boost/cast.hpp>
#include <boost/cstdint.hpp>
#include <boost/utility/in_place_factory.hpp>
#include <QDebug>
#include <QImage>
#include <QString>

#include "GLRasterCoRegistration.h"

#include "GLBuffer.h"
#include "GLContext.h"
#include "GLCubeSubdivision.h"
#include "GLDataRasterSource.h"
#include "GLShaderSource.h"
#include "GLStreamPrimitives.h"
#include "GLUtils.h"
#include "GLVertexUtils.h"
#include "GLViewProjection.h"
#include "OpenGLException.h"

#include "app-logic/ReconstructedFeatureGeometry.h"

#include "global/GPlatesAssert.h"

#include "gui/Colour.h"

#include "maths/CubeCoordinateFrame.h"
#include "maths/types.h"

#include "utils/Base2Utils.h"
#include "utils/CallStackTracker.h"
#include "utils/Profile.h"


namespace GPlatesOpenGL
{
	namespace
	{
		//! Fragment shader source to render region-of-interest geometries.
		const QString RENDER_REGION_OF_INTEREST_GEOMETRIES_FRAGMENT_SHADER_SOURCE_FILE_NAME =
				":/opengl/raster_co_registration/render_region_of_interest_geometries_fragment_shader.glsl";

		//! Vertex shader source to render region-of-interest geometries.
		const QString RENDER_REGION_OF_INTEREST_GEOMETRIES_VERTEX_SHADER_SOURCE_FILE_NAME =
				":/opengl/raster_co_registration/render_region_of_interest_geometries_vertex_shader.glsl";

		/**
		 * Fragment shader source to extract target raster in region-of-interest in preparation
		 * for reduction operations.
		 */
		const QString MASK_REGION_OF_INTEREST_FRAGMENT_SHADER_SOURCE_FILE_NAME =
				":/opengl/raster_co_registration/mask_region_of_interest_fragment_shader.glsl";

		/**
		 * Vertex shader source to extract target raster in region-of-interest in preparation
		 * for reduction operations.
		 */
		const QString MASK_REGION_OF_INTEREST_VERTEX_SHADER_SOURCE_FILE_NAME =
				":/opengl/raster_co_registration/mask_region_of_interest_vertex_shader.glsl";

		/**
		 * Fragment shader source to reduce region-of-interest filter results.
		 */
		const QString REDUCTION_OF_REGION_OF_INTEREST_FRAGMENT_SHADER_SOURCE_FILE_NAME =
				":/opengl/raster_co_registration/reduction_of_region_of_interest_fragment_shader.glsl";

		/**
		 * Vertex shader source to reduce region-of-interest filter results.
		 */
		const QString REDUCTION_OF_REGION_OF_INTEREST_VERTEX_SHADER_SOURCE_FILE_NAME =
				":/opengl/raster_co_registration/reduction_of_region_of_interest_vertex_shader.glsl";
	}
}


GPlatesOpenGL::GLRasterCoRegistration::GLRasterCoRegistration(
		GL &gl) :
	d_rgba_float_texture_cache(texture_cache_type::create()),
	d_have_checked_framebuffer_completeness(false),
	d_identity_quaternion(GPlatesMaths::UnitQuaternion3D::create_identity_rotation())
{
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

	// Initialise OpenGL resources (framebuffer, vertex arrays, shader programs, etc).
	initialise_gl(gl);
}


void
GPlatesOpenGL::GLRasterCoRegistration::initialise_gl(
		GL &gl)
{
	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(gl);

	// Create framebuffer object used to render to floating-point textures.
	d_framebuffer = GLFramebuffer::create(gl);

	// Create buffer object used to stream indices (vertex elements).
	d_streaming_vertex_element_buffer =
			GLStreamBuffer::create(GLBuffer::create(gl), NUM_BYTES_IN_STREAMING_VERTEX_ELEMENT_BUFFER);
	// Create buffer object used to stream vertices.
	d_streaming_vertex_buffer =
			GLStreamBuffer::create(GLBuffer::create(gl), NUM_BYTES_IN_STREAMING_VERTEX_BUFFER);

	//
	// Create the shader programs (and configure vertex attributes in vertex arrays to match programs).
	//

	initialise_point_region_of_interest_shader_programs(gl);

	initialise_line_region_of_interest_shader_program(gl);

	initialise_fill_region_of_interest_shader_program(gl);

	initialise_mask_region_of_interest_shader_program(gl);

	initialise_reduction_of_region_of_interest_shader_programs(gl);
	initialise_reduction_of_region_of_interest_vertex_array(gl);

#if defined(DEBUG_RASTER_COREGISTRATION_RENDER_TARGET)
	// A pixel buffer object for debugging render targets.
	d_debug_pixel_buffer = GLBuffer::create(gl);

	// Bind the pixel buffer and allocate its memory.
	gl.BindBuffer(GL_PIXEL_PACK_BUFFER, d_debug_pixel_buffer);
	gl.BufferData(GL_PIXEL_PACK_BUFFER, PIXEL_BUFFER_SIZE_IN_BYTES, nullptr, GL_STREAM_READ);
#endif
}


void
GPlatesOpenGL::GLRasterCoRegistration::initialise_point_region_of_interest_shader_programs(
		GL &gl)
{
	// Add this scope to the call stack trace printed if exception thrown in this scope (eg, failure to compile/link shader).
	TRACK_CALL_STACK();

	//
	// Shader program to render points of seed geometries bounded by a loose target raster tile.
	//
	// This version clips to the loose frustum (shouldn't be rendering pixels outside the loose frustum
	// anyway - since ROI-radius expanded seed geometry should be bounded by loose frustum - but
	// just in case this will prevent pixels being rendered into the sub-viewport, of render target,
	// of an adjacent seed geometry thus polluting its results).

	//
	// For small region-of-interest angles (retains accuracy for very small angles).
	//

	// Vertex shader source.
	GLShaderSource small_roi_angle_vertex_shader_source;
	small_roi_angle_vertex_shader_source.add_code_segment("#define POINT_REGION_OF_INTEREST\n");
	small_roi_angle_vertex_shader_source.add_code_segment("#define SMALL_ROI_ANGLE\n");
	small_roi_angle_vertex_shader_source.add_code_segment("#define ENABLE_SEED_FRUSTUM_CLIPPING\n");
	small_roi_angle_vertex_shader_source.add_code_segment_from_file(GLShaderSource::UTILS_FILE_NAME);
	small_roi_angle_vertex_shader_source.add_code_segment_from_file(RENDER_REGION_OF_INTEREST_GEOMETRIES_VERTEX_SHADER_SOURCE_FILE_NAME);

	// Vertex shader.
	GLShader::shared_ptr_type small_roi_angle_vertex_shader = GLShader::create(gl, GL_VERTEX_SHADER);
	small_roi_angle_vertex_shader->shader_source(gl, small_roi_angle_vertex_shader_source);
	small_roi_angle_vertex_shader->compile_shader(gl);

	// Fragment shader source.
	GLShaderSource small_roi_angle_fragment_shader_source;
	small_roi_angle_fragment_shader_source.add_code_segment("#define POINT_REGION_OF_INTEREST\n");
	small_roi_angle_fragment_shader_source.add_code_segment("#define SMALL_ROI_ANGLE\n");
	small_roi_angle_fragment_shader_source.add_code_segment("#define ENABLE_SEED_FRUSTUM_CLIPPING\n");
	small_roi_angle_fragment_shader_source.add_code_segment_from_file(GLShaderSource::UTILS_FILE_NAME);
	small_roi_angle_fragment_shader_source.add_code_segment_from_file(RENDER_REGION_OF_INTEREST_GEOMETRIES_FRAGMENT_SHADER_SOURCE_FILE_NAME);

	// Fragment shader.
	GLShader::shared_ptr_type small_roi_angle_fragment_shader = GLShader::create(gl, GL_FRAGMENT_SHADER);
	small_roi_angle_fragment_shader->shader_source(gl, small_roi_angle_fragment_shader_source);
	small_roi_angle_fragment_shader->compile_shader(gl);

	// Vertex-fragment program.
	d_render_points_of_seed_geometries_with_small_roi_angle_program = GLProgram::create(gl);
	d_render_points_of_seed_geometries_with_small_roi_angle_program->attach_shader(gl, small_roi_angle_vertex_shader);
	d_render_points_of_seed_geometries_with_small_roi_angle_program->attach_shader(gl, small_roi_angle_fragment_shader);
	d_render_points_of_seed_geometries_with_small_roi_angle_program->link_program(gl);

	//
	// For larger region-of-interest angles (retains accuracy for angles very near 90 degrees).
	//

	// Vertex shader source.
	GLShaderSource large_roi_angle_vertex_shader_source;
	large_roi_angle_vertex_shader_source.add_code_segment("#define POINT_REGION_OF_INTEREST\n");
	large_roi_angle_vertex_shader_source.add_code_segment("#define LARGE_ROI_ANGLE\n");
	large_roi_angle_vertex_shader_source.add_code_segment("#define ENABLE_SEED_FRUSTUM_CLIPPING\n");
	large_roi_angle_vertex_shader_source.add_code_segment_from_file(GLShaderSource::UTILS_FILE_NAME);
	large_roi_angle_vertex_shader_source.add_code_segment_from_file(RENDER_REGION_OF_INTEREST_GEOMETRIES_VERTEX_SHADER_SOURCE_FILE_NAME);

	// Vertex shader.
	GLShader::shared_ptr_type large_roi_angle_vertex_shader = GLShader::create(gl, GL_VERTEX_SHADER);
	large_roi_angle_vertex_shader->shader_source(gl, large_roi_angle_vertex_shader_source);
	large_roi_angle_vertex_shader->compile_shader(gl);

	// Fragment shader source.
	GLShaderSource large_roi_angle_fragment_shader_source;
	large_roi_angle_fragment_shader_source.add_code_segment("#define POINT_REGION_OF_INTEREST\n");
	large_roi_angle_fragment_shader_source.add_code_segment("#define LARGE_ROI_ANGLE\n");
	large_roi_angle_fragment_shader_source.add_code_segment("#define ENABLE_SEED_FRUSTUM_CLIPPING\n");
	large_roi_angle_fragment_shader_source.add_code_segment_from_file(GLShaderSource::UTILS_FILE_NAME);
	large_roi_angle_fragment_shader_source.add_code_segment_from_file(RENDER_REGION_OF_INTEREST_GEOMETRIES_FRAGMENT_SHADER_SOURCE_FILE_NAME);

	// Fragment shader.
	GLShader::shared_ptr_type large_roi_angle_fragment_shader = GLShader::create(gl, GL_FRAGMENT_SHADER);
	large_roi_angle_fragment_shader->shader_source(gl, large_roi_angle_fragment_shader_source);
	large_roi_angle_fragment_shader->compile_shader(gl);

	// Vertex-fragment program.
	d_render_points_of_seed_geometries_with_large_roi_angle_program = GLProgram::create(gl);
	d_render_points_of_seed_geometries_with_large_roi_angle_program->attach_shader(gl, large_roi_angle_vertex_shader);
	d_render_points_of_seed_geometries_with_large_roi_angle_program->attach_shader(gl, large_roi_angle_fragment_shader);
	d_render_points_of_seed_geometries_with_large_roi_angle_program->link_program(gl);


	//
	// Create and initialise the point region-of-interest vertex array.
	//

	d_point_region_of_interest_vertex_array = GLVertexArray::create(gl);

	// Bind vertex array object.
	gl.BindVertexArray(d_point_region_of_interest_vertex_array);

	// Bind vertex element buffer object to currently bound vertex array object.
	gl.BindBuffer(GL_ELEMENT_ARRAY_BUFFER, d_streaming_vertex_element_buffer->get_buffer());

	// Bind vertex buffer object (used by vertex attribute arrays, not vertex array object).
	gl.BindBuffer(GL_ARRAY_BUFFER, d_streaming_vertex_buffer->get_buffer());

	// Specify vertex attributes in currently bound vertex buffer object.
	// This transfers each vertex attribute array (parameters + currently bound vertex buffer object)
	// to currently bound vertex array object.
	//
	// The following reflects the structure of 'struct PointRegionOfInterestVertex'.
	// It tells OpenGL how the elements of the vertex are packed together in the vertex and
	// which parts of the vertex bind to the named attributes in the shader program.
	//

	gl.EnableVertexAttribArray(0);
	gl.VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(PointRegionOfInterestVertex),
			BUFFER_OFFSET(PointRegionOfInterestVertex, point_centre));
	gl.BindAttribLocation(d_render_points_of_seed_geometries_with_small_roi_angle_program,
			0, "point_centre");
	gl.BindAttribLocation(d_render_points_of_seed_geometries_with_large_roi_angle_program,
			0, "point_centre");

	gl.EnableVertexAttribArray(1);
	gl.VertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(PointRegionOfInterestVertex),
			BUFFER_OFFSET(PointRegionOfInterestVertex, tangent_frame_weights));
	gl.BindAttribLocation(d_render_points_of_seed_geometries_with_small_roi_angle_program,
			1, "tangent_frame_weights");
	gl.BindAttribLocation(d_render_points_of_seed_geometries_with_large_roi_angle_program,
			1, "tangent_frame_weights");

	gl.EnableVertexAttribArray(2);
	gl.VertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(PointRegionOfInterestVertex),
			BUFFER_OFFSET(PointRegionOfInterestVertex, world_space_quaternion));
	gl.BindAttribLocation(d_render_points_of_seed_geometries_with_small_roi_angle_program,
			2, "world_space_quaternion");
	gl.BindAttribLocation(d_render_points_of_seed_geometries_with_large_roi_angle_program,
			2, "world_space_quaternion");

	gl.EnableVertexAttribArray(3);
	gl.VertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(PointRegionOfInterestVertex),
			BUFFER_OFFSET(PointRegionOfInterestVertex, raster_frustum_to_seed_frustum_clip_space_transform));
	gl.BindAttribLocation(d_render_points_of_seed_geometries_with_small_roi_angle_program,
			3, "raster_frustum_to_seed_frustum_clip_space_transform");
	gl.BindAttribLocation(d_render_points_of_seed_geometries_with_large_roi_angle_program,
			3, "raster_frustum_to_seed_frustum_clip_space_transform");

	gl.EnableVertexAttribArray(4);
	gl.VertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(PointRegionOfInterestVertex),
			BUFFER_OFFSET(PointRegionOfInterestVertex, seed_frustum_to_render_target_clip_space_transform));
	gl.BindAttribLocation(d_render_points_of_seed_geometries_with_small_roi_angle_program,
			4, "seed_frustum_to_render_target_clip_space_transform");
	gl.BindAttribLocation(d_render_points_of_seed_geometries_with_large_roi_angle_program,
			4, "seed_frustum_to_render_target_clip_space_transform");

	// Now that we've changed the attribute bindings in the program object we need to
	// re-link it in order for them to take effect.
	d_render_points_of_seed_geometries_with_small_roi_angle_program->link_program(gl);
	d_render_points_of_seed_geometries_with_large_roi_angle_program->link_program(gl);
}


void
GPlatesOpenGL::GLRasterCoRegistration::initialise_line_region_of_interest_shader_program(
		GL &gl)
{
	// Add this scope to the call stack trace printed if exception thrown in this scope (eg, failure to compile/link shader).
	TRACK_CALL_STACK();

	//
	// Shader program to render lines (GCAs) of seed geometries bounded by a loose target raster tile.
	// This version clips to the loose frustum (shouldn't be rendering pixels outside the loose frustum
	// anyway - since ROI-radius expanded seed geometry should be bounded by loose frustum - but
	// just in case this will prevent pixels being rendered into the sub-viewport, of render target,
	// of an adjacent seed geometry thus polluting its results).
	//

	//
	// For small region-of-interest angles (retains accuracy for very small angles).
	//

	// Vertex shader source.
	GLShaderSource small_roi_angle_vertex_shader_source;
	small_roi_angle_vertex_shader_source.add_code_segment("#define LINE_REGION_OF_INTEREST\n");
	small_roi_angle_vertex_shader_source.add_code_segment("#define SMALL_ROI_ANGLE\n");
	small_roi_angle_vertex_shader_source.add_code_segment("#define ENABLE_SEED_FRUSTUM_CLIPPING\n");
	small_roi_angle_vertex_shader_source.add_code_segment_from_file(GLShaderSource::UTILS_FILE_NAME);
	small_roi_angle_vertex_shader_source.add_code_segment_from_file(RENDER_REGION_OF_INTEREST_GEOMETRIES_VERTEX_SHADER_SOURCE_FILE_NAME);

	// Vertex shader.
	GLShader::shared_ptr_type small_roi_angle_vertex_shader = GLShader::create(gl, GL_VERTEX_SHADER);
	small_roi_angle_vertex_shader->shader_source(gl, small_roi_angle_vertex_shader_source);
	small_roi_angle_vertex_shader->compile_shader(gl);

	// Fragment shader source.
	GLShaderSource small_roi_angle_fragment_shader_source;
	small_roi_angle_fragment_shader_source.add_code_segment("#define LINE_REGION_OF_INTEREST\n");
	small_roi_angle_fragment_shader_source.add_code_segment("#define SMALL_ROI_ANGLE\n");
	small_roi_angle_fragment_shader_source.add_code_segment("#define ENABLE_SEED_FRUSTUM_CLIPPING\n");
	small_roi_angle_fragment_shader_source.add_code_segment_from_file(GLShaderSource::UTILS_FILE_NAME);
	small_roi_angle_fragment_shader_source.add_code_segment_from_file(RENDER_REGION_OF_INTEREST_GEOMETRIES_FRAGMENT_SHADER_SOURCE_FILE_NAME);

	// Fragment shader.
	GLShader::shared_ptr_type small_roi_angle_fragment_shader = GLShader::create(gl, GL_FRAGMENT_SHADER);
	small_roi_angle_fragment_shader->shader_source(gl, small_roi_angle_fragment_shader_source);
	small_roi_angle_fragment_shader->compile_shader(gl);

	// Vertex-fragment program.
	d_render_lines_of_seed_geometries_with_small_roi_angle_program = GLProgram::create(gl);
	d_render_lines_of_seed_geometries_with_small_roi_angle_program->attach_shader(gl, small_roi_angle_vertex_shader);
	d_render_lines_of_seed_geometries_with_small_roi_angle_program->attach_shader(gl, small_roi_angle_fragment_shader);
	d_render_lines_of_seed_geometries_with_small_roi_angle_program->link_program(gl);

	//
	// For larger region-of-interest angles (retains accuracy for angles very near 90 degrees).
	//

	// Vertex shader source.
	GLShaderSource large_roi_angle_vertex_shader_source;
	large_roi_angle_vertex_shader_source.add_code_segment("#define LINE_REGION_OF_INTEREST\n");
	large_roi_angle_vertex_shader_source.add_code_segment("#define LARGE_ROI_ANGLE\n");
	large_roi_angle_vertex_shader_source.add_code_segment("#define ENABLE_SEED_FRUSTUM_CLIPPING\n");
	large_roi_angle_vertex_shader_source.add_code_segment_from_file(GLShaderSource::UTILS_FILE_NAME);
	large_roi_angle_vertex_shader_source.add_code_segment_from_file(RENDER_REGION_OF_INTEREST_GEOMETRIES_VERTEX_SHADER_SOURCE_FILE_NAME);

	// Vertex shader.
	GLShader::shared_ptr_type large_roi_angle_vertex_shader = GLShader::create(gl, GL_VERTEX_SHADER);
	large_roi_angle_vertex_shader->shader_source(gl, large_roi_angle_vertex_shader_source);
	large_roi_angle_vertex_shader->compile_shader(gl);

	// Fragment shader source.
	GLShaderSource large_roi_angle_fragment_shader_source;
	large_roi_angle_fragment_shader_source.add_code_segment("#define LINE_REGION_OF_INTEREST\n");
	large_roi_angle_fragment_shader_source.add_code_segment("#define LARGE_ROI_ANGLE\n");
	large_roi_angle_fragment_shader_source.add_code_segment("#define ENABLE_SEED_FRUSTUM_CLIPPING\n");
	large_roi_angle_fragment_shader_source.add_code_segment_from_file(GLShaderSource::UTILS_FILE_NAME);
	large_roi_angle_fragment_shader_source.add_code_segment_from_file(RENDER_REGION_OF_INTEREST_GEOMETRIES_FRAGMENT_SHADER_SOURCE_FILE_NAME);

	// Fragment shader.
	GLShader::shared_ptr_type large_roi_angle_fragment_shader = GLShader::create(gl, GL_FRAGMENT_SHADER);
	large_roi_angle_fragment_shader->shader_source(gl, large_roi_angle_fragment_shader_source);
	large_roi_angle_fragment_shader->compile_shader(gl);

	// Vertex-fragment program.
	d_render_lines_of_seed_geometries_with_large_roi_angle_program = GLProgram::create(gl);
	d_render_lines_of_seed_geometries_with_large_roi_angle_program->attach_shader(gl, large_roi_angle_vertex_shader);
	d_render_lines_of_seed_geometries_with_large_roi_angle_program->attach_shader(gl, large_roi_angle_fragment_shader);
	d_render_lines_of_seed_geometries_with_large_roi_angle_program->link_program(gl);


	//
	// Create and initialise the line region-of-interest vertex array.
	//

	d_line_region_of_interest_vertex_array = GLVertexArray::create(gl);

	// Bind vertex array object.
	gl.BindVertexArray(d_line_region_of_interest_vertex_array);

	// Bind vertex element buffer object to currently bound vertex array object.
	gl.BindBuffer(GL_ELEMENT_ARRAY_BUFFER, d_streaming_vertex_element_buffer->get_buffer());

	// Bind vertex buffer object (used by vertex attribute arrays, not vertex array object).
	gl.BindBuffer(GL_ARRAY_BUFFER, d_streaming_vertex_buffer->get_buffer());

	// Specify vertex attributes in currently bound vertex buffer object.
	// This transfers each vertex attribute array (parameters + currently bound vertex buffer object)
	// to currently bound vertex array object.
	//
	// The following reflects the structure of 'struct LineRegionOfInterestVertex'.
	// It tells OpenGL how the elements of the vertex are packed together in the vertex and
	// which parts of the vertex bind to the named attributes in the shader program.
	//

	gl.EnableVertexAttribArray(0);
	gl.VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(LineRegionOfInterestVertex),
			BUFFER_OFFSET(LineRegionOfInterestVertex, line_arc_start_point));
	gl.BindAttribLocation(d_render_lines_of_seed_geometries_with_small_roi_angle_program,
			0, "line_arc_start_point");
	gl.BindAttribLocation(d_render_lines_of_seed_geometries_with_large_roi_angle_program,
			0, "line_arc_start_point");

	gl.EnableVertexAttribArray(1);
	gl.VertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(LineRegionOfInterestVertex),
			BUFFER_OFFSET(LineRegionOfInterestVertex, line_arc_normal));
	gl.BindAttribLocation(d_render_lines_of_seed_geometries_with_small_roi_angle_program,
			1, "line_arc_normal");
	gl.BindAttribLocation(d_render_lines_of_seed_geometries_with_large_roi_angle_program,
			1, "line_arc_normal");

	gl.EnableVertexAttribArray(2);
	gl.VertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(LineRegionOfInterestVertex),
			BUFFER_OFFSET(LineRegionOfInterestVertex, tangent_frame_weights));
	gl.BindAttribLocation(d_render_lines_of_seed_geometries_with_small_roi_angle_program,
			2, "tangent_frame_weights");
	gl.BindAttribLocation(d_render_lines_of_seed_geometries_with_large_roi_angle_program,
			2, "tangent_frame_weights");

	gl.EnableVertexAttribArray(3);
	gl.VertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(LineRegionOfInterestVertex),
			BUFFER_OFFSET(LineRegionOfInterestVertex, world_space_quaternion));
	gl.BindAttribLocation(d_render_lines_of_seed_geometries_with_small_roi_angle_program,
			3, "world_space_quaternion");
	gl.BindAttribLocation(d_render_lines_of_seed_geometries_with_large_roi_angle_program,
			3, "world_space_quaternion");

	gl.EnableVertexAttribArray(4);
	gl.VertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(LineRegionOfInterestVertex),
			BUFFER_OFFSET(LineRegionOfInterestVertex, raster_frustum_to_seed_frustum_clip_space_transform));
	gl.BindAttribLocation(d_render_lines_of_seed_geometries_with_small_roi_angle_program,
			4, "raster_frustum_to_seed_frustum_clip_space_transform");
	gl.BindAttribLocation(d_render_lines_of_seed_geometries_with_large_roi_angle_program,
			4, "raster_frustum_to_seed_frustum_clip_space_transform");

	gl.EnableVertexAttribArray(5);
	gl.VertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(LineRegionOfInterestVertex),
			BUFFER_OFFSET(LineRegionOfInterestVertex, seed_frustum_to_render_target_clip_space_transform));
	gl.BindAttribLocation(d_render_lines_of_seed_geometries_with_small_roi_angle_program,
			5, "seed_frustum_to_render_target_clip_space_transform");
	gl.BindAttribLocation(d_render_lines_of_seed_geometries_with_large_roi_angle_program,
			5, "seed_frustum_to_render_target_clip_space_transform");

	// Now that we've changed the attribute bindings in the program object we need to
	// re-link it in order for them to take effect.
	d_render_lines_of_seed_geometries_with_small_roi_angle_program->link_program(gl);
	d_render_lines_of_seed_geometries_with_large_roi_angle_program->link_program(gl);
}


void
GPlatesOpenGL::GLRasterCoRegistration::initialise_fill_region_of_interest_shader_program(
		GL &gl)
{
	// Add this scope to the call stack trace printed if exception thrown in this scope (eg, failure to compile/link shader).
	TRACK_CALL_STACK();

	//
	// Shader program to render interior of seed polygons bounded by a loose target raster tile.
	//
	// Also used when rasterizing point and line primitive (ie, GL_POINTS and GL_LINES, not GL_TRIANGLES).
	//

	// Vertex shader source.
	GLShaderSource render_fill_of_seed_geometries_vertex_shader_source;
	render_fill_of_seed_geometries_vertex_shader_source.add_code_segment("#define FILL_REGION_OF_INTEREST\n");
	render_fill_of_seed_geometries_vertex_shader_source.add_code_segment("#define ENABLE_SEED_FRUSTUM_CLIPPING\n");
	render_fill_of_seed_geometries_vertex_shader_source.add_code_segment_from_file(GLShaderSource::UTILS_FILE_NAME);
	render_fill_of_seed_geometries_vertex_shader_source.add_code_segment_from_file(RENDER_REGION_OF_INTEREST_GEOMETRIES_VERTEX_SHADER_SOURCE_FILE_NAME);

	// Vertex shader.
	GLShader::shared_ptr_type render_fill_of_seed_geometries_vertex_shader = GLShader::create(gl, GL_VERTEX_SHADER);
	render_fill_of_seed_geometries_vertex_shader->shader_source(gl, render_fill_of_seed_geometries_vertex_shader_source);
	render_fill_of_seed_geometries_vertex_shader->compile_shader(gl);

	// Fragment shader source.
	GLShaderSource render_fill_of_seed_geometries_fragment_shader_source;
	render_fill_of_seed_geometries_fragment_shader_source.add_code_segment("#define FILL_REGION_OF_INTEREST\n");
	render_fill_of_seed_geometries_fragment_shader_source.add_code_segment("#define ENABLE_SEED_FRUSTUM_CLIPPING\n");
	render_fill_of_seed_geometries_fragment_shader_source.add_code_segment_from_file(GLShaderSource::UTILS_FILE_NAME);
	render_fill_of_seed_geometries_fragment_shader_source.add_code_segment_from_file(RENDER_REGION_OF_INTEREST_GEOMETRIES_FRAGMENT_SHADER_SOURCE_FILE_NAME);

	// Fragment shader.
	GLShader::shared_ptr_type render_fill_of_seed_geometries_fragment_shader = GLShader::create(gl, GL_FRAGMENT_SHADER);
	render_fill_of_seed_geometries_fragment_shader->shader_source(gl, render_fill_of_seed_geometries_fragment_shader_source);
	render_fill_of_seed_geometries_fragment_shader->compile_shader(gl);

	// Vertex-fragment program.
	d_render_fill_of_seed_geometries_program = GLProgram::create(gl);
	d_render_fill_of_seed_geometries_program->attach_shader(gl, render_fill_of_seed_geometries_vertex_shader);
	d_render_fill_of_seed_geometries_program->attach_shader(gl, render_fill_of_seed_geometries_fragment_shader);
	d_render_fill_of_seed_geometries_program->link_program(gl);


	//
	// Create and initialise the mask region-of-interest vertex array.
	//
	// All mask region-of-interest shader programs use the same attribute data and hence the same vertex array.
	//

	d_fill_region_of_interest_vertex_array = GLVertexArray::create(gl);

	// Bind vertex array object.
	gl.BindVertexArray(d_fill_region_of_interest_vertex_array);

	// Bind vertex element buffer object to currently bound vertex array object.
	gl.BindBuffer(GL_ELEMENT_ARRAY_BUFFER, d_streaming_vertex_element_buffer->get_buffer());

	// Bind vertex buffer object (used by vertex attribute arrays, not vertex array object).
	gl.BindBuffer(GL_ARRAY_BUFFER, d_streaming_vertex_buffer->get_buffer());

	// Specify vertex attributes in currently bound vertex buffer object.
	// This transfers each vertex attribute array (parameters + currently bound vertex buffer object)
	// to currently bound vertex array object.
	//
	// The following reflects the structure of 'struct FillRegionOfInterestVertex'.
	// It tells OpenGL how the elements of the vertex are packed together in the vertex and
	// which parts of the vertex bind to the named attributes in the shader program.
	//

	gl.EnableVertexAttribArray(0);
	gl.VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(FillRegionOfInterestVertex),
			BUFFER_OFFSET(FillRegionOfInterestVertex, fill_position));
	gl.BindAttribLocation(d_render_fill_of_seed_geometries_program,
			0, "fill_position");

	gl.EnableVertexAttribArray(1);
	gl.VertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(FillRegionOfInterestVertex),
			BUFFER_OFFSET(FillRegionOfInterestVertex, world_space_quaternion));
	gl.BindAttribLocation(d_render_fill_of_seed_geometries_program,
			1, "world_space_quaternion");

	gl.EnableVertexAttribArray(2);
	gl.VertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(FillRegionOfInterestVertex),
			BUFFER_OFFSET(FillRegionOfInterestVertex, raster_frustum_to_seed_frustum_clip_space_transform));
	gl.BindAttribLocation(d_render_fill_of_seed_geometries_program,
			2, "raster_frustum_to_seed_frustum_clip_space_transform");

	gl.EnableVertexAttribArray(3);
	gl.VertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(FillRegionOfInterestVertex),
			BUFFER_OFFSET(FillRegionOfInterestVertex, seed_frustum_to_render_target_clip_space_transform));
	gl.BindAttribLocation(d_render_fill_of_seed_geometries_program,
			3, "seed_frustum_to_render_target_clip_space_transform");

	// Now that we've changed the attribute bindings in the program object we need to
	// re-link it in order for them to take effect.
	d_render_fill_of_seed_geometries_program->link_program(gl);
}


void
GPlatesOpenGL::GLRasterCoRegistration::initialise_mask_region_of_interest_shader_program(
		GL &gl)
{
	// Add this scope to the call stack trace printed if exception thrown in this scope (eg, failure to compile/link shader).
	TRACK_CALL_STACK();

	// Vertex shader source to copy target raster moments into seed sub-viewport with region-of-interest masking.
	GLShaderSource mask_region_of_interest_moments_vertex_shader_source;
	mask_region_of_interest_moments_vertex_shader_source.add_code_segment("#define FILTER_MOMENTS\n");
	mask_region_of_interest_moments_vertex_shader_source.add_code_segment_from_file(GLShaderSource::UTILS_FILE_NAME);
	mask_region_of_interest_moments_vertex_shader_source.add_code_segment_from_file(MASK_REGION_OF_INTEREST_VERTEX_SHADER_SOURCE_FILE_NAME);

	// Vertex shader.
	GLShader::shared_ptr_type mask_region_of_interest_moments_vertex_shader = GLShader::create(gl, GL_VERTEX_SHADER);
	mask_region_of_interest_moments_vertex_shader->shader_source(gl, mask_region_of_interest_moments_vertex_shader_source);
	mask_region_of_interest_moments_vertex_shader->compile_shader(gl);

	// Fragment shader source to copy target raster moments into seed sub-viewport with region-of-interest masking.
	GLShaderSource mask_region_of_interest_moments_fragment_shader_source;
	mask_region_of_interest_moments_fragment_shader_source.add_code_segment("#define FILTER_MOMENTS\n");
	mask_region_of_interest_moments_fragment_shader_source.add_code_segment_from_file(GLShaderSource::UTILS_FILE_NAME);
	mask_region_of_interest_moments_fragment_shader_source.add_code_segment_from_file(MASK_REGION_OF_INTEREST_FRAGMENT_SHADER_SOURCE_FILE_NAME);

	// Fragment shader.
	GLShader::shared_ptr_type mask_region_of_interest_moments_fragment_shader = GLShader::create(gl, GL_FRAGMENT_SHADER);
	mask_region_of_interest_moments_fragment_shader->shader_source(gl, mask_region_of_interest_moments_fragment_shader_source);
	mask_region_of_interest_moments_fragment_shader->compile_shader(gl);

	// Vertex-fragment program.
	d_mask_region_of_interest_moments_program = GLProgram::create(gl);
	d_mask_region_of_interest_moments_program->attach_shader(gl, mask_region_of_interest_moments_vertex_shader);
	d_mask_region_of_interest_moments_program->attach_shader(gl, mask_region_of_interest_moments_fragment_shader);
	d_mask_region_of_interest_moments_program->link_program(gl);

	// Vertex shader to copy target raster min/max into seed sub-viewport with region-of-interest masking.
	GLShaderSource mask_region_of_interest_minmax_vertex_shader_source;
	mask_region_of_interest_minmax_vertex_shader_source.add_code_segment("#define FILTER_MIN_MAX\n");
	mask_region_of_interest_minmax_vertex_shader_source.add_code_segment_from_file(GLShaderSource::UTILS_FILE_NAME);
	mask_region_of_interest_minmax_vertex_shader_source.add_code_segment_from_file(MASK_REGION_OF_INTEREST_VERTEX_SHADER_SOURCE_FILE_NAME);

	// Vertex shader.
	GLShader::shared_ptr_type mask_region_of_interest_minmax_vertex_shader = GLShader::create(gl, GL_VERTEX_SHADER);
	mask_region_of_interest_minmax_vertex_shader->shader_source(gl, mask_region_of_interest_minmax_vertex_shader_source);
	mask_region_of_interest_minmax_vertex_shader->compile_shader(gl);

	// Fragment shader to copy target raster min/max into seed sub-viewport with region-of-interest masking.
	GLShaderSource mask_region_of_interest_minmax_fragment_shader_source;
	mask_region_of_interest_minmax_fragment_shader_source.add_code_segment("#define FILTER_MIN_MAX\n");
	mask_region_of_interest_minmax_fragment_shader_source.add_code_segment_from_file(GLShaderSource::UTILS_FILE_NAME);
	mask_region_of_interest_minmax_fragment_shader_source.add_code_segment_from_file(MASK_REGION_OF_INTEREST_FRAGMENT_SHADER_SOURCE_FILE_NAME);

	// Fragment shader.
	GLShader::shared_ptr_type mask_region_of_interest_minmax_fragment_shader = GLShader::create(gl, GL_FRAGMENT_SHADER);
	mask_region_of_interest_minmax_fragment_shader->shader_source(gl, mask_region_of_interest_minmax_fragment_shader_source);
	mask_region_of_interest_minmax_fragment_shader->compile_shader(gl);

	// Vertex-fragment program.
	d_mask_region_of_interest_minmax_program = GLProgram::create(gl);
	d_mask_region_of_interest_minmax_program->attach_shader(gl, mask_region_of_interest_minmax_vertex_shader);
	d_mask_region_of_interest_minmax_program->attach_shader(gl, mask_region_of_interest_minmax_fragment_shader);
	d_mask_region_of_interest_minmax_program->link_program(gl);


	//
	// Create and initialise the mask region-of-interest vertex array.
	//
	// All mask region-of-interest shader programs use the same attribute data and hence the same vertex array.
	//

	d_mask_region_of_interest_vertex_array = GLVertexArray::create(gl);

	// Bind vertex array object.
	gl.BindVertexArray(d_mask_region_of_interest_vertex_array);

	// Bind vertex element buffer object to currently bound vertex array object.
	gl.BindBuffer(GL_ELEMENT_ARRAY_BUFFER, d_streaming_vertex_element_buffer->get_buffer());

	// Bind vertex buffer object (used by vertex attribute arrays, not vertex array object).
	gl.BindBuffer(GL_ARRAY_BUFFER, d_streaming_vertex_buffer->get_buffer());

	// Specify vertex attributes in currently bound vertex buffer object.
	// This transfers each vertex attribute array (parameters + currently bound vertex buffer object)
	// to currently bound vertex array object.
	//
	// The following reflects the structure of 'struct MaskRegionOfInterestVertex'.
	// It tells OpenGL how the elements of the vertex are packed together in the vertex and
	// which parts of the vertex bind to the named attributes in the shader program.
	//
	gl.EnableVertexAttribArray(0);
	gl.VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(MaskRegionOfInterestVertex),
			BUFFER_OFFSET(MaskRegionOfInterestVertex, screen_space_position));
	gl.EnableVertexAttribArray(1);
	gl.VertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(MaskRegionOfInterestVertex),
			BUFFER_OFFSET(MaskRegionOfInterestVertex, raster_frustum_to_seed_frustum_clip_space_transform));
	gl.EnableVertexAttribArray(2);
	gl.VertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(MaskRegionOfInterestVertex),
			BUFFER_OFFSET(MaskRegionOfInterestVertex, seed_frustum_to_render_target_clip_space_transform));
}


void
GPlatesOpenGL::GLRasterCoRegistration::initialise_reduction_of_region_of_interest_shader_programs(
		GL &gl)
{
	// Add this scope to the call stack trace printed if exception thrown in this scope (eg, failure to compile/link shader).
	TRACK_CALL_STACK();

	//
	// Compile the common vertex shader used by all reduction operation shader programs.
	//

	// Vertex shader source.
	GLShaderSource reduction_vertex_shader_source;
	reduction_vertex_shader_source.add_code_segment("#define FILTER_MOMENTS\n");
	reduction_vertex_shader_source.add_code_segment_from_file(GLShaderSource::UTILS_FILE_NAME);
	reduction_vertex_shader_source.add_code_segment_from_file(REDUCTION_OF_REGION_OF_INTEREST_VERTEX_SHADER_SOURCE_FILE_NAME);

	// Vertex shader.
	GLShader::shared_ptr_type reduction_vertex_shader = GLShader::create(gl, GL_VERTEX_SHADER);
	reduction_vertex_shader->shader_source(gl, reduction_vertex_shader_source);
	reduction_vertex_shader->compile_shader(gl);

	//
	// Fragment shader to calculate the sum of region-of-interest filter results.
	//

	// Fragment shader source.
	GLShaderSource reduction_sum_fragment_shader_source;
	reduction_sum_fragment_shader_source.add_code_segment("#define REDUCTION_SUM\n");
	reduction_sum_fragment_shader_source.add_code_segment_from_file(GLShaderSource::UTILS_FILE_NAME);
	reduction_sum_fragment_shader_source.add_code_segment_from_file(REDUCTION_OF_REGION_OF_INTEREST_FRAGMENT_SHADER_SOURCE_FILE_NAME);

	// Fragment shader.
	GLShader::shared_ptr_type reduction_sum_fragment_shader = GLShader::create(gl, GL_FRAGMENT_SHADER);
	reduction_sum_fragment_shader->shader_source(gl, reduction_sum_fragment_shader_source);
	reduction_sum_fragment_shader->compile_shader(gl);

	// Vertex-fragment program.
	d_reduction_sum_program = GLProgram::create(gl);
	d_reduction_sum_program->attach_shader(gl, reduction_vertex_shader);
	d_reduction_sum_program->attach_shader(gl, reduction_sum_fragment_shader);
	d_reduction_sum_program->link_program(gl);

	//
	// Fragment shader to calculate the minimum of region-of-interest filter results.
	//

	// Fragment shader source.
	GLShaderSource reduction_min_fragment_shader_source;
	reduction_min_fragment_shader_source.add_code_segment("#define REDUCTION_MIN\n");
	reduction_min_fragment_shader_source.add_code_segment_from_file(GLShaderSource::UTILS_FILE_NAME);
	reduction_min_fragment_shader_source.add_code_segment_from_file(REDUCTION_OF_REGION_OF_INTEREST_FRAGMENT_SHADER_SOURCE_FILE_NAME);

	// Fragment shader.
	GLShader::shared_ptr_type reduction_min_fragment_shader = GLShader::create(gl, GL_FRAGMENT_SHADER);
	reduction_min_fragment_shader->shader_source(gl, reduction_min_fragment_shader_source);
	reduction_min_fragment_shader->compile_shader(gl);

	// Vertex-fragment program.
	d_reduction_min_program = GLProgram::create(gl);
	d_reduction_min_program->attach_shader(gl, reduction_vertex_shader);
	d_reduction_min_program->attach_shader(gl, reduction_min_fragment_shader);
	d_reduction_min_program->link_program(gl);

	//
	// Fragment shader to calculate the maximum of region-of-interest filter results.
	//

	// Fragment shader source.
	GLShaderSource reduction_max_fragment_shader_source;
	reduction_max_fragment_shader_source.add_code_segment("#define REDUCTION_MAX\n");
	reduction_max_fragment_shader_source.add_code_segment_from_file(GLShaderSource::UTILS_FILE_NAME);
	reduction_max_fragment_shader_source.add_code_segment_from_file(REDUCTION_OF_REGION_OF_INTEREST_FRAGMENT_SHADER_SOURCE_FILE_NAME);

	// Fragment shader.
	GLShader::shared_ptr_type reduction_max_fragment_shader = GLShader::create(gl, GL_FRAGMENT_SHADER);
	reduction_max_fragment_shader->shader_source(gl, reduction_max_fragment_shader_source);
	reduction_max_fragment_shader->compile_shader(gl);

	// Vertex-fragment program.
	d_reduction_max_program = GLProgram::create(gl);
	d_reduction_max_program->attach_shader(gl, reduction_vertex_shader);
	d_reduction_max_program->attach_shader(gl, reduction_max_fragment_shader);
	d_reduction_max_program->link_program(gl);
}


void
GPlatesOpenGL::GLRasterCoRegistration::initialise_reduction_of_region_of_interest_vertex_array(
		GL &gl)
{
	std::vector<GLVertexUtils::TextureVertex> vertices;
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

	d_reduction_vertex_array = GLVertexArray::create(gl);
	d_reduction_vertex_buffer = GLBuffer::create(gl);
	d_reduction_vertex_element_buffer = GLBuffer::create(gl);

	// Bind vertex array object.
	gl.BindVertexArray(d_reduction_vertex_array);

	// Bind vertex element buffer object to currently bound vertex array object.
	gl.BindBuffer(GL_ELEMENT_ARRAY_BUFFER, d_reduction_vertex_element_buffer);

	// Transfer vertex element data to currently bound vertex element buffer object.
	gl.BufferData(
			GL_ELEMENT_ARRAY_BUFFER,
			vertex_elements.size() * sizeof(vertex_elements[0]),
			vertex_elements.data(),
			GL_STATIC_DRAW);

	// Bind vertex buffer object (used by vertex attribute arrays, not vertex array object).
	gl.BindBuffer(GL_ARRAY_BUFFER, d_reduction_vertex_buffer);

	// Transfer vertex data to currently bound vertex buffer object.
	gl.BufferData(
			GL_ARRAY_BUFFER,
			vertices.size() * sizeof(vertices[0]),
			vertices.data(),
			GL_STATIC_DRAW);

	// Specify vertex attributes (position and texture coord) in currently bound vertex buffer object.
	// This transfers each vertex attribute array (parameters + currently bound vertex buffer object)
	// to currently bound vertex array object.
	gl.EnableVertexAttribArray(0);
	gl.VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLVertexUtils::TextureVertex),
			BUFFER_OFFSET(GLVertexUtils::TextureVertex, x));
	gl.EnableVertexAttribArray(1);
	gl.VertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GLVertexUtils::TextureVertex),
			BUFFER_OFFSET(GLVertexUtils::TextureVertex, u));
}


void
GPlatesOpenGL::GLRasterCoRegistration::initialise_reduction_vertex_array_in_quad_tree_traversal_order(
		std::vector<GLVertexUtils::TextureVertex> &vertices,
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

		vertices.push_back(GLVertexUtils::TextureVertex(x0, y0, 0, u0, v0));
		vertices.push_back(GLVertexUtils::TextureVertex(x0, y1, 0, u0, v1));
		vertices.push_back(GLVertexUtils::TextureVertex(x1, y1, 0, u1, v1));
		vertices.push_back(GLVertexUtils::TextureVertex(x1, y0, 0, u1, v0));

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
		GL &gl,
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
					GLViewProjection(
							GLViewport(0, 0, TEXTURE_DIMENSION, TEXTURE_DIMENSION),
							view_transform->get_matrix(),
							projection_transform->get_matrix()),
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
	//  - difficulty having a reduce quad tree (a quad tree is a simple and elegant solution to this problem),
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
		GL &gl,
		std::vector<Operation> &operations,
		const std::vector<GPlatesAppLogic::ReconstructContext::ReconstructedFeature> &seed_features,
		const GLMultiResolutionRasterInterface::non_null_ptr_type &target_raster,
		unsigned int raster_level_of_detail)
{
	PROFILE_FUNC();

	// Make sure we leave the OpenGL global state the way it was.
	// We don't really need this (since we already save/restore where needed) but put it here just in case.
	GL::StateScope save_restore_state(gl);


	//
	// The following is *preparation* for co-registration processing...
	//

	// Ensure the raster level of detail is within a valid range.
	raster_level_of_detail = boost::numeric_cast<unsigned int>(
			target_raster->clamp_level_of_detail(raster_level_of_detail));

	// Initialise details to do with texture viewports and cube quad tree level-of-detail
	// transitions that depend on the target raster *resolution*.
	unsigned int raster_texture_cube_quad_tree_depth;
	unsigned int seed_geometries_spatial_partition_depth;
	initialise_texture_level_of_detail_parameters(
			gl,
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
	ResultsQueue results_queue(gl);


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
	filter_reduce_seed_geometries_spatial_partition(gl, co_registration_parameters);

	// Finally make sure the results from the GPU are flushed before we return results to the caller.
	// This is done last to minimise any blocking required to wait for the GPU to finish generating
	// the result data and transferring it to CPU memory.
	//
	// TODO: Delay this until the caller actually retrieves the results - then we can advise clients
	// to delay the retrieval of results by doing other work in between initiating the co-registration
	// (this method) and actually reading the results (allowing greater parallelism between GPU and CPU).
	results_queue.flush_results(gl, seed_feature_partial_results);

	// Now that the results have all been retrieved from the GPU we need combine multiple
	// (potentially partial) co-registration results into a single result per seed feature.
	return_co_registration_results_to_caller(co_registration_parameters);
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
	std::vector<GPlatesMaths::AngularExtent> operation_regions_of_interest;
	for (const Operation &operation : operations)
	{
		operation_regions_of_interest.push_back(
			GPlatesMaths::AngularExtent::create_from_angle(operation.d_region_of_interest_radius));
	}

	// Add the seed feature geometries to the spatial partition.
	for (unsigned int feature_index = 0; feature_index < seed_features.size(); ++feature_index)
	{
		const GPlatesAppLogic::ReconstructContext::ReconstructedFeature &reconstructed_feature = seed_features[feature_index];

		// Each seed feature could have multiple geometries.
		const GPlatesAppLogic::ReconstructContext::ReconstructedFeature::reconstruction_seq_type &reconstructions =
				reconstructed_feature.get_reconstructions();
		for (const GPlatesAppLogic::ReconstructContext::Reconstruction &reconstruction : reconstructions)
		{
			// NOTE: To avoid reconstructing geometries when it might not be needed we add the
			// *unreconstructed* geometry (and a finite rotation) to the spatial partition.
			// The spatial partition will rotate only the centroid of the *unreconstructed*
			// geometry (instead of reconstructing the entire geometry) and then use that as the
			// insertion location (along with the *unreconstructed* geometry's bounding circle extents).
			// An example where transforming might not be needed is data mining co-registration
			// where might not need to transform all geometries to determine if seed and target
			// features are close enough within a region of interest.

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
				// Hence we can't assume its shape is the same and hence can't assume the
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
		GL &gl,
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
				gl,
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
		GL &gl,
		const CoRegistrationParameters &co_registration_parameters,
		seed_geometries_spatial_partition_type::node_reference_type seed_geometries_spatial_partition_node,
		const seed_geometries_spatial_partition_node_list_type &parent_seed_geometries_intersecting_node_list,
		const seed_geometries_intersecting_nodes_type &seed_geometries_intersecting_nodes,
		cube_subdivision_cache_type &cube_subdivision_cache,
		const cube_subdivision_cache_type::node_reference_type &cube_subdivision_cache_node,
		unsigned int level_of_detail)
{
	// If we've reached the level-of-detail at which to render the target raster .
	if (level_of_detail == co_registration_parameters.raster_texture_cube_quad_tree_depth)
	{
		co_register_seed_geometries(
				gl,
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
					gl,
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
		GL &gl,
		const CoRegistrationParameters &co_registration_parameters,
		seed_geometries_spatial_partition_type::node_reference_type seed_geometries_spatial_partition_node,
		const seed_geometries_spatial_partition_node_list_type &parent_seed_geometries_intersecting_node_list,
		const seed_geometries_intersecting_nodes_type &seed_geometries_intersecting_nodes,
		cube_subdivision_cache_type &cube_subdivision_cache,
		const cube_subdivision_cache_type::node_reference_type &cube_subdivision_cache_node)
{
	// Co-register any seed geometries collected so far during the cube quad tree traversal.
	co_register_seed_geometries_with_target_raster(
			gl,
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
						gl,
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
		GL &gl,
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

	// The centre of the cube face currently being visited.
	// This is used to adjust for the area-sampling distortion of pixels introduced by the cube map.
	const GPlatesMaths::UnitVector3D &cube_face_centre =
		GPlatesMaths::CubeCoordinateFrame::get_cube_face_centre(
					cube_subdivision_cache_node.get_cube_face());

	// Now that we have a list of seed geometries we can co-register them with the current target raster tile.
	co_register_seed_geometries_with_target_raster(
			gl,
			co_registration_parameters,
			seed_geometries_intersecting_node_list,
			cube_face_centre,
			view_transform,
			projection_transform);
}


void
GPlatesOpenGL::GLRasterCoRegistration::co_register_seed_geometries_with_target_raster(
		GL &gl,
		const CoRegistrationParameters &co_registration_parameters,
		const seed_geometries_spatial_partition_node_list_type &seed_geometries_intersecting_node_list,
		const GPlatesMaths::UnitVector3D &cube_face_centre,
		const GLTransform::non_null_ptr_to_const_type &view_transform,
		const GLTransform::non_null_ptr_to_const_type &projection_transform)
{
	// Acquire a floating-point texture to render the target raster into.
	GLTexture::shared_ptr_type target_raster_texture = acquire_rgba_float_texture(gl);

	// Render the target raster into the view frustum (into render texture).
	if (!render_target_raster(gl, co_registration_parameters, target_raster_texture, *view_transform, *projection_transform))
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
				gl,
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
	for (const auto &node : seed_geometries_intersecting_node_list)
	{
		const seed_geometries_spatial_partition_type::node_reference_type &node_reference = node.node_reference;

		// Iterate over the seed co-registrations of the current node.
		for (SeedCoRegistration &seed_co_registration : node_reference)
		{
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
		GL &gl,
		const CoRegistrationParameters &co_registration_parameters,
		seed_geometries_spatial_partition_type::node_reference_type seed_geometries_spatial_partition_node,
		cube_subdivision_cache_type &cube_subdivision_cache,
		const cube_subdivision_cache_type::node_reference_type &cube_subdivision_cache_node)
{
	// Acquire a floating-point texture to render the target raster into.
	GLTexture::shared_ptr_type target_raster_texture = acquire_rgba_float_texture(gl);

	const GLTransform::non_null_ptr_to_const_type view_transform =
			cube_subdivision_cache.get_view_transform(cube_subdivision_cache_node);
	// NOTE: We are now rendering to *loose* tiles (frustums) so use loose projection transform.
	const GLTransform::non_null_ptr_to_const_type projection_transform =
			cube_subdivision_cache.get_loose_projection_transform(cube_subdivision_cache_node);

	// Render the target raster into the view frustum (into render texture).
	if (!render_target_raster(gl, co_registration_parameters, target_raster_texture, *view_transform, *projection_transform))
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

	// The centre of the cube face currently being visited.
	// This is used to adjust for the area-sampling distortion of pixels introduced by the cube map.
	const GPlatesMaths::UnitVector3D &cube_face_centre =
		GPlatesMaths::CubeCoordinateFrame::get_cube_face_centre(
					cube_subdivision_cache_node.get_cube_face());

	// Iterate over the operations and co-register the seed geometries associated with each operation.
	for (unsigned int operation_index = 0;
		operation_index < co_registration_parameters.operations.size();
		++operation_index)
	{
		render_seed_geometries_to_reduce_pyramids(
				gl,
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
	for (SeedCoRegistration &seed_co_registration : seed_geometries_spatial_partition_node)
	{
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
		GL &gl,
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

	SeedCoRegistrationReduceStageLists &operation_reduce_stage_list = operation_reduce_stage_lists[operation_index];

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
						gl,
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
		co_registration_parameters.results_queue.queue_reduce_pyramid_output(
				gl,
				d_framebuffer,
				d_have_checked_framebuffer_completeness,
				reduce_stage_textures[NUM_REDUCE_STAGES - 1].get(),
				reduce_quad_tree,
				co_registration_parameters.seed_feature_partial_results);
	}
	while (reduce_stage_index < NUM_REDUCE_STAGES); // While not yet finished with all reduce stages.
}


unsigned int
GPlatesOpenGL::GLRasterCoRegistration::render_seed_geometries_to_reduce_quad_tree_internal_node(
		GL &gl,
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
								gl,
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
							acquire_rgba_float_texture(gl);

					// Clear acquired texture - there are no partial results.
					clear_reduce_texture = true;
				}

				// Render the geometries into the reduce stage texture.
				render_seed_geometries_in_reduce_stage_render_list(
						gl,
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
							acquire_rgba_float_texture(gl);

					// Clear acquired texture - there are no partial results.
					clear_parent_reduce_texture = true;
				}

				// Do the 2x2 -> 1x1 reduction.
				//
				// NOTE: If we ran out of geometries before the child sub-tree could be filled then
				// this could be a reduction of *less* than TEXTURE_DIMENSION x TEXTURE_DIMENSION pixels.
				render_reduction_of_reduce_stage(
						gl,
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
		GL &gl,
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

	//
	// Prepare for rendering into the region-of-interest mask texture.
	//

	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state_region_of_interest_mask(
			gl,
			// We're rendering to a render target so reset to the default OpenGL state...
			true/*reset_to_default_state*/);

	// Acquire a floating-point texture to render the region-of-interest masks into.
	GLTexture::shared_ptr_type region_of_interest_mask_texture = acquire_rgba_float_texture(gl);

	// Bind our framebuffer object.
	gl.BindFramebuffer(GL_FRAMEBUFFER, d_framebuffer);

	// Begin rendering to the region-of-interest mask texture.
	gl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, region_of_interest_mask_texture, 0/*level*/);

	// Check our framebuffer object for completeness (now that a texture is attached to it).
	// We only need to do this once because, while the texture changes, the framebuffer configuration
	// does not (ie, same texture internal format, dimensions, etc).
	if (!d_have_checked_framebuffer_completeness)
	{
		// Throw OpenGLException if not complete.
		// This should succeed since we should only be using texture formats that are required by OpenGL 3.3 core.
		const GLenum completeness = gl.CheckFramebufferStatus(GL_FRAMEBUFFER);
		GPlatesGlobal::Assert<OpenGLException>(
				completeness == GL_FRAMEBUFFER_COMPLETE,
				GPLATES_ASSERTION_SOURCE,
				"Framebuffer not complete for coregistering rasters.");

		d_have_checked_framebuffer_completeness = true;
	}

	// Render to the entire texture.
	gl.Viewport(0, 0, TEXTURE_DIMENSION, TEXTURE_DIMENSION);

	// Clear the region-of-interest mask texture.
	// Clear colour to all zeros - only those areas inside the regions-of-interest will be non-zero.
	gl.ClearColor();
	gl.Clear(GL_COLOR_BUFFER_BIT); // Clear only the colour buffer.

	// The view projection.
	//
	// All seed geometries will use the same view/projection matrices and simply make
	// post-projection adjustments in the vertex shader as needed (using vertex data - attributes).
	// NOTE: This greatly minimises the number of OpenGL calls we need to make (each OpenGL call
	// can be quite expensive in terms of CPU cost - very little GPU cost though) since it avoids
	// per-seed-geometry OpenGL calls and there could be *lots* of seed geometries.
	const GLViewProjection target_raster_view_projection(
			GLViewport(0, 0, TEXTURE_DIMENSION, TEXTURE_DIMENSION),
			target_raster_view_transform->get_matrix(),
			target_raster_projection_transform->get_matrix());

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
				gl,
				target_raster_view_projection,
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
					gl,
					target_raster_view_projection,
					geometry_lists,
					operation.d_region_of_interest_radius);
		}
		else
		{
			// Render the line region-of-interest geometries (meshes).
			// The seed geometry is *not* bounded by a loose cube quad tree tile.
			render_unbounded_line_region_of_interest_geometries(
					gl,
					target_raster_view_projection,
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
					gl,
					target_raster_view_projection,
					geometry_lists,
					operation.d_region_of_interest_radius);
		}
		else
		{
			// Render the point region-of-interest geometries (meshes).
			// The seed geometry is *not* bounded by a loose cube quad tree tile.
			render_unbounded_point_region_of_interest_geometries(
					gl,
					target_raster_view_projection,
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
			gl,
			target_raster_view_projection,
			geometry_lists);

	// As an extra precaution we also render the point region-of-interest geometries as points (not quads) of
	// point size 1 (with no anti-aliasing). This is done in case the region-of-interest radius is so
	// small that the point quads fall between pixels in the render target because the points are so small.
	//
	// UPDATE: This is also required for the case when a zero region-of-interest radius is specified
	// which can be used to indicate point-sampling rather than area sampling.
	render_single_pixel_size_point_region_of_interest_geometries(
			gl,
			target_raster_view_projection,
			geometry_lists);

	//debug_floating_point_render_target(gl, "region_of_interest_mask", false/*coverage_is_in_green_channel*/);

	//
	// Now that we've generated the region-of-interest masks we can copy seed sub-viewport sections
	// of the target raster texture into the reduce stage texture with region-of-interest masking.
	//
	// Prepare for rendering into the reduce stage floating-point texture.
	//

	// Bind our framebuffer object.
	gl.BindFramebuffer(GL_FRAMEBUFFER, d_framebuffer);

	// Begin rendering to the floating-point reduce stage texture.
	gl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, reduce_stage_texture, 0/*level*/);

	// Check our framebuffer object for completeness (now that a texture is attached to it).
	// We only need to do this once because, while the texture changes, the framebuffer configuration
	// does not (ie, same texture internal format, dimensions, etc).
	if (!d_have_checked_framebuffer_completeness)
	{
		// Throw OpenGLException if not complete.
		// This should succeed since we should only be using texture formats that are required by OpenGL 3.3 core.
		const GLenum completeness = gl.CheckFramebufferStatus(GL_FRAMEBUFFER);
		GPlatesGlobal::Assert<OpenGLException>(
				completeness == GL_FRAMEBUFFER_COMPLETE,
				GPLATES_ASSERTION_SOURCE,
				"Framebuffer not complete for coregistering rasters.");

		d_have_checked_framebuffer_completeness = true;
	}

	// No need to change the viewport - it's already TEXTURE_DIMENSION x TEXTURE_DIMENSION;

	// If the reduce stage texture does not contain partial results then it'll need to be cleared.
	// This happens when starting afresh with a newly acquired reduce stage texture.
	if (clear_reduce_stage_texture)
	{
		// Clear colour to all zeros - this means pixels outside the regions-of-interest will
		// have coverage values of zero (causing them to not contribute to the co-registration result).
		gl.ClearColor();
		gl.Clear(GL_COLOR_BUFFER_BIT); // Clear only the colour buffer.
	}

	// We use the same view and projection matrices (set for the target raster) but,
	// in the vertex shader, we don't transform the vertex position using them.
	// This is because the vertices will be in screen-space (range [-1,1]) and will only need
	// a translate/scale adjustment of the 'x' and 'y' components to effectively map its texture
	// coordinates to the appropriate sub-viewport of the target raster texture and map its position
	// to the appropriate sub-viewport of the render target.
	// And these adjustments will be transferred to the vertex shader using vertex data (attributes).
	//
	// We do however use the inverse view-projection matrix to inverse transform from screen-space
	// back to view space so we can then perform a dot-product of the (normalised) view space position
	// with the cube face centre in order to adjust the raster coverage to counteract the distortion
	// of a pixel's area on the surface of the globe.
	// Near the face corners a cube map pixel projects to a smaller area on the globe than at the
	// cube face centre. This can affect area-weighted operations like mean and standard deviation
	// which assume each pixel projects to the same area on the globe. The dot product or cosine
	// adjustment counteracts that (assuming each pixel is infinitesimally small - which is close
	// enough for small pixels).
	//
	// We could have done the cube map distortion adjustment when rendering the region-of-interest
	// geometries but we do it with the floating-point render target here instead.
	// Note that originally we rendered region-of-interest geometries to a *fixed-point* (8-bit per component)
	// render target which lacked precision that would have introduced noise into the co-registration operation
	// (eg, mean, std-dev), and so we did it with the floating-point render target here instead.
	// But now we render region-of-interest geometries to a *floating-point* render target, so we
	// could now do it there (instead of here). However just keeping it here (for minimal changes).
	//

	//
	// Render the seed sub-viewports of the regions-of-interest of all seed geometries.
	// This means geometries that are points, multipoints, polylines and polygons.
	//

	// Copy the target raster to the reduce stage texture with region-of-interest masking.
	mask_target_raster_with_regions_of_interest(
			gl,
			operation,
			cube_face_centre,
			target_raster_view_projection,
			target_raster_texture,
			region_of_interest_mask_texture,
			geometry_lists);

	//debug_floating_point_render_target(
	//		gl, "region_of_interest_masked_raster", false/*coverage_is_in_green_channel*/);
}


void
GPlatesOpenGL::GLRasterCoRegistration::render_bounded_point_region_of_interest_geometries(
		GL &gl,
		const GLViewProjection &target_raster_view_projection,
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
	gl.UseProgram(d_render_points_of_seed_geometries_with_small_roi_angle_program);

	const double tan_region_of_interest_angle = std::tan(region_of_interest_radius);
	const double tan_squared_region_of_interest_angle = tan_region_of_interest_angle * tan_region_of_interest_angle;

	// Set the view-projection matrix.
	GLfloat view_projection_float_matrix[16];
	target_raster_view_projection.get_view_projection_transform().get_float_matrix(view_projection_float_matrix);
	gl.UniformMatrix4fv(
			d_render_points_of_seed_geometries_with_small_roi_angle_program->get_uniform_location(gl, "view_projection"),
			1, GL_FALSE/*transpose*/, view_projection_float_matrix);

	// Set the region-of-interest radius.
	gl.Uniform1f(
			d_render_points_of_seed_geometries_with_small_roi_angle_program->get_uniform_location(gl, "tan_squared_region_of_interest_angle"),
			tan_squared_region_of_interest_angle);

	// Bind the point region-of-interest vertex array.
	gl.BindVertexArray(d_point_region_of_interest_vertex_array);

	// Need to bind vertex buffer before streaming into it.
	//
	// Note that we don't bind the vertex element buffer since binding the vertex array does that
	// (however it does not bind the GL_ARRAY_BUFFER, only records vertex attribute buffers).
	gl.BindBuffer(GL_ARRAY_BUFFER, d_streaming_vertex_buffer->get_buffer());

	// For streaming PointRegionOfInterestVertex vertices.
	point_region_of_interest_stream_primitives_type point_stream;
	point_region_of_interest_stream_primitives_type::MapStreamBufferScope point_stream_target(
			gl,
			point_stream,
			*d_streaming_vertex_element_buffer,
			*d_streaming_vertex_buffer,
			MINIMUM_BYTES_TO_STREAM_IN_VERTEX_ELEMENT_BUFFER,
			MINIMUM_BYTES_TO_STREAM_IN_VERTEX_BUFFER);

	// Start streaming point region-of-interest geometries.
	point_stream_target.start_streaming();

	point_region_of_interest_stream_primitives_type::Primitives point_stream_quads(point_stream);

	// Iterate over the point geometries.
	seed_co_registration_points_list_type::const_iterator points_iter = geometry_lists.points_list.begin();
	seed_co_registration_points_list_type::const_iterator points_end = geometry_lists.points_list.end();
	for ( ; points_iter != points_end; ++points_iter)
	{
		const SeedCoRegistration &seed_co_registration = *points_iter;

		// We're currently traversing the 'GPlatesMaths::PointOnSphere' list so the dynamic_cast should not fail.
		const GPlatesMaths::PointOnSphere &point_on_sphere =
				dynamic_cast<const GPlatesMaths::PointGeometryOnSphere &>(seed_co_registration.geometry).position();

		// Most of the vertex data is the same for all vertices in the seed geometry.
		PointRegionOfInterestVertex vertex;
		vertex.initialise_seed_geometry_constants(seed_co_registration);

		render_bounded_point_region_of_interest_geometry(
				gl,
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
					gl,
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
					gl,
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

		// Iterate over the points of the exterior ring of the current polygon.
		GPlatesMaths::PolygonOnSphere::ring_vertex_const_iterator exterior_points_iter = polygon_on_sphere.exterior_ring_vertex_begin();
		GPlatesMaths::PolygonOnSphere::ring_vertex_const_iterator exterior_points_end = polygon_on_sphere.exterior_ring_vertex_end();
		for ( ; exterior_points_iter != exterior_points_end; ++exterior_points_iter)
		{
			const GPlatesMaths::PointOnSphere &point = *exterior_points_iter;

			// Render the point region-of-interest geometry filling in the vertex data attributes
			// that are *not* constant across the seed geometry.
			render_bounded_point_region_of_interest_geometry(
					gl,
					point_stream_target,
					point_stream_quads,
					point.position_vector(),
					vertex,
					tan_region_of_interest_angle);
		}

		// Iterate over the points of the interior rings of the current polygon (if any).
		const unsigned int num_interior_rings = polygon_on_sphere.number_of_interior_rings();
		for (unsigned int interior_ring_index = 0; interior_ring_index < num_interior_rings; ++interior_ring_index)
		{
			GPlatesMaths::PolygonOnSphere::ring_vertex_const_iterator interior_points_iter =
					polygon_on_sphere.interior_ring_vertex_begin(interior_ring_index);
			GPlatesMaths::PolygonOnSphere::ring_vertex_const_iterator interior_points_end =
					polygon_on_sphere.interior_ring_vertex_end(interior_ring_index);
			for ( ; interior_points_iter != interior_points_end; ++interior_points_iter)
			{
				const GPlatesMaths::PointOnSphere &point = *interior_points_iter;

				// Render the point region-of-interest geometry filling in the vertex data attributes
				// that are *not* constant across the seed geometry.
				render_bounded_point_region_of_interest_geometry(
						gl,
						point_stream_target,
						point_stream_quads,
						point.position_vector(),
						vertex,
						tan_region_of_interest_angle);
			}
		}
	}

	// Stop streaming point region-of-interest geometries so we can render the last batch.
	if (point_stream_target.stop_streaming())
	{	// Only render if buffer contents are not undefined...

		// Render the last batch of streamed point region-of-interest geometries (if any).
		if (point_stream_target.get_num_streamed_vertex_elements() > 0)
		{
			gl.DrawRangeElements(
					GL_TRIANGLES,
					point_stream_target.get_start_streaming_vertex_count()/*start*/,
					point_stream_target.get_start_streaming_vertex_count() +
							point_stream_target.get_num_streamed_vertices() - 1/*end*/,
					point_stream_target.get_num_streamed_vertex_elements()/*count*/,
					GLVertexUtils::ElementTraits<streaming_vertex_element_type>::type,
					GLVertexUtils::buffer_offset(
							point_stream_target.get_start_streaming_vertex_element_count() *
									sizeof(PointRegionOfInterestVertex)/*indices_offset*/));
		}
	}
}


void
GPlatesOpenGL::GLRasterCoRegistration::render_bounded_point_region_of_interest_geometry(
		GL &gl,
		point_region_of_interest_stream_primitives_type::MapStreamBufferScope &point_stream_target,
		point_region_of_interest_stream_primitives_type::Primitives &point_stream_quads,
		const GPlatesMaths::UnitVector3D &point,
		PointRegionOfInterestVertex &vertex,
		const double &tan_region_of_interest_angle)
{
	//PROFILE_FUNC();

	// There are four vertices for the current point (each point gets a quad) and
	// two triangles (three indices each).
	if (!point_stream_quads.begin_primitive(4/*max_num_vertices*/, 6/*max_num_vertex_elements*/))
	{
		// There's not enough vertices or indices so render what we have so far and obtain new stream buffers.
		if (point_stream_target.stop_streaming())
		{	// Only render if buffer contents are not undefined...
			gl.DrawRangeElements(
					GL_TRIANGLES,
					point_stream_target.get_start_streaming_vertex_count()/*start*/,
					point_stream_target.get_start_streaming_vertex_count() +
							point_stream_target.get_num_streamed_vertices() - 1/*end*/,
					point_stream_target.get_num_streamed_vertex_elements()/*count*/,
					GLVertexUtils::ElementTraits<streaming_vertex_element_type>::type,
					GLVertexUtils::buffer_offset(
							point_stream_target.get_start_streaming_vertex_element_count() *
									sizeof(PointRegionOfInterestVertex)/*indices_offset*/));
		}
		point_stream_target.start_streaming();

		point_stream_quads.begin_primitive(4/*max_num_vertices*/, 6/*max_num_vertex_elements*/);
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
		GL &gl,
		const GLViewProjection &target_raster_view_projection,
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
	GLProgram::shared_ptr_type render_points_of_seed_geometries_program;
	if (region_of_interest_radius < GPlatesMaths::PI / 4)
	{
		// Bind the shader program for rendering point regions-of-interest.
		render_points_of_seed_geometries_program = d_render_points_of_seed_geometries_with_small_roi_angle_program;
		gl.UseProgram(render_points_of_seed_geometries_program);

		// Note that 'tan' is undefined at 90 degrees but we're safe since we're restricted to 45 degrees or less.
		const double tan_region_of_interest_angle = std::tan(region_of_interest_radius);
		const double tan_squared_region_of_interest_angle = tan_region_of_interest_angle * tan_region_of_interest_angle;

		// Set the region-of-interest radius.
		gl.Uniform1f(
				render_points_of_seed_geometries_program->get_uniform_location(gl, "tan_squared_region_of_interest_angle"),
				tan_squared_region_of_interest_angle);
	}
	else // Use a shader program that's accurate for angles very near 90 degrees...
	{
		// Bind the shader program for rendering point regions-of-interest.
		render_points_of_seed_geometries_program = d_render_points_of_seed_geometries_with_large_roi_angle_program;
		gl.UseProgram(render_points_of_seed_geometries_program);

		// Set the region-of-interest radius.
		gl.Uniform1f(
				render_points_of_seed_geometries_program->get_uniform_location(gl, "cos_region_of_interest_angle"),
				cos_region_of_interest_angle);
	}

	// Set the view-projection matrix.
	GLfloat view_projection_float_matrix[16];
	target_raster_view_projection.get_view_projection_transform().get_float_matrix(view_projection_float_matrix);
	gl.UniformMatrix4fv(
			render_points_of_seed_geometries_program->get_uniform_location(gl, "view_projection"),
			1, GL_FALSE/*transpose*/, view_projection_float_matrix);

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
	gl.BindVertexArray(d_point_region_of_interest_vertex_array);

	// Need to bind vertex buffer before streaming into it.
	//
	// Note that we don't bind the vertex element buffer since binding the vertex array does that
	// (however it does not bind the GL_ARRAY_BUFFER, only records vertex attribute buffers).
	gl.BindBuffer(GL_ARRAY_BUFFER, d_streaming_vertex_buffer->get_buffer());

	// For streaming PointRegionOfInterestVertex vertices.
	point_region_of_interest_stream_primitives_type point_stream;
	point_region_of_interest_stream_primitives_type::MapStreamBufferScope point_stream_target(
			gl,
			point_stream,
			*d_streaming_vertex_element_buffer,
			*d_streaming_vertex_buffer,
			MINIMUM_BYTES_TO_STREAM_IN_VERTEX_ELEMENT_BUFFER,
			MINIMUM_BYTES_TO_STREAM_IN_VERTEX_BUFFER);

	// Start streaming point region-of-interest geometries.
	point_stream_target.start_streaming();

	point_region_of_interest_stream_primitives_type::Primitives point_stream_meshes(point_stream);

	// Iterate over the point geometries.
	for (const SeedCoRegistration &seed_co_registration : geometry_lists.points_list)
	{
		// We're currently traversing the 'GPlatesMaths::PointOnSphere' list so the dynamic_cast should not fail.
		const GPlatesMaths::PointOnSphere &point_on_sphere =
				dynamic_cast<const GPlatesMaths::PointGeometryOnSphere &>(seed_co_registration.geometry).position();

		// Most of the vertex data is the same for all vertices in the seed geometry.
		PointRegionOfInterestVertex vertex;
		vertex.initialise_seed_geometry_constants(seed_co_registration);

		render_unbounded_point_region_of_interest_geometry(
				gl,
				point_stream_target,
				point_stream_meshes,
				point_on_sphere.position_vector(),
				vertex,
				centre_point_weight,
				tangent_weight);
	}

	// Iterate over the multipoint geometries.
	for (const SeedCoRegistration &seed_co_registration : geometry_lists.multi_points_list)
	{
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
					gl,
					point_stream_target,
					point_stream_meshes,
					point.position_vector(),
					vertex,
					centre_point_weight,
					tangent_weight);
		}
	}

	// Iterate over the polyline geometries.
	for (const SeedCoRegistration &seed_co_registration : geometry_lists.polylines_list)
	{
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
					gl,
					point_stream_target,
					point_stream_meshes,
					point.position_vector(),
					vertex,
					centre_point_weight,
					tangent_weight);
		}
	}

	// Iterate over the polygon geometries.
	for (const SeedCoRegistration &seed_co_registration : geometry_lists.polygons_list)
	{
		// We're currently traversing the 'GPlatesMaths::PolygonOnSphere' list so the dynamic_cast should not fail.
		const GPlatesMaths::PolygonOnSphere &polygon_on_sphere =
				dynamic_cast<const GPlatesMaths::PolygonOnSphere &>(seed_co_registration.geometry);

		// Most of the vertex data is the same for all vertices in the seed geometry.
		PointRegionOfInterestVertex vertex;
		vertex.initialise_seed_geometry_constants(seed_co_registration);

		// Iterate over the points of the exterior ring of the current polygon.
		GPlatesMaths::PolygonOnSphere::ring_vertex_const_iterator exterior_points_iter = polygon_on_sphere.exterior_ring_vertex_begin();
		GPlatesMaths::PolygonOnSphere::ring_vertex_const_iterator exterior_points_end = polygon_on_sphere.exterior_ring_vertex_end();
		for ( ; exterior_points_iter != exterior_points_end; ++exterior_points_iter)
		{
			const GPlatesMaths::PointOnSphere &point = *exterior_points_iter;

			// Render the point region-of-interest geometry filling in the vertex data attributes
			// that are *not* constant across the seed geometry.
			render_unbounded_point_region_of_interest_geometry(
					gl,
					point_stream_target,
					point_stream_meshes,
					point.position_vector(),
					vertex,
					centre_point_weight,
					tangent_weight);
		}

		// Iterate over the points of the interior rings of the current polygon (if any).
		const unsigned int num_interior_rings = polygon_on_sphere.number_of_interior_rings();
		for (unsigned int interior_ring_index = 0; interior_ring_index < num_interior_rings; ++interior_ring_index)
		{
			GPlatesMaths::PolygonOnSphere::ring_vertex_const_iterator interior_points_iter =
					polygon_on_sphere.interior_ring_vertex_begin(interior_ring_index);
			GPlatesMaths::PolygonOnSphere::ring_vertex_const_iterator interior_points_end =
					polygon_on_sphere.interior_ring_vertex_end(interior_ring_index);
			for ( ; interior_points_iter != interior_points_end; ++interior_points_iter)
			{
				const GPlatesMaths::PointOnSphere &point = *interior_points_iter;

				// Render the point region-of-interest geometry filling in the vertex data attributes
				// that are *not* constant across the seed geometry.
				render_unbounded_point_region_of_interest_geometry(
						gl,
						point_stream_target,
						point_stream_meshes,
						point.position_vector(),
						vertex,
						centre_point_weight,
						tangent_weight);
			}
		}
	}

	// Stop streaming point region-of-interest geometries so we can render the last batch.
	if (point_stream_target.stop_streaming())
	{	// Only render if buffer contents are not undefined...

		// Render the last batch of streamed point region-of-interest geometries (if any).
		if (point_stream_target.get_num_streamed_vertex_elements() > 0)
		{
			gl.DrawRangeElements(
					GL_TRIANGLES,
					point_stream_target.get_start_streaming_vertex_count()/*start*/,
					point_stream_target.get_start_streaming_vertex_count() +
							point_stream_target.get_num_streamed_vertices() - 1/*end*/,
					point_stream_target.get_num_streamed_vertex_elements()/*count*/,
					GLVertexUtils::ElementTraits<streaming_vertex_element_type>::type,
					GLVertexUtils::buffer_offset(
							point_stream_target.get_start_streaming_vertex_element_count() *
									sizeof(PointRegionOfInterestVertex)/*indices_offset*/));
		}
	}
}


void
GPlatesOpenGL::GLRasterCoRegistration::render_unbounded_point_region_of_interest_geometry(
		GL &gl,
		point_region_of_interest_stream_primitives_type::MapStreamBufferScope &point_stream_target,
		point_region_of_interest_stream_primitives_type::Primitives &point_stream_meshes,
		const GPlatesMaths::UnitVector3D &point,
		PointRegionOfInterestVertex &vertex,
		const double &centre_point_weight,
		const double &tangent_weight)
{
	//PROFILE_FUNC();

	// There are five vertices for the current point (each point gets a fan mesh) and
	// four triangles (three indices each).
	if (!point_stream_meshes.begin_primitive(5/*max_num_vertices*/, 12/*max_num_vertex_elements*/))
	{
		// There's not enough vertices or indices so render what we have so far and obtain new stream buffers.
		if (point_stream_target.stop_streaming())
		{	// Only render if buffer contents are not undefined...
			gl.DrawRangeElements(
					GL_TRIANGLES,
					point_stream_target.get_start_streaming_vertex_count()/*start*/,
					point_stream_target.get_start_streaming_vertex_count() +
							point_stream_target.get_num_streamed_vertices() - 1/*end*/,
					point_stream_target.get_num_streamed_vertex_elements()/*count*/,
					GLVertexUtils::ElementTraits<streaming_vertex_element_type>::type,
					GLVertexUtils::buffer_offset(
							point_stream_target.get_start_streaming_vertex_element_count() *
									sizeof(PointRegionOfInterestVertex)/*indices_offset*/));
		}
		point_stream_target.start_streaming();

		point_stream_meshes.begin_primitive(5/*max_num_vertices*/, 12/*max_num_vertex_elements*/);
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
		GL &gl,
		const GLViewProjection &target_raster_view_projection,
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
	gl.UseProgram(d_render_lines_of_seed_geometries_with_small_roi_angle_program);

	// Set the view-projection matrix.
	GLfloat view_projection_float_matrix[16];
	target_raster_view_projection.get_view_projection_transform().get_float_matrix(view_projection_float_matrix);
	gl.UniformMatrix4fv(
			d_render_lines_of_seed_geometries_with_small_roi_angle_program->get_uniform_location(gl, "view_projection"),
			1, GL_FALSE/*transpose*/, view_projection_float_matrix);

	// Set the region-of-interest radius.
	gl.Uniform1f(
			d_render_lines_of_seed_geometries_with_small_roi_angle_program->get_uniform_location(gl, "sin_region_of_interest_angle"),
			sin_region_of_interest_angle);

	// Bind the line region-of-interest vertex array.
	gl.BindVertexArray(d_line_region_of_interest_vertex_array);

	// Need to bind vertex buffer before streaming into it.
	//
	// Note that we don't bind the vertex element buffer since binding the vertex array does that
	// (however it does not bind the GL_ARRAY_BUFFER, only records vertex attribute buffers).
	gl.BindBuffer(GL_ARRAY_BUFFER, d_streaming_vertex_buffer->get_buffer());

	// For streaming LineRegionOfInterestVertex vertices.
	line_region_of_interest_stream_primitives_type line_stream;
	line_region_of_interest_stream_primitives_type::MapStreamBufferScope line_stream_target(
			gl,
			line_stream,
			*d_streaming_vertex_element_buffer,
			*d_streaming_vertex_buffer,
			MINIMUM_BYTES_TO_STREAM_IN_VERTEX_ELEMENT_BUFFER,
			MINIMUM_BYTES_TO_STREAM_IN_VERTEX_BUFFER);

	// Start streaming line region-of-interest geometries.
	line_stream_target.start_streaming();

	line_region_of_interest_stream_primitives_type::Primitives line_stream_quads(line_stream);

	// Iterate over the polyline geometries.
	for (const SeedCoRegistration& seed_co_registration : geometry_lists.polylines_list)
	{
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
					gl,
					line_stream_target,
					line_stream_quads,
					line,
					vertex,
					tan_region_of_interest_angle);
		}
	}

	// Iterate over the polygon geometries.
	for (const SeedCoRegistration &seed_co_registration : geometry_lists.polygons_list)
	{
		// We're currently traversing the 'GPlatesMaths::PolygonOnSphere' list so the dynamic_cast should not fail.
		const GPlatesMaths::PolygonOnSphere &polygon_on_sphere =
				dynamic_cast<const GPlatesMaths::PolygonOnSphere &>(seed_co_registration.geometry);

		// Most of the vertex data is the same for all vertices in the seed geometry.
		LineRegionOfInterestVertex vertex;
		vertex.initialise_seed_geometry_constants(seed_co_registration);

		// Iterate over the lines (great circle arcs) of the exterior ring of the current polygon.
		GPlatesMaths::PolygonOnSphere::ring_const_iterator exterior_arcs_iter = polygon_on_sphere.exterior_ring_begin();
		GPlatesMaths::PolygonOnSphere::ring_const_iterator exterior_arcs_end = polygon_on_sphere.exterior_ring_end();
		for ( ; exterior_arcs_iter != exterior_arcs_end; ++exterior_arcs_iter)
		{
			const GPlatesMaths::GreatCircleArc &line = *exterior_arcs_iter;

			// If the line is degenerate (within numerical precision) then it's endpoints are
			// too close together to get a rotation axis. We can ignore these lines since the
			// region-of-interest quad that would've been generated would also be degenerate (zero-area).
			// And the two end-point *point* region-of-interest geometries cover the region-of-interest nicely.
			if (line.is_zero_length())
			{
				continue;
			}

			render_bounded_line_region_of_interest_geometry(
					gl,
					line_stream_target,
					line_stream_quads,
					line,
					vertex,
					tan_region_of_interest_angle);
		}

		// Iterate over the lines (great circle arcs) of the interior rings of the current polygon (if any).
		const unsigned int num_interior_rings = polygon_on_sphere.number_of_interior_rings();
		for (unsigned int interior_ring_index = 0; interior_ring_index < num_interior_rings; ++interior_ring_index)
		{
			GPlatesMaths::PolygonOnSphere::ring_const_iterator interior_arcs_iter =
					polygon_on_sphere.interior_ring_begin(interior_ring_index);
			GPlatesMaths::PolygonOnSphere::ring_const_iterator interior_arcs_end =
					polygon_on_sphere.interior_ring_end(interior_ring_index);
			for ( ; interior_arcs_iter != interior_arcs_end; ++interior_arcs_iter)
			{
				const GPlatesMaths::GreatCircleArc &line = *interior_arcs_iter;

				// If the line is degenerate (within numerical precision) then it's endpoints are
				// too close together to get a rotation axis. We can ignore these lines since the
				// region-of-interest quad that would've been generated would also be degenerate (zero-area).
				// And the two end-point *point* region-of-interest geometries cover the region-of-interest nicely.
				if (line.is_zero_length())
				{
					continue;
				}

				render_bounded_line_region_of_interest_geometry(
						gl,
						line_stream_target,
						line_stream_quads,
						line,
						vertex,
						tan_region_of_interest_angle);
			}
		}
	}

	// Stop streaming line region-of-interest geometries so we can render the last batch.
	if (line_stream_target.stop_streaming())
	{	// Only render if buffer contents are not undefined...

		// Render the last batch of streamed line region-of-interest geometries (if any).
		if (line_stream_target.get_num_streamed_vertex_elements() > 0)
		{
			gl.DrawRangeElements(
					GL_TRIANGLES,
					line_stream_target.get_start_streaming_vertex_count()/*start*/,
					line_stream_target.get_start_streaming_vertex_count() +
							line_stream_target.get_num_streamed_vertices() - 1/*end*/,
					line_stream_target.get_num_streamed_vertex_elements()/*count*/,
					GLVertexUtils::ElementTraits<streaming_vertex_element_type>::type,
					GLVertexUtils::buffer_offset(
							line_stream_target.get_start_streaming_vertex_element_count() *
									sizeof(LineRegionOfInterestVertex)/*indices_offset*/));
		}
	}
}


void
GPlatesOpenGL::GLRasterCoRegistration::render_bounded_line_region_of_interest_geometry(
		GL &gl,
		line_region_of_interest_stream_primitives_type::MapStreamBufferScope &line_stream_target,
		line_region_of_interest_stream_primitives_type::Primitives &line_stream_quads,
		const GPlatesMaths::GreatCircleArc &line,
		LineRegionOfInterestVertex &vertex,
		const double &tan_region_of_interest_angle)
{
	// There are four vertices for the current line (each line gets a quad) and
	// two triangles (three indices each).
	if (!line_stream_quads.begin_primitive(4/*max_num_vertices*/, 6/*max_num_vertex_elements*/))
	{
		// There's not enough vertices or indices so render what we have so far and obtain new stream buffers.
		if (line_stream_target.stop_streaming())
		{	// Only render if buffer contents are not undefined...
			gl.DrawRangeElements(
					GL_TRIANGLES,
					line_stream_target.get_start_streaming_vertex_count()/*start*/,
					line_stream_target.get_start_streaming_vertex_count() +
							line_stream_target.get_num_streamed_vertices() - 1/*end*/,
					line_stream_target.get_num_streamed_vertex_elements()/*count*/,
					GLVertexUtils::ElementTraits<streaming_vertex_element_type>::type,
					GLVertexUtils::buffer_offset(
							line_stream_target.get_start_streaming_vertex_element_count() *
									sizeof(LineRegionOfInterestVertex)/*indices_offset*/));
		}
		line_stream_target.start_streaming();

		line_stream_quads.begin_primitive(4/*max_num_vertices*/, 6/*max_num_vertex_elements*/);
	}

	// We should only be called if line (arc) has a rotation axis.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			!line.is_zero_length(),
			GPLATES_ASSERTION_SOURCE);

	const GPlatesMaths::UnitVector3D &first_point = line.start_point().position_vector();
	const GPlatesMaths::UnitVector3D &second_point = line.end_point().position_vector();
	const GPlatesMaths::UnitVector3D &arc_normal = line.rotation_axis();

	const GLfloat first_point_gl[3] =  { GLfloat(first_point.x().dval()),  GLfloat(first_point.y().dval()),  GLfloat(first_point.z().dval()) };
	const GLfloat second_point_gl[3] = { GLfloat(second_point.x().dval()), GLfloat(second_point.y().dval()), GLfloat(second_point.z().dval()) };

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
		GL &gl,
		const GLViewProjection &target_raster_view_projection,
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
	GLProgram::shared_ptr_type render_lines_of_seed_geometries_program;
	if (region_of_interest_radius < GPlatesMaths::PI / 4)
	{
		// Bind the shader program for rendering point regions-of-interest.
		render_lines_of_seed_geometries_program = d_render_lines_of_seed_geometries_with_small_roi_angle_program;
		gl.UseProgram(render_lines_of_seed_geometries_program);

		// Set the region-of-interest radius.
		gl.Uniform1f(
				render_lines_of_seed_geometries_program->get_uniform_location(gl, "sin_region_of_interest_angle"),
				sin_region_of_interest_angle);
	}
	else // Use a shader program that's accurate for angles very near 90 degrees...
	{
		// Bind the shader program for rendering point regions-of-interest.
		render_lines_of_seed_geometries_program = d_render_lines_of_seed_geometries_with_large_roi_angle_program;
		gl.UseProgram(render_lines_of_seed_geometries_program);

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
		gl.Uniform1f(
				render_lines_of_seed_geometries_program->get_uniform_location(gl, "tan_squared_region_of_interest_complementary_angle"),
				tan_region_of_interest_complementary_angle * tan_region_of_interest_complementary_angle);
	}

	// Set the view-projection matrix.
	GLfloat view_projection_float_matrix[16];
	target_raster_view_projection.get_view_projection_transform().get_float_matrix(view_projection_float_matrix);
	gl.UniformMatrix4fv(
			render_lines_of_seed_geometries_program->get_uniform_location(gl, "view_projection"),
			1, GL_FALSE/*transpose*/, view_projection_float_matrix);

	// Tangent frame weights used for each 'line' to determine position of the line's mesh vertices.
	// These weights place the mesh vertices on the unit sphere.
	const double arc_point_weight = cos_region_of_interest_angle;
	const double tangent_weight = sin_region_of_interest_angle;

	// Bind the line region-of-interest vertex array.
	gl.BindVertexArray(d_line_region_of_interest_vertex_array);

	// Need to bind vertex buffer before streaming into it.
	//
	// Note that we don't bind the vertex element buffer since binding the vertex array does that
	// (however it does not bind the GL_ARRAY_BUFFER, only records vertex attribute buffers).
	gl.BindBuffer(GL_ARRAY_BUFFER, d_streaming_vertex_buffer->get_buffer());

	// For streaming LineRegionOfInterestVertex vertices.
	line_region_of_interest_stream_primitives_type line_stream;
	line_region_of_interest_stream_primitives_type::MapStreamBufferScope line_stream_target(
			gl,
			line_stream,
			*d_streaming_vertex_element_buffer,
			*d_streaming_vertex_buffer,
			MINIMUM_BYTES_TO_STREAM_IN_VERTEX_ELEMENT_BUFFER,
			MINIMUM_BYTES_TO_STREAM_IN_VERTEX_BUFFER);

	// Start streaming line region-of-interest geometries.
	line_stream_target.start_streaming();

	line_region_of_interest_stream_primitives_type::Primitives line_stream_meshes(line_stream);

	// Iterate over the polyline geometries.
	for (const SeedCoRegistration &seed_co_registration : geometry_lists.polylines_list)
	{
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
					gl,
					line_stream_target,
					line_stream_meshes,
					line,
					vertex,
					arc_point_weight,
					tangent_weight);
		}
	}

	// Iterate over the polygon geometries.
	for (const SeedCoRegistration &seed_co_registration : geometry_lists.polygons_list)
	{
		// We're currently traversing the 'GPlatesMaths::PolygonOnSphere' list so the dynamic_cast should not fail.
		const GPlatesMaths::PolygonOnSphere &polygon_on_sphere =
				dynamic_cast<const GPlatesMaths::PolygonOnSphere &>(seed_co_registration.geometry);

		// Most of the vertex data is the same for all vertices in the seed geometry.
		LineRegionOfInterestVertex vertex;
		vertex.initialise_seed_geometry_constants(seed_co_registration);

		// Iterate over the lines (great circle arcs) of the exterior ring of the current polygon.
		GPlatesMaths::PolygonOnSphere::ring_const_iterator polygon_arcs_iter = polygon_on_sphere.exterior_ring_begin();
		GPlatesMaths::PolygonOnSphere::ring_const_iterator polygon_arcs_end = polygon_on_sphere.exterior_ring_end();
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
					gl,
					line_stream_target,
					line_stream_meshes,
					line,
					vertex,
					arc_point_weight,
					tangent_weight);
		}

		// Iterate over the lines (great circle arcs) of the interior rings of the current polygon (if any).
		const unsigned int num_interior_rings = polygon_on_sphere.number_of_interior_rings();
		for (unsigned int interior_ring_index = 0; interior_ring_index < num_interior_rings; ++interior_ring_index)
		{
			GPlatesMaths::PolygonOnSphere::ring_const_iterator interior_arcs_iter =
					polygon_on_sphere.interior_ring_begin(interior_ring_index);
			GPlatesMaths::PolygonOnSphere::ring_const_iterator interior_arcs_end =
					polygon_on_sphere.interior_ring_end(interior_ring_index);
			for ( ; interior_arcs_iter != interior_arcs_end; ++interior_arcs_iter)
			{
				const GPlatesMaths::GreatCircleArc &line = *interior_arcs_iter;

				// If the line is degenerate (within numerical precision) then it's endpoints are
				// too close together to get a rotation axis. We can ignore these lines since the
				// region-of-interest mesh that would've been generated would also be degenerate (zero-area).
				// And the two end-point *point* region-of-interest geometries cover the region-of-interest nicely.
				if (line.is_zero_length())
				{
					continue;
				}

				render_unbounded_line_region_of_interest_geometry(
						gl,
						line_stream_target,
						line_stream_meshes,
						line,
						vertex,
						arc_point_weight,
						tangent_weight);
			}
		}
	}

	// Stop streaming line region-of-interest geometries so we can render the last batch.
	if (line_stream_target.stop_streaming())
	{	// Only render if buffer contents are not undefined...

		// Render the last batch of streamed line region-of-interest geometries (if any).
		if (line_stream_target.get_num_streamed_vertex_elements() > 0)
		{
			gl.DrawRangeElements(
					GL_TRIANGLES,
					line_stream_target.get_start_streaming_vertex_count()/*start*/,
					line_stream_target.get_start_streaming_vertex_count() +
							line_stream_target.get_num_streamed_vertices() - 1/*end*/,
					line_stream_target.get_num_streamed_vertex_elements()/*count*/,
					GLVertexUtils::ElementTraits<streaming_vertex_element_type>::type,
					GLVertexUtils::buffer_offset(
							line_stream_target.get_start_streaming_vertex_element_count() *
									sizeof(LineRegionOfInterestVertex)/*indices_offset*/));
		}
	}
}


void
GPlatesOpenGL::GLRasterCoRegistration::render_unbounded_line_region_of_interest_geometry(
		GL &gl,
		line_region_of_interest_stream_primitives_type::MapStreamBufferScope &line_stream_target,
		line_region_of_interest_stream_primitives_type::Primitives &line_stream_meshes,
		const GPlatesMaths::GreatCircleArc &line,
		LineRegionOfInterestVertex &vertex,
		const double &arc_point_weight,
		const double &tangent_weight)
{
	// There are six vertices for the current line (each line gets a mesh) and 
	// four triangles (three indices each).
	if (!line_stream_meshes.begin_primitive(6/*max_num_vertices*/, 12/*max_num_vertex_elements*/))
	{
		// There's not enough vertices or indices so render what we have so far and obtain new stream buffers.
		if (line_stream_target.stop_streaming())
		{	// Only render if buffer contents are not undefined...
			gl.DrawRangeElements(
					GL_TRIANGLES,
					line_stream_target.get_start_streaming_vertex_count()/*start*/,
					line_stream_target.get_start_streaming_vertex_count() +
							line_stream_target.get_num_streamed_vertices() - 1/*end*/,
					line_stream_target.get_num_streamed_vertex_elements()/*count*/,
					GLVertexUtils::ElementTraits<streaming_vertex_element_type>::type,
					GLVertexUtils::buffer_offset(
							line_stream_target.get_start_streaming_vertex_element_count() *
									sizeof(LineRegionOfInterestVertex)/*indices_offset*/));
		}
		line_stream_target.start_streaming();

		line_stream_meshes.begin_primitive(6/*max_num_vertices*/, 12/*max_num_vertex_elements*/);
	}

	// We should only be called if line (arc) has a rotation axis.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			!line.is_zero_length(),
			GPLATES_ASSERTION_SOURCE);

	const GPlatesMaths::UnitVector3D &first_point = line.start_point().position_vector();
	const GPlatesMaths::UnitVector3D &second_point = line.end_point().position_vector();
	const GPlatesMaths::UnitVector3D &arc_normal = line.rotation_axis();

	const GLfloat first_point_gl[3] =  { GLfloat(first_point.x().dval()),  GLfloat(first_point.y().dval()),  GLfloat(first_point.z().dval()) };
        const GLfloat second_point_gl[3] = { GLfloat(second_point.x().dval()), GLfloat(second_point.y().dval()), GLfloat(second_point.z().dval()) };
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
		GL &gl,
		const GLViewProjection &target_raster_view_projection,
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
	gl.UseProgram(d_render_fill_of_seed_geometries_program);

	// Set the view-projection matrix.
	GLfloat view_projection_float_matrix[16];
	target_raster_view_projection.get_view_projection_transform().get_float_matrix(view_projection_float_matrix);
	gl.UniformMatrix4fv(
			d_render_fill_of_seed_geometries_program->get_uniform_location(gl, "view_projection"),
			1, GL_FALSE/*transpose*/, view_projection_float_matrix);

	// Bind the fill region-of-interest vertex array.
	gl.BindVertexArray(d_fill_region_of_interest_vertex_array);

	// Need to bind vertex buffer before streaming into it.
	//
	// Note that we don't bind the vertex element buffer since binding the vertex array does that
	// (however it does not bind the GL_ARRAY_BUFFER, only records vertex attribute buffers).
	gl.BindBuffer(GL_ARRAY_BUFFER, d_streaming_vertex_buffer->get_buffer());

	// For streaming FillRegionOfInterestVertex vertices.
	fill_region_of_interest_stream_primitives_type fill_stream;
	fill_region_of_interest_stream_primitives_type::MapStreamBufferScope fill_stream_target(
			gl,
			fill_stream,
			*d_streaming_vertex_element_buffer,
			*d_streaming_vertex_buffer,
			MINIMUM_BYTES_TO_STREAM_IN_VERTEX_ELEMENT_BUFFER,
			MINIMUM_BYTES_TO_STREAM_IN_VERTEX_BUFFER);

	// Start streaming fill region-of-interest geometries.
	fill_stream_target.start_streaming();

	// Input points for the vertices of the seed geometries.
	fill_region_of_interest_stream_primitives_type::Points fill_stream_points(fill_stream);

	fill_stream_points.begin_points();

	// Iterate over the point geometries.
	for (const SeedCoRegistration& seed_co_registration : geometry_lists.points_list)
	{
		// We're currently traversing the 'GPlatesMaths::PointOnSphere' list so the dynamic_cast should not fail.
		const GPlatesMaths::PointOnSphere &point_on_sphere =
				dynamic_cast<const GPlatesMaths::PointGeometryOnSphere &>(seed_co_registration.geometry).position();

		const GPlatesMaths::UnitVector3D &point_position = point_on_sphere.position_vector();

		FillRegionOfInterestVertex vertex;
		vertex.initialise_seed_geometry_constants(seed_co_registration);
		vertex.fill_position[0] = point_position.x().dval();
		vertex.fill_position[1] = point_position.y().dval();
		vertex.fill_position[2] = point_position.z().dval();

		if (!fill_stream_points.add_vertex(vertex))
		{
			if (fill_stream_target.stop_streaming())
			{	// Only render if buffer contents are not undefined...
				gl.DrawRangeElements(
						// These are actually rasterised points not quads (triangles)...
						GL_POINTS,
						fill_stream_target.get_start_streaming_vertex_count()/*start*/,
						fill_stream_target.get_start_streaming_vertex_count() +
								fill_stream_target.get_num_streamed_vertices() - 1/*end*/,
						fill_stream_target.get_num_streamed_vertex_elements()/*count*/,
						GLVertexUtils::ElementTraits<streaming_vertex_element_type>::type,
						GLVertexUtils::buffer_offset(
								fill_stream_target.get_start_streaming_vertex_element_count() *
										sizeof(FillRegionOfInterestVertex)/*indices_offset*/));
			}
			fill_stream_target.start_streaming();

			fill_stream_points.add_vertex(vertex);
		}
	}

	// Iterate over the multipoint geometries.
	for (const SeedCoRegistration &seed_co_registration : geometry_lists.multi_points_list)
	{
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
				if (fill_stream_target.stop_streaming())
				{	// Only render if buffer contents are not undefined...
					gl.DrawRangeElements(
							// These are actually rasterised points not quads (triangles)...
							GL_POINTS,
							fill_stream_target.get_start_streaming_vertex_count()/*start*/,
							fill_stream_target.get_start_streaming_vertex_count() +
									fill_stream_target.get_num_streamed_vertices() - 1/*end*/,
							fill_stream_target.get_num_streamed_vertex_elements()/*count*/,
							GLVertexUtils::ElementTraits<streaming_vertex_element_type>::type,
							GLVertexUtils::buffer_offset(
									fill_stream_target.get_start_streaming_vertex_element_count() *
											sizeof(FillRegionOfInterestVertex)/*indices_offset*/));
				}
				fill_stream_target.start_streaming();

				fill_stream_points.add_vertex(vertex);
			}
		}
	}

	// Iterate over the polyline geometries.
	for (const SeedCoRegistration &seed_co_registration : geometry_lists.polylines_list)
	{
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
				if (fill_stream_target.stop_streaming())
				{	// Only render if buffer contents are not undefined...
					gl.DrawRangeElements(
							// These are actually rasterised points not quads (triangles)...
							GL_POINTS,
							fill_stream_target.get_start_streaming_vertex_count()/*start*/,
							fill_stream_target.get_start_streaming_vertex_count() +
									fill_stream_target.get_num_streamed_vertices() - 1/*end*/,
							fill_stream_target.get_num_streamed_vertex_elements()/*count*/,
							GLVertexUtils::ElementTraits<streaming_vertex_element_type>::type,
							GLVertexUtils::buffer_offset(
									fill_stream_target.get_start_streaming_vertex_element_count() *
											sizeof(FillRegionOfInterestVertex)/*indices_offset*/));
				}
				fill_stream_target.start_streaming();

				fill_stream_points.add_vertex(vertex);
			}
		}
	}

	// Iterate over the polygon geometries.
	for (const SeedCoRegistration &seed_co_registration : geometry_lists.polygons_list)
	{
		// We're currently traversing the 'GPlatesMaths::PolygonOnSphere' list so the dynamic_cast should not fail.
		const GPlatesMaths::PolygonOnSphere &polygon_on_sphere =
				dynamic_cast<const GPlatesMaths::PolygonOnSphere &>(seed_co_registration.geometry);

		// Most of the vertex data is the same for all vertices in the seed geometry.
		FillRegionOfInterestVertex vertex;
		vertex.initialise_seed_geometry_constants(seed_co_registration);

		// Iterate over the points of the exterior ring of the current polygon.
		GPlatesMaths::PolygonOnSphere::ring_vertex_const_iterator exterior_points_iter = polygon_on_sphere.exterior_ring_vertex_begin();
		GPlatesMaths::PolygonOnSphere::ring_vertex_const_iterator exterior_points_end = polygon_on_sphere.exterior_ring_vertex_end();
		for ( ; exterior_points_iter != exterior_points_end; ++exterior_points_iter)
		{
			const GPlatesMaths::UnitVector3D &point_position = exterior_points_iter->position_vector();

			vertex.fill_position[0] = point_position.x().dval();
			vertex.fill_position[1] = point_position.y().dval();
			vertex.fill_position[2] = point_position.z().dval();

			if (!fill_stream_points.add_vertex(vertex))
			{
				if (fill_stream_target.stop_streaming())
				{	// Only render if buffer contents are not undefined...
					gl.DrawRangeElements(
							// These are actually rasterised points not quads (triangles)...
							GL_POINTS,
							fill_stream_target.get_start_streaming_vertex_count()/*start*/,
							fill_stream_target.get_start_streaming_vertex_count() +
									fill_stream_target.get_num_streamed_vertices() - 1/*end*/,
							fill_stream_target.get_num_streamed_vertex_elements()/*count*/,
							GLVertexUtils::ElementTraits<streaming_vertex_element_type>::type,
							GLVertexUtils::buffer_offset(
									fill_stream_target.get_start_streaming_vertex_element_count() *
											sizeof(FillRegionOfInterestVertex)/*indices_offset*/));
				}
				fill_stream_target.start_streaming();

				fill_stream_points.add_vertex(vertex);
			}
		}

		// Iterate over the points of the interior rings of the current polygon (if any).
		const unsigned int num_interior_rings = polygon_on_sphere.number_of_interior_rings();
		for (unsigned int interior_ring_index = 0; interior_ring_index < num_interior_rings; ++interior_ring_index)
		{
			GPlatesMaths::PolygonOnSphere::ring_vertex_const_iterator interior_points_iter =
					polygon_on_sphere.interior_ring_vertex_begin(interior_ring_index);
			GPlatesMaths::PolygonOnSphere::ring_vertex_const_iterator interior_points_end =
					polygon_on_sphere.interior_ring_vertex_end(interior_ring_index);
			for ( ; interior_points_iter != interior_points_end; ++interior_points_iter)
			{
				const GPlatesMaths::UnitVector3D &point_position = interior_points_iter->position_vector();

				vertex.fill_position[0] = point_position.x().dval();
				vertex.fill_position[1] = point_position.y().dval();
				vertex.fill_position[2] = point_position.z().dval();

				if (!fill_stream_points.add_vertex(vertex))
				{
					if (fill_stream_target.stop_streaming())
					{	// Only render if buffer contents are not undefined...
						gl.DrawRangeElements(
								// These are actually rasterised points not quads (triangles)...
								GL_POINTS,
								fill_stream_target.get_start_streaming_vertex_count()/*start*/,
								fill_stream_target.get_start_streaming_vertex_count() +
										fill_stream_target.get_num_streamed_vertices() - 1/*end*/,
								fill_stream_target.get_num_streamed_vertex_elements()/*count*/,
								GLVertexUtils::ElementTraits<streaming_vertex_element_type>::type,
								GLVertexUtils::buffer_offset(
										fill_stream_target.get_start_streaming_vertex_element_count() *
												sizeof(FillRegionOfInterestVertex)/*indices_offset*/));
					}
					fill_stream_target.start_streaming();

					fill_stream_points.add_vertex(vertex);
				}
			}
		}
	}

	fill_stream_points.end_points();

	// Stop streaming so we can render the last batch.
	if (fill_stream_target.stop_streaming())
	{	// Only render if buffer contents are not undefined...

		// Render the last batch (if any).
		if (fill_stream_target.get_num_streamed_vertex_elements() > 0)
		{
			gl.DrawRangeElements(
					// These are actually rasterised points not quads (triangles)...
					GL_POINTS,
					fill_stream_target.get_start_streaming_vertex_count()/*start*/,
					fill_stream_target.get_start_streaming_vertex_count() +
							fill_stream_target.get_num_streamed_vertices() - 1/*end*/,
					fill_stream_target.get_num_streamed_vertex_elements()/*count*/,
					GLVertexUtils::ElementTraits<streaming_vertex_element_type>::type,
					GLVertexUtils::buffer_offset(
							fill_stream_target.get_start_streaming_vertex_element_count() *
									sizeof(FillRegionOfInterestVertex)/*indices_offset*/));
		}
	}
}


void
GPlatesOpenGL::GLRasterCoRegistration::render_single_pixel_wide_line_region_of_interest_geometries(
		GL &gl,
		const GLViewProjection &target_raster_view_projection,
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
	gl.UseProgram(d_render_fill_of_seed_geometries_program);

	// Set the view-projection matrix.
	GLfloat view_projection_float_matrix[16];
	target_raster_view_projection.get_view_projection_transform().get_float_matrix(view_projection_float_matrix);
	gl.UniformMatrix4fv(
			d_render_fill_of_seed_geometries_program->get_uniform_location(gl, "view_projection"),
			1, GL_FALSE/*transpose*/, view_projection_float_matrix);

	// Bind the fill region-of-interest vertex array.
	gl.BindVertexArray(d_fill_region_of_interest_vertex_array);

	// Need to bind vertex buffer before streaming into it.
	//
	// Note that we don't bind the vertex element buffer since binding the vertex array does that
	// (however it does not bind the GL_ARRAY_BUFFER, only records vertex attribute buffers).
	gl.BindBuffer(GL_ARRAY_BUFFER, d_streaming_vertex_buffer->get_buffer());

	// For streaming FillRegionOfInterestVertex vertices.
	fill_region_of_interest_stream_primitives_type fill_stream;
	fill_region_of_interest_stream_primitives_type::MapStreamBufferScope fill_stream_target(
			gl,
			fill_stream,
			*d_streaming_vertex_element_buffer,
			*d_streaming_vertex_buffer,
			MINIMUM_BYTES_TO_STREAM_IN_VERTEX_ELEMENT_BUFFER,
			MINIMUM_BYTES_TO_STREAM_IN_VERTEX_BUFFER);

	// Start streaming fill region-of-interest geometries.
	fill_stream_target.start_streaming();

	// Input a line strip for each polyline.
	fill_region_of_interest_stream_primitives_type::LineStrips fill_stream_line_strips(fill_stream);

	// Iterate over the polyline geometries.
	for (const SeedCoRegistration &seed_co_registration : geometry_lists.polylines_list)
	{
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
				if (fill_stream_target.stop_streaming())
				{	// Only render if buffer contents are not undefined...
					gl.DrawRangeElements(
							// These are actually rasterised lines not quads (triangles)...
							GL_LINES,
							fill_stream_target.get_start_streaming_vertex_count()/*start*/,
							fill_stream_target.get_start_streaming_vertex_count() +
									fill_stream_target.get_num_streamed_vertices() - 1/*end*/,
							fill_stream_target.get_num_streamed_vertex_elements()/*count*/,
							GLVertexUtils::ElementTraits<streaming_vertex_element_type>::type,
							GLVertexUtils::buffer_offset(
									fill_stream_target.get_start_streaming_vertex_element_count() *
											sizeof(FillRegionOfInterestVertex)/*indices_offset*/));
				}
				fill_stream_target.start_streaming();

				fill_stream_line_strips.add_vertex(vertex);
			}
		}

		fill_stream_line_strips.end_line_strip();
	}

	// Input a line loop for each polygon.
	fill_region_of_interest_stream_primitives_type::LineLoops fill_stream_line_loops(fill_stream);

	// Iterate over the polygon geometries.
	for (const SeedCoRegistration &seed_co_registration : geometry_lists.polygons_list)
	{
		// We're currently traversing the 'GPlatesMaths::PolygonOnSphere' list so the dynamic_cast should not fail.
		const GPlatesMaths::PolygonOnSphere &polygon_on_sphere =
				dynamic_cast<const GPlatesMaths::PolygonOnSphere &>(seed_co_registration.geometry);

		// Most of the vertex data is the same for all vertices in the seed geometry.
		FillRegionOfInterestVertex vertex;
		vertex.initialise_seed_geometry_constants(seed_co_registration);

		fill_stream_line_loops.begin_line_loop();

		// Iterate over the points of the exterior ring of the current polygon.
		GPlatesMaths::PolygonOnSphere::ring_vertex_const_iterator exterior_points_iter = polygon_on_sphere.exterior_ring_vertex_begin();
		GPlatesMaths::PolygonOnSphere::ring_vertex_const_iterator exterior_points_end = polygon_on_sphere.exterior_ring_vertex_end();
		for ( ; exterior_points_iter != exterior_points_end; ++exterior_points_iter)
		{
			const GPlatesMaths::UnitVector3D &point_position = exterior_points_iter->position_vector();

			vertex.fill_position[0] = point_position.x().dval();
			vertex.fill_position[1] = point_position.y().dval();
			vertex.fill_position[2] = point_position.z().dval();
			if (!fill_stream_line_loops.add_vertex(vertex))
			{
				if (fill_stream_target.stop_streaming())
				{	// Only render if buffer contents are not undefined...
					gl.DrawRangeElements(
							// These are actually rasterised lines not quads (triangles)...
							GL_LINES,
							fill_stream_target.get_start_streaming_vertex_count()/*start*/,
							fill_stream_target.get_start_streaming_vertex_count() +
									fill_stream_target.get_num_streamed_vertices() - 1/*end*/,
							fill_stream_target.get_num_streamed_vertex_elements()/*count*/,
							GLVertexUtils::ElementTraits<streaming_vertex_element_type>::type,
							GLVertexUtils::buffer_offset(
									fill_stream_target.get_start_streaming_vertex_element_count() *
											sizeof(FillRegionOfInterestVertex)/*indices_offset*/));
				}
				fill_stream_target.start_streaming();

				fill_stream_line_loops.add_vertex(vertex);
			}
		}

		fill_stream_line_loops.end_line_loop();

		// Iterate over the points of the interior rings of the current polygon (if any).
		const unsigned int num_interior_rings = polygon_on_sphere.number_of_interior_rings();
		for (unsigned int interior_ring_index = 0; interior_ring_index < num_interior_rings; ++interior_ring_index)
		{
			fill_stream_line_loops.begin_line_loop();

			GPlatesMaths::PolygonOnSphere::ring_vertex_const_iterator interior_points_iter =
					polygon_on_sphere.interior_ring_vertex_begin(interior_ring_index);
			GPlatesMaths::PolygonOnSphere::ring_vertex_const_iterator interior_points_end =
					polygon_on_sphere.interior_ring_vertex_end(interior_ring_index);
			for ( ; interior_points_iter != interior_points_end; ++interior_points_iter)
			{
				const GPlatesMaths::UnitVector3D &point_position = interior_points_iter->position_vector();

				vertex.fill_position[0] = point_position.x().dval();
				vertex.fill_position[1] = point_position.y().dval();
				vertex.fill_position[2] = point_position.z().dval();
				if (!fill_stream_line_loops.add_vertex(vertex))
				{
					if (fill_stream_target.stop_streaming())
					{	// Only render if buffer contents are not undefined...
						gl.DrawRangeElements(
								// These are actually rasterised lines not quads (triangles)...
								GL_LINES,
								fill_stream_target.get_start_streaming_vertex_count()/*start*/,
								fill_stream_target.get_start_streaming_vertex_count() +
										fill_stream_target.get_num_streamed_vertices() - 1/*end*/,
								fill_stream_target.get_num_streamed_vertex_elements()/*count*/,
								GLVertexUtils::ElementTraits<streaming_vertex_element_type>::type,
								GLVertexUtils::buffer_offset(
										fill_stream_target.get_start_streaming_vertex_element_count() *
												sizeof(FillRegionOfInterestVertex)/*indices_offset*/));
					}
					fill_stream_target.start_streaming();

					fill_stream_line_loops.add_vertex(vertex);
				}
			}

			fill_stream_line_loops.end_line_loop();
		}
	}

	// Stop streaming so we can render the last batch.
	if (fill_stream_target.stop_streaming())
	{	// Only render if buffer contents are not undefined...

		// Render the last batch (if any).
		if (fill_stream_target.get_num_streamed_vertex_elements() > 0)
		{
			gl.DrawRangeElements(
					// These are actually rasterised lines not quads (triangles)...
					GL_LINES,
					fill_stream_target.get_start_streaming_vertex_count()/*start*/,
					fill_stream_target.get_start_streaming_vertex_count() +
							fill_stream_target.get_num_streamed_vertices() - 1/*end*/,
					fill_stream_target.get_num_streamed_vertex_elements()/*count*/,
					GLVertexUtils::ElementTraits<streaming_vertex_element_type>::type,
					GLVertexUtils::buffer_offset(
							fill_stream_target.get_start_streaming_vertex_element_count() *
									sizeof(FillRegionOfInterestVertex)/*indices_offset*/));
		}
	}
}


void
GPlatesOpenGL::GLRasterCoRegistration::render_fill_region_of_interest_geometries(
		GL &gl,
		const GLViewProjection &target_raster_view_projection,
		const SeedCoRegistrationGeometryLists &geometry_lists)
{
	//PROFILE_FUNC();

	// Nothing to do if there's no polygons.
	if (geometry_lists.polygons_list.empty())
	{
		return;
	}

	// Make sure we leave the OpenGL global state the way it was.
	// We want to restore the GL_BLEND state.
	GL::StateScope save_restore_state(gl);

	// Alpha-blend state set to invert destination alpha (and colour) every time a pixel
	// is rendered (this means we get 1 where a pixel is covered by an odd number of triangles
	// and 0 by an even number of triangles).
	// The end result is zero outside the polygon and one inside.
	gl.Enable(GL_BLEND);
	gl.BlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ZERO);

	// Bind the shader program for rendering fill regions-of-interest.
	gl.UseProgram(d_render_fill_of_seed_geometries_program);

	// Set the view-projection matrix.
	GLfloat view_projection_float_matrix[16];
	target_raster_view_projection.get_view_projection_transform().get_float_matrix(view_projection_float_matrix);
	gl.UniformMatrix4fv(
			d_render_fill_of_seed_geometries_program->get_uniform_location(gl, "view_projection"),
			1, GL_FALSE/*transpose*/, view_projection_float_matrix);

	// Bind the fill region-of-interest vertex array.
	gl.BindVertexArray(d_fill_region_of_interest_vertex_array);

	// Need to bind vertex buffer before streaming into it.
	//
	// Note that we don't bind the vertex element buffer since binding the vertex array does that
	// (however it does not bind the GL_ARRAY_BUFFER, only records vertex attribute buffers).
	gl.BindBuffer(GL_ARRAY_BUFFER, d_streaming_vertex_buffer->get_buffer());

	// For streaming LineRegionOfInterestVertex vertices.
	fill_region_of_interest_stream_primitives_type fill_stream;
	fill_region_of_interest_stream_primitives_type::MapStreamBufferScope fill_stream_target(
			gl,
			fill_stream,
			*d_streaming_vertex_element_buffer,
			*d_streaming_vertex_buffer,
			MINIMUM_BYTES_TO_STREAM_IN_VERTEX_ELEMENT_BUFFER,
			MINIMUM_BYTES_TO_STREAM_IN_VERTEX_BUFFER);

	// Start streaming fill region-of-interest geometries.
	fill_stream_target.start_streaming();

	// Render each polygon as a triangle fan with the fan apex being the polygon centroid.
	fill_region_of_interest_stream_primitives_type::TriangleFans fill_stream_triangle_fans(fill_stream);

	// Iterate over the polygon geometries - the only geometry type that supports fill (has an interior).
	for (const SeedCoRegistration &seed_co_registration : geometry_lists.polygons_list)
	{
		// We're currently traversing the 'GPlatesMaths::PolygonOnSphere' list so the dynamic_cast should not fail.
		const GPlatesMaths::PolygonOnSphere &polygon_on_sphere =
				dynamic_cast<const GPlatesMaths::PolygonOnSphere &>(seed_co_registration.geometry);

		const GPlatesMaths::UnitVector3D &centroid = polygon_on_sphere.get_boundary_centroid();

		// Most of the vertex data is the same for all vertices for polygon triangle fan.
		FillRegionOfInterestVertex vertex;
		vertex.initialise_seed_geometry_constants(seed_co_registration);

		fill_stream_triangle_fans.begin_triangle_fan();

		// The first vertex is the polygon centroid.
		vertex.fill_position[0] = centroid.x().dval();
		vertex.fill_position[1] = centroid.y().dval();
		vertex.fill_position[2] = centroid.z().dval();
		if (!fill_stream_triangle_fans.add_vertex(vertex))
		{
			if (fill_stream_target.stop_streaming())
			{	// Only render if buffer contents are not undefined...
				gl.DrawRangeElements(
						GL_TRIANGLES,
						fill_stream_target.get_start_streaming_vertex_count()/*start*/,
						fill_stream_target.get_start_streaming_vertex_count() +
								fill_stream_target.get_num_streamed_vertices() - 1/*end*/,
						fill_stream_target.get_num_streamed_vertex_elements()/*count*/,
						GLVertexUtils::ElementTraits<streaming_vertex_element_type>::type,
						GLVertexUtils::buffer_offset(
								fill_stream_target.get_start_streaming_vertex_element_count() *
										sizeof(FillRegionOfInterestVertex)/*indices_offset*/));
			}
			fill_stream_target.start_streaming();

			fill_stream_triangle_fans.add_vertex(vertex);
		}

		// Iterate over the points of the exterior ring of the current polygon.
		const GPlatesMaths::PolygonOnSphere::ring_vertex_const_iterator exterior_points_begin =
				polygon_on_sphere.exterior_ring_vertex_begin();
		const GPlatesMaths::PolygonOnSphere::ring_vertex_const_iterator exterior_points_end =
				polygon_on_sphere.exterior_ring_vertex_end();
		for (GPlatesMaths::PolygonOnSphere::ring_vertex_const_iterator exterior_points_iter = exterior_points_begin;
			exterior_points_iter != exterior_points_end;
			++exterior_points_iter)
		{
			const GPlatesMaths::UnitVector3D &point_position = exterior_points_iter->position_vector();

			vertex.fill_position[0] = point_position.x().dval();
			vertex.fill_position[1] = point_position.y().dval();
			vertex.fill_position[2] = point_position.z().dval();
			if (!fill_stream_triangle_fans.add_vertex(vertex))
			{
				if (fill_stream_target.stop_streaming())
				{	// Only render if buffer contents are not undefined...
					gl.DrawRangeElements(
							GL_TRIANGLES,
							fill_stream_target.get_start_streaming_vertex_count()/*start*/,
							fill_stream_target.get_start_streaming_vertex_count() +
									fill_stream_target.get_num_streamed_vertices() - 1/*end*/,
							fill_stream_target.get_num_streamed_vertex_elements()/*count*/,
							GLVertexUtils::ElementTraits<streaming_vertex_element_type>::type,
							GLVertexUtils::buffer_offset(
									fill_stream_target.get_start_streaming_vertex_element_count() *
											sizeof(FillRegionOfInterestVertex)/*indices_offset*/));
				}
				fill_stream_target.start_streaming();

				fill_stream_triangle_fans.add_vertex(vertex);
			}
		}

		// Wraparound back to the first polygon vertex in the exterior ring to close off the ring.
		const GPlatesMaths::UnitVector3D &first_exterior_point_position = exterior_points_begin->position_vector();
		vertex.fill_position[0] = first_exterior_point_position.x().dval();
		vertex.fill_position[1] = first_exterior_point_position.y().dval();
		vertex.fill_position[2] = first_exterior_point_position.z().dval();
		if (!fill_stream_triangle_fans.add_vertex(vertex))
		{
			if (fill_stream_target.stop_streaming())
			{	// Only render if buffer contents are not undefined...
				gl.DrawRangeElements(
						GL_TRIANGLES,
						fill_stream_target.get_start_streaming_vertex_count()/*start*/,
						fill_stream_target.get_start_streaming_vertex_count() +
								fill_stream_target.get_num_streamed_vertices() - 1/*end*/,
						fill_stream_target.get_num_streamed_vertex_elements()/*count*/,
						GLVertexUtils::ElementTraits<streaming_vertex_element_type>::type,
						GLVertexUtils::buffer_offset(
								fill_stream_target.get_start_streaming_vertex_element_count() *
										sizeof(FillRegionOfInterestVertex)/*indices_offset*/));
			}
			fill_stream_target.start_streaming();

			fill_stream_triangle_fans.add_vertex(vertex);
		}

		fill_stream_triangle_fans.end_triangle_fan();

		// The interior rings of the current polygon (if any).
		const unsigned int num_interior_rings = polygon_on_sphere.number_of_interior_rings();
		for (unsigned int interior_ring_index = 0; interior_ring_index < num_interior_rings; ++interior_ring_index)
		{
			fill_stream_triangle_fans.begin_triangle_fan();

			// The first vertex is the polygon centroid.
			vertex.fill_position[0] = centroid.x().dval();
			vertex.fill_position[1] = centroid.y().dval();
			vertex.fill_position[2] = centroid.z().dval();
			if (!fill_stream_triangle_fans.add_vertex(vertex))
			{
				if (fill_stream_target.stop_streaming())
				{	// Only render if buffer contents are not undefined...
					gl.DrawRangeElements(
							GL_TRIANGLES,
							fill_stream_target.get_start_streaming_vertex_count()/*start*/,
							fill_stream_target.get_start_streaming_vertex_count() +
									fill_stream_target.get_num_streamed_vertices() - 1/*end*/,
							fill_stream_target.get_num_streamed_vertex_elements()/*count*/,
							GLVertexUtils::ElementTraits<streaming_vertex_element_type>::type,
							GLVertexUtils::buffer_offset(
									fill_stream_target.get_start_streaming_vertex_element_count() *
											sizeof(FillRegionOfInterestVertex)/*indices_offset*/));
				}
				fill_stream_target.start_streaming();

				fill_stream_triangle_fans.add_vertex(vertex);
			}

			// Iterate over the points of the current interior ring of the current polygon.
			const GPlatesMaths::PolygonOnSphere::ring_vertex_const_iterator interior_points_begin =
					polygon_on_sphere.interior_ring_vertex_begin(interior_ring_index);
			const GPlatesMaths::PolygonOnSphere::ring_vertex_const_iterator interior_points_end =
					polygon_on_sphere.interior_ring_vertex_end(interior_ring_index);
			for (GPlatesMaths::PolygonOnSphere::ring_vertex_const_iterator interior_points_iter = interior_points_begin;
				interior_points_iter != interior_points_end;
				++interior_points_iter)
			{
				const GPlatesMaths::UnitVector3D &point_position = interior_points_iter->position_vector();

				vertex.fill_position[0] = point_position.x().dval();
				vertex.fill_position[1] = point_position.y().dval();
				vertex.fill_position[2] = point_position.z().dval();
				if (!fill_stream_triangle_fans.add_vertex(vertex))
				{
					if (fill_stream_target.stop_streaming())
					{	// Only render if buffer contents are not undefined...
						gl.DrawRangeElements(
								GL_TRIANGLES,
								fill_stream_target.get_start_streaming_vertex_count()/*start*/,
								fill_stream_target.get_start_streaming_vertex_count() +
										fill_stream_target.get_num_streamed_vertices() - 1/*end*/,
								fill_stream_target.get_num_streamed_vertex_elements()/*count*/,
								GLVertexUtils::ElementTraits<streaming_vertex_element_type>::type,
								GLVertexUtils::buffer_offset(
										fill_stream_target.get_start_streaming_vertex_element_count() *
												sizeof(FillRegionOfInterestVertex)/*indices_offset*/));
					}
					fill_stream_target.start_streaming();

					fill_stream_triangle_fans.add_vertex(vertex);
				}
			}

			// Wraparound back to the first polygon vertex in the interior ring to close off the ring.
			const GPlatesMaths::UnitVector3D &first_interior_point_position = interior_points_begin->position_vector();
			vertex.fill_position[0] = first_interior_point_position.x().dval();
			vertex.fill_position[1] = first_interior_point_position.y().dval();
			vertex.fill_position[2] = first_interior_point_position.z().dval();
			if (!fill_stream_triangle_fans.add_vertex(vertex))
			{
				if (fill_stream_target.stop_streaming())
				{	// Only render if buffer contents are not undefined...
					gl.DrawRangeElements(
							GL_TRIANGLES,
							fill_stream_target.get_start_streaming_vertex_count()/*start*/,
							fill_stream_target.get_start_streaming_vertex_count() +
									fill_stream_target.get_num_streamed_vertices() - 1/*end*/,
							fill_stream_target.get_num_streamed_vertex_elements()/*count*/,
							GLVertexUtils::ElementTraits<streaming_vertex_element_type>::type,
							GLVertexUtils::buffer_offset(
									fill_stream_target.get_start_streaming_vertex_element_count() *
											sizeof(FillRegionOfInterestVertex)/*indices_offset*/));
				}
				fill_stream_target.start_streaming();

				fill_stream_triangle_fans.add_vertex(vertex);
			}

			fill_stream_triangle_fans.end_triangle_fan();
		}
	}

	// Stop streaming so we can render the last batch.
	if (fill_stream_target.stop_streaming())
	{	// Only render if buffer contents are not undefined...

		// Render the last batch streamed (if any).
		if (fill_stream_target.get_num_streamed_vertex_elements() > 0)
		{
			gl.DrawRangeElements(
					GL_TRIANGLES,
					fill_stream_target.get_start_streaming_vertex_count()/*start*/,
					fill_stream_target.get_start_streaming_vertex_count() +
							fill_stream_target.get_num_streamed_vertices() - 1/*end*/,
					fill_stream_target.get_num_streamed_vertex_elements()/*count*/,
					GLVertexUtils::ElementTraits<streaming_vertex_element_type>::type,
					GLVertexUtils::buffer_offset(
							fill_stream_target.get_start_streaming_vertex_element_count() *
									sizeof(FillRegionOfInterestVertex)/*indices_offset*/));
		}
	}
}


void
GPlatesOpenGL::GLRasterCoRegistration::mask_target_raster_with_regions_of_interest(
		GL &gl,
		const Operation &operation,
		const GPlatesMaths::UnitVector3D &cube_face_centre,
		const GLViewProjection &target_raster_view_projection,
		const GLTexture::shared_ptr_type &target_raster_texture,
		const GLTexture::shared_ptr_type &region_of_interest_mask_texture,
		const SeedCoRegistrationGeometryLists &geometry_lists)
{
	//PROFILE_FUNC();

	// Determine which filter operation to use.
	GLProgram::shared_ptr_type mask_region_of_interest_program;
	switch (operation.d_operation)
	{
		// Both mean and standard deviation are filtered using moments.
	case OPERATION_MEAN:
	case OPERATION_STANDARD_DEVIATION:
		mask_region_of_interest_program = d_mask_region_of_interest_moments_program;
		// Bind the shader program for masking target raster with regions-of-interest.
		gl.UseProgram(mask_region_of_interest_program);
		// Set the cube face centre - needed to adjust for cube map area-weighting distortion.
		gl.Uniform3f(
				mask_region_of_interest_program->get_uniform_location(gl, "cube_face_centre"),
				cube_face_centre.x().dval(), cube_face_centre.y().dval(), cube_face_centre.z().dval());

		{
			boost::optional<GLMatrix> inverse_view_projection_matrix = target_raster_view_projection.get_inverse_view_projection_transform();
			if (!inverse_view_projection_matrix)
			{
				// Log a warning (only once) and just use identity matrices (which will render incorrectly, but this should never happen).
				static bool warned = false;
				if (!warned)
				{
					qWarning() << "View or projection transform not invertible. Raster co-registration masking will be incorrect.";
					warned = true;
				}

				inverse_view_projection_matrix = GLMatrix::IDENTITY;
			}

			GLfloat inverse_view_projection_float_matrix[16];
			inverse_view_projection_matrix->get_float_matrix(inverse_view_projection_float_matrix);
			gl.UniformMatrix4fv(
					mask_region_of_interest_program->get_uniform_location(gl, "view_projection_inverse"),
					1, GL_FALSE/*transpose*/, inverse_view_projection_float_matrix);
		}
		break;

		// Both min and max are filtered using minmax.
	case OPERATION_MINIMUM:
	case OPERATION_MAXIMUM:
		mask_region_of_interest_program = d_mask_region_of_interest_minmax_program;
		// Bind the shader program for masking target raster with regions-of-interest.
		gl.UseProgram(mask_region_of_interest_program);
		break;

	default:
		// Shouldn't get here.
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
		break;
	}

	// Set the target raster texture sampler to texture unit 0.
	gl.Uniform1i(
			mask_region_of_interest_program->get_uniform_location(gl, "target_raster_texture_sampler"),
			0/*texture unit*/);
	// Bind the target raster texture to texture unit 0.
	gl.BindTextureUnit(0, target_raster_texture);

	// Set the region-of-interest mask texture sampler to texture unit 1.
	gl.Uniform1i(
			mask_region_of_interest_program->get_uniform_location(gl, "region_of_interest_mask_texture_sampler"),
			1/*texture unit*/);
	// Bind the region-of-interest mask texture to texture unit 1.
	gl.BindTextureUnit(1, region_of_interest_mask_texture);

	// Bind the mask target raster with regions-of-interest vertex array.
	gl.BindVertexArray(d_mask_region_of_interest_vertex_array);

	// Need to bind vertex buffer before streaming into it.
	//
	// Note that we don't bind the vertex element buffer since binding the vertex array does that
	// (however it does not bind the GL_ARRAY_BUFFER, only records vertex attribute buffers).
	gl.BindBuffer(GL_ARRAY_BUFFER, d_streaming_vertex_buffer->get_buffer());

	// For streaming MaskRegionOfInterestVertex vertices.
	mask_region_of_interest_stream_primitives_type mask_stream;
	mask_region_of_interest_stream_primitives_type::MapStreamBufferScope mask_stream_target(
			gl,
			mask_stream,
			*d_streaming_vertex_element_buffer,
			*d_streaming_vertex_buffer,
			MINIMUM_BYTES_TO_STREAM_IN_VERTEX_ELEMENT_BUFFER,
			MINIMUM_BYTES_TO_STREAM_IN_VERTEX_BUFFER);

	// Start streaming point region-of-interest geometries.
	mask_stream_target.start_streaming();

	mask_region_of_interest_stream_primitives_type::Primitives mask_stream_quads(mask_stream);

	// Iterate over the seed points.
	for (const SeedCoRegistration &seed_co_registration : geometry_lists.points_list)
	{
		// Copy the seed geometry's frustum region of the target raster.
		mask_target_raster_with_region_of_interest(
				gl,
				mask_stream_target,
				mask_stream_quads,
				seed_co_registration);
	}

	// Iterate over the seed multipoints.
	for (const SeedCoRegistration &seed_co_registration : geometry_lists.multi_points_list)
	{
		// Copy the seed geometry's frustum region of the target raster.
		mask_target_raster_with_region_of_interest(
				gl,
				mask_stream_target,
				mask_stream_quads,
				seed_co_registration);
	}

	// Iterate over the seed polylines.
	for (const SeedCoRegistration &seed_co_registration : geometry_lists.polylines_list)
	{
		// Copy the seed geometry's frustum region of the target raster.
		mask_target_raster_with_region_of_interest(
				gl,
				mask_stream_target,
				mask_stream_quads,
				seed_co_registration);
	}

	// Iterate over the seed polygons.
	for (const SeedCoRegistration &seed_co_registration : geometry_lists.polygons_list)
	{
		// Copy the seed geometry's frustum region of the target raster.
		mask_target_raster_with_region_of_interest(
				gl,
				mask_stream_target,
				mask_stream_quads,
				seed_co_registration);
	}

	// Stop streaming so we can render the last batch.
	if (mask_stream_target.stop_streaming())
	{	// Only render if buffer contents are not undefined...

		// Render the last batch of streamed primitives (if any).
		if (mask_stream_target.get_num_streamed_vertex_elements() > 0)
		{
			gl.DrawRangeElements(
					GL_TRIANGLES,
					mask_stream_target.get_start_streaming_vertex_count()/*start*/,
					mask_stream_target.get_start_streaming_vertex_count() +
							mask_stream_target.get_num_streamed_vertices() - 1/*end*/,
					mask_stream_target.get_num_streamed_vertex_elements()/*count*/,
					GLVertexUtils::ElementTraits<streaming_vertex_element_type>::type,
					GLVertexUtils::buffer_offset(
							mask_stream_target.get_start_streaming_vertex_element_count() *
									sizeof(MaskRegionOfInterestVertex)/*indices_offset*/));
		}
	}
}


void
GPlatesOpenGL::GLRasterCoRegistration::mask_target_raster_with_region_of_interest(
		GL &gl,
		mask_region_of_interest_stream_primitives_type::MapStreamBufferScope &mask_stream_target,
		mask_region_of_interest_stream_primitives_type::Primitives &mask_stream_quads,
		const SeedCoRegistration &seed_co_registration)
{
	// There are four vertices for the current quad and two triangles (three indices each).
	if (!mask_stream_quads.begin_primitive(4/*max_num_vertices*/, 6/*max_num_vertex_elements*/))
	{
		// There's not enough vertices or indices so render what we have so far and obtain new stream buffers.
		if (mask_stream_target.stop_streaming())
		{	// Only render if buffer contents are not undefined...
			gl.DrawRangeElements(
					GL_TRIANGLES,
					mask_stream_target.get_start_streaming_vertex_count()/*start*/,
					mask_stream_target.get_start_streaming_vertex_count() +
							mask_stream_target.get_num_streamed_vertices() - 1/*end*/,
					mask_stream_target.get_num_streamed_vertex_elements()/*count*/,
					GLVertexUtils::ElementTraits<streaming_vertex_element_type>::type,
					GLVertexUtils::buffer_offset(
							mask_stream_target.get_start_streaming_vertex_element_count() *
									sizeof(MaskRegionOfInterestVertex)/*indices_offset*/));
		}
		mask_stream_target.start_streaming();

		mask_stream_quads.begin_primitive(4/*max_num_vertices*/, 6/*max_num_vertex_elements*/);
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


void
GPlatesOpenGL::GLRasterCoRegistration::render_reduction_of_reduce_stage(
		GL &gl,
		const Operation &operation,
		const ReduceQuadTreeInternalNode &dst_reduce_quad_tree_node,
		unsigned int src_child_x_offset,
		unsigned int src_child_y_offset,
		bool clear_dst_reduce_stage_texture,
		const GLTexture::shared_ptr_type &dst_reduce_stage_texture,
		const GLTexture::shared_ptr_type &src_reduce_stage_texture)
{
	//PROFILE_FUNC();

	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(
			gl,
			// We're rendering to a render target so reset to the default OpenGL state...
			true/*reset_to_default_state*/);

	// Bind our framebuffer object.
	gl.BindFramebuffer(GL_FRAMEBUFFER, d_framebuffer);

	// Begin rendering to the destination reduce stage texture.
	gl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, dst_reduce_stage_texture, 0/*level*/);

	// Check our framebuffer object for completeness (now that a texture is attached to it).
	// We only need to do this once because, while the texture changes, the framebuffer configuration
	// does not (ie, same texture internal format, dimensions, etc).
	if (!d_have_checked_framebuffer_completeness)
	{
		// Throw OpenGLException if not complete.
		// This should succeed since we should only be using texture formats that are required by OpenGL 3.3 core.
		const GLenum completeness = gl.CheckFramebufferStatus(GL_FRAMEBUFFER);
		GPlatesGlobal::Assert<OpenGLException>(
				completeness == GL_FRAMEBUFFER_COMPLETE,
				GPLATES_ASSERTION_SOURCE,
				"Framebuffer not complete for coregistering rasters.");

		d_have_checked_framebuffer_completeness = true;
	}

	// Render to the entire texture.
	gl.Viewport(0, 0, TEXTURE_DIMENSION, TEXTURE_DIMENSION);

	// If the destination reduce stage texture does not contain partial results then it'll need to be cleared.
	// This happens when starting afresh with a newly acquired destination reduce stage texture.
	if (clear_dst_reduce_stage_texture)
	{
		// Clear colour to all zeros - this means when texels with zero coverage get discarded the framebuffer
		// will have coverage values of zero (causing them to not contribute to the co-registration result).
		gl.ClearColor();
		gl.Clear(GL_COLOR_BUFFER_BIT); // Clear only the colour buffer.
	}

	// Determine which reduction operation to use.
	GLProgram::shared_ptr_type reduction_program;
	switch (operation.d_operation)
	{
		// Both mean and standard deviation can be reduced using summation.
	case OPERATION_MEAN:
	case OPERATION_STANDARD_DEVIATION:
		reduction_program = d_reduction_sum_program;
		break;

	case OPERATION_MINIMUM:
		reduction_program = d_reduction_min_program;
		break;

	case OPERATION_MAXIMUM:
		reduction_program = d_reduction_max_program;
		break;

	default:
		// Shouldn't get here.
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
		break;
	}

	// Bind the shader program for reducing the regions-of-interest filter results.
	gl.UseProgram(reduction_program);

	// Set the reduce source texture sampler to texture unit 0.
	gl.Uniform1i(
			reduction_program->get_uniform_location(gl, "reduce_source_texture_sampler"),
			0/*texture unit*/);
	// Bind the source reduce stage texture to texture unit 0.
	gl.BindTextureUnit(0, src_reduce_stage_texture);

	// Set the half-texel offset of the reduce source texture (all reduce textures have same dimension).
	const double half_texel_offset = 0.5 / TEXTURE_DIMENSION;
	gl.Uniform2f(
			reduction_program->get_uniform_location(gl, "reduce_source_texture_half_texel_offset"),
			half_texel_offset, -half_texel_offset);
	// Determine which quadrant of the destination reduce texture to render to.
	// Map the range [-1,1] to one of [-1,0] or [0,1] for both x and y directions.
	gl.Uniform3f(
			reduction_program->get_uniform_location(gl, "target_quadrant_translate_scale"),
			0.5 * (src_child_x_offset ? 1 : -1), // translate_x
			0.5 * (src_child_y_offset ? 1 : -1),  // translate_y
			0.5); // scale

	// Bind the reduction vertex array.
	gl.BindVertexArray(d_reduction_vertex_array);

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
	gl.DrawRangeElements(
			GL_TRIANGLES,
			0/*start*/,
			4 * num_reduce_quads_spanned - 1/*end*/, // Each quad has four vertices.
			6 * num_reduce_quads_spanned/*count*/,   // Each quad has two triangles of three indices each.
			GLVertexUtils::ElementTraits<reduction_vertex_element_type>::type,
			nullptr/*indices_offset*/);

	//debug_floating_point_render_target(
	//		gl, "reduction_raster", false/*coverage_is_in_green_channel*/);
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
		GL &gl,
		const CoRegistrationParameters &co_registration_parameters,
		const GLTexture::shared_ptr_type &target_raster_texture,
		const GLTransform &view_transform,
		const GLTransform &projection_transform)
{
	//PROFILE_FUNC();

	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(
			gl,
			// We're rendering to a render target so reset to the default OpenGL state...
			true/*reset_to_default_state*/);

	// Bind our framebuffer object.
	gl.BindFramebuffer(GL_FRAMEBUFFER, d_framebuffer);

	// Begin rendering to the 2D texture.
	gl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target_raster_texture, 0/*level*/);

	// Check our framebuffer object for completeness (now that a texture is attached to it).
	// We only need to do this once because, while the texture changes, the framebuffer configuration
	// does not (ie, same texture internal format, dimensions, etc).
	if (!d_have_checked_framebuffer_completeness)
	{
		// Throw OpenGLException if not complete.
		// This should succeed since we should only be using texture formats that are required by OpenGL 3.3 core.
		const GLenum completeness = gl.CheckFramebufferStatus(GL_FRAMEBUFFER);
		GPlatesGlobal::Assert<OpenGLException>(
				completeness == GL_FRAMEBUFFER_COMPLETE,
				GPLATES_ASSERTION_SOURCE,
				"Framebuffer not complete for coregistering rasters.");

		d_have_checked_framebuffer_completeness = true;
	}

	// Render to the entire texture.
	gl.Viewport(0, 0, TEXTURE_DIMENSION, TEXTURE_DIMENSION);

	// Clear the render target (only has colour, no depth/stencil).
	gl.ClearColor();
	gl.Clear(GL_COLOR_BUFFER_BIT);

	// The view projection.
	const GLViewProjection view_projection(
			GLViewport(0, 0, TEXTURE_DIMENSION, TEXTURE_DIMENSION),
			view_transform.get_matrix(),
			projection_transform.get_matrix());

	// Render the target raster into the view frustum.
	GLMultiResolutionRasterInterface::cache_handle_type cache_handle;
	// Render target raster and return true if there was any rendering into the view frustum.
	const bool raster_rendered = co_registration_parameters.target_raster->render(
			gl,
			view_projection.get_view_projection_transform(),
			co_registration_parameters.raster_level_of_detail,
			cache_handle);

	//debug_floating_point_render_target(
	//		gl, "raster", true/*coverage_is_in_green_channel*/);

	return raster_rendered;
}


GPlatesOpenGL::GLTexture::shared_ptr_type
GPlatesOpenGL::GLRasterCoRegistration::acquire_rgba_float_texture(
		GL &gl)
{
	// Attempt to acquire a recycled texture object.
	boost::optional<GLTexture::shared_ptr_type> texture_opt = d_rgba_float_texture_cache->allocate_object();
	if (texture_opt)
	{
		return texture_opt.get();
	}

	// Create a new object and add it to the cache.
	const GLTexture::shared_ptr_type texture = d_rgba_float_texture_cache->allocate_object(
			GLTexture::create_unique(gl, GL_TEXTURE_2D));

	//
	// No mipmaps needed so we specify no mipmap filtering.
	//

	// Bilinear filtering for GL_TEXTURE_MIN_FILTER and GL_TEXTURE_MAG_FILTER.
	gl.TextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	gl.TextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	// Clamp texture coordinates to centre of edge texels -
	// it's easier for hardware to implement - and doesn't affect our calculations.
	gl.TextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	gl.TextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Allocate texture but don't load any data into it.
	// Leave it uninitialised because we will be rendering into it to initialise it.
	gl.TextureStorage2D(texture, 1/*levels*/, GL_RGBA32F,
			TEXTURE_DIMENSION, TEXTURE_DIMENSION);

	// Check there are no OpenGL errors.
	GLUtils::check_gl_errors(gl, GPLATES_ASSERTION_SOURCE);

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

		const unsigned int num_seed_features = co_registration_parameters.seed_features.size();
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

					for (const SeedCoRegistrationPartialResult &partial_result : partial_results_list)
					{
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

					for (const SeedCoRegistrationPartialResult &partial_result : partial_results_list)
					{
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

					for (const SeedCoRegistrationPartialResult &partial_result : partial_results_list)
					{
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

					for (const SeedCoRegistrationPartialResult &partial_result : partial_results_list)
					{
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
		GL &gl,
		const QString &image_file_basename)
{
	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(gl);

	// Bind the pixel buffer so that all subsequent 'gl.ReadPixels()' calls go into that buffer.
	gl.BindBuffer(GL_PIXEL_PACK_BUFFER, d_debug_pixel_buffer);

	// NOTE: We don't need to worry about changing the default GL_PACK_ALIGNMENT (rows aligned to 4 bytes)
	// since our data is RGBA (already 4-byte aligned).
	gl.ReadPixels(0, 0, TEXTURE_DIMENSION, TEXTURE_DIMENSION, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	// Map the pixel buffer to access its data.
	const GLvoid *result_data = gl.MapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
	// If there was an error during mapping then report it and throw exception.
	if (!result_data)
	{
		// Log OpenGL error - a mapped data pointer of NULL should generate an OpenGL error.
		GLUtils::check_gl_errors(gl, GPLATES_ASSERTION_SOURCE);

		throw GPlatesOpenGL::OpenGLException(
				GPLATES_ASSERTION_SOURCE,
				"GLRasterCoRegistration: Failed to map OpenGL buffer object.");
	}

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

	if (!gl.UnmapBuffer(GL_PIXEL_PACK_BUFFER))
	{
		// Check OpenGL errors in case gl.UnmapBuffer used incorrectly.
		GLUtils::check_gl_errors(gl, GPLATES_ASSERTION_SOURCE);

		// Otherwise the buffer contents have been corrupted, so just emit a warning.
		qWarning() << "GLRasterCoRegistration: Failed to unmap OpenGL buffer object. "
				"Buffer object contents have been corrupted (such as an ALT+TAB switch between applications).";
	}

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
		GL &gl,
		const QString &image_file_basename,
		bool coverage_is_in_green_channel)
{
	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(gl);

	// Bind the pixel buffer so that all subsequent 'gl.ReadPixels()' calls go into that buffer.
	gl.BindBuffer(GL_PIXEL_PACK_BUFFER, d_debug_pixel_buffer);

	// NOTE: We don't need to worry about changing the default GL_PACK_ALIGNMENT (rows aligned to 4 bytes)
	// since our data is floats (each float is already 4-byte aligned).
	gl.ReadPixels(0, 0, TEXTURE_DIMENSION, TEXTURE_DIMENSION, GL_RGBA, GL_FLOAT, nullptr);

	// Map the pixel buffer to access its data.
	const GLvoid *result_data = gl.MapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
	// If there was an error during mapping then report it and throw exception.
	if (!result_data)
	{
		// Log OpenGL error - a mapped data pointer of NULL should generate an OpenGL error.
		GLUtils::check_gl_errors(gl, GPLATES_ASSERTION_SOURCE);

		throw GPlatesOpenGL::OpenGLException(
				GPLATES_ASSERTION_SOURCE,
				"GLRasterCoRegistration: Failed to map OpenGL buffer object.");
	}

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

	if (!gl.UnmapBuffer(GL_PIXEL_PACK_BUFFER))
	{
		// Check OpenGL errors in case gl.UnmapBuffer used incorrectly.
		GLUtils::check_gl_errors(gl, GPLATES_ASSERTION_SOURCE);

		// Otherwise the buffer contents have been corrupted, so just emit a warning.
		qWarning() << "GLRasterCoRegistration: Failed to unmap OpenGL buffer object. "
				"Buffer object contents have been corrupted (such as an ALT+TAB switch between applications).";
	}

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
		GL &gl)
{
	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(gl);

	for (unsigned int n = 0; n < NUM_PIXEL_BUFFERS; ++n)
	{
		// Allocate enough memory in each pixel buffer to read back a 4-channel floating-point texture.
		GLBuffer::shared_ptr_type buffer = GLBuffer::create(gl);

		// Bind the pixel buffer and allocate its memory.
		gl.BindBuffer(GL_PIXEL_PACK_BUFFER, buffer);
		gl.BufferData(GL_PIXEL_PACK_BUFFER, PIXEL_BUFFER_SIZE_IN_BYTES, nullptr, GL_STREAM_READ);

		// Add to our free list of pixel buffers.
		d_free_pixel_buffers.push_back(buffer);
	}

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			GPlatesUtils::Base2::is_power_of_two(MIN_DISTRIBUTE_READ_BACK_PIXEL_DIMENSION),
			GPLATES_ASSERTION_SOURCE);
}


void
GPlatesOpenGL::GLRasterCoRegistration::ResultsQueue::queue_reduce_pyramid_output(
		GL &gl,
		const GLFramebuffer::shared_ptr_type &framebuffer,
		bool &have_checked_framebuffer_completeness,
		const GLTexture::shared_ptr_type &results_texture,
		const ReduceQuadTree::non_null_ptr_to_const_type &reduce_quad_tree,
		std::vector<OperationSeedFeaturePartialResults> &seed_feature_partial_results)
{
	//PROFILE_FUNC();

	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(gl);

	if (d_free_pixel_buffers.empty())
	{
		// Free up a pixel buffer by extracting the results from the least-recently queued pixel buffer.
		flush_least_recently_queued_result(gl, seed_feature_partial_results);
	}

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			!d_free_pixel_buffers.empty(),
			GPLATES_ASSERTION_SOURCE);

	// Remove an unused pixel buffer from the free list.
	GLBuffer::shared_ptr_type pixel_buffer = d_free_pixel_buffers.back();
	d_free_pixel_buffers.pop_back();

	// Bind our framebuffer object to the results texture so that 'gl.ReadPixels' will read from it.
	gl.BindFramebuffer(GL_FRAMEBUFFER, framebuffer);

	// The texture to read from.
	//
	// Note that since we're using 'GL_COLOR_ATTACHMENT0' we don't need to call 'glReadBuffer'
	// because 'GL_COLOR_ATTACHMENT0' is the default GL_READ_BUFFER state for a framebuffer object.
	gl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, results_texture, 0/*level*/);

	// Check our framebuffer object for completeness (now that a texture is attached to it).
	// We only need to do this once because, while the texture changes, the framebuffer configuration
	// does not (ie, same texture internal format, dimensions, etc).
	if (!have_checked_framebuffer_completeness)
	{
		// Throw OpenGLException if not complete.
		// This should succeed since we should only be using texture formats that are required by OpenGL 3.3 core.
		const GLenum completeness = gl.CheckFramebufferStatus(GL_FRAMEBUFFER);
		GPlatesGlobal::Assert<OpenGLException>(
				completeness == GL_FRAMEBUFFER_COMPLETE,
				GPLATES_ASSERTION_SOURCE,
				"Framebuffer not complete for coregistering rasters.");

		have_checked_framebuffer_completeness = true;
	}

	// Start an asynchronous read back of the results texture to CPU memory (the pixel buffer).
	// OpenGL won't block until we attempt to read from the pixel buffer (so we delay that as much as possible).
	//
	// Bind the pixel buffer so that all subsequent 'gl.ReadPixels()' calls go into that buffer.
	//
	// NOTE: We don't need to worry about changing the default GL_PACK_ALIGNMENT (rows aligned to 4 bytes)
	// since our data is RGBA (already 4-byte aligned).
	gl.BindBuffer(GL_PIXEL_PACK_BUFFER, pixel_buffer);

	// Recurse into the reduce quad tree to determine which parts of the results texture need to be read back.
	//
	// Normally it's better to have one larger 'gl.ReadPixels' call instead of many small ones.
	// However our 'gl.ReadPixels()' calls are non-blocking since they're targeting a pixel buffer (async)
	// so they're not nearly as expensive as a 'gl.ReadPixels' to raw client memory (which would cause
	// the CPU to sync with the GPU thus leaving the GPU pipeline empty and hence stalling the GPU
	// until we can start feeding it again).
	// So the only cost for us, per 'gl.ReadPixels', is the time spent in the OpenGL driver setting
	// up the read command which, while not insignificant, is not as significant as a GPU stall so we
	// don't want to go overboard with the number of read calls but we do want to avoid downloading
	// TEXTURE_DIMENSION x TEXTURE_DIMENSION pixels of data (with one large read call) when only a
	// small portion of that contains actual result data (downloading a 1024x1024 texture can take
	// a few milliseconds which is a relatively long time when you think of how many CPU cycles
	// that is the equivalent of).
	distribute_async_read_back(gl, *reduce_quad_tree);

	// Add to the front of the results queue - we'll read the results later to avoid blocking.
	d_results_queue.push_front(ReducePyramidOutput(reduce_quad_tree, pixel_buffer));
}


void
GPlatesOpenGL::GLRasterCoRegistration::ResultsQueue::flush_results(
		GL &gl,
		std::vector<OperationSeedFeaturePartialResults> &seed_feature_partial_results)
{
	while (!d_results_queue.empty())
	{
		flush_least_recently_queued_result(gl, seed_feature_partial_results);
	}
}


void
GPlatesOpenGL::GLRasterCoRegistration::ResultsQueue::flush_least_recently_queued_result(
		GL &gl,
		std::vector<OperationSeedFeaturePartialResults> &seed_feature_partial_results)
{
	//PROFILE_FUNC();

	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(gl);

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

	// Bind the pixel buffer before mapping.
	gl.BindBuffer(GL_PIXEL_PACK_BUFFER, result.pixel_buffer);

	// Map the pixel buffer to access its data.
	//
	// Note that this is where blocking occurs if the data is not ready yet (eg, because GPU
	// is still generating it or still transferring to pixel buffer memory).
	const GLvoid *result_data = gl.MapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
	// If there was an error during mapping then report it and throw exception.
	if (!result_data)
	{
		// Log OpenGL error - a mapped data pointer of NULL should generate an OpenGL error.
		GLUtils::check_gl_errors(gl, GPLATES_ASSERTION_SOURCE);

		throw OpenGLException(
				GPLATES_ASSERTION_SOURCE,
				"GLRasterCoRegistration: Failed to map OpenGL buffer object.");
	}

	// Traverse the reduce quad tree and distribute the pixel buffer results to SeedCoRegistration objects.
	distribute_result_data(gl, result_data, *result.reduce_quad_tree, seed_feature_partial_results);

	if (!gl.UnmapBuffer(GL_PIXEL_PACK_BUFFER))
	{
		// Check OpenGL errors in case gl.UnmapBuffer used incorrectly.
		GLUtils::check_gl_errors(gl, GPLATES_ASSERTION_SOURCE);

		// Otherwise the buffer contents have been corrupted, so just emit a warning.
		qWarning() << "GLRasterCoRegistration: Failed to unmap OpenGL buffer object. "
				"Buffer object contents have been corrupted (such as an ALT+TAB switch between applications).";
	}
}


void
GPlatesOpenGL::GLRasterCoRegistration::ResultsQueue::distribute_async_read_back(
		GL &gl,
		const ReduceQuadTree &reduce_quad_tree)
{
	// Start reading to the beginning of the buffer.
	GLint pixel_buffer_offset = 0;

	distribute_async_read_back(
			gl,
			reduce_quad_tree.get_root_node(),
			pixel_buffer_offset,
			0/*pixel_rect_offset_x*/,
			0/*pixel_rect_offset_y*/,
			TEXTURE_DIMENSION/*pixel_rect_dimension*/);
}


void
GPlatesOpenGL::GLRasterCoRegistration::ResultsQueue::distribute_async_read_back(
		GL &gl,
		const ReduceQuadTreeInternalNode &reduce_quad_tree_internal_node,
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
		gl.ReadPixels(
				pixel_rect_offset_x,
				pixel_rect_offset_y,
				pixel_rect_dimension,
				pixel_rect_dimension,
				GL_RGBA,
				GL_FLOAT,
				GLVertexUtils::buffer_offset(pixel_buffer_offset));

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
						gl,
						*child_reduce_quad_tree_internal_node,
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
		GL &gl,
		const void *result_data,
		const ReduceQuadTree &reduce_quad_tree,
		std::vector<OperationSeedFeaturePartialResults> &seed_feature_partial_results)
{
	const ResultPixel *result_pixel_data = static_cast<const ResultPixel *>(result_data);
	// Start reading from the beginning of the result data buffer in units of *pixels* (not bytes).
	unsigned int result_data_pixel_offset = 0;

	distribute_result_data(
			gl,
			reduce_quad_tree.get_root_node(),
			result_pixel_data,
			result_data_pixel_offset,
			TEXTURE_DIMENSION/*pixel_rect_dimension*/,
			seed_feature_partial_results);
}


void
GPlatesOpenGL::GLRasterCoRegistration::ResultsQueue::distribute_result_data(
		GL &gl,
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
		// The beginning of the result data for the current 'gl.ReadPixels()' pixel rectangle.
		const ResultPixel *gl_read_pixels_result_data = result_pixel_data + result_data_pixel_offset;
		// The dimension of the current 'gl.ReadPixels()' pixel rectangle.
		const GLsizei gl_read_pixels_rect_dimension = pixel_rect_dimension;

		// Recurse into the reduce quad tree to extract data from the current pixel rectangle
		// that was originally read by a single 'gl.ReadPixels()' call.
		//
		// NOTE: The pixel x/y offsets are relative to the 'gl.ReadPixels()' pixel rectangle.
		distribute_result_data_from_gl_read_pixels_rect(
				gl,
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
						gl,
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
		GL &gl,
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
							gl,
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
