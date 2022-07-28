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

#include <QString>

#include "OpenGLFunctions.h"

#include "OpenGLException.h"


namespace GPlatesOpenGL
{
	namespace
	{
		/**
		 * Functions in OpenGL 1.0 - 3.3 core.
		 */
		template <class VersionFunctionsType>
		class OpenGLFunctions_3_3_Core :
				public OpenGLFunctions
		{
		public:
			explicit
			OpenGLFunctions_3_3_Core(
					VersionFunctionsType *version_functions,
					int major_version,
					int minor_version) :
				OpenGLFunctions(major_version, minor_version),
				d_version_functions(version_functions)
			{  }

			void glActiveTexture(GLenum texture) override
			{
				d_version_functions->glActiveTexture(texture);
			}
			void glAttachShader(GLuint program, GLuint shader) override
			{
				d_version_functions->glAttachShader(program, shader);
			}
			void glBindAttribLocation(GLuint program, GLuint index, const GLchar *name) override
			{
				d_version_functions->glBindAttribLocation(program, index, name);
			}
			void glBindBuffer(GLenum target, GLuint buffer) override
			{
				d_version_functions->glBindBuffer(target, buffer);
			}
			void glBindBufferBase(GLenum target, GLuint index, GLuint buffer) override
			{
				d_version_functions->glBindBufferBase(target, index, buffer);
			}
			void glBindBufferRange(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size) override
			{
				d_version_functions->glBindBufferRange(target, index, buffer, offset, size);
			}
			void glBindFramebuffer(GLenum target, GLuint framebuffer) override
			{
				d_version_functions->glBindFramebuffer(target, framebuffer);
			}
			void glBindRenderbuffer(GLenum target, GLuint renderbuffer) override
			{
				d_version_functions->glBindRenderbuffer(target, renderbuffer);
			}
			void glBindSampler(GLuint unit, GLuint sampler) override
			{
				d_version_functions->glBindSampler(unit, sampler);
			}
			void glBindTexture(GLenum target, GLuint texture) override
			{
				d_version_functions->glBindTexture(target, texture);
			}
			void glBindVertexArray(GLuint array) override
			{
				d_version_functions->glBindVertexArray(array);
			}
			void glBlendColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) override
			{
				d_version_functions->glBlendColor(red, green, blue, alpha);
			}
			void glBlendEquation(GLenum mode) override
			{
				d_version_functions->glBlendEquation(mode);
			}
			void glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha) override
			{
				d_version_functions->glBlendEquationSeparate(modeRGB, modeAlpha);
			}
			void glBlendFunc(GLenum sfactor, GLenum dfactor) override
			{
				d_version_functions->glBlendFunc(sfactor, dfactor);
			}
			void glBlendFuncSeparate(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha) override
			{
				d_version_functions->glBlendFuncSeparate(sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha);
			}
			void glBufferData(GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage) override
			{
				d_version_functions->glBufferData(target, size, data, usage);
			}
			void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid *data) override
			{
				d_version_functions->glBufferSubData(target, offset, size, data);
			}
			GLenum glCheckFramebufferStatus(GLenum target) override
			{
				return d_version_functions->glCheckFramebufferStatus(target);
			}
			void glClampColor(GLenum target, GLenum clamp) override
			{
				d_version_functions->glClampColor(target, clamp);
			}
			void glClear(GLbitfield mask) override
			{
				d_version_functions->glClear(mask);
			}
			void glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) override
			{
				d_version_functions->glClearColor(red, green, blue, alpha);
			}
			void glClearDepth(GLdouble depth) override
			{
				d_version_functions->glClearDepth(depth);
			}
			void glClearStencil(GLint s) override
			{
				d_version_functions->glClearStencil(s);
			}
			void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) override
			{
				d_version_functions->glColorMask(red, green, blue, alpha);
			}
			void glColorMaski(GLuint index, GLboolean r, GLboolean g, GLboolean b, GLboolean a) override
			{
				d_version_functions->glColorMaski(index, r, g, b, a);
			}
			void glCompileShader(GLuint shader) override
			{
				d_version_functions->glCompileShader(shader);
			}
			GLuint glCreateProgram() override
			{
				return d_version_functions->glCreateProgram();
			}
			GLuint glCreateShader(GLenum type) override
			{
				return d_version_functions->glCreateShader(type);
			}
			void glCullFace(GLenum mode) override
			{
				d_version_functions->glCullFace(mode);
			}
			void glDeleteBuffers(GLsizei n, const GLuint *buffers) override
			{
				d_version_functions->glDeleteBuffers(n, buffers);
			}
			void glDeleteFramebuffers(GLsizei n, const GLuint *framebuffers) override
			{
				d_version_functions->glDeleteFramebuffers(n, framebuffers);
			}
			void glDeleteProgram(GLuint program) override
			{
				d_version_functions->glDeleteProgram(program);
			}
			void glDeleteRenderbuffers(GLsizei n, const GLuint *renderbuffers) override
			{
				d_version_functions->glDeleteRenderbuffers(n, renderbuffers);
			}
			void glDeleteSamplers(GLsizei count, const GLuint *samplers) override
			{
				d_version_functions->glDeleteSamplers(count, samplers);
			}
			void glDeleteShader(GLuint shader) override
			{
				d_version_functions->glDeleteShader(shader);
			}
			void glDeleteTextures(GLsizei n, const GLuint *textures) override
			{
				d_version_functions->glDeleteTextures(n, textures);
			}
			void glDeleteVertexArrays(GLsizei n, const GLuint *arrays) override
			{
				d_version_functions->glDeleteVertexArrays(n, arrays);
			}
			void glDepthFunc(GLenum func) override
			{
				d_version_functions->glDepthFunc(func);
			}
			void glDepthMask(GLboolean flag) override
			{
				d_version_functions->glDepthMask(flag);
			}
			void glDepthRange(GLdouble nearVal, GLdouble farVal) override
			{
				d_version_functions->glDepthRange(nearVal, farVal);
			}
			void glDetachShader(GLuint program, GLuint shader) override
			{
				d_version_functions->glDetachShader(program, shader);
			}
			void glDisable(GLenum cap) override
			{
				d_version_functions->glDisable(cap);
			}
			void glDisablei(GLenum target, GLuint index) override
			{
				d_version_functions->glDisablei(target, index);
			}
			void glDisableVertexAttribArray(GLuint index) override
			{
				d_version_functions->glDisableVertexAttribArray(index);
			}
			void glDrawArrays(GLenum mode, GLint first, GLsizei count) override
			{
				d_version_functions->glDrawArrays(mode, first, count);
			}
			void glDrawBuffer(GLenum mode) override
			{
				d_version_functions->glDrawBuffer(mode);
			}
			void glDrawBuffers(GLsizei n, const GLenum *bufs) override
			{
				d_version_functions->glDrawBuffers(n, bufs);
			}
			void glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices) override
			{
				d_version_functions->glDrawElements(mode, count, type, indices);
			}
			void glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices) override
			{
				d_version_functions->glDrawRangeElements(mode, start, end, count, type, indices);
			}
			void glEnable(GLenum cap) override
			{
				d_version_functions->glEnable(cap);
			}
			void glEnablei(GLenum target, GLuint index) override
			{
				d_version_functions->glEnablei(target, index);
			}
			void glEnableVertexAttribArray(GLuint index) override
			{
				d_version_functions->glEnableVertexAttribArray(index);
			}
			void glFlushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length) override
			{
				d_version_functions->glFlushMappedBufferRange(target, offset, length);
			}
			void glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) override
			{
				d_version_functions->glFramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer);
			}
			void glFramebufferTexture1D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) override
			{
				d_version_functions->glFramebufferTexture1D(target, attachment, textarget, texture, level);
			}
			void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) override
			{
				d_version_functions->glFramebufferTexture2D(target, attachment, textarget, texture, level);
			}
			void glFramebufferTexture3D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset) override
			{
				d_version_functions->glFramebufferTexture3D(target, attachment, textarget, texture, level, zoffset);
			}
			void glFramebufferTexture(GLenum target, GLenum attachment, GLuint texture, GLint level) override
			{
				d_version_functions->glFramebufferTexture(target, attachment, texture, level);
			}
			void glFramebufferTextureLayer(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer) override
			{
				d_version_functions->glFramebufferTextureLayer(target, attachment, texture, level, layer);
			}
			void glFrontFace(GLenum mode) override
			{
				d_version_functions->glFrontFace(mode);
			}
			void glGenBuffers(GLsizei n, GLuint *buffers) override
			{
				d_version_functions->glGenBuffers(n, buffers);
			}
			void glGenFramebuffers(GLsizei n, GLuint *framebuffers) override
			{
				d_version_functions->glGenFramebuffers(n, framebuffers);
			}
			void glGenRenderbuffers(GLsizei n, GLuint *renderbuffers) override
			{
				d_version_functions->glGenRenderbuffers(n, renderbuffers);
			}
			void glGenSamplers(GLsizei count, GLuint *samplers) override
			{
				d_version_functions->glGenSamplers(count, samplers);
			}
			void glGenTextures(GLsizei n, GLuint *textures) override
			{
				d_version_functions->glGenTextures(n, textures);
			}
			void glGenVertexArrays(GLsizei n, GLuint *arrays) override
			{
				d_version_functions->glGenVertexArrays(n, arrays);
			}
			GLenum glGetError() override
			{
				return d_version_functions->glGetError();
			}
			void glGetIntegerv(GLenum pname, GLint *params) override
			{
				d_version_functions->glGetIntegerv(pname, params);
			}
			void glGetProgramiv(GLuint program, GLenum pname, GLint *params) override
			{
				d_version_functions->glGetProgramiv(program, pname, params);
			}
			void glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog) override
			{
				d_version_functions->glGetProgramInfoLog(program, bufSize, length, infoLog);
			}
			void glGetShaderiv(GLuint shader, GLenum pname, GLint *params) override
			{
				d_version_functions->glGetShaderiv(shader, pname, params);
			}
			void glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog) override
			{
				d_version_functions->glGetShaderInfoLog(shader, bufSize, length, infoLog);
			}
			void glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels) override
			{
				d_version_functions->glGetTexImage(target, level, format, type, pixels);
			}
			GLuint glGetUniformBlockIndex(GLuint program, const GLchar *uniformBlockName) override
			{
				return d_version_functions->glGetUniformBlockIndex(program, uniformBlockName);
			}
			GLint glGetUniformLocation(GLuint program, const GLchar *name) override
			{
				return d_version_functions->glGetUniformLocation(program, name);
			}
			void glHint(GLenum target, GLenum mode) override
			{
				d_version_functions->glHint(target, mode);
			}
			void glLineWidth(GLfloat width) override
			{
				d_version_functions->glLineWidth(width);
			}
			void glLinkProgram(GLuint program) override
			{
				d_version_functions->glLinkProgram(program);
			}
			GLvoid* glMapBuffer(GLenum target, GLenum access) override
			{
				return d_version_functions->glMapBuffer(target, access);
			}
			GLvoid* glMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access) override
			{
				return d_version_functions->glMapBufferRange(target, offset, length, access);
			}
			void glPixelStorei(GLenum pname, GLint param) override
			{
				d_version_functions->glPixelStorei(pname, param);
			}
			void glPointSize(GLfloat size) override
			{
				d_version_functions->glPointSize(size);
			}
			void glPolygonMode(GLenum face, GLenum mode) override
			{
				d_version_functions->glPolygonMode(face, mode);
			}
			void glPolygonOffset(GLfloat factor, GLfloat units) override
			{
				d_version_functions->glPolygonOffset(factor, units);
			}
			void glPrimitiveRestartIndex(GLuint index) override
			{
				d_version_functions->glPrimitiveRestartIndex(index);
			}
			void glReadBuffer(GLenum mode) override
			{
				d_version_functions->glReadBuffer(mode);
			}
			void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels) override
			{
				d_version_functions->glReadPixels(x, y, width, height, format, type, pixels);
			}
			void glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) override
			{
				d_version_functions->glRenderbufferStorage(target, internalformat, width, height);
			}
			void glSampleCoverage(GLfloat value, GLboolean invert) override
			{
				d_version_functions->glSampleCoverage(value, invert);
			}
			void glSampleMaski(GLuint index, GLbitfield mask) override
			{
				d_version_functions->glSampleMaski(index, mask);
			}
			void glSamplerParameterf(GLuint sampler, GLenum pname, GLfloat param) override
			{
				d_version_functions->glSamplerParameterf(sampler, pname, param);
			}
			void glSamplerParameterfv(GLuint sampler, GLenum pname, const GLfloat *param) override
			{
				d_version_functions->glSamplerParameterfv(sampler, pname, param);
			}
			void glSamplerParameteri(GLuint sampler, GLenum pname, GLint param) override
			{
				d_version_functions->glSamplerParameteri(sampler, pname, param);
			}
			void glSamplerParameteriv(GLuint sampler, GLenum pname, const GLint *param) override
			{
				d_version_functions->glSamplerParameteriv(sampler, pname, param);
			}
			void glSamplerParameterIiv(GLuint sampler, GLenum pname, const GLint *param) override
			{
				d_version_functions->glSamplerParameterIiv(sampler, pname, param);
			}
			void glSamplerParameterIuiv(GLuint sampler, GLenum pname, const GLuint *param) override
			{
				d_version_functions->glSamplerParameterIuiv(sampler, pname, param);
			}
			void glScissor(GLint x, GLint y, GLsizei width, GLsizei height) override
			{
				d_version_functions->glScissor(x, y, width, height);
			}
			void glShaderSource(GLuint shader, GLsizei count, const GLchar* const *string, const GLint *length) override
			{
				d_version_functions->glShaderSource(shader, count, string, length);
			}
			void glStencilFunc(GLenum func, GLint ref, GLuint mask) override
			{
				d_version_functions->glStencilFunc(func, ref, mask);
			}
			void glStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask) override
			{
				d_version_functions->glStencilFuncSeparate(face, func, ref, mask);
			}
			void glStencilMask(GLuint mask) override
			{
				d_version_functions->glStencilMask(mask);
			}
			void glStencilMaskSeparate(GLenum face, GLuint mask) override
			{
				d_version_functions->glStencilMaskSeparate(face, mask);
			}
			void glStencilOp(GLenum fail, GLenum zfail, GLenum zpass) override
			{
				d_version_functions->glStencilOp(fail, zfail, zpass);
			}
			void glStencilOpSeparate(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass) override
			{
				d_version_functions->glStencilOpSeparate(face, sfail, dpfail, dppass);
			}
			void glTexParameterf(GLenum target, GLenum pname, GLfloat param) override
			{
				d_version_functions->glTexParameterf(target, pname, param);
			}
			void glTexParameterfv(GLenum target, GLenum pname, const GLfloat *params) override
			{
				d_version_functions->glTexParameterfv(target, pname, params);
			}
			void glTexParameteri(GLenum target, GLenum pname, GLint param) override
			{
				d_version_functions->glTexParameteri(target, pname, param);
			}
			void glTexParameteriv(GLenum target, GLenum pname, const GLint *params) override
			{
				d_version_functions->glTexParameteriv(target, pname, params);
			}
			void glTexParameterIiv(GLenum target, GLenum pname, const GLint *params) override
			{
				d_version_functions->glTexParameterIiv(target, pname, params);
			}
			void glTexParameterIuiv(GLenum target, GLenum pname, const GLuint *params) override
			{
				d_version_functions->glTexParameterIuiv(target, pname, params);
			}
			void glTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels) override
			{
				d_version_functions->glTexSubImage1D(target, level, xoffset, width, format, type, pixels);
			}
			void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels) override
			{
				d_version_functions->glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
			}
			void glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels) override
			{
				d_version_functions->glTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
			}
			void glTexImage1D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels) override
			{
				d_version_functions->glTexImage1D(target, level, internalformat, width, border, format, type, pixels);
			}
			void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels) override
			{
				d_version_functions->glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
			}
			void glTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels) override
			{
				d_version_functions->glTexImage3D(target, level, internalformat, width, height, depth, border, format, type, pixels);
			}
			void glUniform1f(GLint location, GLfloat v0) override
			{
				d_version_functions->glUniform1f(location, v0);
			}
			void glUniform1fv(GLint location, GLsizei count, const GLfloat *value) override
			{
				d_version_functions->glUniform1fv(location, count, value);
			}
			void glUniform1i(GLint location, GLint v0) override
			{
				d_version_functions->glUniform1i(location, v0);
			}
			void glUniform1iv(GLint location, GLsizei count, const GLint *value) override
			{
				d_version_functions->glUniform1iv(location, count, value);
			}
			void glUniform1ui(GLint location, GLuint v0) override
			{
				d_version_functions->glUniform1ui(location, v0);
			}
			void glUniform1uiv(GLint location, GLsizei count, const GLuint *value) override
			{
				d_version_functions->glUniform1uiv(location, count, value);
			}
			void glUniform2f(GLint location, GLfloat v0, GLfloat v1) override
			{
				d_version_functions->glUniform2f(location, v0, v1);
			}
			void glUniform2fv(GLint location, GLsizei count, const GLfloat *value) override
			{
				d_version_functions->glUniform2fv(location, count, value);
			}
			void glUniform2i(GLint location, GLint v0, GLint v1) override
			{
				d_version_functions->glUniform2i(location, v0, v1);
			}
			void glUniform2iv(GLint location, GLsizei count, const GLint *value) override
			{
				d_version_functions->glUniform2iv(location, count, value);
			}
			void glUniform2ui(GLint location, GLuint v0, GLuint v1) override
			{
				d_version_functions->glUniform2ui(location, v0, v1);
			}
			void glUniform2uiv(GLint location, GLsizei count, const GLuint *value) override
			{
				d_version_functions->glUniform2uiv(location, count, value);
			}
			void glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2) override
			{
				d_version_functions->glUniform3f(location, v0, v1, v2);
			}
			void glUniform3fv(GLint location, GLsizei count, const GLfloat *value) override
			{
				d_version_functions->glUniform3fv(location, count, value);
			}
			void glUniform3i(GLint location, GLint v0, GLint v1, GLint v2) override
			{
				d_version_functions->glUniform3i(location, v0, v1, v2);
			}
			void glUniform3iv(GLint location, GLsizei count, const GLint *value) override
			{
				d_version_functions->glUniform3iv(location, count, value);
			}
			void glUniform3ui(GLint location, GLuint v0, GLuint v1, GLuint v2) override
			{
				d_version_functions->glUniform3ui(location, v0, v1, v2);
			}
			void glUniform3uiv(GLint location, GLsizei count, const GLuint *value) override
			{
				d_version_functions->glUniform3uiv(location, count, value);
			}
			void glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) override
			{
				d_version_functions->glUniform4f(location, v0, v1, v2, v3);
			}
			void glUniform4fv(GLint location, GLsizei count, const GLfloat *value) override
			{
				d_version_functions->glUniform4fv(location, count, value);
			}
			void glUniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3) override
			{
				d_version_functions->glUniform4i(location, v0, v1, v2, v3);
			}
			void glUniform4iv(GLint location, GLsizei count, const GLint *value) override
			{
				d_version_functions->glUniform4iv(location, count, value);
			}
			void glUniform4ui(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3) override
			{
				d_version_functions->glUniform4ui(location, v0, v1, v2, v3);
			}
			void glUniform4uiv(GLint location, GLsizei count, const GLuint *value) override
			{
				d_version_functions->glUniform4uiv(location, count, value);
			}
			void glUniformBlockBinding(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding) override
			{
				d_version_functions->glUniformBlockBinding(program, uniformBlockIndex, uniformBlockBinding);
			}
			void glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) override
			{
				d_version_functions->glUniformMatrix2fv(location, count, transpose, value);
			}
			void glUniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) override
			{
				d_version_functions->glUniformMatrix2x3fv(location, count, transpose, value);
			}
			void glUniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) override
			{
				d_version_functions->glUniformMatrix2x4fv(location, count, transpose, value);
			}
			void glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) override
			{
				d_version_functions->glUniformMatrix3fv(location, count, transpose, value);
			}
			void glUniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) override
			{
				d_version_functions->glUniformMatrix3x2fv(location, count, transpose, value);
			}
			void glUniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) override
			{
				d_version_functions->glUniformMatrix3x4fv(location, count, transpose, value);
			}
			void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) override
			{
				d_version_functions->glUniformMatrix4fv(location, count, transpose, value);
			}
			void glUniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) override
			{
				d_version_functions->glUniformMatrix4x2fv(location, count, transpose, value);
			}
			void glUniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) override
			{
				d_version_functions->glUniformMatrix4x3fv(location, count, transpose, value);
			}
			GLboolean glUnmapBuffer(GLenum target) override
			{
				return d_version_functions->glUnmapBuffer(target);
			}
			void glUseProgram(GLuint program) override
			{
				d_version_functions->glUseProgram(program);
			}
			void glValidateProgram(GLuint program) override
			{
				d_version_functions->glValidateProgram(program);
			}
			void glVertexAttribDivisor(GLuint index, GLuint divisor) override
			{
				d_version_functions->glVertexAttribDivisor(index, divisor);
			}
			void glVertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) override
			{
				d_version_functions->glVertexAttribIPointer(index, size, type, stride, pointer);
			}
			void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer) override
			{
				d_version_functions->glVertexAttribPointer(index, size, type, normalized, stride, pointer);
			}
			void glViewport(GLint x, GLint y, GLsizei width, GLsizei height) override
			{
				d_version_functions->glViewport(x, y, width, height);
			}

		private:
			VersionFunctionsType *d_version_functions;
		};


		/**
		 * Functions in OpenGL 4.0 core.
		 *
		 * Derived class contains functions introduced in OpenGL 4.0 core.
		 * Base class contains functions in OpenGL 1.0 - 3.3 core.
		 */
		template <class VersionFunctionsType>
		class OpenGLFunctions_4_0_Core :
				public OpenGLFunctions_3_3_Core<VersionFunctionsType>
		{
		public:
			explicit
			OpenGLFunctions_4_0_Core(
					VersionFunctionsType *version_functions,
					int major_version,
					int minor_version) :
				OpenGLFunctions_3_3_Core<VersionFunctionsType>(version_functions, major_version, minor_version),
				d_version_functions(version_functions)
			{  }

		private:
			VersionFunctionsType *d_version_functions;
		};


		/**
		 * Functions in OpenGL 4.1 core.
		 *
		 * Derived class contains functions introduced in OpenGL 4.1 core.
		 * Base class contains functions in OpenGL 1.0 - 4.0 core.
		 */
		template <class VersionFunctionsType>
		class OpenGLFunctions_4_1_Core :
				public OpenGLFunctions_4_0_Core<VersionFunctionsType>
		{
		public:
			explicit
			OpenGLFunctions_4_1_Core(
					VersionFunctionsType *version_functions,
					int major_version,
					int minor_version) :
				OpenGLFunctions_4_0_Core<VersionFunctionsType>(version_functions, major_version, minor_version),
				d_version_functions(version_functions)
			{  }

		private:
			VersionFunctionsType *d_version_functions;
		};


		/**
		 * Functions in OpenGL 4.2 core.
		 *
		 * Derived class contains functions introduced in OpenGL 4.2 core.
		 * Base class contains functions in OpenGL 1.0 - 4.1 core.
		 */
		template <class VersionFunctionsType>
		class OpenGLFunctions_4_2_Core :
				public OpenGLFunctions_4_1_Core<VersionFunctionsType>
		{
		public:
			explicit
			OpenGLFunctions_4_2_Core(
					VersionFunctionsType *version_functions,
					int major_version,
					int minor_version) :
				OpenGLFunctions_4_1_Core<VersionFunctionsType>(version_functions, major_version, minor_version),
				d_version_functions(version_functions)
			{  }

		private:
			VersionFunctionsType *d_version_functions;
		};

		/**
		 * Functions in OpenGL 4.3 core.
		 *
		 * Derived class contains functions introduced in OpenGL 4.3 core.
		 * Base class contains functions in OpenGL 1.0 - 4.2 core.
		 */
		template <class VersionFunctionsType>
		class OpenGLFunctions_4_3_Core :
				public OpenGLFunctions_4_2_Core<VersionFunctionsType>
		{
		public:
			explicit
			OpenGLFunctions_4_3_Core(
					VersionFunctionsType *version_functions,
					int major_version,
					int minor_version) :
				OpenGLFunctions_4_2_Core<VersionFunctionsType>(version_functions, major_version, minor_version),
				d_version_functions(version_functions)
			{  }

		private:
			VersionFunctionsType *d_version_functions;
		};
	}
}


