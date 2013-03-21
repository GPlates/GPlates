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

/*
 * The OpenGL Extension Wrangler Library (GLEW).
 * Must be included before the OpenGL headers (which also means before Qt headers).
 * For this reason it's best to try and include it in ".cc" files only.
 */
#include <GL/glew.h>

#include "GLStateSets.h"

#include "GLCapabilities.h"
#include "GLState.h"
#include "OpenGLException.h"

#include "global/CompilerWarnings.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


// We use macros in <GL/glew.h> that contain old-style casts.
DISABLE_GCC_WARNING("-Wold-style-cast")

namespace GPlatesOpenGL
{
	namespace
	{
		/**
		 * Ensures the matrix mode is set to @a matrix mode and changes it if necessary.
		 */
		void
		set_matrix_mode(
				GLenum mode,
				GLState &last_applied_state)
		{
			// Make sure the correct matrix mode is set.
			if (last_applied_state.get_matrix_mode() != mode)
			{
				// Apply the change to OpenGL.
				glMatrixMode(mode);

				// And record the change we've made.
				last_applied_state.set_matrix_mode(mode);
			}
		}


		/**
		 * Ensures the active texture unit is set to @a texture_unit and changes it if necessary.
		 */
		void
		set_active_texture(
				const GLCapabilities &capabilities,
				GLenum texture_unit,
				GLState &last_applied_state)
		{
			// Make sure the correct texture unit is currently active.
			if (last_applied_state.get_active_texture() != texture_unit)
			{
				// GL_ARB_multitexture not required if texture_unit is zero (so we test instead of assert)...
				if (GLEW_ARB_multitexture)
				{
					// Apply the change to OpenGL.
					glActiveTextureARB(texture_unit);

					// And record the change we've made.
					last_applied_state.set_active_texture(capabilities, texture_unit);
				}
			}
		}


		/**
		 * Ensures the client active texture unit is set to @a texture_unit and changes it if necessary.
		 */
		void
		set_client_active_texture(
				const GLCapabilities &capabilities,
				GLenum texture_unit,
				GLState &last_applied_state)
		{
			// Make sure the correct texture unit is currently active.
			if (last_applied_state.get_client_active_texture() != texture_unit)
			{
				// GL_ARB_multitexture not required if texture_unit is zero (so we test instead of assert)...
				if (GLEW_ARB_multitexture)
				{
					// Apply the change to OpenGL.
					glClientActiveTextureARB(texture_unit);

					// And record the change we've made.
					last_applied_state.set_client_active_texture(capabilities, texture_unit);
				}
			}
		}


		/**
		 * Ensures the currently bound buffer object (for its target type) is @a buffer_object_resource.
		 */
		void
		bind_buffer_object(
				GLBufferObject::resource_handle_type buffer_object_resource,
				const GLBufferObject::shared_ptr_to_const_type &buffer_object,
				GLenum target,
				GLState &last_applied_state)
		{
			// Make sure the correct buffer object is currently bound.
			if (last_applied_state.get_bind_buffer_object_resource(target) != buffer_object_resource)
			{
				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						GPLATES_OPENGL_BOOL(GLEW_ARB_vertex_buffer_object),
						GPLATES_ASSERTION_SOURCE);

				// Bind the buffer object.
				glBindBufferARB(target, buffer_object_resource);

				// And record the change we've made.
				last_applied_state.set_bind_buffer_object(buffer_object, target);
			}
		}


		/**
		 * Ensures there is no currently bound buffer object for the specified target type.
		 */
		void
		unbind_buffer_object(
				GLenum target,
				GLState &last_applied_state)
		{
			// Make sure the no buffer object is currently bound.
			if (last_applied_state.get_bind_buffer_object_resource(target))
			{
				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						GPLATES_OPENGL_BOOL(GLEW_ARB_vertex_buffer_object),
						GPLATES_ASSERTION_SOURCE);

				// No buffer object - back to client memory arrays.
				glBindBufferARB(target, 0);

				// And record the change we've made.
				last_applied_state.set_unbind_buffer_object(target);
			}
		}


		//! Applies texture coordinate generation state based on param variant type.
		class TexGenVisitor :
				public boost::static_visitor<>
		{
		public:
			TexGenVisitor(
					GLenum coord,
					GLenum pname) :
				d_coord(coord),
				d_pname(pname)
			{  }

			void
			operator()(
					const GLint &param) const
			{
				glTexGeni(d_coord, d_pname, param);
			}

			void
			operator()(
					const GLfloat &param) const
			{
				glTexGenf(d_coord, d_pname, param);
			}

			void
			operator()(
					const GLdouble &param) const
			{
				glTexGend(d_coord, d_pname, param);
			}

			void
			operator()(
					const std::vector<GLint> &param) const
			{
				// All uses of glTexGen*v expect four parameters.
				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						param.size() == 4,
						GPLATES_ASSERTION_SOURCE);

				glTexGeniv(d_coord, d_pname, &param[0]);
			}

			void
			operator()(
					const std::vector<GLfloat> &param) const
			{
				// All uses of glTexGen*v expect four parameters.
				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						param.size() == 4,
						GPLATES_ASSERTION_SOURCE);

				glTexGenfv(d_coord, d_pname, &param[0]);
			}

			void
			operator()(
					const std::vector<GLdouble> &param) const
			{
				// All uses of glTexGen*v expect four parameters.
				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						param.size() == 4,
						GPLATES_ASSERTION_SOURCE);

				glTexGendv(d_coord, d_pname, &param[0]);
			}

		private:
			GLenum d_coord;
			GLenum d_pname;
		};


		//! Applies texture environment state based on param variant type.
		class TexEnvVisitor :
				public boost::static_visitor<>
		{
		public:
			TexEnvVisitor(
					GLenum target,
					GLenum pname) :
				d_target(target),
				d_pname(pname)
			{  }

			void
			operator()(
					const GLint &param) const
			{
				glTexEnvi(d_target, d_pname, param);
			}

			void
			operator()(
					const GLfloat &param) const
			{
				glTexEnvf(d_target, d_pname, param);
			}

			void
			operator()(
					const std::vector<GLint> &param) const
			{
				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						!param.empty(),
						GPLATES_ASSERTION_SOURCE);

				glTexEnviv(d_target, d_pname, &param[0]);
			}

			void
			operator()(
					const std::vector<GLfloat> &param) const
			{
				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						!param.empty(),
						GPLATES_ASSERTION_SOURCE);

				glTexEnvfv(d_target, d_pname, &param[0]);
			}

		private:
			GLenum d_target;
			GLenum d_pname;
		};


		//! Compares state that consists of GLint, GLfloat, GLdouble or a std::vector of any of those types.
		class EqualityVisitor :
				public boost::static_visitor<bool>
		{
		public:
			//! This is needed since can't really compare say a GLint with a std::vector<GLint>.
			template <typename ParamDataType1, typename ParamDataType2>
			bool
			operator()(
					const ParamDataType1 &param1,
					const ParamDataType2 &param2) const
			{
				return false;
			}

			//! For GLint.
			bool
			operator()(
					GLint param1,
					GLint param2) const
			{
				return param1 == param2;
			}

			//! For GLfloat.
			bool
			operator()(
					GLfloat param1,
					GLfloat param2) const
			{
				return GPlatesMaths::are_almost_exactly_equal(param1, param2);
			}

			//! For GLdouble.
			bool
			operator()(
					GLdouble param1,
					GLdouble param2) const
			{
				return GPlatesMaths::are_almost_exactly_equal(param1, param2);
			}

			/**
			 * The template types can be GLint, GLfloat or GLdouble.
			 *
			 * We compare as if they are floating-point types using epsilon comparisons.
			 */
			template <typename ParamDataType1, typename ParamDataType2>
			bool
			operator()(
					const std::vector<ParamDataType1> &param1,
					const std::vector<ParamDataType2> &param2) const
			{
				// Both vectors should be the same size - this should always be the case actually.
				if (param1.size() != param2.size())
				{
					return false;
				}

				// Test to see if any parameters differ within epsilon.
				const unsigned int num_params = param1.size();
				for (unsigned int n = 0; n < num_params; ++n)
				{
					if (!GPlatesMaths::are_almost_exactly_equal(param1[n], param2[n]))
					{
						return false;
					}
				}

				return true;
			}
		};


		//! Converts an array of 4 numbers into a std::vector.
		template <typename Type>
		std::vector<Type>
		create_4_vector(
				const Type &x,
				const Type &y,
				const Type &z,
				const Type &w)
		{
			std::vector<Type> vec4(4);

			vec4[0] = x;
			vec4[1] = y;
			vec4[2] = z;
			vec4[3] = w;

			return vec4;
		}
	}
}


bool
GPlatesOpenGL::Implementation::GLVertexAttributeBuffer::has_changed_state(
		const GLVertexAttributeBuffer &last_applied_buffer) const
{
	bool updated_buffer = false;

	// Determine if we're using a vertex buffer object or client memory.
	const GLBufferImpl::shared_ptr_to_const_type *buffer_impl =
			boost::get<GLBufferImpl::shared_ptr_to_const_type>(&d_buffer_variant);

	if (buffer_impl) // using client memory...
	{
		// See if the client memory array has changed.
		// NOTE: 'this' and 'last_applied_buffer' may refer to the *same* object.
		const GLvoid *buffer_pointer = (*buffer_impl)->get_buffer_resource() + d_offset;
		if (buffer_pointer != last_applied_buffer.d_pointer_to_apply)
		{
			updated_buffer = true;
		}
		d_pointer_to_apply = buffer_pointer;
	}
	else // using buffer object...
	{
		// See if the buffer offset has changed.
		// NOTE: 'this' and 'last_applied_buffer' may refer to the *same* object.
		const GLvoid *buffer_pointer = GPLATES_OPENGL_BUFFER_OFFSET(d_offset);
		if (buffer_pointer != last_applied_buffer.d_pointer_to_apply)
		{
			updated_buffer = true;
		}
		d_pointer_to_apply = buffer_pointer;
	}

	// The following applies to *both* types of buffers.
	//
	// If it's a different buffer (includes changing from buffer object to client memory or vice versa) *or*
	// (it's the same buffer *and* it has been allocated since last specified to OpenGL).
	//
	// If it's the same buffer then see if the internal buffer has been reallocated
	// (ie, if 'glBufferData' has been called).
	// This shouldn't be necessary for native buffer objects (ie, the GLBufferObject type) - seems
	// to work fine without this on nVidia hardware but ATI hardware seems to need it (at least the
	// Macbook AMD HD6750 tested on) - seems needs the vertex array pointers to be rebound whenever
	// 'glBufferData' is called.
	// Maybe this isn't in the spec and nVidia do it anyway - not sure what the spec says?
	// 
	if (d_buffer != last_applied_buffer.d_buffer ||
		d_buffer->has_buffer_been_allocated_since(d_buffer_allocation_observer))
	{
		updated_buffer = true;
	}

	return updated_buffer;
}


