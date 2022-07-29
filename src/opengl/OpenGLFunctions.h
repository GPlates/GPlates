
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


		//! Returns major version of our supported OpenGL version.
		int
		get_major_version() const
		{
			return d_major_version;
		}

		//! Returns minor version of our supported OpenGL version.
		int
		get_minor_version() const
		{
			return d_minor_version;
		}


		//
		// OpenGL 1.0 - 3.3 (core profile).
		//
		// Note: OpenGL 3.3 (core profile) is our MINIMUM requirement.
		//
		virtual void glActiveTexture(GLenum texture) { throw_if_not_overridden("glActiveTexture", 3, 3); }
		virtual void glAttachShader(GLuint program, GLuint shader) { throw_if_not_overridden("glAttachShader", 3, 3); }
		virtual void glBindAttribLocation(GLuint program, GLuint index, const GLchar *name) { throw_if_not_overridden("glBindAttribLocation", 3, 3); }
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
		virtual void glBufferData(GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage) { throw_if_not_overridden("glBufferData", 3, 3); }
		virtual void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid *data) { throw_if_not_overridden("glBufferSubData", 3, 3); }
		virtual GLenum glCheckFramebufferStatus(GLenum target) { throw_if_not_overridden("glCheckFramebufferStatus", 3, 3); }
		virtual void glClampColor(GLenum target, GLenum clamp) { throw_if_not_overridden("glClampColor", 3, 3); }
		virtual void glClear(GLbitfield mask) { throw_if_not_overridden("glClear", 3, 3); }
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
		virtual void glDrawArrays(GLenum mode, GLint first, GLsizei count) { throw_if_not_overridden("glDrawArrays", 3, 3); }
		virtual void glDrawBuffer(GLenum mode) { throw_if_not_overridden("glDrawBuffer", 3, 3); }
		virtual void glDrawBuffers(GLsizei n, const GLenum *bufs) { throw_if_not_overridden("glDrawBuffers", 3, 3); }
		virtual void glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices) { throw_if_not_overridden("glDrawElements", 3, 3); }
		virtual void glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices) { throw_if_not_overridden("glDrawElements", 3, 3); }
		virtual void glEnable(GLenum cap) { throw_if_not_overridden("glEnable", 3, 3); }
		virtual void glEnablei(GLenum target, GLuint index) { throw_if_not_overridden("glEnablei", 3, 3); }
		virtual void glEnableVertexAttribArray(GLuint index) { throw_if_not_overridden("glEnableVertexAttribArray", 3, 3); }
		virtual void glFlushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length) { throw_if_not_overridden("glFlushMappedBufferRange", 3, 3); }
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
		virtual GLenum glGetError() { throw_if_not_overridden("glGetError", 3, 3); }
		virtual void glGetIntegerv(GLenum pname, GLint *params) { throw_if_not_overridden("glGetIntegerv", 3, 3); }
		virtual void glGetProgramiv(GLuint program, GLenum pname, GLint *params) { throw_if_not_overridden("glGetProgramiv", 3, 3); }
		virtual void glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog) { throw_if_not_overridden("glGetProgramInfoLog", 3, 3); }
		virtual void glGetShaderiv(GLuint shader, GLenum pname, GLint *params) { throw_if_not_overridden("glGetShaderiv", 3, 3); }
		virtual void glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog) { throw_if_not_overridden("glGetShaderInfoLog", 3, 3); }
		virtual void glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels) { throw_if_not_overridden("glGetTexImage", 3, 3); }
		virtual GLuint glGetUniformBlockIndex(GLuint program, const GLchar *uniformBlockName) { throw_if_not_overridden("glGetUniformBlockIndex", 3, 3); }
		virtual GLint glGetUniformLocation(GLuint program, const GLchar *name) { throw_if_not_overridden("glGetUniformLocation", 3, 3); }
		virtual void glHint(GLenum target, GLenum mode) { throw_if_not_overridden("glHint", 3, 3); }
		virtual void glLineWidth(GLfloat width) { throw_if_not_overridden("glLineWidth", 3, 3); }
		virtual void glLinkProgram(GLuint program) { throw_if_not_overridden("glLinkProgram", 3, 3); }
		virtual GLvoid *glMapBuffer(GLenum target, GLenum access) { throw_if_not_overridden("glMapBuffer", 3, 3); }
		virtual GLvoid *glMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access) { throw_if_not_overridden("glMapBufferRange", 3, 3); }
		virtual void glPixelStorei(GLenum pname, GLint param) { throw_if_not_overridden("glPixelStorei", 3, 3); }
		virtual void glPointSize(GLfloat size) { throw_if_not_overridden("glPointSize", 3, 3); }
		virtual void glPolygonMode(GLenum face, GLenum mode) { throw_if_not_overridden("glPolygonMode", 3, 3); }
		virtual void glPolygonOffset(GLfloat factor, GLfloat units) { throw_if_not_overridden("glPolygonOffset", 3, 3); }
		virtual void glPrimitiveRestartIndex(GLuint index) { throw_if_not_overridden("glPrimitiveRestartIndex", 3, 3); }
		virtual void glReadBuffer(GLenum mode) { throw_if_not_overridden("glReadBuffer", 3, 3); }
		virtual void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels) { throw_if_not_overridden("glReadPixels", 3, 3); }
		virtual void glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) { throw_if_not_overridden("glRenderbufferStorage", 3, 3); }
		virtual void glSampleCoverage(GLfloat value, GLboolean invert) { throw_if_not_overridden("glSampleCoverage", 3, 3); }
		virtual void glSampleMaski(GLuint index, GLbitfield mask) { throw_if_not_overridden("glSampleMaski", 3, 3); }
		virtual void glSamplerParameterf(GLuint sampler, GLenum pname, GLfloat param) { throw_if_not_overridden("glSamplerParameterf", 3, 3); }
		virtual void glSamplerParameterfv(GLuint sampler, GLenum pname, const GLfloat *param) { throw_if_not_overridden("glSamplerParameterfv", 3, 3); }
		virtual void glSamplerParameteri(GLuint sampler, GLenum pname, GLint param) { throw_if_not_overridden("glSamplerParameteri", 3, 3); }
		virtual void glSamplerParameteriv(GLuint sampler, GLenum pname, const GLint *param) { throw_if_not_overridden("glSamplerParameteriv", 3, 3); }
		virtual void glSamplerParameterIiv(GLuint sampler, GLenum pname, const GLint *param) { throw_if_not_overridden("glSamplerParameterIiv", 3, 3); }
		virtual void glSamplerParameterIuiv(GLuint sampler, GLenum pname, const GLuint *param) { throw_if_not_overridden("glSamplerParameterIuiv", 3, 3); }
		virtual void glScissor(GLint x, GLint y, GLsizei width, GLsizei height) { throw_if_not_overridden("glScissor", 3, 3); }
		virtual void glShaderSource(GLuint shader, GLsizei count, const GLchar* const *string, const GLint *length) { throw_if_not_overridden("glShaderSource", 3, 3); }
		virtual void glStencilFunc(GLenum func, GLint ref, GLuint mask) { throw_if_not_overridden("glStencilFunc", 3, 3); }
		virtual void glStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask) { throw_if_not_overridden("glStencilFuncSeparate", 3, 3); }
		virtual void glStencilMask(GLuint mask) { throw_if_not_overridden("glStencilMask", 3, 3); }
		virtual void glStencilMaskSeparate(GLenum face, GLuint mask) { throw_if_not_overridden("glStencilMaskSeparate", 3, 3); }
		virtual void glStencilOp(GLenum fail, GLenum zfail, GLenum zpass) { throw_if_not_overridden("glStencilOp", 3, 3); }
		virtual void glStencilOpSeparate(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass) { throw_if_not_overridden("glStencilOpSeparate", 3, 3); }
		virtual void glTexParameterf(GLenum target, GLenum pname, GLfloat param) { throw_if_not_overridden("glTexParameterf", 3, 3); }
		virtual void glTexParameterfv(GLenum target, GLenum pname, const GLfloat *params) { throw_if_not_overridden("glTexParameterfv", 3, 3); }
		virtual void glTexParameteri(GLenum target, GLenum pname, GLint param) { throw_if_not_overridden("glTexParameteri", 3, 3); }
		virtual void glTexParameteriv(GLenum target, GLenum pname, const GLint *params) { throw_if_not_overridden("glTexParameteriv", 3, 3); }
		virtual void glTexParameterIiv(GLenum target, GLenum pname, const GLint *params) { throw_if_not_overridden("glTexParameterIiv", 3, 3); }
		virtual void glTexParameterIuiv(GLenum target, GLenum pname, const GLuint *params) { throw_if_not_overridden("glTexParameterIuiv", 3, 3); }
		virtual void glTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels) { throw_if_not_overridden("glTexSubImage1D", 3, 3); }
		virtual void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels) { throw_if_not_overridden("glTexSubImage1D", 3, 3); }
		virtual void glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels) { throw_if_not_overridden("glTexSubImage3D", 3, 3); }
		virtual void glTexImage1D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels) { throw_if_not_overridden("glTexSubImage2D", 3, 3); }
		virtual void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels) { throw_if_not_overridden("glTexImage2D", 3, 3); }
		virtual void glTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels) { throw_if_not_overridden("glTexImage3D", 3, 3); }
		virtual void glUniform1f(GLint location, GLfloat v0) { throw_if_not_overridden("glUniform1f", 3, 3); }
		virtual void glUniform1fv(GLint location, GLsizei count, const GLfloat *value) { throw_if_not_overridden("glUniform1fv", 3, 3); }
		virtual void glUniform1i(GLint location, GLint v0) { throw_if_not_overridden("glUniform1i", 3, 3); }
		virtual void glUniform1iv(GLint location, GLsizei count, const GLint *value) { throw_if_not_overridden("glUniform1iv", 3, 3); }
		virtual void glUniform1ui(GLint location, GLuint v0) { throw_if_not_overridden("glUniform1ui", 3, 3); }
		virtual void glUniform1uiv(GLint location, GLsizei count, const GLuint *value) { throw_if_not_overridden("glUniform1uiv", 3, 3); }
		virtual void glUniform2f(GLint location, GLfloat v0, GLfloat v1) { throw_if_not_overridden("glUniform2f", 3, 3); }
		virtual void glUniform2fv(GLint location, GLsizei count, const GLfloat *value) { throw_if_not_overridden("glUniform2fv", 3, 3); }
		virtual void glUniform2i(GLint location, GLint v0, GLint v1) { throw_if_not_overridden("glUniform2i", 3, 3); }
		virtual void glUniform2iv(GLint location, GLsizei count, const GLint *value) { throw_if_not_overridden("glUniform2iv", 3, 3); }
		virtual void glUniform2ui(GLint location, GLuint v0, GLuint v1) { throw_if_not_overridden("glUniform2ui", 3, 3); }
		virtual void glUniform2uiv(GLint location, GLsizei count, const GLuint *value) { throw_if_not_overridden("glUniform2uiv", 3, 3); }
		virtual void glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2) { throw_if_not_overridden("glUniform3f", 3, 3); }
		virtual void glUniform3fv(GLint location, GLsizei count, const GLfloat *value) { throw_if_not_overridden("glUniform3fv", 3, 3); }
		virtual void glUniform3i(GLint location, GLint v0, GLint v1, GLint v2) { throw_if_not_overridden("glUniform3i", 3, 3); }
		virtual void glUniform3iv(GLint location, GLsizei count, const GLint *value) { throw_if_not_overridden("glUniform3iv", 3, 3); }
		virtual void glUniform3ui(GLint location, GLuint v0, GLuint v1, GLuint v2) { throw_if_not_overridden("glUniform3ui", 3, 3); }
		virtual void glUniform3uiv(GLint location, GLsizei count, const GLuint *value) { throw_if_not_overridden("glUniform3uiv", 3, 3); }
		virtual void glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) { throw_if_not_overridden("glUniform4f", 3, 3); }
		virtual void glUniform4fv(GLint location, GLsizei count, const GLfloat *value) { throw_if_not_overridden("glUniform4fv", 3, 3); }
		virtual void glUniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3) { throw_if_not_overridden("glUniform4i", 3, 3); }
		virtual void glUniform4iv(GLint location, GLsizei count, const GLint *value) { throw_if_not_overridden("glUniform4iv", 3, 3); }
		virtual void glUniform4ui(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3) { throw_if_not_overridden("glUniform4ui", 3, 3); }
		virtual void glUniform4uiv(GLint location, GLsizei count, const GLuint *value) { throw_if_not_overridden("glUniform4uiv", 3, 3); }
		virtual void glUniformBlockBinding(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding) { throw_if_not_overridden("glUniformBlockBinding", 3, 3); }
		virtual void glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) { throw_if_not_overridden("glUniformMatrix2fv", 3, 3); }
		virtual void glUniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) { throw_if_not_overridden("glUniformMatrix2x3fv", 3, 3); }
		virtual void glUniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) { throw_if_not_overridden("glUniformMatrix2x4fv", 3, 3); }
		virtual void glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) { throw_if_not_overridden("glUniformMatrix3fv", 3, 3); }
		virtual void glUniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) { throw_if_not_overridden("glUniformMatrix3x2fv", 3, 3); }
		virtual void glUniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) { throw_if_not_overridden("glUniformMatrix3x4fv", 3, 3); }
		virtual void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) { throw_if_not_overridden("glUniformMatrix4fv", 3, 3); }
		virtual void glUniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) { throw_if_not_overridden("glUniformMatrix4x2fv", 3, 3); }
		virtual void glUniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) { throw_if_not_overridden("glUniformMatrix4x3fv", 3, 3); }
		virtual GLboolean glUnmapBuffer(GLenum target) { throw_if_not_overridden("glUnmapBuffer", 3, 3); }
		virtual void glUseProgram(GLuint program) { throw_if_not_overridden("glUseProgram", 3, 3); }
		virtual void glValidateProgram(GLuint program) { throw_if_not_overridden("glValidateProgram", 3, 3); }
		virtual void glVertexAttribDivisor(GLuint index, GLuint divisor) { throw_if_not_overridden("glVertexAttribDivisor", 3, 3); }
		virtual void glVertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) { throw_if_not_overridden("glVertexAttribIPointer", 3, 3); }
		virtual void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer) { throw_if_not_overridden("glVertexAttribPointer", 3, 3); }
		virtual void glViewport(GLint x, GLint y, GLsizei width, GLsizei height) { throw_if_not_overridden("glViewport", 3, 3); }


		//
		// Introduced in OpenGL 4.0 (core profile).
		//


		//
		// Introduced in OpenGL 4.1 (core profile).
		//


		//
		// Introduced in OpenGL 4.2 (core profile).
		//


		//
		// Introduced in OpenGL 4.3 (core profile).
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