GPlatesOpenGL::OpenGLFunctions::non_null_ptr_type
GPlatesOpenGL::OpenGLFunctions::create(
		QOpenGLFunctions_3_3_Core *version_functions)
{
	return non_null_ptr_type(new OpenGLFunctions_3_3_Core<QOpenGLFunctions_3_3_Core>(version_functions, 3, 3));
}


GPlatesOpenGL::OpenGLFunctions::non_null_ptr_type
GPlatesOpenGL::OpenGLFunctions::create(
		QOpenGLFunctions_4_0_Core *version_functions)
{
	return non_null_ptr_type(new OpenGLFunctions_4_0_Core<QOpenGLFunctions_4_0_Core>(version_functions, 4, 0));
}


GPlatesOpenGL::OpenGLFunctions::non_null_ptr_type
GPlatesOpenGL::OpenGLFunctions::create(
		QOpenGLFunctions_4_1_Core *version_functions)
{
	return non_null_ptr_type(new OpenGLFunctions_4_1_Core<QOpenGLFunctions_4_1_Core>(version_functions, 4, 1));
}


GPlatesOpenGL::OpenGLFunctions::non_null_ptr_type
GPlatesOpenGL::OpenGLFunctions::create(
		QOpenGLFunctions_4_2_Core *version_functions)
{
	return non_null_ptr_type(new OpenGLFunctions_4_2_Core<QOpenGLFunctions_4_2_Core>(version_functions, 4, 2));
}


GPlatesOpenGL::OpenGLFunctions::non_null_ptr_type
GPlatesOpenGL::OpenGLFunctions::create(
		QOpenGLFunctions_4_3_Core *version_functions)
{
	return non_null_ptr_type(new OpenGLFunctions_4_3_Core<QOpenGLFunctions_4_3_Core>(version_functions, 4, 3));
}


void
GPlatesOpenGL::OpenGLFunctions::throw_if_not_overridden(
		const char *function_name,
		int required_major_version,
		int required_minor_version)
{
	throw OpenGLException(
			GPLATES_EXCEPTION_SOURCE,
			QString("Calling '%1' requires OpenGL %2.%3 but only have OpenGL %4.%5.")
					.arg(function_name)
					.arg(required_major_version)
					.arg(required_minor_version)
					.arg(d_major_version)
					.arg(d_minor_version).toStdString());
}