bool
GPlatesOpenGL::Implementation::GLVertexAttributeBuffer::has_changed_from_default_state() const
{
	bool updated_buffer = false;

	// Determine if we're using a vertex buffer object or client memory.
	const GLBufferImpl::shared_ptr_to_const_type *buffer_impl =
			boost::get<GLBufferImpl::shared_ptr_to_const_type>(&d_buffer_variant);

	if (buffer_impl) // using client memory...
	{
		// No need to check for a different buffer because the default is a NULL client memory array.

		// See if the client memory array has changed.
		const GLvoid *buffer_pointer = (*buffer_impl)->get_buffer_resource() + d_offset;
		if (buffer_pointer != NULL/*default client memory pointer*/)
		{
			updated_buffer = true;
		}
		d_pointer_to_apply = buffer_pointer;
	}
	else // using buffer object...
	{
		// The very fact we're using a buffer object means we're different than the default
		// (which is client memory arrays).
		updated_buffer = true;

		// Update the buffer offset.
		d_pointer_to_apply = GPLATES_OPENGL_BUFFER_OFFSET(d_offset);
	}

	return updated_buffer;
}


bool
GPlatesOpenGL::Implementation::GLVertexAttributeBuffer::has_changed_to_default_state() const
{
	bool updated_buffer = false;

	// Determine if we're using a vertex buffer object or client memory.
	const GLBufferImpl::shared_ptr_to_const_type *buffer_impl =
			boost::get<GLBufferImpl::shared_ptr_to_const_type>(&d_buffer_variant);

	if (buffer_impl) // using client memory...
	{
		// No need to check for a different buffer because the default is a NULL client memory array.

		// We are the last applied state - we're going *to* the default state.
		// See if the client memory array has changed.
		if (d_pointer_to_apply != NULL/*default client memory pointer*/)
		{
			updated_buffer = true;
			d_pointer_to_apply = NULL; // Not really used - but just in case.
		}
	}
	else // using buffer object...
	{
		// The very fact we're using a buffer object means we're different than the default
		// (which is client memory arrays).
		updated_buffer = true;

		// Not really used - but just in case.
		d_pointer_to_apply = NULL;

		// We are the last applied state - we're going *to* the default state.
	}

	return updated_buffer;
}


void
GPlatesOpenGL::Implementation::GLVertexAttributeBuffer::bind_buffer(
		GLState &last_applied_state) const
{
	// Determine if we're using a vertex buffer object or client memory.
	const GLBufferImpl::shared_ptr_to_const_type *buffer_impl =
			boost::get<GLBufferImpl::shared_ptr_to_const_type>(&d_buffer_variant);

	if (buffer_impl) // using client memory...
	{
		// Make sure we're using client memory by unbinding any currently bound buffer object.
		unbind_buffer_object(GLVertexBufferObject::get_target_type(), last_applied_state);
	}
	else // using buffer object...
	{
		const GLBufferObject::shared_ptr_to_const_type &buffer_object =
				boost::get<GLBufferObject::shared_ptr_to_const_type>(d_buffer_variant);

		// Bind the buffer object.
		bind_buffer_object(
				buffer_object->get_buffer_resource_handle(),
				buffer_object,
				GLVertexBufferObject::get_target_type(),
				last_applied_state);
	}
}


void
GPlatesOpenGL::Implementation::GLVertexAttributeBuffer::unbind_buffer(
		GLState &last_applied_state) const
{
	// Make sure we're using client memory by unbinding any currently bound buffer object.
	unbind_buffer_object(GLVertexBufferObject::get_target_type(), last_applied_state);
}


GPlatesOpenGL::GLActiveTextureStateSet::GLActiveTextureStateSet(
		const GLCapabilities &capabilities,
		GLenum active_texture) :
	d_active_texture(active_texture)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			active_texture >= GL_TEXTURE0 &&
					active_texture < GL_TEXTURE0 + capabilities.texture.gl_max_texture_image_units,
			GPLATES_ASSERTION_SOURCE);
}


void
GPlatesOpenGL::GLActiveTextureStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &last_applied_state_set,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (d_active_texture ==
			// Throws exception if downcast fails...
			dynamic_cast<const GLActiveTextureStateSet &>(last_applied_state_set).d_active_texture)
	{
		return;
	}

	if (GLEW_ARB_multitexture)
	{
		glActiveTextureARB(d_active_texture);
	}
	// No GL_ARB_multitexture means we're always using texture unit 0.
}

void
GPlatesOpenGL::GLActiveTextureStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (d_active_texture == GL_TEXTURE0)
	{
		return;
	}

	if (GLEW_ARB_multitexture)
	{
		glActiveTextureARB(d_active_texture);
	}
	// No GL_ARB_multitexture means we're always using texture unit 0.
}

void
GPlatesOpenGL::GLActiveTextureStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (GL_TEXTURE0 == d_active_texture)
	{
		return;
	}

	if (GLEW_ARB_multitexture)
	{
		// Texture unit 0.
		glActiveTextureARB(GL_TEXTURE0);
	}
	// No GL_ARB_multitexture means we're always using texture unit 0.
}


void
GPlatesOpenGL::GLAlphaFuncStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &last_applied_state_set,
		GLState &last_applied_state) const
{
	// Throws exception if downcast fails...
	const GLAlphaFuncStateSet &last_applied = dynamic_cast<const GLAlphaFuncStateSet &>(last_applied_state_set);

	// Return early if no state change...
	// Note that 'd_ref' is an epsilon comparison.
	if (d_alpha_func == last_applied.d_alpha_func && d_ref == last_applied.d_ref)
	{
		return;
	}

	glAlphaFunc(d_alpha_func, d_ref.dval());
}

void
GPlatesOpenGL::GLAlphaFuncStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	// Note that 'd_ref' is an epsilon comparison.
	if (d_alpha_func == GL_ALWAYS && d_ref == 0)
	{
		return;
	}

	glAlphaFunc(d_alpha_func, d_ref.dval());
}

void
GPlatesOpenGL::GLAlphaFuncStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	// Note that 'd_ref' is an epsilon comparison.
	if (GL_ALWAYS == d_alpha_func && 0 == d_ref)
	{
		return;
	}

	glAlphaFunc(GL_ALWAYS, 0);
}


void
GPlatesOpenGL::GLBindBufferObjectStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &last_applied_state_set,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (d_buffer_object_resource ==
			// Throws exception if downcast fails...
			dynamic_cast<const GLBindBufferObjectStateSet &>(last_applied_state_set).d_buffer_object_resource)
	{
		return;
	}

	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPLATES_OPENGL_BOOL(GLEW_ARB_vertex_buffer_object),
			GPLATES_ASSERTION_SOURCE);

	if (d_buffer_object_resource)
	{
		// Bind the buffer object.
		glBindBufferARB(d_target, d_buffer_object_resource.get());
	}
	else
	{
		// No buffer object - back to client memory arrays.
		glBindBufferARB(d_target, 0);
	}
}

void
GPlatesOpenGL::GLBindBufferObjectStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (!d_buffer_object_resource)
	{
		return;
	}

	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPLATES_OPENGL_BOOL(GLEW_ARB_vertex_buffer_object),
			GPLATES_ASSERTION_SOURCE);

	// Bind the buffer object.
	glBindBufferARB(d_target, d_buffer_object_resource.get());
}

void
GPlatesOpenGL::GLBindBufferObjectStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (!d_buffer_object_resource)
	{
		return;
	}

	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPLATES_OPENGL_BOOL(GLEW_ARB_vertex_buffer_object),
			GPLATES_ASSERTION_SOURCE);

	// The default is zero (no buffer object - back to client memory arrays).
	glBindBufferARB(d_target, 0);
}


void
GPlatesOpenGL::GLBindFrameBufferObjectStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &last_applied_state_set,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (d_frame_buffer_object ==
			// Throws exception if downcast fails...
			dynamic_cast<const GLBindFrameBufferObjectStateSet &>(last_applied_state_set).d_frame_buffer_object)
	{
		return;
	}

	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPLATES_OPENGL_BOOL(GLEW_EXT_framebuffer_object),
			GPLATES_ASSERTION_SOURCE);

	if (d_frame_buffer_object)
	{
		// Bind the frame buffer object.
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, d_frame_buffer_object.get()->get_frame_buffer_resource_handle());
	}
	else
	{
		// No framebuffer object - back to using the main framebuffer.
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	}
}

void
GPlatesOpenGL::GLBindFrameBufferObjectStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (!d_frame_buffer_object)
	{
		return;
	}

	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPLATES_OPENGL_BOOL(GLEW_EXT_framebuffer_object),
			GPLATES_ASSERTION_SOURCE);

	// Bind the frame buffer object.
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, d_frame_buffer_object.get()->get_frame_buffer_resource_handle());
}

void
GPlatesOpenGL::GLBindFrameBufferObjectStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (!d_frame_buffer_object)
	{
		return;
	}

	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPLATES_OPENGL_BOOL(GLEW_EXT_framebuffer_object),
			GPLATES_ASSERTION_SOURCE);

	// The default is zero (the main framebuffer).
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}


void
GPlatesOpenGL::GLBindProgramObjectStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &last_applied_state_set,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (d_program_object ==
			// Throws exception if downcast fails...
			dynamic_cast<const GLBindProgramObjectStateSet &>(last_applied_state_set).d_program_object)
	{
		return;
	}

	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPLATES_OPENGL_BOOL(GLEW_ARB_shader_objects),
			GPLATES_ASSERTION_SOURCE);

	if (d_program_object)
	{
		// Bind the shader program object.
		glUseProgramObjectARB(d_program_object.get()->get_program_resource_handle());
	}
	else
	{
		// No shader program object - back to using the fixed-function pipeline.
		glUseProgramObjectARB(0);
	}
}

void
GPlatesOpenGL::GLBindProgramObjectStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (!d_program_object)
	{
		return;
	}

	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPLATES_OPENGL_BOOL(GLEW_ARB_shader_objects),
			GPLATES_ASSERTION_SOURCE);

	// Bind the shader program object.
	glUseProgramObjectARB(d_program_object.get()->get_program_resource_handle());
}

void
GPlatesOpenGL::GLBindProgramObjectStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (!d_program_object)
	{
		return;
	}

	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPLATES_OPENGL_BOOL(GLEW_ARB_shader_objects),
			GPLATES_ASSERTION_SOURCE);

	// The default is zero (the fixed-function pipeline).
    glUseProgramObjectARB(0);
}


GPlatesOpenGL::GLBindTextureStateSet::GLBindTextureStateSet(
		const GLCapabilities &capabilities,
		const GLTexture::shared_ptr_to_const_type &texture_object,
		GLenum texture_unit,
		GLenum texture_target) :
	d_texture_object(texture_object),
	d_texture_unit(texture_unit),
	d_texture_target(texture_target)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			texture_unit >= GL_TEXTURE0 &&
					texture_unit < GL_TEXTURE0 + capabilities.texture.gl_max_texture_image_units,
			GPLATES_ASSERTION_SOURCE);
}

GPlatesOpenGL::GLBindTextureStateSet::GLBindTextureStateSet(
		const GLCapabilities &capabilities,
		GLenum texture_unit,
		GLenum texture_target) :
	d_texture_unit(texture_unit),
	d_texture_target(texture_target)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			texture_unit >= GL_TEXTURE0 &&
					texture_unit < GL_TEXTURE0 + capabilities.texture.gl_max_texture_image_units,
			GPLATES_ASSERTION_SOURCE);
}

