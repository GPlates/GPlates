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
#include <limits>
#include <QDebug>

#include "GLLight.h"

#include "GL.h"
#include "GLContext.h"
#include "GLFramebuffer.h"
#include "GLMatrix.h"
#include "GLShader.h"
#include "GLShaderSource.h"
#include "GLUtils.h"
#include "OpenGLException.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "maths/Vector3D.h"

#include "utils/CallStackTracker.h"


namespace GPlatesOpenGL
{
	namespace
	{
		/**
		 * Dimension of the map view light direction cube texture.
		 */
		const unsigned int MAP_VIEW_LIGHT_DIRECTION_CUBE_TEXTURE_DIMENSION = 256;

		/**
		 * Vertex shader source code to render light direction into cube texture for a 2D map view.
		 */
		const char *RENDER_MAP_VIEW_LIGHT_DIRECTION_VERTEX_SHADER_SOURCE =
			R"(
				uniform mat4 view_inverse;
				uniform vec3 view_space_light_direction;
			
				layout(location = 0) in vec4 position;

				out vec3 world_space_light_direction;
			
				void main (void)
				{
					gl_Position = position;
				
					// If the light direction is attached to view space then reverse rotate to world space.
					world_space_light_direction = mat3(view_inverse) * view_space_light_direction;
				}
			)";
		/**
		 * Fragment shader source code to render light direction into cube texture for a 2D map view.
		 */
		const char *RENDER_MAP_VIEW_LIGHT_DIRECTION_FRAGMENT_SHADER_SOURCE =
			R"(
				in vec3 world_space_light_direction;

				layout(location = 0) out vec4 light_direction;

				void main (void)
				{
					// Just return the light direction - it doesn't vary across the cube texture face_target.
					// But first convert from range [-1,1] to range [0,1] to store in unsigned 8-bit render face_target.
					light_direction = vec4(0.5 * world_space_light_direction + 0.5, 1);
				}
			)";
	}
}


GPlatesOpenGL::GLLight::non_null_ptr_type
GPlatesOpenGL::GLLight::create(
		GL &gl,
		const GPlatesGui::SceneLightingParameters &scene_lighting_params,
		const GLMatrix &view_orientation,
		boost::optional<const GPlatesGui::MapProjection &> map_projection)
{
	return non_null_ptr_type(
			new GLLight(
					gl,
					scene_lighting_params,
					view_orientation,
					map_projection));
}


GPlatesOpenGL::GLLight::GLLight(
		GL &gl,
		const GPlatesGui::SceneLightingParameters &scene_lighting_params,
		const GLMatrix &view_orientation,
		boost::optional<const GPlatesGui::MapProjection &> map_projection) :
	d_scene_lighting_params(scene_lighting_params),
	d_view_orientation(view_orientation),
	d_globe_view_light_direction(scene_lighting_params.get_globe_view_light_direction()/*not necessarily in world-space yet!*/),
	d_map_view_constant_lighting(0),
	d_map_projection(map_projection),
	d_map_view_light_direction_cube_texture_dimension(
			(std::min)(MAP_VIEW_LIGHT_DIRECTION_CUBE_TEXTURE_DIMENSION, gl.get_capabilities().gl_max_cube_map_texture_size)),
	d_map_view_light_direction_cube_texture(GLTexture::create(gl)),
	d_map_view_light_direction_cube_framebuffer(GLFramebuffer::create(gl)),
	d_render_map_view_light_direction_program(GLProgram::create(gl)),
	d_full_screen_quad(gl.get_context().get_shared_state()->get_full_screen_quad(gl))
{
	compile_link_programs(gl);

	// Allocate image space for light direction cube map and check we can render into it.
	create_map_view_light_direction_cube_texture(gl);
	check_framebuffer_completeness_map_view_light_direction_cube_texture(gl);

	if (d_map_projection)
	{
		update_map_view(gl);
	}
	else
	{
		// Make sure globe-view light direction is in world-space (and not view-space).
		update_globe_view(gl);
	}
}


