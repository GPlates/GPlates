
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

#include <qopengl.h>  // For OpenGL constants and typedefs.

#include "global/config.h" // For GPLATES_USE_VULKAN_BACKEND
#if defined(GPLATES_USE_VULKAN_BACKEND)
#	include <QVulkanDeviceFunctions>
#else
#	include <QOpenGLFunctions_4_5_Core>
#endif

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	/**
	 * OpenGL functions for 4.5 core.
	 *
	 * This class wraps the Qt versioned OpenGL function class 'QOpenGLFunctions_4_5_Core'
	 * to provide a interface for OpenGL core profile version 4.5.
	 */
	class OpenGLFunctions :
			public GPlatesUtils::ReferenceCount<OpenGLFunctions>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a OpenGLFunctions.
		typedef GPlatesUtils::non_null_intrusive_ptr<OpenGLFunctions> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a OpenGLFunctions.
		typedef GPlatesUtils::non_null_intrusive_ptr<const OpenGLFunctions> non_null_ptr_to_const_type;


		//! Create OpenGL function for version 4.5 core profile.
		static
		non_null_ptr_type
		create(
#if defined(GPLATES_USE_VULKAN_BACKEND)
			QVulkanDeviceFunctions *functions
#else
			QOpenGLFunctions_4_5_Core *functions
#endif
		)
		{
			return non_null_ptr_type(new OpenGLFunctions(functions));
		}


		void glActiveTexture(GLenum texture);
		void glAttachShader(GLuint program, GLuint shader);
		void glBindAttribLocation(GLuint program, GLuint index, const GLchar *name);
		void glBindBuffer(GLenum target, GLuint buffer);
		void glBindBufferBase(GLenum target, GLuint index, GLuint buffer);
		void glBindBufferRange(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size);
		void glBindFramebuffer(GLenum target, GLuint framebuffer);
		void glBindRenderbuffer(GLenum target, GLuint renderbuffer);
		void glBindSampler(GLuint unit, GLuint sampler);
		void glBindTexture(GLenum target, GLuint texture);
		void glBindVertexArray(GLuint array);
		void glBlendColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
		void glBlendEquation(GLenum mode);
		void glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha);
		void glBlendFunc(GLenum sfactor, GLenum dfactor);
		void glBlendFuncSeparate(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);
		void glBufferData(GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage);
		void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid *data);
		GLenum glCheckFramebufferStatus(GLenum target);
		void glClampColor(GLenum target, GLenum clamp);
		void glClear(GLbitfield mask);
		void glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
		void glClearDepth(GLdouble depth);
		void glClearStencil(GLint s);
		void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
		void glColorMaski(GLuint index, GLboolean r, GLboolean g, GLboolean b, GLboolean a);
		void glCompileShader(GLuint shader);
		GLuint glCreateProgram();
		GLuint glCreateShader(GLenum type);
		void glCullFace(GLenum mode);
		void glDeleteBuffers(GLsizei n, const GLuint *buffers);
		void glDeleteFramebuffers(GLsizei n, const GLuint *framebuffers);
		void glDeleteProgram(GLuint program);
		void glDeleteRenderbuffers(GLsizei n, const GLuint *renderbuffers);
		void glDeleteSamplers(GLsizei count, const GLuint *samplers);
		void glDeleteShader(GLuint shader);
		void glDeleteTextures(GLsizei n, const GLuint *textures);
		void glDeleteVertexArrays(GLsizei n, const GLuint *arrays);
		void glDepthFunc(GLenum func);
		void glDepthMask(GLboolean flag);
		void glDepthRange(GLdouble nearVal, GLdouble farVal);
		void glDetachShader(GLuint program, GLuint shader);
		void glDisable(GLenum cap);
		void glDisablei(GLenum target, GLuint index);
		void glDisableVertexAttribArray(GLuint index);
		void glDrawArrays(GLenum mode, GLint first, GLsizei count);
		void glDrawBuffer(GLenum mode);
		void glDrawBuffers(GLsizei n, const GLenum *bufs);
		void glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
		void glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices);
		void glEnable(GLenum cap);
		void glEnablei(GLenum target, GLuint index);
		void glEnableVertexAttribArray(GLuint index);
		void glFlushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length);
		void glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
		void glFramebufferTexture1D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
		void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
		void glFramebufferTexture3D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset);
		void glFramebufferTexture(GLenum target, GLenum attachment, GLuint texture, GLint level);
		void glFramebufferTextureLayer(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer);
		void glFrontFace(GLenum mode);
		void glGenBuffers(GLsizei n, GLuint *buffers);
		void glGenFramebuffers(GLsizei n, GLuint *framebuffers);
		void glGenRenderbuffers(GLsizei n, GLuint *renderbuffers);
		void glGenSamplers(GLsizei count, GLuint *samplers);
		void glGenTextures(GLsizei n, GLuint *textures);
		void glGenVertexArrays(GLsizei n, GLuint *arrays);
		GLenum glGetError();
		void glGetIntegerv(GLenum pname, GLint *params);
		void glGetInteger64v(GLenum pname, GLint64* params);
		void glGetProgramiv(GLuint program, GLenum pname, GLint *params);
		void glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
		void glGetShaderiv(GLuint shader, GLenum pname, GLint *params);
		void glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
		void glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels);
		GLuint glGetUniformBlockIndex(GLuint program, const GLchar *uniformBlockName);
		GLint glGetUniformLocation(GLuint program, const GLchar *name);
		void glHint(GLenum target, GLenum mode);
		void glLineWidth(GLfloat width);
		void glLinkProgram(GLuint program);
		GLvoid *glMapBuffer(GLenum target, GLenum access);
		GLvoid *glMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access);
		void glPixelStorei(GLenum pname, GLint param);
		void glPointSize(GLfloat size);
		void glPolygonMode(GLenum face, GLenum mode);
		void glPolygonOffset(GLfloat factor, GLfloat units);
		void glPrimitiveRestartIndex(GLuint index);
		void glReadBuffer(GLenum mode);
		void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
		void glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
		void glSampleCoverage(GLfloat value, GLboolean invert);
		void glSampleMaski(GLuint index, GLbitfield mask);
		void glSamplerParameterf(GLuint sampler, GLenum pname, GLfloat param);
		void glSamplerParameterfv(GLuint sampler, GLenum pname, const GLfloat *param);
		void glSamplerParameteri(GLuint sampler, GLenum pname, GLint param);
		void glSamplerParameteriv(GLuint sampler, GLenum pname, const GLint *param);
		void glSamplerParameterIiv(GLuint sampler, GLenum pname, const GLint *param);
		void glSamplerParameterIuiv(GLuint sampler, GLenum pname, const GLuint *param);
		void glScissor(GLint x, GLint y, GLsizei width, GLsizei height);
		void glShaderSource(GLuint shader, GLsizei count, const GLchar* const *string, const GLint *length);
		void glStencilFunc(GLenum func, GLint ref, GLuint mask);
		void glStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask);
		void glStencilMask(GLuint mask);
		void glStencilMaskSeparate(GLenum face, GLuint mask);
		void glStencilOp(GLenum fail, GLenum zfail, GLenum zpass);
		void glStencilOpSeparate(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass);
		void glTexParameterf(GLenum target, GLenum pname, GLfloat param);
		void glTexParameterfv(GLenum target, GLenum pname, const GLfloat *params);
		void glTexParameteri(GLenum target, GLenum pname, GLint param);
		void glTexParameteriv(GLenum target, GLenum pname, const GLint *params);
		void glTexParameterIiv(GLenum target, GLenum pname, const GLint *params);
		void glTexParameterIuiv(GLenum target, GLenum pname, const GLuint *params);
		void glTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels);
		void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
		void glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels);
		void glTexImage1D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
		void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
		void glTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
		void glUniform1f(GLint location, GLfloat v0);
		void glUniform1fv(GLint location, GLsizei count, const GLfloat *value);
		void glUniform1i(GLint location, GLint v0);
		void glUniform1iv(GLint location, GLsizei count, const GLint *value);
		void glUniform1ui(GLint location, GLuint v0);
		void glUniform1uiv(GLint location, GLsizei count, const GLuint *value);
		void glUniform2f(GLint location, GLfloat v0, GLfloat v1);
		void glUniform2fv(GLint location, GLsizei count, const GLfloat *value);
		void glUniform2i(GLint location, GLint v0, GLint v1);
		void glUniform2iv(GLint location, GLsizei count, const GLint *value);
		void glUniform2ui(GLint location, GLuint v0, GLuint v1);
		void glUniform2uiv(GLint location, GLsizei count, const GLuint *value);
		void glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
		void glUniform3fv(GLint location, GLsizei count, const GLfloat *value);
		void glUniform3i(GLint location, GLint v0, GLint v1, GLint v2);
		void glUniform3iv(GLint location, GLsizei count, const GLint *value);
		void glUniform3ui(GLint location, GLuint v0, GLuint v1, GLuint v2);
		void glUniform3uiv(GLint location, GLsizei count, const GLuint *value);
		void glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
		void glUniform4fv(GLint location, GLsizei count, const GLfloat *value);
		void glUniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
		void glUniform4iv(GLint location, GLsizei count, const GLint *value);
		void glUniform4ui(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3);
		void glUniform4uiv(GLint location, GLsizei count, const GLuint *value);
		void glUniformBlockBinding(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding);
		void glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
		void glUniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
		void glUniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
		void glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
		void glUniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
		void glUniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
		void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
		void glUniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
		void glUniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
		GLboolean glUnmapBuffer(GLenum target);
		void glUseProgram(GLuint program);
		void glValidateProgram(GLuint program);
		void glVertexAttribDivisor(GLuint index, GLuint divisor);
		void glVertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
		void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer);
		void glViewport(GLint x, GLint y, GLsizei width, GLsizei height);

	private:
#if defined(GPLATES_USE_VULKAN_BACKEND)
		QVulkanDeviceFunctions *d_functions;
#else
		QOpenGLFunctions_4_5_Core *d_functions;
#endif


		OpenGLFunctions(
#if defined(GPLATES_USE_VULKAN_BACKEND)
				QVulkanDeviceFunctions *functions
#else
				QOpenGLFunctions_4_5_Core *functions
#endif
				) :
			d_functions(functions)
		{  }
	};
}

#endif // GPLATES_OPENGL_OPENGLFUNCTIONS_H