void
GPlatesOpenGL::GLBindTextureStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &last_applied_state_set,
		GLState &last_applied_state) const
{
	// Note that we're only comparing the applied state (which is the texture object).
	// It's the texture object we're interested in binding - the other parameters are expected
	// to be the same across 'this' and 'last_applied_state_set'.
	//
	// Return early if no state change...
	if (d_texture_object ==
			// Throws exception if downcast fails...
			dynamic_cast<const GLBindTextureStateSet &>(last_applied_state_set).d_texture_object)
	{
		return;
	}

	// Make sure the correct texture unit is currently active.
	set_active_texture(capabilities, d_texture_unit, last_applied_state);

	if (d_texture_object)
	{
		// Bind the texture.
		glBindTexture(d_texture_target, d_texture_object.get()->get_texture_resource_handle());
	}
	else
	{
		// Bind the unnamed texture 0.
		glBindTexture(d_texture_target, 0);
	}
}

void
GPlatesOpenGL::GLBindTextureStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (!d_texture_object)
	{
		return;
	}

	// Make sure the correct texture unit is currently active.
	set_active_texture(capabilities, d_texture_unit, last_applied_state);

	// Bind the texture.
	glBindTexture(d_texture_target, d_texture_object.get()->get_texture_resource_handle());
}

void
GPlatesOpenGL::GLBindTextureStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (!d_texture_object)
	{
		return;
	}

	// Make sure the correct texture unit is currently active.
	set_active_texture(capabilities, d_texture_unit, last_applied_state);

	// Bind the default unnamed texture 0.
	glBindTexture(d_texture_target, 0);
}


void
GPlatesOpenGL::GLBindVertexArrayObjectStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &last_applied_state_set,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (d_resource_handle ==
			// Throws exception if downcast fails...
			dynamic_cast<const GLBindVertexArrayObjectStateSet &>(last_applied_state_set).d_resource_handle)
	{
		return;
	}

#ifdef GL_ARB_vertex_array_object // In case old 'glew.h' header
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPLATES_OPENGL_BOOL(GLEW_ARB_vertex_array_object),
			GPLATES_ASSERTION_SOURCE);

	if (d_resource_handle)
	{
		// Bind the vertex array object.
		glBindVertexArray(d_resource_handle.get());
	}
	else
	{
		// No vertex array object - back to managing individual vertex attribute arrays.
		glBindVertexArray(0);
	}
#else
	throw OpenGLException(GPLATES_EXCEPTION_SOURCE, "Internal Error: GL_ARB_vertex_array_object not supported");
#endif

	// The above binding effectively brings in a bunch of state such as buffer bindings, client
	// enable/disable state, etc, so we mirror this in our shadowed state.
	// That way those individual states won't try to apply themselves thinking the underlying
	// OpenGL state is different than it is.
	//
	// NOTE: It doesn't matter if binding a non-zero vertex array object or zero.
	// The zero object means *no* vertex array object (or the default object) is targeted/accessed.
	// This means reverting to the state that exists before a non-zero object was bound.
 	last_applied_state.copy_vertex_array_state(*d_current_resource_state);
}

void
GPlatesOpenGL::GLBindVertexArrayObjectStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (!d_resource_handle)
	{
		return;
	}

#ifdef GL_ARB_vertex_array_object // In case old 'glew.h' header
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPLATES_OPENGL_BOOL(GLEW_ARB_vertex_array_object),
			GPLATES_ASSERTION_SOURCE);

	// Bind the vertex array object.
	glBindVertexArray(d_resource_handle.get());
#else
	throw OpenGLException(GPLATES_EXCEPTION_SOURCE, "Internal Error: GL_ARB_vertex_array_object not supported");
#endif

	// The above binding effectively brings in a bunch of state such as buffer bindings, client
	// enable/disable state, etc, so we mirror this in our shadowed state.
	// That way those individual states won't try to apply themselves thinking the underlying
	// OpenGL state is different than it is.
	//
	// NOTE: It doesn't matter if binding a non-zero vertex array object or zero.
	// The zero object means *no* vertex array object (or the default object) is targeted/accessed.
	// This means reverting to the state that exists before a non-zero object was bound.
 	last_applied_state.copy_vertex_array_state(*d_current_resource_state);
}

void
GPlatesOpenGL::GLBindVertexArrayObjectStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (!d_resource_handle)
	{
		return;
	}

#ifdef GL_ARB_vertex_array_object // In case old 'glew.h' header
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPLATES_OPENGL_BOOL(GLEW_ARB_vertex_array_object),
			GPLATES_ASSERTION_SOURCE);

	// The default is zero (no vertex array object - back to managing individual vertex attribute arrays).
    glBindVertexArray(0);
#else
	throw OpenGLException(GPLATES_EXCEPTION_SOURCE, "Internal Error: GL_ARB_vertex_array_object not supported");
#endif

	// Mirror the fact that the above bind has just reverted to the current default vertex array state.
 	last_applied_state.copy_vertex_array_state(*d_current_default_state);
}


GPlatesOpenGL::GLBlendEquationStateSet::GLBlendEquationStateSet(
		GLenum mode) :
	d_mode_RGB(mode),
	d_mode_A(mode),
	d_separate_equations(false)
{
	//! The GL_EXT_blend_minmax extension exposes 'glBlendEquationEXT()'.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPLATES_OPENGL_BOOL(GLEW_EXT_blend_minmax),
			GPLATES_ASSERTION_SOURCE);
}


GPlatesOpenGL::GLBlendEquationStateSet::GLBlendEquationStateSet(
		GLenum modeRGB,
		GLenum modeAlpha) :
	d_mode_RGB(modeRGB),
	d_mode_A(modeAlpha),
	d_separate_equations(true)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPLATES_OPENGL_BOOL(GLEW_EXT_blend_equation_separate),
			GPLATES_ASSERTION_SOURCE);
}


void
GPlatesOpenGL::GLBlendEquationStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &last_applied_state_set,
		GLState &last_applied_state) const
{
	// Throws exception if downcast fails...
	const GLBlendEquationStateSet &last_applied = dynamic_cast<const GLBlendEquationStateSet &>(last_applied_state_set);

	// Return early if no state change...
	if (d_mode_RGB == last_applied.d_mode_RGB &&
		d_mode_A == last_applied.d_mode_A)
	{
		return;
	}

	if (d_separate_equations)
	{
		glBlendEquationSeparateEXT(d_mode_RGB, d_mode_A);
	}
	else
	{
		// The RGB and A equations are the same so can use either to specify for RGBA.
		glBlendEquationEXT(d_mode_RGB);
	}
}

void
GPlatesOpenGL::GLBlendEquationStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (d_mode_RGB == GL_FUNC_ADD_EXT &&
		d_mode_A == GL_FUNC_ADD_EXT)
	{
		return;
	}

	if (d_separate_equations)
	{
		glBlendEquationSeparateEXT(d_mode_RGB, d_mode_A);
	}
	else
	{
		// The RGB and A equations are the same so can use either to specify for RGBA.
		glBlendEquationEXT(d_mode_RGB);
	}
}

void
GPlatesOpenGL::GLBlendEquationStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (d_mode_RGB == GL_FUNC_ADD_EXT &&
		d_mode_A == GL_FUNC_ADD_EXT)
	{
		return;
	}

	// Applies to both RGB and A.
	glBlendEquationEXT(GL_FUNC_ADD_EXT);
}


GPlatesOpenGL::GLBlendFuncStateSet::GLBlendFuncStateSet(
		GLenum sfactorRGB,
		GLenum dfactorRGB,
		GLenum sfactorAlpha,
		GLenum dfactorAlpha) :
	d_src_factor_RGB(sfactorRGB),
	d_dst_factor_RGB(dfactorRGB),
	d_src_factor_A(sfactorAlpha),
	d_dst_factor_A(dfactorAlpha),
	d_separate_factors(true)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPLATES_OPENGL_BOOL(GLEW_EXT_blend_func_separate),
			GPLATES_ASSERTION_SOURCE);
}


void
GPlatesOpenGL::GLBlendFuncStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &last_applied_state_set,
		GLState &last_applied_state) const
{
	// Throws exception if downcast fails...
	const GLBlendFuncStateSet &last_applied = dynamic_cast<const GLBlendFuncStateSet &>(last_applied_state_set);

	// Return early if no state change...
	if (d_src_factor_RGB == last_applied.d_src_factor_RGB &&
		d_dst_factor_RGB == last_applied.d_dst_factor_RGB &&
		d_src_factor_A == last_applied.d_src_factor_A &&
		d_dst_factor_A == last_applied.d_dst_factor_A)
	{
		return;
	}

	if (d_separate_factors)
	{
		glBlendFuncSeparateEXT(d_src_factor_RGB, d_dst_factor_RGB, d_src_factor_A, d_dst_factor_A);
	}
	else
	{
		// The RGB and A factors are the same so can use either to specify for RGBA.
		glBlendFunc(d_src_factor_RGB, d_dst_factor_RGB);
	}
}

void
GPlatesOpenGL::GLBlendFuncStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (d_src_factor_RGB == GL_ONE &&
		d_dst_factor_RGB == GL_ZERO &&
		d_src_factor_A == GL_ONE &&
		d_dst_factor_A == GL_ZERO)
	{
		return;
	}

	if (d_separate_factors)
	{
		glBlendFuncSeparateEXT(d_src_factor_RGB, d_dst_factor_RGB, d_src_factor_A, d_dst_factor_A);
	}
	else
	{
		// The RGB and A factors are the same so can use either to specify for RGBA.
		glBlendFunc(d_src_factor_RGB, d_dst_factor_RGB);
	}
}

void
GPlatesOpenGL::GLBlendFuncStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (d_src_factor_RGB == GL_ONE &&
		d_dst_factor_RGB == GL_ZERO &&
		d_src_factor_A == GL_ONE &&
		d_dst_factor_A == GL_ZERO)
	{
		return;
	}

	// Applies to both RGB and A.
	glBlendFunc(GL_ONE, GL_ZERO);
}


void
GPlatesOpenGL::GLClearColorStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &last_applied_state_set,
		GLState &last_applied_state) const
{
	// Throws exception if downcast fails...
	const GLClearColorStateSet &last_applied = dynamic_cast<const GLClearColorStateSet &>(last_applied_state_set);

	// Return early if no state change...
	// Note that these are epsilon comparisons.
	if (d_red == last_applied.d_red &&
		d_green == last_applied.d_green &&
		d_blue == last_applied.d_blue &&
		d_alpha == last_applied.d_alpha)
	{
		return;
	}

	glClearColor(
			static_cast<GLclampf>(d_red.dval()),
			static_cast<GLclampf>(d_green.dval()),
			static_cast<GLclampf>(d_blue.dval()),
			static_cast<GLclampf>(d_alpha.dval()));
}

void
GPlatesOpenGL::GLClearColorStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	// Note that these are epsilon comparisons.
	if (d_red == 0 && d_green == 0 && d_blue == 0 && d_alpha == 0)
	{
		return;
	}

	glClearColor(
			static_cast<GLclampf>(d_red.dval()),
			static_cast<GLclampf>(d_green.dval()),
			static_cast<GLclampf>(d_blue.dval()),
			static_cast<GLclampf>(d_alpha.dval()));
}

void
GPlatesOpenGL::GLClearColorStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	// Note that these are epsilon comparisons.
	if (d_red == 0 && d_green == 0 && d_blue == 0 && d_alpha == 0)
	{
		return;
	}

	glClearColor(0, 0, 0, 0);
}


