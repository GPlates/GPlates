
/**
 * Copyright (C) 2022 The University of Sydney, Australia
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

#ifndef GPLATES_OPENGL_OPENGLFUNCTIONS_H
#define GPLATES_OPENGL_OPENGLFUNCTIONS_H

#include <qopengl.h>
#include <QOpenGLFunctions_4_3_Core>
#include <QOpenGLFunctions_4_2_Core>
#include <QOpenGLFunctions_4_1_Core>
#include <QOpenGLFunctions_4_0_Core>
#include <QOpenGLFunctions_3_3_Core>

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	/**
	 * OpenGL functions for 3.3 core (minimum) up to 4.3 core (optional).
	 *
	 * This class wraps the Qt versioned OpenGL function classes (like 'QOpenGLFunctions_3_3_Core')
	 * to provide a single interface for all OpenGL core profile versions 3.3 and above, rather than
	 * having a separate class for each OpenGL version. For example, the caller will use OpenGL 3.3 core
	 * functionality (minimum requirement) but opt in for OpengGL 4.x functionality where it's available.
	 */
	class OpenGLFunctions :
			public GPlatesUtils::ReferenceCount<OpenGLFunctions>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a OpenGLFunctions.
		typedef GPlatesUtils::non_null_intrusive_ptr<OpenGLFunctions> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a OpenGLFunctions.
		typedef GPlatesUtils::non_null_intrusive_ptr<const OpenGLFunctions> non_null_ptr_to_const_type;


		//! Create OpenGL function for version 3.3 core.
		static
		non_null_ptr_type
		create(
				QOpenGLFunctions_3_3_Core *version_functions);

		//! Create OpenGL function for version 4.0 core.
		static
		non_null_ptr_type
		create(
				QOpenGLFunctions_4_0_Core *version_functions);

		//! Create OpenGL function for version 4.1 core.
		static
		non_null_ptr_type
		create(
				QOpenGLFunctions_4_1_Core *version_functions);

		//! Create OpenGL function for version 4.2 core.
		static
		non_null_ptr_type
		create(
				QOpenGLFunctions_4_2_Core *version_functions);

		//! Create OpenGL function for version 4.3 core.
		static
		non_null_ptr_type
		create(
				QOpenGLFunctions_4_3_Core *version_functions);


		virtual
		~OpenGLFunctions()
		{  }


		//! Version 3.3 core is always supported (it's our minimum requirement).
		bool
		supports_3_3_core() const
		{
			return true;
		}

		//! Returns true if we have OpenGL 4.0 or greater.
		bool
		supports_4_0_core() const
		{
			return d_major_version == 4 && d_minor_version >= 0;
		}

		//! Returns true if we have OpenGL 4.1 or greater.
		bool
		supports_4_1_core() const
		{
			return d_major_version == 4 && d_minor_version >= 1;
		}

		//! Returns true if we have OpenGL 4.2 or greater.
		bool
		supports_4_2_core() const
		{
			return d_major_version == 4 && d_minor_version >= 2;
		}

		//! Returns true if we have OpenGL 4.3 or greater.
		bool
		supports_4_3_core() const
		{
			return d_major_version == 4 && d_minor_version >= 3;
		}


		//
		// OpenGL 1.0 - 3.3 core.
		//
		virtual void glActiveTexture(GLenum texture) { throw_if_not_overridden("glActiveTexture", 3, 3); }
		virtual void glAttachShader(GLuint program, GLuint shader) { throw_if_not_overridden("glAttachShader", 3, 3); }
		virtual void glBindBuffer(GLenum target, GLuint buffer) { throw_if_not_overridden("glBindBuffer", 3, 3); }
		virtual void glBindBufferBase(GLenum target, GLuint index, GLuint buffer) { throw_if_not_overridden("glBindBufferBase", 3, 3); }
		virtual void glBindBufferRange(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size) { throw_if_not_overridden("glBindBufferRange", 3, 3); }
		virtual void glBindFramebuffer(GLenum target, GLuint framebuffer) { throw_if_not_overridden("glBindFramebuffer", 3, 3); }
		virtual void glBindRenderbuffer(GLenum target, GLuint renderbuffer) { throw_if_not_overridden("glBindRenderbuffer", 3, 3); }
		virtual void glBindSampler(GLuint unit, GLuint sampler) { throw_if_not_overridden("glBindSampler", 3, 3); }
		virtual void glBindTexture(GLenum target, GLuint texture) { throw_if_not_overridden("glBindTexture", 3, 3); }
		virtual void glBindVertexArray(GLuint array) { throw_if_not_overridden("glBindVertexArray", 3, 3); }
		virtual void glBlendColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) { throw_if_not_overridden("glBlendColor", 3, 3); }
		virtual void glBlendEquation(GLenum mode) { throw_if_not_overridden("glBlendEquation", 3, 3); }
		virtual void glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha) { throw_if_not_overridden("glBlendEquationSeparate", 3, 3); }
		virtual void glBlendFunc(GLenum sfactor, GLenum dfactor) { throw_if_not_overridden("glBlendFunc", 3, 3); }
		virtual void glBlendFuncSeparate(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha) { throw_if_not_overridden("glBlendFuncSeparate", 3, 3); }
		virtual void glClampColor(GLenum target, GLenum clamp) { throw_if_not_overridden("glClampColor", 3, 3); }
		virtual void glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) { throw_if_not_overridden("glClearColor", 3, 3); }
		virtual void glClearDepth(GLdouble depth) { throw_if_not_overridden("glClearDepth", 3, 3); }
		virtual void glClearStencil(GLint s) { throw_if_not_overridden("glClearStencil", 3, 3); }
		virtual void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) { throw_if_not_overridden("glColorMask", 3, 3); }
		virtual void glColorMaski(GLuint index, GLboolean r, GLboolean g, GLboolean b, GLboolean a) { throw_if_not_overridden("glColorMaski", 3, 3); }
		virtual void glCompileShader(GLuint shader) { throw_if_not_overridden("glCompileShader", 3, 3); }
		virtual GLuint glCreateProgram() { throw_if_not_overridden("glCreateProgram", 3, 3); }
		virtual GLuint glCreateShader(GLenum type) { throw_if_not_overridden("glCreateShader", 3, 3); }
		virtual void glCullFace(GLenum mode) { throw_if_not_overridden("glCullFace", 3, 3); }
		virtual void glDeleteBuffers(GLsizei n, const GLuint *buffers) { throw_if_not_overridden("glDeleteBuffers", 3, 3); }
		virtual void glDeleteFramebuffers(GLsizei n, const GLuint *framebuffers) { throw_if_not_overridden("glDeleteFramebuffers", 3, 3); }
		virtual void glDeleteProgram(GLuint program) { throw_if_not_overridden("glDeleteProgram", 3, 3); }
		virtual void glDeleteRenderbuffers(GLsizei n, const GLuint *renderbuffers) { throw_if_not_overridden("glDeleteRenderbuffers", 3, 3); }
		virtual void glDeleteSamplers(GLsizei count, const GLuint *samplers) { throw_if_not_overridden("glDeleteSamplers", 3, 3); }
		virtual void glDeleteShader(GLuint shader) { throw_if_not_overridden("glDeleteShader", 3, 3); }
		virtual void glDeleteTextures(GLsizei n, const GLuint *textures) { throw_if_not_overridden("glDeleteTextures", 3, 3); }
		virtual void glDeleteVertexArrays(GLsizei n, const GLuint *arrays) { throw_if_not_overridden("glDeleteVertexArrays", 3, 3); }
		virtual void glDepthFunc(GLenum func) { throw_if_not_overridden("glDepthFunc", 3, 3); }
		virtual void glDepthMask(GLboolean flag) { throw_if_not_overridden("glDepthMask", 3, 3); }
		virtual void glDepthRange(GLdouble nearVal, GLdouble farVal) { throw_if_not_overridden("glDepthRange", 3, 3); }
		virtual void glDetachShader(GLuint program, GLuint shader) { throw_if_not_overridden("glDetachShader", 3, 3); }
		virtual void glDisable(GLenum cap) { throw_if_not_overridden("glDisable", 3, 3); }
		virtual void glDisablei(GLenum target, GLuint index) { throw_if_not_overridden("glDisablei", 3, 3); }
		virtual void glDisableVertexAttribArray(GLuint index) { throw_if_not_overridden("glDisableVertexAttribArray", 3, 3); }
		virtual void glDrawBuffer(GLenum mode) { throw_if_not_overridden("glDrawBuffer", 3, 3); }
		virtual void glDrawBuffers(GLsizei n, const GLenum *bufs) { throw_if_not_overridden("glDrawBuffers", 3, 3); }
		virtual void glEnable(GLenum cap) { throw_if_not_overridden("glEnable", 3, 3); }
		virtual void glEnablei(GLenum target, GLuint index) { throw_if_not_overridden("glEnablei", 3, 3); }
		virtual void glEnableVertexAttribArray(GLuint index) { throw_if_not_overridden("glEnableVertexAttribArray", 3, 3); }
		virtual void glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) { throw_if_not_overridden("glFramebufferRenderbuffer", 3, 3); }
		virtual void glFramebufferTexture1D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) { throw_if_not_overridden("glFramebufferTexture1D", 3, 3); }
		virtual void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) { throw_if_not_overridden("glFramebufferTexture2D", 3, 3); }
		virtual void glFramebufferTexture3D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset) { throw_if_not_overridden("glFramebufferTexture3D", 3, 3); }
		virtual void glFramebufferTexture(GLenum target, GLenum attachment, GLuint texture, GLint level) { throw_if_not_overridden("glFramebufferTexture", 3, 3); }
		virtual void glFramebufferTextureLayer(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer) { throw_if_not_overridden("glFramebufferTextureLayer", 3, 3); }
		virtual void glFrontFace(GLenum mode) { throw_if_not_overridden("glFrontFace", 3, 3); }
		virtual void glGenBuffers(GLsizei n, GLuint *buffers) { throw_if_not_overridden("glGenBuffers", 3, 3); }
		virtual void glGenFramebuffers(GLsizei n, GLuint *framebuffers) { throw_if_not_overridden("glGenFramebuffers", 3, 3); }
		virtual void glGenRenderbuffers(GLsizei n, GLuint *renderbuffers) { throw_if_not_overridden("glGenRenderbuffers", 3, 3); }
		virtual void glGenSamplers(GLsizei count, GLuint *samplers) { throw_if_not_overridden("glGenSamplers", 3, 3); }
		virtual void glGenTextures(GLsizei n, GLuint *textures) { throw_if_not_overridden("glGenTextures", 3, 3); }
		virtual void glGenVertexArrays(GLsizei n, GLuint *arrays) { throw_if_not_overridden("glGenVertexArrays", 3, 3); }
		virtual void glGetIntegerv(GLenum pname, GLint *params) { throw_if_not_overridden("glGetIntegerv", 3, 3); }
		virtual void glGetProgramiv(GLuint program, GLenum pname, GLint *params) { throw_if_not_overridden("glGetProgramiv", 3, 3); }
		virtual void glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog) { throw_if_not_overridden("glGetProgramInfoLog", 3, 3); }
		virtual void glGetShaderiv(GLuint shader, GLenum pname, GLint *params) { throw_if_not_overridden("glGetShaderiv", 3, 3); }
		virtual void glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog) { throw_if_not_overridden("glGetShaderInfoLog", 3, 3); }
		virtual GLuint glGetUniformBlockIndex(GLuint program, const GLchar *uniformBlockName) { throw_if_not_overridden("glGetUniformBlockIndex", 3, 3); }
		virtual GLint glGetUniformLocation(GLuint program, const GLchar *name) { throw_if_not_overridden("glGetUniformLocation", 3, 3); }
		virtual void glHint(GLenum target, GLenum mode) { throw_if_not_overridden("glHint", 3, 3); }
		virtual void glLineWidth(GLfloat width) { throw_if_not_overridden("glLineWidth", 3, 3); }
		virtual void glLinkProgram(GLuint program) { throw_if_not_overridden("glLinkProgram", 3, 3); }
		virtual void glPixelStorei(GLenum pname, GLint param) { throw_if_not_overridden("glPixelStorei", 3, 3); }
		virtual void glPointSize(GLfloat size) { throw_if_not_overridden("glPointSize", 3, 3); }
		virtual void glPolygonMode(GLenum face, GLenum mode) { throw_if_not_overridden("glPolygonMode", 3, 3); }
		virtual void glPolygonOffset(GLfloat factor, GLfloat units) { throw_if_not_overridden("glPolygonOffset", 3, 3); }
		virtual void glPrimitiveRestartIndex(GLuint index) { throw_if_not_overridden("glPrimitiveRestartIndex", 3, 3); }
		virtual void glReadBuffer(GLenum mode) { throw_if_not_overridden("glReadBuffer", 3, 3); }
		virtual void glSampleCoverage(GLfloat value, GLboolean invert) { throw_if_not_overridden("glSampleCoverage", 3, 3); }
		virtual void glSampleMaski(GLuint index, GLbitfield mask) { throw_if_not_overridden("glSampleMaski", 3, 3); }
		virtual void glScissor(GLint x, GLint y, GLsizei width, GLsizei height) { throw_if_not_overridden("glScissor", 3, 3); }
		virtual void glShaderSource(GLuint shader, GLsizei count, const GLchar* const *string, const GLint *length) { throw_if_not_overridden("glShaderSource", 3, 3); }
		virtual void glStencilFunc(GLenum func, GLint ref, GLuint mask) { throw_if_not_overridden("glStencilFunc", 3, 3); }
		virtual void glStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask) { throw_if_not_overridden("glStencilFuncSeparate", 3, 3); }
		virtual void glStencilMask(GLuint mask) { throw_if_not_overridden("glStencilMask", 3, 3); }
		virtual void glStencilMaskSeparate(GLenum face, GLuint mask) { throw_if_not_overridden("glStencilMaskSeparate", 3, 3); }
		virtual void glStencilOp(GLenum fail, GLenum zfail, GLenum zpass) { throw_if_not_overridden("glStencilOp", 3, 3); }
		virtual void glStencilOpSeparate(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass) { throw_if_not_overridden("glStencilOpSeparate", 3, 3); }
		virtual void glUseProgram(GLuint program) { throw_if_not_overridden("glUseProgram", 3, 3); }
		virtual void glValidateProgram(GLuint program) { throw_if_not_overridden("glValidateProgram", 3, 3); }
		virtual void glVertexAttribDivisor(GLuint index, GLuint divisor) { throw_if_not_overridden("glVertexAttribDivisor", 3, 3); }
		virtual void glVertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) { throw_if_not_overridden("glVertexAttribIPointer", 3, 3); }
		virtual void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer) { throw_if_not_overridden("glVertexAttribPointer", 3, 3); }
		virtual void glViewport(GLint x, GLint y, GLsizei width, GLsizei height) { throw_if_not_overridden("glViewport", 3, 3); }


		//
		// Introduced in OpenGL 4.0 core.
		//


		//
		// Introduced in OpenGL 4.1 core.
		//


		//
		// Introduced in OpenGL 4.2 core.
		//


		//
		// Introduced in OpenGL 4.3 core.
		//

	protected:

		OpenGLFunctions(
				int major_version,
				int minor_version) :
			d_major_version(major_version),
			d_minor_version(minor_version)
		{  }

	private:

		void
		throw_if_not_overridden(
				const char *function_name,
				int required_major_version,
				int required_minor_version);

		int d_major_version;
		int d_minor_version;
	};
}

#endif // GPLATES_OPENGL_OPENGLFUNCTIONS_H