void
GPlatesOpenGL::GLLight::set_scene_lighting(
		GL &gl,
		const GPlatesGui::SceneLightingParameters &scene_lighting_params,
		const GLMatrix &view_orientation,
		boost::optional<const GPlatesGui::MapProjection &> map_projection)
{
	bool update = false;

	// If switching between globe and map views.
	if (static_cast<bool>(map_projection) != static_cast<bool>(d_map_projection))
	{
		update = true;

		d_map_projection = map_projection;
	}

	if (view_orientation != d_view_orientation)
	{
		// If the light direction is attached to the view frame then we need to update.
		if (scene_lighting_params.is_light_direction_attached_to_view_frame())
		{
			update = true;
		}

		d_view_orientation = view_orientation;
	}

	if (scene_lighting_params != d_scene_lighting_params)
	{
		// If any of the lighting parameters have changed then we need to update.
		update = true;

		d_scene_lighting_params = scene_lighting_params;
	}

	if (update)
	{
		if (d_map_projection)
		{
			// Regenerate the light direction cube map texture.
			update_map_view(gl);
		}
		else
		{
			// Update the world-space light direction.
			update_globe_view(gl);
		}

		// Let clients know in case they need to flush and regenerate their cache.
		d_subject_token.invalidate();
	}
}


void
GPlatesOpenGL::GLLight::compile_link_programs(
		GL &gl)
{
	// Add this scope to the call stack trace printed if exception thrown in this scope (eg, failure to compile/link shader).
	TRACK_CALL_STACK();

	// Vertex shader source.
	GPlatesOpenGL::GLShaderSource vertex_shader_source;
	vertex_shader_source.add_code_segment(RENDER_MAP_VIEW_LIGHT_DIRECTION_VERTEX_SHADER_SOURCE);

	// Vertex shader.
	GPlatesOpenGL::GLShader::shared_ptr_type vertex_shader = GPlatesOpenGL::GLShader::create(gl, GL_VERTEX_SHADER);
	vertex_shader->shader_source(gl, vertex_shader_source);
	vertex_shader->compile_shader(gl);

	// Fragment shader source.
	GPlatesOpenGL::GLShaderSource fragment_shader_source;
	fragment_shader_source.add_code_segment(RENDER_MAP_VIEW_LIGHT_DIRECTION_FRAGMENT_SHADER_SOURCE);

	// Fragment shader.
	GPlatesOpenGL::GLShader::shared_ptr_type fragment_shader = GPlatesOpenGL::GLShader::create(gl, GL_FRAGMENT_SHADER);
	fragment_shader->shader_source(gl, fragment_shader_source);
	fragment_shader->compile_shader(gl);

	// Vertex-fragment program.
	d_render_map_view_light_direction_program->attach_shader(gl, vertex_shader);
	d_render_map_view_light_direction_program->attach_shader(gl, fragment_shader);
	d_render_map_view_light_direction_program->link_program(gl);
}