void
GPlatesOpenGL::GLClearDepthStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &last_applied_state_set,
		GLState &last_applied_state) const
{
	// Throws exception if downcast fails...
	const GLClearDepthStateSet &last_applied = dynamic_cast<const GLClearDepthStateSet &>(last_applied_state_set);

	// Return early if no state change...
	// Note that these are epsilon comparisons.
	if (d_depth == last_applied.d_depth)
	{
		return;
	}

	glClearDepth(static_cast<GLclampd>(d_depth.dval()));
}

void
GPlatesOpenGL::GLClearDepthStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	// Note that these are epsilon comparisons.
	if (d_depth == 1.0)
	{
		return;
	}

	glClearDepth(static_cast<GLclampd>(d_depth.dval()));
}

void
GPlatesOpenGL::GLClearDepthStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	// Note that these are epsilon comparisons.
	if (d_depth == 1.0)
	{
		return;
	}

	glClearDepth(1.0);
}


void
GPlatesOpenGL::GLClearStencilStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &last_applied_state_set,
		GLState &last_applied_state) const
{
	// Throws exception if downcast fails...
	const GLClearStencilStateSet &last_applied = dynamic_cast<const GLClearStencilStateSet &>(last_applied_state_set);

	// Return early if no state change...
	// Note that these are epsilon comparisons.
	if (d_stencil == last_applied.d_stencil)
	{
		return;
	}

	glClearStencil(d_stencil);
}

void
GPlatesOpenGL::GLClearStencilStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	// Note that these are epsilon comparisons.
	if (d_stencil == 0)
	{
		return;
	}

	glClearStencil(d_stencil);
}

void
GPlatesOpenGL::GLClearStencilStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	// Note that these are epsilon comparisons.
	if (d_stencil == 0)
	{
		return;
	}

	glClearStencil(0);
}


GPlatesOpenGL::GLClientActiveTextureStateSet::GLClientActiveTextureStateSet(
		const GLCapabilities &capabilities,
		GLenum client_active_texture) :
	d_client_active_texture(client_active_texture)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			client_active_texture >= GL_TEXTURE0 &&
					client_active_texture < GL_TEXTURE0 + capabilities.texture.gl_max_texture_coords,
			GPLATES_ASSERTION_SOURCE);
}


void
GPlatesOpenGL::GLClientActiveTextureStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &last_applied_state_set,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (d_client_active_texture ==
			// Throws exception if downcast fails...
			dynamic_cast<const GLClientActiveTextureStateSet &>(last_applied_state_set).d_client_active_texture)
	{
		return;
	}

	if (GLEW_ARB_multitexture)
	{
		glClientActiveTextureARB(d_client_active_texture);
	}
	// No GL_ARB_multitexture means we're always using texture unit 0.
}

void
GPlatesOpenGL::GLClientActiveTextureStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (d_client_active_texture == GL_TEXTURE0)
	{
		return;
	}

	if (GLEW_ARB_multitexture)
	{
		glClientActiveTextureARB(d_client_active_texture);
	}
	// No GL_ARB_multitexture means we're always using texture unit 0.
}

void
GPlatesOpenGL::GLClientActiveTextureStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (GL_TEXTURE0 == d_client_active_texture)
	{
		return;
	}

	if (GLEW_ARB_multitexture)
	{
		// Texture unit 0.
		glClientActiveTextureARB(GL_TEXTURE0);
	}
	// No GL_ARB_multitexture means we're always using texture unit 0.
}


void
GPlatesOpenGL::GLColorMaskStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &last_applied_state_set,
		GLState &last_applied_state) const
{
	// Throws exception if downcast fails...
	const GLColorMaskStateSet &last_applied = dynamic_cast<const GLColorMaskStateSet &>(last_applied_state_set);

	// Return early if no state change...
	if (d_red == last_applied.d_red &&
		d_green == last_applied.d_green &&
		d_blue == last_applied.d_blue &&
		d_alpha == last_applied.d_alpha)
	{
		return;
	}

	glColorMask(d_red, d_green, d_blue, d_alpha);
}

void
GPlatesOpenGL::GLColorMaskStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (d_red && d_green && d_blue && d_alpha)
	{
		return;
	}

	glColorMask(d_red, d_green, d_blue, d_alpha);
}

void
GPlatesOpenGL::GLColorMaskStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (d_red && d_green && d_blue && d_alpha)
	{
		return;
	}

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}


void
GPlatesOpenGL::GLColorPointerStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &last_applied_state_set,
		GLState &last_applied_state) const
{
	// Throws exception if downcast fails...
	const GLColorPointerStateSet &last_applied = dynamic_cast<const GLColorPointerStateSet &>(last_applied_state_set);

	// Return early if no state change...
	if (!d_buffer.has_changed_state(last_applied.d_buffer) &&
		d_size == last_applied.d_size &&
		d_type == last_applied.d_type &&
		d_stride == last_applied.d_stride)
	{
		return;
	}

	// Ensure the buffer is bound.
	d_buffer.bind_buffer(last_applied_state);

	// GL_COLOR_ARRAY_BUFFER_BINDING now captures the vertex buffer object binding (if any).
	glColorPointer(d_size, d_type, d_stride, d_buffer.get_buffer_pointer_to_apply());
	d_buffer.applied_buffer_pointer_to_opengl();
}

void
GPlatesOpenGL::GLColorPointerStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (!d_buffer.has_changed_from_default_state() &&
		d_size == 4 &&
		d_type == GL_FLOAT &&
		d_stride == 0)
	{
		return;
	}

	// Ensure the buffer is bound.
	d_buffer.bind_buffer(last_applied_state);

	// GL_COLOR_ARRAY_BUFFER_BINDING now captures the vertex buffer object binding (if any).
	glColorPointer(d_size, d_type, d_stride, d_buffer.get_buffer_pointer_to_apply());
	d_buffer.applied_buffer_pointer_to_opengl();
}

void
GPlatesOpenGL::GLColorPointerStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (!d_buffer.has_changed_to_default_state() &&
		d_size == 4 &&
		d_type == GL_FLOAT &&
		d_stride == 0)
	{
		return;
	}

	// Ensure the buffer is unbound.
	d_buffer.unbind_buffer(last_applied_state);

	// These are the default parameters.
	glColorPointer(4, GL_FLOAT, 0, NULL);
	d_buffer.applied_buffer_pointer_to_opengl();

	// Any GL_COLOR_ARRAY_BUFFER_BINDING binding to a vertex buffer object (if any) is now released.
}


void
GPlatesOpenGL::GLCullFaceStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &last_applied_state_set,
		GLState &last_applied_state) const
{
	// Throws exception if downcast fails...
	const GLCullFaceStateSet &last_applied = dynamic_cast<const GLCullFaceStateSet &>(last_applied_state_set);

	// Return early if no state change...
	if (d_mode == last_applied.d_mode)
	{
		return;
	}

	glCullFace(d_mode);
}

void
GPlatesOpenGL::GLCullFaceStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (d_mode == GL_BACK)
	{
		return;
	}

	glCullFace(d_mode);
}

void
GPlatesOpenGL::GLCullFaceStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (d_mode == GL_BACK)
	{
		return;
	}

	glCullFace(GL_BACK);
}


void
GPlatesOpenGL::GLDepthFuncStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &last_applied_state_set,
		GLState &last_applied_state) const
{
	// Throws exception if downcast fails...
	const GLDepthFuncStateSet &last_applied = dynamic_cast<const GLDepthFuncStateSet &>(last_applied_state_set);

	// Return early if no state change...
	if (d_depth_func == last_applied.d_depth_func)
	{
		return;
	}

	glDepthFunc(d_depth_func);
}

void
GPlatesOpenGL::GLDepthFuncStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (d_depth_func == GL_LESS)
	{
		return;
	}

	glDepthFunc(d_depth_func);
}

void
GPlatesOpenGL::GLDepthFuncStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (d_depth_func == GL_LESS)
	{
		return;
	}

	glDepthFunc(GL_LESS);
}


void
GPlatesOpenGL::GLDepthMaskStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &last_applied_state_set,
		GLState &last_applied_state) const
{
	// Throws exception if downcast fails...
	const GLDepthMaskStateSet &last_applied = dynamic_cast<const GLDepthMaskStateSet &>(last_applied_state_set);

	// Return early if no state change...
	if (d_flag == last_applied.d_flag)
	{
		return;
	}

	glDepthMask(d_flag);
}

void
GPlatesOpenGL::GLDepthMaskStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (d_flag == GL_TRUE)
	{
		return;
	}

	glDepthMask(d_flag);
}

void
GPlatesOpenGL::GLDepthMaskStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (d_flag == GL_TRUE)
	{
		return;
	}

	glDepthMask(GL_TRUE);
}

GPlatesOpenGL::GLDepthRange GPlatesOpenGL::GLDepthRangeStateSet::DEFAULT_DEPTH_RANGE;

GPlatesOpenGL::GLDepthRangeStateSet::GLDepthRangeStateSet(
		const GLCapabilities &capabilities,
		const GLDepthRange &all_depth_ranges) :
	d_depth_ranges(1, all_depth_ranges), // Just store one viewport (no need to duplicate).
	d_all_depth_ranges_are_the_same(true)
{
}

GPlatesOpenGL::GLDepthRangeStateSet::GLDepthRangeStateSet(
		const GLCapabilities &capabilities,
		const depth_range_seq_type &all_depth_ranges) :
	d_depth_ranges(all_depth_ranges),
	d_all_depth_ranges_are_the_same(false)
{
	// The client is required to set all available depth ranges.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			all_depth_ranges.size() == capabilities.viewport.gl_max_viewports,
			GPLATES_ASSERTION_SOURCE);
}

void
GPlatesOpenGL::GLDepthRangeStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &last_applied_state_set,
		GLState &last_applied_state) const
{
	// Throws exception if downcast fails...
	const GLDepthRangeStateSet &last_applied = dynamic_cast<const GLDepthRangeStateSet &>(last_applied_state_set);

	// Return early if no state change...
	// Only comparing depth ranges if both states have one set of depth range parameters for all viewports.
	if (d_all_depth_ranges_are_the_same && last_applied.d_all_depth_ranges_are_the_same)
	{
		if (d_depth_ranges.front() == last_applied.d_depth_ranges.front())
		{
			return;
		}
	}

	apply_state();
}

void
GPlatesOpenGL::GLDepthRangeStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	// Only comparing depth ranges if have one set of depth range parameters for all viewports.
	if (d_all_depth_ranges_are_the_same)
	{
		if (d_depth_ranges.front() == DEFAULT_DEPTH_RANGE)
		{
			return;
		}
	}

	apply_state();
}

void
GPlatesOpenGL::GLDepthRangeStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	// Only comparing depth ranges if have one set of depth range parameters for all viewports.
	if (d_all_depth_ranges_are_the_same)
	{
		if (d_depth_ranges.front() == DEFAULT_DEPTH_RANGE)
		{
			return;
		}
	}

	// Apply the default depth range to all viewports - glDepthRange does this for us.
	glDepthRange(0.0, 1.0);
}

void
GPlatesOpenGL::GLDepthRangeStateSet::apply_state() const
{
	if (d_all_depth_ranges_are_the_same || (d_depth_ranges.size() == 1))
	{
		// All depth_ranges get set with 'glDepthRange'.
		// Also works if the GL_ARB_viewport_array extension is not present.
		const GLDepthRange &depth_range = d_depth_ranges.front();
		glDepthRange(depth_range.get_z_near(), depth_range.get_z_far());

		return;
	}

#ifdef GL_ARB_viewport_array // In case old 'glew.h' header
	if (GLEW_ARB_viewport_array)
	{
		// Put the depth range parameters into one array so can call 'glDepthRangeArrayv' once
		// rather than call 'glDepthRangeIndexed' multiple times.
		std::vector<GLclampd> depth_ranges;
		depth_ranges.reserve(2 * d_depth_ranges.size());

		BOOST_FOREACH(const GLDepthRange &depth_range, d_depth_ranges)
		{
			depth_ranges.push_back(depth_range.get_z_near());
			depth_ranges.push_back(depth_range.get_z_far());
		}

		glDepthRangeArrayv(0, d_depth_ranges.size(), &depth_ranges[0]);

		return;
	}
#endif
}


void
GPlatesOpenGL::GLEnableClientStateStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &last_applied_state_set,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (d_enable ==
			// Throws exception if downcast fails...
			dynamic_cast<const GLEnableClientStateStateSet &>(last_applied_state_set).d_enable)
	{
		return;
	}

	if (d_enable)
	{
		glEnableClientState(d_array);
	}
	else
	{
		glDisableClientState(d_array);
	}
}

void
GPlatesOpenGL::GLEnableClientStateStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (!d_enable)
	{
		return;
	}

	glEnableClientState(d_array);
}

void
GPlatesOpenGL::GLEnableClientStateStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (!d_enable)
	{
		return;
	}

	glDisableClientState(d_array);
}


void
GPlatesOpenGL::GLEnableClientTextureStateStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &last_applied_state_set,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (d_enable ==
			// Throws exception if downcast fails...
			dynamic_cast<const GLEnableClientTextureStateStateSet &>(last_applied_state_set).d_enable)
	{
		return;
	}

	// Make sure the correct texture unit is currently active.
	set_client_active_texture(capabilities, d_texture_unit, last_applied_state);

	if (d_enable)
	{
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	}
	else
	{
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}
}

void
GPlatesOpenGL::GLEnableClientTextureStateStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (!d_enable)
	{
		return;
	}

	// Make sure the correct texture unit is currently active.
	set_client_active_texture(capabilities, d_texture_unit, last_applied_state);

	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
}

void
GPlatesOpenGL::GLEnableClientTextureStateStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (!d_enable)
	{
		return;
	}

	// Make sure the correct texture unit is currently active.
	set_client_active_texture(capabilities, d_texture_unit, last_applied_state);

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}


void
GPlatesOpenGL::GLEnableStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &last_applied_state_set,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (d_enable ==
			// Throws exception if downcast fails...
			dynamic_cast<const GLEnableStateSet &>(last_applied_state_set).d_enable)
	{
		return;
	}

	if (d_enable)
	{
		glEnable(d_cap);
	}
	else
	{
		glDisable(d_cap);
	}
}

void
GPlatesOpenGL::GLEnableStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	const bool enable_default = get_default(d_cap);

	// Return early if no state change...
	if (d_enable == enable_default)
	{
		return;
	}

	if (d_enable)
	{
		glEnable(d_cap);
	}
	else
	{
		glDisable(d_cap);
	}
}

void
GPlatesOpenGL::GLEnableStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	const bool enable_default = get_default(d_cap);

	// Return early if no state change...
	if (d_enable == enable_default)
	{
		return;
	}

	if (enable_default)
	{
		glEnable(d_cap);
	}
	else
	{
		glDisable(d_cap);
	}
}


bool
GPlatesOpenGL::GLEnableStateSet::get_default(
		GLenum cap)
{
	// All capabilities default to GL_FALSE except two.
	return cap == GL_DITHER || cap == GL_MULTISAMPLE;
}


void
GPlatesOpenGL::GLEnableTextureStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &last_applied_state_set,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (d_enable ==
			// Throws exception if downcast fails...
			dynamic_cast<const GLEnableTextureStateSet &>(last_applied_state_set).d_enable)
	{
		return;
	}

	// Make sure the correct texture unit is currently active.
	set_active_texture(capabilities, d_texture_unit, last_applied_state);

	if (d_enable)
	{
		glEnable(d_texture_target);
	}
	else
	{
		glDisable(d_texture_target);
	}
}

void
GPlatesOpenGL::GLEnableTextureStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (!d_enable)
	{
		return;
	}

	// Make sure the correct texture unit is currently active.
	set_active_texture(capabilities, d_texture_unit, last_applied_state);

	glEnable(d_texture_target);
}

void
GPlatesOpenGL::GLEnableTextureStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (!d_enable)
	{
		return;
	}

	// Make sure the correct texture unit is currently active.
	set_active_texture(capabilities, d_texture_unit, last_applied_state);

	glDisable(d_texture_target);
}


void
GPlatesOpenGL::GLEnableVertexAttribArrayStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &last_applied_state_set,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (d_enable ==
			// Throws exception if downcast fails...
			dynamic_cast<const GLEnableVertexAttribArrayStateSet &>(last_applied_state_set).d_enable)
	{
		return;
	}

	if (d_enable)
	{
		glEnableVertexAttribArrayARB(d_attribute_index);
	}
	else
	{
		glDisableVertexAttribArrayARB(d_attribute_index);
	}
}

void
GPlatesOpenGL::GLEnableVertexAttribArrayStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (!d_enable)
	{
		return;
	}

	glEnableVertexAttribArrayARB(d_attribute_index);
}

void
GPlatesOpenGL::GLEnableVertexAttribArrayStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (!d_enable)
	{
		return;
	}

	glDisableVertexAttribArrayARB(d_attribute_index);
}


void
GPlatesOpenGL::GLFrontFaceStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &last_applied_state_set,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	// Throws exception if downcast fails...
	if (d_mode == dynamic_cast<const GLFrontFaceStateSet &>(last_applied_state_set).d_mode)
	{
		return;
	}

	glFrontFace(d_mode);
}

void
GPlatesOpenGL::GLFrontFaceStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (d_mode == GL_CCW)
	{
		return;
	}

	glFrontFace(d_mode);
}

void
GPlatesOpenGL::GLFrontFaceStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (d_mode == GL_CCW)
	{
		return;
	}

	glFrontFace(GL_CCW);
}


void
GPlatesOpenGL::GLHintStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &last_applied_state_set,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	// Throws exception if downcast fails...
	if (d_mode == dynamic_cast<const GLHintStateSet &>(last_applied_state_set).d_mode)
	{
		return;
	}

	glHint(d_target, d_mode);
}

void
GPlatesOpenGL::GLHintStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (d_mode == GL_DONT_CARE)
	{
		return;
	}

	glHint(d_target, d_mode);
}

void
GPlatesOpenGL::GLHintStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (d_mode == GL_DONT_CARE)
	{
		return;
	}

	glHint(d_target, GL_DONT_CARE);
}


void
GPlatesOpenGL::GLLineWidthStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &last_applied_state_set,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	// Throws exception if downcast fails...
	// NOTE: This is an epsilon test.
	if (d_width == dynamic_cast<const GLLineWidthStateSet &>(last_applied_state_set).d_width)
	{
		return;
	}

	glLineWidth(d_width.dval());
}

void
GPlatesOpenGL::GLLineWidthStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	// NOTE: This is an epsilon test.
	if (d_width == 1.0)
	{
		return;
	}

	glLineWidth(d_width.dval());
}

void
GPlatesOpenGL::GLLineWidthStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	// NOTE: This is an epsilon test.
	if (d_width == 1.0)
	{
		return;
	}

	glLineWidth(GLfloat(1));
}


void
GPlatesOpenGL::GLLoadMatrixStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &last_applied_state_set,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (d_matrix == dynamic_cast<const GLLoadMatrixStateSet &>(last_applied_state_set).d_matrix)
	{
		return;
	}

	// Make sure the correct matrix mode is set.
	set_matrix_mode(d_mode, last_applied_state);

	glLoadMatrixd(d_matrix.get_matrix());
}

void
GPlatesOpenGL::GLLoadMatrixStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (d_matrix == GLMatrix::IDENTITY)
	{
		return;
	}

	// Make sure the correct matrix mode is set.
	set_matrix_mode(d_mode, last_applied_state);

	glLoadMatrixd(d_matrix.get_matrix());
}

void
GPlatesOpenGL::GLLoadMatrixStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (d_matrix == GLMatrix::IDENTITY)
	{
		return;
	}

	// Make sure the correct matrix mode is set.
	set_matrix_mode(d_mode, last_applied_state);

	glLoadIdentity();
}


void
GPlatesOpenGL::GLLoadTextureMatrixStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &last_applied_state_set,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (d_matrix == dynamic_cast<const GLLoadTextureMatrixStateSet &>(last_applied_state_set).d_matrix)
	{
		return;
	}

	// Make sure the correct texture unit is currently active.
	set_active_texture(capabilities, d_texture_unit, last_applied_state);

	// Make sure the correct matrix mode is set.
	set_matrix_mode(GL_TEXTURE, last_applied_state);

	glLoadMatrixd(d_matrix.get_matrix());
}

void
GPlatesOpenGL::GLLoadTextureMatrixStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (d_matrix == GLMatrix::IDENTITY)
	{
		return;
	}

	// Make sure the correct texture unit is currently active.
	set_active_texture(capabilities, d_texture_unit, last_applied_state);

	// Make sure the correct matrix mode is set.
	set_matrix_mode(GL_TEXTURE, last_applied_state);

	glLoadMatrixd(d_matrix.get_matrix());
}

void
GPlatesOpenGL::GLLoadTextureMatrixStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (d_matrix == GLMatrix::IDENTITY)
	{
		return;
	}

	// Make sure the correct texture unit is currently active.
	set_active_texture(capabilities, d_texture_unit, last_applied_state);

	// Make sure the correct matrix mode is set.
	set_matrix_mode(GL_TEXTURE, last_applied_state);

	glLoadIdentity();
}


void
GPlatesOpenGL::GLMatrixModeStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &last_applied_state_set,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (d_mode ==
			// Throws exception if downcast fails...
			dynamic_cast<const GLMatrixModeStateSet &>(last_applied_state_set).d_mode)
	{
		return;
	}

	glMatrixMode(d_mode);
}

void
GPlatesOpenGL::GLMatrixModeStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (d_mode == GL_MODELVIEW)
	{
		return;
	}

	glMatrixMode(d_mode);
}

void
GPlatesOpenGL::GLMatrixModeStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (d_mode == GL_MODELVIEW)
	{
		return;
	}

	glMatrixMode(GL_MODELVIEW);
}


void
GPlatesOpenGL::GLNormalPointerStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &last_applied_state_set,
		GLState &last_applied_state) const
{
	// Throws exception if downcast fails...
	const GLNormalPointerStateSet &last_applied = dynamic_cast<const GLNormalPointerStateSet &>(last_applied_state_set);

	// Return early if no state change...
	if (!d_buffer.has_changed_state(last_applied.d_buffer) &&
		d_type == last_applied.d_type &&
		d_stride == last_applied.d_stride)
	{
		return;
	}

	// Ensure the buffer is bound.
	d_buffer.bind_buffer(last_applied_state);

	// GL_NORMAL_ARRAY_BUFFER_BINDING now captures the vertex buffer object binding (if any).
	glNormalPointer(d_type, d_stride, d_buffer.get_buffer_pointer_to_apply());
	d_buffer.applied_buffer_pointer_to_opengl();
}