void
GPlatesOpenGL::GLLight::create_map_view_light_direction_cube_texture(
		GL &gl)
{
	// Make sure we leave the OpenGL state the way it was.
	GPlatesOpenGL::GL::StateScope save_restore_state(gl);

	gl.BindTexture(GL_TEXTURE_CUBE_MAP, d_map_view_light_direction_cube_texture);

	// Using nearest-neighbour filtering since the 'pixelation' of the light direction is not
	// noticeable once it goes through the dot product with the surface normals.
	// Also it enables us to have distinctly different light directions on either side of the
	// central meridian which we'll make go through the centre of some of the faces of the cube
	// (which is along a boundary between two columns of pixels - provided texture dimension is even).
	gl.TexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	gl.TexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Clamp texture coordinates to centre of edge texels.
	// Not strictly necessary for nearest-neighbour filtering but it is if later we change to use
	// linear filtering to avoid seams.
	gl.TexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	gl.TexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Create the texture but don't load any data into it.
	// Leave it uninitialised because we will be rendering into it to initialise it.

	// Initialise all six faces of the cube texture.
	for (unsigned int face = 0; face < 6; ++face)
	{
		const GLenum face_target = static_cast<GLenum>(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face);

		gl.TexImage2D(face_target, 0, GL_RGBA8,
				d_map_view_light_direction_cube_texture_dimension, d_map_view_light_direction_cube_texture_dimension,
				0,
				// Since the image data is NULL it doesn't really matter what 'format' and 'type' are - just
				// use values that are compatible with all internal color formats to avoid a possible error...
				GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	}

	// Check there are no OpenGL errors.
	GLUtils::check_gl_errors(gl, GPLATES_ASSERTION_SOURCE);
}


void
GPlatesOpenGL::GLLight::check_framebuffer_completeness_map_view_light_direction_cube_texture(
		GL &gl)
{
	// Make sure we leave the OpenGL state the way it was.
	GL::StateScope save_restore_state(gl);

	// Bind our framebuffer object.
	gl.BindFramebuffer(GL_FRAMEBUFFER, d_map_view_light_direction_cube_framebuffer);

	// Attach a face of cube map. Any face should do (if one face passes then the others should also).
	gl.FramebufferTexture2D(GL_FRAMEBUFFER,
			GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X, d_map_view_light_direction_cube_texture, 0/*level*/);

	// Throw OpenGLException if not complete.
	// This should succeed since we're using GL_RGBA8 texture format (which is required by OpenGL 3.3 core).
	const GLenum completeness = gl.CheckFramebufferStatus(GL_FRAMEBUFFER);
	GPlatesGlobal::Assert<OpenGLException>(
			completeness == GL_FRAMEBUFFER_COMPLETE,
			GPLATES_ASSERTION_SOURCE,
			"Framebuffer not complete for rendering light view direction into cube map.");
}


void
GPlatesOpenGL::GLLight::update_map_view(
		GL &gl)
{
	if (!d_map_projection)
	{
		return;
	}

	//
	// Calculate the ambient+diffuse lighting when the surface normal is constant across the map
	// (and also perpendicular to the map plane).
	//
	// This is used when there is *no* surface normal mapping (ie, surface normal, and hence lighting,
	// is constant across the map and can be pre-calculated).
	//

	GPlatesMaths::UnitVector3D map_view_light_direction =
			d_scene_lighting_params.get_map_view_light_direction();

	// FIXME: The map view orientation (3x3 subpart of matrix) contains (x,y) scaling factors
	// and hence is not purely an orthogonal rotation like we need.
	// Currently we don't need to transform the map view light direction from view space to
	// world space because the map surface normal is always (0,0,1) which is perpendicular to
	// the map plane so any rotation of the light direction in the (x,y) plane will not
	// affect the diffuse lighting lambert dot product - so we can ignore the inverse transformation
	// and just use the view-space light direction as if it was in world-space.
	// This will change if we ever allow tilting in the map view which would no longer confine
	// view rotation to the (x,y) plane.
#if 0
	if (d_scene_lighting_params.is_light_direction_attached_to_view_frame())
	{
		// The light direction is in view-space.
		const GPlatesMaths::UnitVector3D &view_space_light_direction =
				d_scene_lighting_params.get_map_view_light_direction();
		const double light_x = view_space_light_direction.x().dval();
		const double light_y = view_space_light_direction.y().dval();
		const double light_z = view_space_light_direction.z().dval();

		// Need to reverse rotate back to world-space.
		// We'll assume the view orientation matrix stores only a 3x3 rotation.
		// In which case the inverse matrix is the transpose.
		const GLMatrix &view = d_view_orientation;

		// Multiply the view-space light direction by the transpose of the 3x3 view orientation.
		map_view_light_direction = GPlatesMaths::Vector3D(
				view.get_element(0,0) * light_x + view.get_element(1,0) * light_y + view.get_element(2,0) * light_z,
				view.get_element(0,1) * light_x + view.get_element(1,1) * light_y + view.get_element(2,1) * light_z,
				view.get_element(0,2) * light_x + view.get_element(1,2) * light_y + view.get_element(2,2) * light_z)
						.get_normalisation();
	}
	// else light direction is attached to world-space.
#endif

	// The constant surface normal direction.
	const GPlatesMaths::UnitVector3D surface_normal(0, 0, 1);

	// Pre-calculate the constant lighting across map for a surface normal perpendicular to the
	// map plane (ie, when no normal map is used and hence surface normal is constant across map).
	const double map_view_lambert_diffuse_lighting = (std::max)(
			0.0,
			dot(surface_normal, map_view_light_direction).dval());

	// Mix in ambient with diffuse lighting.
	d_map_view_constant_lighting =
			d_scene_lighting_params.get_ambient_light_contribution() +
			map_view_lambert_diffuse_lighting * (1 - d_scene_lighting_params.get_ambient_light_contribution());

	//
	// The light direction is constant in 2D map view but varies across the globe.
	// So we capture the variation in a hardware cube map texture.
	//
	// The hardware cube map is used when normal mapping is needed (ie, surface normals vary across the map),
	// otherwise the lighting is constant across the map and can be pre-calculated.
	//

	// Make sure we leave the OpenGL state the way it was.
	GL::StateScope save_restore_state(
			gl,
			// We're rendering to a render target so reset to the default OpenGL state...
			true/*reset_to_default_state*/);

	// Render to the entire texture of each cube face.
	gl.Viewport(0, 0, d_map_view_light_direction_cube_texture_dimension, d_map_view_light_direction_cube_texture_dimension);

	// The clear colour is all zeros.
	gl.ClearColor();

	// Bind the full screen quad.
	gl.BindVertexArray(d_full_screen_quad);

	// Bind the shader program for rendering light direction for the 2D map views.
	gl.UseProgram(d_render_map_view_light_direction_program);

	GLMatrix view_inverse_matrix;
		// FIXME: The map view orientation (3x3 subpart of matrix) contains (x,y) scaling factors
		// and hence is not purely an orthogonal rotation like we need.
		// Currently we don't need to transform the map view light direction from view space to
		// world space because the map surface normal is always (0,0,1) which is perpendicular to
		// the map plane so any rotation of the light direction in the (x,y) plane will not
		// affect the diffuse lighting lambert dot product - so we can ignore the inverse transformation
		// and just use the view-space light direction as if it was in world-space.
		// This will change if we ever allow tilting in the map view which would no longer confine
		// view rotation to the (x,y) plane.
#if 0
	// If the light direction is attached to the view frame then specify a non-identity view transform.
	if (d_scene_lighting_params.is_light_direction_attached_to_view_frame())
	{
		view_inverse_matrix = d_view_orientation;
		view_inverse_matrix.glu_inverse();  // Should always return true for typical view transforms.
	}
#endif
	GLfloat view_inverse_float_matrix[16];
	view_inverse_matrix.get_float_matrix(view_inverse_float_matrix);
	gl.UniformMatrix4fv(
			d_render_map_view_light_direction_program->get_uniform_location(gl, "view_inverse"),
			1, GL_FALSE/*transpose*/, view_inverse_float_matrix);

	// Set the view-space light direction (which is world-space if light not attached to view-space).
	// The shader program will transform it to world-space.
	gl.Uniform3f(
			d_render_map_view_light_direction_program->get_uniform_location(gl, "view_space_light_direction"),
			d_scene_lighting_params.get_map_view_light_direction().x().dval(),
			d_scene_lighting_params.get_map_view_light_direction().y().dval(),
			d_scene_lighting_params.get_map_view_light_direction().z().dval());

	// Bind our framebuffer object.
	gl.BindFramebuffer(GL_FRAMEBUFFER, d_map_view_light_direction_cube_framebuffer);

	// Render to all six faces of the cube texture.
	for (unsigned int face = 0; face < 6; ++face)
	{
		const GLenum face_target = static_cast<GLenum>(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face);

		// Begin rendering to the 2D texture of the current cube face.
		gl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, face_target, d_map_view_light_direction_cube_texture, 0/*level*/);

		// There's only a colour buffer to clear.
		gl.Clear(GL_COLOR_BUFFER_BIT);

		// Draw the full screen quad.
		gl.DrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}
}


void
GPlatesOpenGL::GLLight::update_globe_view(
		GL &gl)
{
	if (d_map_projection)
	{
		return;
	}

	if (d_scene_lighting_params.is_light_direction_attached_to_view_frame())
	{
		// Reverse rotate light direction from view-space back to world-space.
		d_globe_view_light_direction =
				GPlatesGui::transform_globe_view_space_light_direction_to_world_space(
						d_scene_lighting_params.get_globe_view_light_direction(),
						d_view_orientation);
	}
	else // light direction is attached to world-space...
	{
		d_globe_view_light_direction = d_scene_lighting_params.get_globe_view_light_direction();
	}
}