void
GPlatesOpenGL::GLNormalPointerStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (!d_buffer.has_changed_from_default_state() &&
		d_type == GL_FLOAT &&
		d_stride == 0)
	{
		return;
	}

	// Ensure the buffer is bound.
	d_buffer.bind_buffer(last_applied_state);

	// GL_NORMAL_ARRAY_BUFFER_BINDING now captures the vertex buffer object binding (if any).
	glNormalPointer(d_type, d_stride, d_buffer.get_buffer_pointer_to_apply());
	d_buffer.applied_buffer_pointer_to_opengl();
}

void
GPlatesOpenGL::GLNormalPointerStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (!d_buffer.has_changed_to_default_state() &&
		d_type == GL_FLOAT &&
		d_stride == 0)
	{
		return;
	}

	// Ensure the buffer is unbound.
	d_buffer.unbind_buffer(last_applied_state);

	// These are the default parameters.
	glNormalPointer(GL_FLOAT, 0, NULL);
	d_buffer.applied_buffer_pointer_to_opengl();

	// Any GL_NORMAL_ARRAY_BUFFER_BINDING binding to a vertex buffer object (if any) is now released.
}


void
GPlatesOpenGL::GLPointSizeStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &last_applied_state_set,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	// Throws exception if downcast fails...
	// NOTE: This is an epsilon test.
	if (d_size == dynamic_cast<const GLPointSizeStateSet &>(last_applied_state_set).d_size)
	{
		return;
	}

	glPointSize(d_size.dval());
}

void
GPlatesOpenGL::GLPointSizeStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	// NOTE: This is an epsilon test.
	if (d_size == 1.0)
	{
		return;
	}

	glPointSize(d_size.dval());
}

void
GPlatesOpenGL::GLPointSizeStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	// NOTE: This is an epsilon test.
	if (d_size == 1.0)
	{
		return;
	}

	glPointSize(GLfloat(1));
}


void
GPlatesOpenGL::GLPolygonModeStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &last_applied_state_set,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	// Throws exception if downcast fails...
	if (d_mode == dynamic_cast<const GLPolygonModeStateSet &>(last_applied_state_set).d_mode)
	{
		return;
	}

	glPolygonMode(d_face, d_mode);
}

void
GPlatesOpenGL::GLPolygonModeStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (d_mode == GL_FILL)
	{
		return;
	}

	glPolygonMode(d_face, d_mode);
}

void
GPlatesOpenGL::GLPolygonModeStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (d_mode == GL_FILL)
	{
		return;
	}

	// Note that we only set the state for our 'd_face' which is either front *or* back but not both.
	glPolygonMode(d_face, GL_FILL);
}


void
GPlatesOpenGL::GLPolygonOffsetStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &last_applied_state_set,
		GLState &last_applied_state) const
{
	// Throws exception if downcast fails...
	const GLPolygonOffsetStateSet &last_applied = dynamic_cast<const GLPolygonOffsetStateSet &>(last_applied_state_set);

	// Return early if no state change...
	// NOTE: This is an epsilon test.
	if (d_factor == last_applied.d_factor && d_units == last_applied.d_units)
	{
		return;
	}

	glPolygonOffset(d_factor.dval(), d_units.dval());
}

void
GPlatesOpenGL::GLPolygonOffsetStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	// NOTE: This is an epsilon test.
	if (d_factor == 0 && d_units == 0)
	{
		return;
	}

	glPolygonOffset(d_factor.dval(), d_units.dval());
}

void
GPlatesOpenGL::GLPolygonOffsetStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	// NOTE: This is an epsilon test.
	if (d_factor == 0 && d_units == 0)
	{
		return;
	}

	glPolygonMode(GLfloat(0), GLfloat(0));
}


GPlatesOpenGL::GLScissorStateSet::GLScissorStateSet(
		const GLCapabilities &capabilities,
		const GLViewport &all_scissor_rectangles,
		const GLViewport &default_viewport) :
	d_scissor_rectangles(1, all_scissor_rectangles), // Just store one viewport (no need to duplicate).
	d_all_scissor_rectangles_are_the_same(true),
	d_default_viewport(default_viewport)
{
}

GPlatesOpenGL::GLScissorStateSet::GLScissorStateSet(
		const GLCapabilities &capabilities,
		const scissor_rectangle_seq_type &all_scissor_rectangles,
		const GLViewport &default_viewport) :
	d_scissor_rectangles(all_scissor_rectangles),
	d_all_scissor_rectangles_are_the_same(false),
	d_default_viewport(default_viewport)
{
	// The client is required to set all available scissor rectangles.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			all_scissor_rectangles.size() == capabilities.viewport.gl_max_viewports,
			GPLATES_ASSERTION_SOURCE);
}

void
GPlatesOpenGL::GLScissorStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &last_applied_state_set,
		GLState &last_applied_state) const
{
	// Throws exception if downcast fails...
	const GLScissorStateSet &last_applied = dynamic_cast<const GLScissorStateSet &>(last_applied_state_set);

	// Return early if no state change...
	// Only comparing scissor rectangles if both states have one set of scissor parameters for all rectangles.
	if (d_all_scissor_rectangles_are_the_same && last_applied.d_all_scissor_rectangles_are_the_same)
	{
		if (d_scissor_rectangles.front() == last_applied.d_scissor_rectangles.front())
		{
			return;
		}
	}

	apply_state();
}

void
GPlatesOpenGL::GLScissorStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	// Only comparing scissor rectangles if there's only one set of scissor parameters for all rectangles.
	if (d_all_scissor_rectangles_are_the_same)
	{
		if (d_scissor_rectangles.front() == d_default_viewport)
		{
			return;
		}
	}

	apply_state();
}

void
GPlatesOpenGL::GLScissorStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	// Only comparing scissor rectangles if there's only one set of scissor parameters for all rectangles.
	if (d_all_scissor_rectangles_are_the_same)
	{
		if (d_scissor_rectangles.front() == d_default_viewport)
		{
			return;
		}
	}

	// Apply the default viewport to all scissor rectangles - glScissor does this for us.
	glScissor(d_default_viewport.x(), d_default_viewport.y(), d_default_viewport.width(), d_default_viewport.height());
}

const GPlatesOpenGL::GLViewport &
GPlatesOpenGL::GLScissorStateSet::get_scissor(
		const GLCapabilities &capabilities,
		unsigned int viewport_index) const
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			viewport_index < capabilities.viewport.gl_max_viewports,
			GPLATES_ASSERTION_SOURCE);

	return d_scissor_rectangles[viewport_index];
}

void
GPlatesOpenGL::GLScissorStateSet::apply_state() const
{
	if (d_all_scissor_rectangles_are_the_same || (d_scissor_rectangles.size() == 1))
	{
		// All scissor rectangles get set with 'glScissor'.
		// Also works if the GL_ARB_viewport_array extension is not present.
		const GLViewport &scissor_rectangle = d_scissor_rectangles.front();
		glScissor(scissor_rectangle.x(), scissor_rectangle.y(), scissor_rectangle.width(), scissor_rectangle.height());

		return;
	}

#ifdef GL_ARB_viewport_array // In case old 'glew.h' header
	if (GLEW_ARB_viewport_array)
	{
		// Put the scissor rectangle parameters into one array so can call 'glScissorArrayv' once
		// rather than call 'glScissorIndexed' multiple times.
		std::vector<GLint> scissor_rectangles;
		scissor_rectangles.reserve(4 * d_scissor_rectangles.size());

		BOOST_FOREACH(const GLViewport &scissor_rectangle, d_scissor_rectangles)
		{
			scissor_rectangles.push_back(scissor_rectangle.x());
			scissor_rectangles.push_back(scissor_rectangle.y());
			scissor_rectangles.push_back(scissor_rectangle.width());
			scissor_rectangles.push_back(scissor_rectangle.height());
		}

		glScissorArrayv(0, d_scissor_rectangles.size(), &scissor_rectangles[0]);

		return;
	}
#endif
}


void
GPlatesOpenGL::GLStencilMaskStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &last_applied_state_set,
		GLState &last_applied_state) const
{
	// Throws exception if downcast fails...
	const GLStencilMaskStateSet &last_applied = dynamic_cast<const GLStencilMaskStateSet &>(last_applied_state_set);

	// Return early if no state change...
	if (d_stencil == last_applied.d_stencil)
	{
		return;
	}

	glStencilMask(d_stencil);
}

void
GPlatesOpenGL::GLStencilMaskStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (d_stencil == ~GLuint(0)/*all ones*/)
	{
		return;
	}

	glStencilMask(d_stencil);
}

void
GPlatesOpenGL::GLStencilMaskStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (d_stencil == ~GLuint(0)/*all ones*/)
	{
		return;
	}

	glStencilMask(~GLuint(0)/*all ones*/);
}


void
GPlatesOpenGL::GLTexCoordPointerStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &last_applied_state_set,
		GLState &last_applied_state) const
{
	// Throws exception if downcast fails...
	const GLTexCoordPointerStateSet &last_applied = dynamic_cast<const GLTexCoordPointerStateSet &>(last_applied_state_set);

	// Return early if no state change...
	// No need to compare texture unit - different units go into different state-set slots.
	if (!d_buffer.has_changed_state(last_applied.d_buffer) &&
		d_size == last_applied.d_size &&
		d_type == last_applied.d_type &&
		d_stride == last_applied.d_stride)
	{
		return;
	}

	// Ensure the buffer is bound.
	d_buffer.bind_buffer(last_applied_state);

	// Make sure the correct texture unit is currently active.
	set_client_active_texture(capabilities, d_texture_unit, last_applied_state);

	// GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING now captures the vertex buffer object binding (if any).
	glTexCoordPointer(d_size, d_type, d_stride, d_buffer.get_buffer_pointer_to_apply());
	d_buffer.applied_buffer_pointer_to_opengl();
}

void
GPlatesOpenGL::GLTexCoordPointerStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	// No need to compare texture unit - different units go into different state-set slots.
	if (!d_buffer.has_changed_from_default_state() &&
		d_size == 4 &&
		d_type == GL_FLOAT &&
		d_stride == 0)
	{
		return;
	}

	// Ensure the buffer is bound.
	d_buffer.bind_buffer(last_applied_state);

	// Make sure the correct texture unit is currently active.
	set_client_active_texture(capabilities, d_texture_unit, last_applied_state);

	// GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING now captures the vertex buffer object binding (if any).
	glTexCoordPointer(d_size, d_type, d_stride, d_buffer.get_buffer_pointer_to_apply());
	d_buffer.applied_buffer_pointer_to_opengl();
}

void
GPlatesOpenGL::GLTexCoordPointerStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	// No need to compare texture unit - different units go into different state-set slots.
	if (!d_buffer.has_changed_to_default_state() &&
		d_size == 4 &&
		d_type == GL_FLOAT &&
		d_stride == 0)
	{
		return;
	}

	// Ensure the buffer is unbound.
	d_buffer.unbind_buffer(last_applied_state);

	// Make sure the correct texture unit is currently active.
	set_client_active_texture(capabilities, d_texture_unit, last_applied_state);

	// These are the default parameters.
	glTexCoordPointer(4, GL_FLOAT, 0, NULL);
	d_buffer.applied_buffer_pointer_to_opengl();

	// Any GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING binding to a vertex buffer object (if any) is now released.
}


const GPlatesOpenGL::GLTexGenStateSet::param_type GPlatesOpenGL::GLTexGenStateSet::DEFAULT_GEN_MODE = GL_EYE_LINEAR;
const GPlatesOpenGL::GLTexGenStateSet::param_type GPlatesOpenGL::GLTexGenStateSet::DEFAULT_S_PLANE =
		GPlatesOpenGL::GLTexGenStateSet::initialise_plane(1, 0, 0, 0);
const GPlatesOpenGL::GLTexGenStateSet::param_type GPlatesOpenGL::GLTexGenStateSet::DEFAULT_T_PLANE =
		GPlatesOpenGL::GLTexGenStateSet::initialise_plane(0, 1, 0, 0);
const GPlatesOpenGL::GLTexGenStateSet::param_type GPlatesOpenGL::GLTexGenStateSet::DEFAULT_R_AND_Q_PLANE =
		GPlatesOpenGL::GLTexGenStateSet::initialise_plane(0, 0, 0, 0);

GPlatesOpenGL::GLTexGenStateSet::param_type
GPlatesOpenGL::GLTexGenStateSet::initialise_plane(
		const GLdouble &x,
		const GLdouble &y,
		const GLdouble &z,
		const GLdouble &w)
{
	std::vector<GLdouble> plane;

	plane.push_back(x);
	plane.push_back(y);
	plane.push_back(z);
	plane.push_back(w);

	return plane;
}

void
GPlatesOpenGL::GLTexGenStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &last_applied_state_set,
		GLState &last_applied_state) const
{
	// Throws exception if downcast fails...
	const GLTexGenStateSet &last_applied = dynamic_cast<const GLTexGenStateSet &>(last_applied_state_set);

	// Return early if no state change...
	// Determine if the texture coordinate generation state is the same or not.
	if (boost::apply_visitor(EqualityVisitor(), d_param, last_applied.d_param))
	{
		return;
	}

	// Make sure the correct texture unit is currently active.
	set_active_texture(capabilities, d_texture_unit, last_applied_state);

	// Set the texture coordinate generation state depending on the type of parameter being set.
	boost::apply_visitor(TexGenVisitor(d_coord, d_pname), d_param);
}

void
GPlatesOpenGL::GLTexGenStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	const param_type default_param = get_default_param();

	// Return early if no state change...
	// Determine if the texture coordinate generation state is the same or not.
	if (boost::apply_visitor(EqualityVisitor(), d_param, default_param))
	{
		return;
	}

	// Make sure the correct texture unit is currently active.
	set_active_texture(capabilities, d_texture_unit, last_applied_state);

	// Set the texture coordinate generation state depending on the type of parameter being set.
	boost::apply_visitor(TexGenVisitor(d_coord, d_pname), d_param);
}

void
GPlatesOpenGL::GLTexGenStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	const param_type default_param = get_default_param();

	// Return early if no state change...
	// Determine if the texture coordinate generation state is the same or not.
	if (boost::apply_visitor(EqualityVisitor(), d_param, default_param))
	{
		return;
	}

	// Make sure the correct texture unit is currently active.
	set_active_texture(capabilities, d_texture_unit, last_applied_state);

	// Set the texture coordinate generation state depending on the type of parameter being set.
	boost::apply_visitor(TexGenVisitor(d_coord, d_pname), default_param);
}

GPlatesOpenGL::GLTexGenStateSet::param_type
GPlatesOpenGL::GLTexGenStateSet::get_default_param() const
{
	switch (d_pname)
	{
	case GL_TEXTURE_GEN_MODE:
		return DEFAULT_GEN_MODE;

	case GL_OBJECT_PLANE:
	case GL_EYE_PLANE:
		switch (d_coord)
		{
		case GL_S:
			return DEFAULT_S_PLANE;
		case GL_T:
			return DEFAULT_T_PLANE;
		case GL_R:
		case GL_Q:
			return DEFAULT_R_AND_Q_PLANE;
		default:
			// Fall through to the abort.
			break;
		}
		// Fall through to the abort.
		break;

	default:
		// Fall through to the abort.
		break;
	}

	// Unsupported coord or pname.
	GPlatesGlobal::Abort(GPLATES_EXCEPTION_SOURCE);

	// To keep compiler happy - shouldn't be able to get here.
	return DEFAULT_GEN_MODE;
}


void
GPlatesOpenGL::GLTexEnvStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &last_applied_state_set,
		GLState &last_applied_state) const
{
	// Throws exception if downcast fails...
	const GLTexEnvStateSet &last_applied = dynamic_cast<const GLTexEnvStateSet &>(last_applied_state_set);

	// Return early if no state change...
	// Determine if the texture environment state is the same or not.
	if (boost::apply_visitor(EqualityVisitor(), d_param, last_applied.d_param))
	{
		return;
	}

	// Make sure the correct texture unit is currently active.
	set_active_texture(capabilities, d_texture_unit, last_applied_state);

	// Set the texture environment state depending on the type of parameter being set.
	boost::apply_visitor(TexEnvVisitor(d_target, d_pname), d_param);
}

void
GPlatesOpenGL::GLTexEnvStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	const param_type default_param = get_default_param();

	// Return early if no state change...
	// Determine if the texture environment state is the same or not.
	if (boost::apply_visitor(EqualityVisitor(), d_param, default_param))
	{
		return;
	}

	// Make sure the correct texture unit is currently active.
	set_active_texture(capabilities, d_texture_unit, last_applied_state);

	// Set the texture environment state depending on the type of parameter being set.
	boost::apply_visitor(TexEnvVisitor(d_target, d_pname), d_param);
}

void
GPlatesOpenGL::GLTexEnvStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	const param_type default_param = get_default_param();

	// Return early if no state change...
	// Determine if the texture environment state is the same or not.
	if (boost::apply_visitor(EqualityVisitor(), d_param, default_param))
	{
		return;
	}

	// Make sure the correct texture unit is currently active.
	set_active_texture(capabilities, d_texture_unit, last_applied_state);

	// Set the texture environment state depending on the type of parameter being set.
	boost::apply_visitor(TexEnvVisitor(d_target, d_pname), default_param);
}

GPlatesOpenGL::GLTexEnvStateSet::param_type
GPlatesOpenGL::GLTexEnvStateSet::get_default_param() const
{
	switch (d_target)
	{
	case GL_TEXTURE_ENV:
		switch (d_pname)
		{
		case GL_TEXTURE_ENV_MODE:
			return GL_MODULATE;
		case GL_TEXTURE_ENV_COLOR:
			{
				static const param_type DEFAULT_TEX_ENV_COLOR(create_4_vector<GLfloat>(0, 0, 0, 0));
				return DEFAULT_TEX_ENV_COLOR;
			}
		case GL_COMBINE_RGB_ARB:
			return GL_MODULATE;
		case GL_COMBINE_ALPHA_ARB:
			return GL_MODULATE;
		case GL_SOURCE0_RGB_ARB:
			return GL_TEXTURE;
		case GL_SOURCE0_ALPHA_ARB:
			return GL_TEXTURE;
		case GL_SOURCE1_RGB_ARB:
			return GL_PREVIOUS_ARB;
		case GL_SOURCE1_ALPHA_ARB:
			return GL_PREVIOUS_ARB;
		case GL_SOURCE2_RGB_ARB:
			return GL_CONSTANT_ARB;
		case GL_SOURCE2_ALPHA_ARB:
			return GL_CONSTANT_ARB;
		case GL_OPERAND0_RGB_ARB:
			return GL_SRC_COLOR;
		case GL_OPERAND0_ALPHA_ARB:
			return GL_SRC_ALPHA;
		case GL_OPERAND1_RGB_ARB:
			return GL_SRC_COLOR;
		case GL_OPERAND1_ALPHA_ARB:
			return GL_SRC_ALPHA;
		case GL_OPERAND2_RGB_ARB:
			return GL_SRC_ALPHA;
		case GL_OPERAND2_ALPHA_ARB:
			return GL_SRC_ALPHA;
		case GL_RGB_SCALE_ARB:
			return GLfloat(1.0);
		case GL_ALPHA_SCALE:
			return GLfloat(1.0);

		default:
			// Fall through to the abort.
			break;
		}
		break;

	default:
		// Fall through to the abort.
		break;
	}

	// Unsupported target or pname.
	GPlatesGlobal::Abort(GPLATES_EXCEPTION_SOURCE);

	// To keep compiler happy - shouldn't be able to get here.
	return GLint(0);
}


GPlatesOpenGL::GLVertexAttribPointerStateSet::GLVertexAttribPointerStateSet(
		GLuint attribute_index,
		VertexAttribAPIType vertex_attrib_api,
		GLint size,
		GLenum type,
		boost::optional<GLboolean> normalized,
		GLsizei stride,
		GLint offset,
		const GLBufferObject::shared_ptr_to_const_type &buffer_object) :
	d_buffer(offset, buffer_object),
	d_attribute_index(attribute_index),
	d_vertex_attrib_api(vertex_attrib_api),
	d_size(size),
	d_type(type),
	d_normalized(normalized),
	d_stride(stride)
{
	// The relevant extension(s) must be supported...
	switch (d_vertex_attrib_api)
	{
	case VERTEX_ATTRIB_POINTER:
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				GPLATES_OPENGL_BOOL(GLEW_ARB_vertex_shader) && normalized,
				GPLATES_ASSERTION_SOURCE);
		break;

	case VERTEX_ATTRIB_I_POINTER:
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				GLEW_ARB_vertex_shader &&
#ifdef GL_EXT_gpu_shader4 // In case old 'glew.h' (since extension added relatively recently).
					GLEW_EXT_gpu_shader4 &&
#else
					false &&
#endif
					!normalized,
				GPLATES_ASSERTION_SOURCE);
		break;

	case VERTEX_ATTRIB_L_POINTER:
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				GLEW_ARB_vertex_shader &&
#ifdef GL_ARB_vertex_attrib_64bit // In case old 'glew.h' (since extension added relatively recently).
					GLEW_ARB_vertex_attrib_64bit &&
#else
					false &&
#endif
					!normalized,
				GPLATES_ASSERTION_SOURCE);
		break;

	default:
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
		break;
	}
}

GPlatesOpenGL::GLVertexAttribPointerStateSet::GLVertexAttribPointerStateSet(
		GLuint attribute_index,
		VertexAttribAPIType vertex_attrib_api,
		GLint size,
		GLenum type,
		boost::optional<GLboolean> normalized,
		GLsizei stride,
		GLint offset,
		const GLBufferImpl::shared_ptr_to_const_type &buffer_impl) :
	d_buffer(offset, buffer_impl),
	d_attribute_index(attribute_index),
	d_vertex_attrib_api(vertex_attrib_api),
	d_size(size),
	d_type(type),
	d_normalized(normalized),
	d_stride(stride)
{
	// The relevant extension(s) must be supported...
	switch (d_vertex_attrib_api)
	{
	case VERTEX_ATTRIB_POINTER:
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				GPLATES_OPENGL_BOOL(GLEW_ARB_vertex_shader) && normalized,
				GPLATES_ASSERTION_SOURCE);
		break;

	case VERTEX_ATTRIB_I_POINTER:
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				GLEW_ARB_vertex_shader &&
#ifdef GL_EXT_gpu_shader4 // In case old 'glew.h' (since extension added relatively recently).
					GLEW_EXT_gpu_shader4 &&
#else
					false &&
#endif
					!normalized,
				GPLATES_ASSERTION_SOURCE);
		break;

	case VERTEX_ATTRIB_L_POINTER:
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				GLEW_ARB_vertex_shader &&
#ifdef GL_ARB_vertex_attrib_64bit // In case old 'glew.h' (since extension added relatively recently).
					GLEW_ARB_vertex_attrib_64bit &&
#else
					false &&
#endif
					!normalized,
				GPLATES_ASSERTION_SOURCE);
		break;

	default:
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
		break;
	}
}


void
GPlatesOpenGL::GLVertexAttribPointerStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &last_applied_state_set,
		GLState &last_applied_state) const
{
	// Throws exception if downcast fails...
	const GLVertexAttribPointerStateSet &last_applied = dynamic_cast<const GLVertexAttribPointerStateSet &>(last_applied_state_set);

	// Return early if no state change...
	// No need to compare attribute index - different indices go into different state-set slots.
	if (!d_buffer.has_changed_state(last_applied.d_buffer) &&
		d_vertex_attrib_api == last_applied.d_vertex_attrib_api &&
		d_size == last_applied.d_size &&
		d_type == last_applied.d_type &&
		d_normalized == last_applied.d_normalized &&
		d_stride == last_applied.d_stride)
	{
		return;
	}

	// Ensure the buffer is bound.
	d_buffer.bind_buffer(last_applied_state);

	// GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING now captures the vertex buffer object binding (if any).
	switch (d_vertex_attrib_api)
	{
	case VERTEX_ATTRIB_POINTER:
		glVertexAttribPointerARB(
				d_attribute_index, d_size, d_type, d_normalized.get(), d_stride, d_buffer.get_buffer_pointer_to_apply());
		break;
	case VERTEX_ATTRIB_I_POINTER:
#ifdef GL_EXT_gpu_shader4 // In case old 'glew.h' (since extension added relatively recently).
		glVertexAttribIPointerEXT(
				d_attribute_index, d_size, d_type, d_stride, d_buffer.get_buffer_pointer_to_apply());
#endif
		break;
	case VERTEX_ATTRIB_L_POINTER:
#ifdef GL_ARB_vertex_attrib_64bit // In case old 'glew.h' (since extension added relatively recently).
		glVertexAttribLPointer(
				d_attribute_index, d_size, d_type, d_stride, d_buffer.get_buffer_pointer_to_apply());
#endif
		break;
	default:
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
		break;
	}
	d_buffer.applied_buffer_pointer_to_opengl();
}

void
GPlatesOpenGL::GLVertexAttribPointerStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	// Note that for the default state we arbitrarily chose the 'glVertexAttribPointer' API.
	// No need to compare attribute index - different indices go into different state-set slots.
	if (!d_buffer.has_changed_from_default_state() &&
		d_vertex_attrib_api == VERTEX_ATTRIB_POINTER &&
		d_size == 4 &&
		d_type == GL_FLOAT &&
		d_normalized == GL_FALSE &&
		d_stride == 0)
	{
		return;
	}

	// Ensure the buffer is bound.
	d_buffer.bind_buffer(last_applied_state);

	// GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING now captures the vertex buffer object binding (if any).
	switch (d_vertex_attrib_api)
	{
	case VERTEX_ATTRIB_POINTER:
		glVertexAttribPointerARB(
				d_attribute_index, d_size, d_type, d_normalized.get(), d_stride, d_buffer.get_buffer_pointer_to_apply());
		break;
	case VERTEX_ATTRIB_I_POINTER:
#ifdef GL_EXT_gpu_shader4 // In case old 'glew.h' (since extension added relatively recently).
		glVertexAttribIPointerEXT(
				d_attribute_index, d_size, d_type, d_stride, d_buffer.get_buffer_pointer_to_apply());
#endif
		break;
	case VERTEX_ATTRIB_L_POINTER:
#ifdef GL_ARB_vertex_attrib_64bit // In case old 'glew.h' (since extension added relatively recently).
		glVertexAttribLPointer(
				d_attribute_index, d_size, d_type, d_stride, d_buffer.get_buffer_pointer_to_apply());
#endif
		break;
	default:
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
		break;
	}
	d_buffer.applied_buffer_pointer_to_opengl();
}

void
GPlatesOpenGL::GLVertexAttribPointerStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	// Note that for the default state we arbitrarily chose the 'glVertexAttribPointer' API.
	// No need to compare attribute index - different indices go into different state-set slots.
	if (!d_buffer.has_changed_to_default_state() &&
		d_vertex_attrib_api == VERTEX_ATTRIB_POINTER &&
		d_size == 4 &&
		d_type == GL_FLOAT &&
		d_normalized == GL_FALSE &&
		d_stride == 0)
	{
		return;
	}

	// Ensure the buffer is unbound.
	d_buffer.unbind_buffer(last_applied_state);

	// These are the default parameters.
	// Note that for the default state we arbitrarily chose the 'glVertexAttribPointer' API.
	glVertexAttribPointerARB(d_attribute_index, 4, GL_FLOAT, GL_FALSE, 0, NULL);
	d_buffer.applied_buffer_pointer_to_opengl();

	// Any GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING binding to a vertex buffer object (if any) is now released.
}


void
GPlatesOpenGL::GLVertexPointerStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &last_applied_state_set,
		GLState &last_applied_state) const
{
	// Throws exception if downcast fails...
	const GLVertexPointerStateSet &last_applied = dynamic_cast<const GLVertexPointerStateSet &>(last_applied_state_set);

	// Return early if no state change...
	if (!d_buffer.has_changed_state(last_applied.d_buffer) &&
		d_size == last_applied.d_size &&
		d_type == last_applied.d_type &&
		d_stride == last_applied.d_stride)
	{
		return;
	}

	// Ensure the buffer is bound.
	d_buffer.bind_buffer(last_applied_state);

	// GL_VERTEX_ARRAY_BUFFER_BINDING now captures the vertex buffer object binding (if any).
	glVertexPointer(d_size, d_type, d_stride, d_buffer.get_buffer_pointer_to_apply());
	d_buffer.applied_buffer_pointer_to_opengl();
}

void
GPlatesOpenGL::GLVertexPointerStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (!d_buffer.has_changed_from_default_state() &&
		d_size == 4 &&
		d_type == GL_FLOAT &&
		d_stride == 0)
	{
		return;
	}

	// Ensure the buffer is bound.
	d_buffer.bind_buffer(last_applied_state);

	// GL_VERTEX_ARRAY_BUFFER_BINDING now captures the vertex buffer object binding (if any).
	glVertexPointer(d_size, d_type, d_stride, d_buffer.get_buffer_pointer_to_apply());
	d_buffer.applied_buffer_pointer_to_opengl();
}

void
GPlatesOpenGL::GLVertexPointerStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	if (!d_buffer.has_changed_to_default_state() &&
		d_size == 4 &&
		d_type == GL_FLOAT &&
		d_stride == 0)
	{
		return;
	}

	// Ensure the buffer is unbound.
	d_buffer.unbind_buffer(last_applied_state);

	// These are the default parameters.
	glVertexPointer(4, GL_FLOAT, 0, NULL);
	d_buffer.applied_buffer_pointer_to_opengl();

	// Any GL_VERTEX_ARRAY_BUFFER_BINDING binding to a vertex buffer object (if any) is now released.
}


GPlatesOpenGL::GLViewportStateSet::GLViewportStateSet(
		const GLCapabilities &capabilities,
		const GLViewport &all_viewports,
		const GLViewport &default_viewport) :
	d_viewports(1, all_viewports), // Just store one viewport (no need to duplicate).
	d_all_viewports_are_the_same(true),
	d_default_viewport(default_viewport)
{
}

GPlatesOpenGL::GLViewportStateSet::GLViewportStateSet(
		const GLCapabilities &capabilities,
		const viewport_seq_type &all_viewports,
		const GLViewport &default_viewport) :
	d_viewports(all_viewports),
	d_all_viewports_are_the_same(false),
	d_default_viewport(default_viewport)
{
	// The client is required to set all available viewports.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			all_viewports.size() == capabilities.viewport.gl_max_viewports,
			GPLATES_ASSERTION_SOURCE);
}

void
GPlatesOpenGL::GLViewportStateSet::apply_state(
		const GLCapabilities &capabilities,
		const GLStateSet &last_applied_state_set,
		GLState &last_applied_state) const
{
	// Throws exception if downcast fails...
	const GLViewportStateSet &last_applied = dynamic_cast<const GLViewportStateSet &>(last_applied_state_set);

	// Return early if no state change...
	// Only comparing viewports if both states have one set of viewport parameters for all viewports.
	if (d_all_viewports_are_the_same && last_applied.d_all_viewports_are_the_same)
	{
		if (d_viewports.front() == last_applied.d_viewports.front())
		{
			return;
		}
	}

	apply_state();
}

void
GPlatesOpenGL::GLViewportStateSet::apply_from_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	// Only comparing viewports if there's only one set of viewport parameters for all rectangles.
	if (d_all_viewports_are_the_same)
	{
		if (d_viewports.front() == d_default_viewport)
		{
			return;
		}
	}

	apply_state();
}

void
GPlatesOpenGL::GLViewportStateSet::apply_to_default_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	// Return early if no state change...
	// Only comparing viewports if there's only one set of viewport parameters for all rectangles.
	if (d_all_viewports_are_the_same)
	{
		if (d_viewports.front() == d_default_viewport)
		{
			return;
		}
	}

	// Apply the default viewport to all viewports - glViewport does this for us.
	glViewport(d_default_viewport.x(), d_default_viewport.y(), d_default_viewport.width(), d_default_viewport.height());
}

void
GPlatesOpenGL::GLViewportStateSet::apply_state() const
{
	if (d_all_viewports_are_the_same || (d_viewports.size() == 1))
	{
		// All viewports get set with 'glViewport'.
		// Also works if the GL_ARB_viewport_array extension is not present.
		const GLViewport &viewport = d_viewports.front();
		glViewport(viewport.x(), viewport.y(), viewport.width(), viewport.height());

		return;
	}

#ifdef GL_ARB_viewport_array // In case old 'glew.h' header
	if (GLEW_ARB_viewport_array)
	{
		// Put the viewport parameters into one array so can call 'glViewportArrayv' once
		// rather than call 'glViewportIndexedf' multiple times.
		//
		// NOTE: the GL_ARB_viewport_array extension supports floating-point coordinates (instead of integer).
		std::vector<GLfloat> viewports;
		viewports.reserve(4 * d_viewports.size());

		BOOST_FOREACH(const GLViewport &viewport, d_viewports)
		{
			viewports.push_back(viewport.x());
			viewports.push_back(viewport.y());
			viewports.push_back(viewport.width());
			viewports.push_back(viewport.height());
		}

		glViewportArrayv(0, d_viewports.size(), &viewports[0]);

		return;
	}
#endif
}

const GPlatesOpenGL::GLViewport &
GPlatesOpenGL::GLViewportStateSet::get_viewport(
		const GLCapabilities &capabilities,
		unsigned int viewport_index) const
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			viewport_index < capabilities.viewport.gl_max_viewports,
			GPLATES_ASSERTION_SOURCE);

	return d_viewports[viewport_index];
}

ENABLE_GCC_WARNING("-Wold-style-cast")
