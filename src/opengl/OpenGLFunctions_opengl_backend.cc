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

#include "OpenGLFunctions.h"


void GPlatesOpenGL::OpenGLFunctions::glAttachShader(GLuint program, GLuint shader)
{
	d_functions->glAttachShader(program, shader);
}

void GPlatesOpenGL::OpenGLFunctions::glBindAttribLocation(GLuint program, GLuint index, const GLchar* name)
{
	d_functions->glBindAttribLocation(program, index, name);
}

void GPlatesOpenGL::OpenGLFunctions::glBindBuffer(GLenum target, GLuint buffer)
{
	d_functions->glBindBuffer(target, buffer);
}

void GPlatesOpenGL::OpenGLFunctions::glBindBufferBase(GLenum target, GLuint index, GLuint buffer)
{
	d_functions->glBindBufferBase(target, index, buffer);
}

void GPlatesOpenGL::OpenGLFunctions::glBindBufferRange(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size)
{
	d_functions->glBindBufferRange(target, index, buffer, offset, size);
}

void GPlatesOpenGL::OpenGLFunctions::glBindFramebuffer(GLenum target, GLuint framebuffer)
{
	d_functions->glBindFramebuffer(target, framebuffer);
}

void GPlatesOpenGL::OpenGLFunctions::glBindImageTexture(GLuint image_unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format)
{
	d_functions->glBindImageTexture(image_unit, texture, level, layered, layer, access, format);
}

void GPlatesOpenGL::OpenGLFunctions::glBindRenderbuffer(GLenum target, GLuint renderbuffer)
{
	d_functions->glBindRenderbuffer(target, renderbuffer);
}

void GPlatesOpenGL::OpenGLFunctions::glBindSampler(GLuint unit, GLuint sampler)
{
	d_functions->glBindSampler(unit, sampler);
}

void GPlatesOpenGL::OpenGLFunctions::glBindTextureUnit(GLuint unit, GLuint texture)
{
	d_functions->glBindTextureUnit(unit, texture);
}

void GPlatesOpenGL::OpenGLFunctions::glBindVertexArray(GLuint array)
{
	d_functions->glBindVertexArray(array);
}

void GPlatesOpenGL::OpenGLFunctions::glBlendColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	d_functions->glBlendColor(red, green, blue, alpha);
}

void GPlatesOpenGL::OpenGLFunctions::glBlendEquation(GLenum mode)
{
	d_functions->glBlendEquation(mode);
}

void GPlatesOpenGL::OpenGLFunctions::glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha)
{
	d_functions->glBlendEquationSeparate(modeRGB, modeAlpha);
}

void GPlatesOpenGL::OpenGLFunctions::glBlendFunc(GLenum sfactor, GLenum dfactor)
{
	d_functions->glBlendFunc(sfactor, dfactor);
}

void GPlatesOpenGL::OpenGLFunctions::glBlendFuncSeparate(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha)
{
	d_functions->glBlendFuncSeparate(sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha);
}

void GPlatesOpenGL::OpenGLFunctions::glBufferData(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage)
{
	d_functions->glBufferData(target, size, data, usage);
}

void GPlatesOpenGL::OpenGLFunctions::glBufferStorage(GLenum target, GLsizeiptr size, const void* data, GLbitfield flags)
{
	d_functions->glBufferStorage(target, size, data, flags);
}

void GPlatesOpenGL::OpenGLFunctions::glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data)
{
	d_functions->glBufferSubData(target, offset, size, data);
}

GLenum GPlatesOpenGL::OpenGLFunctions::glCheckFramebufferStatus(GLenum target)
{
	return d_functions->glCheckFramebufferStatus(target);
}

void GPlatesOpenGL::OpenGLFunctions::glClampColor(GLenum target, GLenum clamp)
{
	d_functions->glClampColor(target, clamp);
}

void GPlatesOpenGL::OpenGLFunctions::glClear(GLbitfield mask)
{
	d_functions->glClear(mask);
}

void GPlatesOpenGL::OpenGLFunctions::glClearBufferData(GLenum target, GLenum internalformat, GLenum format, GLenum type, const void *data)
{
	d_functions->glClearBufferData(target, internalformat, format, type, data);
}

void GPlatesOpenGL::OpenGLFunctions::glClearBufferSubData(GLenum target, GLenum internalformat, GLintptr offset, GLsizeiptr size, GLenum format, GLenum type, const void *data)
{
	d_functions->glClearBufferSubData(target, internalformat, offset, size, format, type, data);
}

void GPlatesOpenGL::OpenGLFunctions::glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	d_functions->glClearColor(red, green, blue, alpha);
}

void GPlatesOpenGL::OpenGLFunctions::glClearDepth(GLdouble depth)
{
	d_functions->glClearDepth(depth);
}

void GPlatesOpenGL::OpenGLFunctions::glClearNamedBufferData(GLuint buffer, GLenum internalformat, GLenum format, GLenum type, const void *data)
{
	d_functions->glClearNamedBufferData(buffer, internalformat, format, type, data);
}

void GPlatesOpenGL::OpenGLFunctions::glClearNamedBufferSubData(GLuint buffer, GLenum internalformat, GLintptr offset, GLsizei size, GLenum format, GLenum type, const void *data)
{
	d_functions->glClearNamedBufferSubData(buffer, internalformat, offset, size, format, type, data);
}

void GPlatesOpenGL::OpenGLFunctions::glClearStencil(GLint s)
{
	d_functions->glClearStencil(s);
}

void GPlatesOpenGL::OpenGLFunctions::glClearTexSubImage(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *data)
{
	d_functions->glClearTexSubImage(texture, level, xoffset, yoffset, zoffset, width, height, depth, format, type, data);
}

void GPlatesOpenGL::OpenGLFunctions::glClearTexImage(GLuint texture, GLint level, GLenum format, GLenum type, const void *data)
{
	d_functions->glClearTexImage(texture, level, format, type, data);
}

void GPlatesOpenGL::OpenGLFunctions::glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
	d_functions->glColorMask(red, green, blue, alpha);
}

void GPlatesOpenGL::OpenGLFunctions::glColorMaski(GLuint index, GLboolean r, GLboolean g, GLboolean b, GLboolean a)
{
	d_functions->glColorMaski(index, r, g, b, a);
}

void GPlatesOpenGL::OpenGLFunctions::glCompileShader(GLuint shader)
{
	d_functions->glCompileShader(shader);
}

void GPlatesOpenGL::OpenGLFunctions::glCreateBuffers(GLsizei n, GLuint* buffers)
{
	d_functions->glCreateBuffers(n, buffers);
}

void GPlatesOpenGL::OpenGLFunctions::glCreateFramebuffers(GLsizei n, GLuint* framebuffers)
{
	d_functions->glCreateFramebuffers(n, framebuffers);
}

GLuint GPlatesOpenGL::OpenGLFunctions::glCreateProgram()
{
	return d_functions->glCreateProgram();
}

void GPlatesOpenGL::OpenGLFunctions::glCreateRenderbuffers(GLsizei n, GLuint* renderbuffers)
{
	d_functions->glCreateRenderbuffers(n, renderbuffers);
}

void GPlatesOpenGL::OpenGLFunctions::glCreateSamplers(GLsizei count, GLuint* samplers)
{
	d_functions->glCreateSamplers(count, samplers);
}

GLuint GPlatesOpenGL::OpenGLFunctions::glCreateShader(GLenum type)
{
	return d_functions->glCreateShader(type);
}

void GPlatesOpenGL::OpenGLFunctions::glCreateTextures(GLenum target, GLsizei n, GLuint* textures)
{
	d_functions->glCreateTextures(target, n, textures);
}

void GPlatesOpenGL::OpenGLFunctions::glCreateVertexArrays(GLsizei n, GLuint* arrays)
{
	d_functions->glCreateVertexArrays(n, arrays);
}

void GPlatesOpenGL::OpenGLFunctions::glCullFace(GLenum mode)
{
	d_functions->glCullFace(mode);
}

void GPlatesOpenGL::OpenGLFunctions::glDeleteBuffers(GLsizei n, const GLuint* buffers)
{
	d_functions->glDeleteBuffers(n, buffers);
}

void GPlatesOpenGL::OpenGLFunctions::glDeleteFramebuffers(GLsizei n, const GLuint* framebuffers)
{
	d_functions->glDeleteFramebuffers(n, framebuffers);
}

void GPlatesOpenGL::OpenGLFunctions::glDeleteProgram(GLuint program)
{
	d_functions->glDeleteProgram(program);
}

void GPlatesOpenGL::OpenGLFunctions::glDeleteRenderbuffers(GLsizei n, const GLuint* renderbuffers)
{
	d_functions->glDeleteRenderbuffers(n, renderbuffers);
}

void GPlatesOpenGL::OpenGLFunctions::glDeleteSamplers(GLsizei count, const GLuint* samplers)
{
	d_functions->glDeleteSamplers(count, samplers);
}

void GPlatesOpenGL::OpenGLFunctions::glDeleteShader(GLuint shader)
{
	d_functions->glDeleteShader(shader);
}

void GPlatesOpenGL::OpenGLFunctions::glDeleteTextures(GLsizei n, const GLuint* textures)
{
	d_functions->glDeleteTextures(n, textures);
}

void GPlatesOpenGL::OpenGLFunctions::glDeleteVertexArrays(GLsizei n, const GLuint* arrays)
{
	d_functions->glDeleteVertexArrays(n, arrays);
}

void GPlatesOpenGL::OpenGLFunctions::glDepthFunc(GLenum func)
{
	d_functions->glDepthFunc(func);
}

void GPlatesOpenGL::OpenGLFunctions::glDepthMask(GLboolean flag)
{
	d_functions->glDepthMask(flag);
}

void GPlatesOpenGL::OpenGLFunctions::glDepthRange(GLdouble nearVal, GLdouble farVal)
{
	d_functions->glDepthRange(nearVal, farVal);
}

void GPlatesOpenGL::OpenGLFunctions::glDetachShader(GLuint program, GLuint shader)
{
	d_functions->glDetachShader(program, shader);
}

void GPlatesOpenGL::OpenGLFunctions::glDisable(GLenum cap)
{
	d_functions->glDisable(cap);
}

void GPlatesOpenGL::OpenGLFunctions::glDisablei(GLenum target, GLuint index)
{
	d_functions->glDisablei(target, index);
}

void GPlatesOpenGL::OpenGLFunctions::glDisableVertexAttribArray(GLuint index)
{
	d_functions->glDisableVertexAttribArray(index);
}

void GPlatesOpenGL::OpenGLFunctions::glDrawArrays(GLenum mode, GLint first, GLsizei count)
{
	d_functions->glDrawArrays(mode, first, count);
}

void GPlatesOpenGL::OpenGLFunctions::glDrawBuffer(GLenum mode)
{
	d_functions->glDrawBuffer(mode);
}

void GPlatesOpenGL::OpenGLFunctions::glDrawBuffers(GLsizei n, const GLenum* bufs)
{
	d_functions->glDrawBuffers(n, bufs);
}

void GPlatesOpenGL::OpenGLFunctions::glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices)
{
	d_functions->glDrawElements(mode, count, type, indices);
}

void GPlatesOpenGL::OpenGLFunctions::glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid* indices)
{
	d_functions->glDrawRangeElements(mode, start, end, count, type, indices);
}

void GPlatesOpenGL::OpenGLFunctions::glEnable(GLenum cap)
{
	d_functions->glEnable(cap);
}

void GPlatesOpenGL::OpenGLFunctions::glEnablei(GLenum target, GLuint index)
{
	d_functions->glEnablei(target, index);
}

void GPlatesOpenGL::OpenGLFunctions::glEnableVertexAttribArray(GLuint index)
{
	d_functions->glEnableVertexAttribArray(index);
}

void GPlatesOpenGL::OpenGLFunctions::glFlushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length)
{
	d_functions->glFlushMappedBufferRange(target, offset, length);
}

void GPlatesOpenGL::OpenGLFunctions::glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
{
	d_functions->glFramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer);
}

void GPlatesOpenGL::OpenGLFunctions::glFramebufferTexture1D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
	d_functions->glFramebufferTexture1D(target, attachment, textarget, texture, level);
}

void GPlatesOpenGL::OpenGLFunctions::glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
	d_functions->glFramebufferTexture2D(target, attachment, textarget, texture, level);
}

void GPlatesOpenGL::OpenGLFunctions::glFramebufferTexture3D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset)
{
	d_functions->glFramebufferTexture3D(target, attachment, textarget, texture, level, zoffset);
}

void GPlatesOpenGL::OpenGLFunctions::glFramebufferTexture(GLenum target, GLenum attachment, GLuint texture, GLint level)
{
	d_functions->glFramebufferTexture(target, attachment, texture, level);
}

void GPlatesOpenGL::OpenGLFunctions::glFramebufferTextureLayer(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer)
{
	d_functions->glFramebufferTextureLayer(target, attachment, texture, level, layer);
}

void GPlatesOpenGL::OpenGLFunctions::glFrontFace(GLenum mode)
{
	d_functions->glFrontFace(mode);
}

GLenum GPlatesOpenGL::OpenGLFunctions::glGetError()
{
	return d_functions->glGetError();
}

void GPlatesOpenGL::OpenGLFunctions::glGetIntegerv(GLenum pname, GLint* params)
{
	d_functions->glGetIntegerv(pname, params);
}

void GPlatesOpenGL::OpenGLFunctions::glGetInteger64v(GLenum pname, GLint64* params)
{
	d_functions->glGetInteger64v(pname, params);
}

void GPlatesOpenGL::OpenGLFunctions::glGetProgramiv(GLuint program, GLenum pname, GLint* params)
{
	d_functions->glGetProgramiv(program, pname, params);
}

void GPlatesOpenGL::OpenGLFunctions::glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog)
{
	d_functions->glGetProgramInfoLog(program, bufSize, length, infoLog);
}

void GPlatesOpenGL::OpenGLFunctions::glGetShaderiv(GLuint shader, GLenum pname, GLint* params)
{
	d_functions->glGetShaderiv(shader, pname, params);
}

void GPlatesOpenGL::OpenGLFunctions::glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog)
{
	d_functions->glGetShaderInfoLog(shader, bufSize, length, infoLog);
}

void GPlatesOpenGL::OpenGLFunctions::glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, GLvoid* pixels)
{
	d_functions->glGetTexImage(target, level, format, type, pixels);
}

GLuint GPlatesOpenGL::OpenGLFunctions::glGetUniformBlockIndex(GLuint program, const GLchar *uniformBlockName)
{
	return d_functions->glGetUniformBlockIndex(program, uniformBlockName);
}

GLint GPlatesOpenGL::OpenGLFunctions::glGetUniformLocation(GLuint program, const GLchar *name)
{
	return d_functions->glGetUniformLocation(program, name);
}

void GPlatesOpenGL::OpenGLFunctions::glHint(GLenum target, GLenum mode)
{
	d_functions->glHint(target, mode);
}

void GPlatesOpenGL::OpenGLFunctions::glLineWidth(GLfloat width)
{
	d_functions->glLineWidth(width);
}

void GPlatesOpenGL::OpenGLFunctions::glLinkProgram(GLuint program)
{
	d_functions->glLinkProgram(program);
}

GLvoid * GPlatesOpenGL::OpenGLFunctions::glMapBuffer(GLenum target, GLenum access)
{
	return d_functions->glMapBuffer(target, access);
}

GLvoid * GPlatesOpenGL::OpenGLFunctions::glMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access)
{
	return d_functions->glMapBufferRange(target, offset, length, access);
}

void GPlatesOpenGL::OpenGLFunctions::glMemoryBarrier(GLbitfield barriers)
{
	d_functions->glMemoryBarrier(barriers);
}

void GPlatesOpenGL::OpenGLFunctions::glMemoryBarrierByRegion(GLbitfield barriers)
{
	d_functions->glMemoryBarrierByRegion(barriers);
}

void GPlatesOpenGL::OpenGLFunctions::glNamedBufferStorage(GLuint buffer, GLsizei size, const void* data, GLbitfield flags)
{
	d_functions->glNamedBufferStorage(buffer, size, data, flags);
}

void GPlatesOpenGL::OpenGLFunctions::glNamedBufferSubData(GLuint buffer, GLintptr offset, GLsizei size, const void* data)
{
	d_functions->glNamedBufferSubData(buffer, offset, size, data);
}

void GPlatesOpenGL::OpenGLFunctions::glPixelStorei(GLenum pname, GLint param)
{
	d_functions->glPixelStorei(pname, param);
}

void GPlatesOpenGL::OpenGLFunctions::glPointSize(GLfloat size)
{
	d_functions->glPointSize(size);
}

void GPlatesOpenGL::OpenGLFunctions::glPolygonMode(GLenum face, GLenum mode)
{
	d_functions->glPolygonMode(face, mode);
}

void GPlatesOpenGL::OpenGLFunctions::glPolygonOffset(GLfloat factor, GLfloat units)
{
	d_functions->glPolygonOffset(factor, units);
}

void GPlatesOpenGL::OpenGLFunctions::glPrimitiveRestartIndex(GLuint index)
{
	d_functions->glPrimitiveRestartIndex(index);
}

void GPlatesOpenGL::OpenGLFunctions::glReadBuffer(GLenum mode)
{
	d_functions->glReadBuffer(mode);
}

void GPlatesOpenGL::OpenGLFunctions::glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels)
{
	d_functions->glReadPixels(x, y, width, height, format, type, pixels);
}

void GPlatesOpenGL::OpenGLFunctions::glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
{
	d_functions->glRenderbufferStorage(target, internalformat, width, height);
}

void GPlatesOpenGL::OpenGLFunctions::glSampleCoverage(GLfloat value, GLboolean invert)
{
	d_functions->glSampleCoverage(value, invert);
}

void GPlatesOpenGL::OpenGLFunctions::glSampleMaski(GLuint index, GLbitfield mask)
{
	d_functions->glSampleMaski(index, mask);
}

void GPlatesOpenGL::OpenGLFunctions::glSamplerParameterf(GLuint sampler, GLenum pname, GLfloat param)
{
	d_functions->glSamplerParameterf(sampler, pname, param);
}

void GPlatesOpenGL::OpenGLFunctions::glSamplerParameterfv(GLuint sampler, GLenum pname, const GLfloat* param)
{
	d_functions->glSamplerParameterfv(sampler, pname, param);
}

void GPlatesOpenGL::OpenGLFunctions::glSamplerParameteri(GLuint sampler, GLenum pname, GLint param)
{
	d_functions->glSamplerParameteri(sampler, pname, param);
}

void GPlatesOpenGL::OpenGLFunctions::glSamplerParameteriv(GLuint sampler, GLenum pname, const GLint* param)
{
	d_functions->glSamplerParameteriv(sampler, pname, param);
}

void GPlatesOpenGL::OpenGLFunctions::glSamplerParameterIiv(GLuint sampler, GLenum pname, const GLint* param)
{
	d_functions->glSamplerParameterIiv(sampler, pname, param);
}

void GPlatesOpenGL::OpenGLFunctions::glSamplerParameterIuiv(GLuint sampler, GLenum pname, const GLuint* param)
{
	d_functions->glSamplerParameterIuiv(sampler, pname, param);
}

void GPlatesOpenGL::OpenGLFunctions::glScissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
	d_functions->glScissor(x, y, width, height);
}

void GPlatesOpenGL::OpenGLFunctions::glShaderStorageBlockBinding(GLuint program, GLuint storageBlockIndex, GLuint storageBlockBinding)
{
	d_functions->glShaderStorageBlockBinding(program, storageBlockIndex, storageBlockBinding);
}

void GPlatesOpenGL::OpenGLFunctions::glShaderSource(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length)
{
	d_functions->glShaderSource(shader, count, string, length);
}

void GPlatesOpenGL::OpenGLFunctions::glStencilFunc(GLenum func, GLint ref, GLuint mask)
{
	d_functions->glStencilFunc(func, ref, mask);
}

void GPlatesOpenGL::OpenGLFunctions::glStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask)
{
	d_functions->glStencilFuncSeparate(face, func, ref, mask);
}

void GPlatesOpenGL::OpenGLFunctions::glStencilMask(GLuint mask)
{
	d_functions->glStencilMask(mask);
}

void GPlatesOpenGL::OpenGLFunctions::glStencilMaskSeparate(GLenum face, GLuint mask)
{
	d_functions->glStencilMaskSeparate(face, mask);
}

void GPlatesOpenGL::OpenGLFunctions::glStencilOp(GLenum fail, GLenum zfail, GLenum zpass)
{
	d_functions->glStencilOp(fail, zfail, zpass);
}

void GPlatesOpenGL::OpenGLFunctions::glStencilOpSeparate(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass)
{
	d_functions->glStencilOpSeparate(face, sfail, dpfail, dppass);
}

void GPlatesOpenGL::OpenGLFunctions::glTexParameterf(GLenum target, GLenum pname, GLfloat param)
{
	d_functions->glTexParameterf(target, pname, param);
}

void GPlatesOpenGL::OpenGLFunctions::glTexParameterfv(GLenum target, GLenum pname, const GLfloat* params)
{
	d_functions->glTexParameterfv(target, pname, params);
}

void GPlatesOpenGL::OpenGLFunctions::glTexParameteri(GLenum target, GLenum pname, GLint param)
{
	d_functions->glTexParameteri(target, pname, param);
}

void GPlatesOpenGL::OpenGLFunctions::glTexParameteriv(GLenum target, GLenum pname, const GLint* params)
{
	d_functions->glTexParameteriv(target, pname, params);
}

void GPlatesOpenGL::OpenGLFunctions::glTexParameterIiv(GLenum target, GLenum pname, const GLint* params)
{
	d_functions->glTexParameterIiv(target, pname, params);
}

void GPlatesOpenGL::OpenGLFunctions::glTexParameterIuiv(GLenum target, GLenum pname, const GLuint* params)
{
	d_functions->glTexParameterIuiv(target, pname, params);
}

void GPlatesOpenGL::OpenGLFunctions::glTexStorage1D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width)
{
	d_functions->glTexStorage1D(target, levels, internalformat, width);
}

void GPlatesOpenGL::OpenGLFunctions::glTexStorage2D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height)
{
	d_functions->glTexStorage2D(target, levels, internalformat, width, height);
}

void GPlatesOpenGL::OpenGLFunctions::glTexStorage3D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth)
{
	d_functions->glTexStorage3D(target, levels, internalformat, width, height, depth);
}

void GPlatesOpenGL::OpenGLFunctions::glTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid* pixels)
{
	d_functions->glTexSubImage1D(target, level, xoffset, width, format, type, pixels);
}

void GPlatesOpenGL::OpenGLFunctions::glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels)
{
	d_functions->glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
}

void GPlatesOpenGL::OpenGLFunctions::glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid* pixels)
{
	d_functions->glTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
}

void GPlatesOpenGL::OpenGLFunctions::glTexImage1D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid* pixels)
{
	d_functions->glTexImage1D(target, level, internalformat, width, border, format, type, pixels);
}

void GPlatesOpenGL::OpenGLFunctions::glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* pixels)
{
	d_functions->glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
}

void GPlatesOpenGL::OpenGLFunctions::glTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid* pixels)
{
	d_functions->glTexImage3D(target, level, internalformat, width, height, depth, border, format, type, pixels);
}

void GPlatesOpenGL::OpenGLFunctions::glTextureBuffer(GLuint texture, GLenum internalformat, GLuint buffer)
{
	d_functions->glTextureBuffer(texture, internalformat, buffer);
}

void GPlatesOpenGL::OpenGLFunctions::glTextureBufferRange(GLuint texture, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizei size)
{
	d_functions->glTextureBufferRange(texture, internalformat, buffer, offset, size);
}

void GPlatesOpenGL::OpenGLFunctions::glUniform1f(GLint location, GLfloat v0)
{
	d_functions->glUniform1f(location, v0);
}

void GPlatesOpenGL::OpenGLFunctions::glUniform1fv(GLint location, GLsizei count, const GLfloat* value)
{
	d_functions->glUniform1fv(location, count, value);
}

void GPlatesOpenGL::OpenGLFunctions::glUniform1i(GLint location, GLint v0)
{
	d_functions->glUniform1i(location, v0);
}

void GPlatesOpenGL::OpenGLFunctions::glUniform1iv(GLint location, GLsizei count, const GLint* value)
{
	d_functions->glUniform1iv(location, count, value);
}

void GPlatesOpenGL::OpenGLFunctions::glUniform1ui(GLint location, GLuint v0)
{
	d_functions->glUniform1ui(location, v0);
}

void GPlatesOpenGL::OpenGLFunctions::glUniform1uiv(GLint location, GLsizei count, const GLuint* value)
{
	d_functions->glUniform1uiv(location, count, value);
}

void GPlatesOpenGL::OpenGLFunctions::glUniform2f(GLint location, GLfloat v0, GLfloat v1)
{
	d_functions->glUniform2f(location, v0, v1);
}

void GPlatesOpenGL::OpenGLFunctions::glUniform2fv(GLint location, GLsizei count, const GLfloat* value)
{
	d_functions->glUniform2fv(location, count, value);
}

void GPlatesOpenGL::OpenGLFunctions::glUniform2i(GLint location, GLint v0, GLint v1)
{
	d_functions->glUniform2i(location, v0, v1);
}

void GPlatesOpenGL::OpenGLFunctions::glUniform2iv(GLint location, GLsizei count, const GLint* value)
{
	d_functions->glUniform2iv(location, count, value);
}

void GPlatesOpenGL::OpenGLFunctions::glUniform2ui(GLint location, GLuint v0, GLuint v1)
{
	d_functions->glUniform2ui(location, v0, v1);
}

void GPlatesOpenGL::OpenGLFunctions::glUniform2uiv(GLint location, GLsizei count, const GLuint* value)
{
	d_functions->glUniform2uiv(location, count, value);
}

void GPlatesOpenGL::OpenGLFunctions::glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2)
{
	d_functions->glUniform3f(location, v0, v1, v2);
}

void GPlatesOpenGL::OpenGLFunctions::glUniform3fv(GLint location, GLsizei count, const GLfloat* value)
{
	d_functions->glUniform3fv(location, count, value);
}

void GPlatesOpenGL::OpenGLFunctions::glUniform3i(GLint location, GLint v0, GLint v1, GLint v2)
{
	d_functions->glUniform3i(location, v0, v1, v2);
}

void GPlatesOpenGL::OpenGLFunctions::glUniform3iv(GLint location, GLsizei count, const GLint* value)
{
	d_functions->glUniform3iv(location, count, value);
}

void GPlatesOpenGL::OpenGLFunctions::glUniform3ui(GLint location, GLuint v0, GLuint v1, GLuint v2)
{
	d_functions->glUniform3ui(location, v0, v1, v2);
}

void GPlatesOpenGL::OpenGLFunctions::glUniform3uiv(GLint location, GLsizei count, const GLuint* value)
{
	d_functions->glUniform3uiv(location, count, value);
}

void GPlatesOpenGL::OpenGLFunctions::glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3)
{
	d_functions->glUniform4f(location, v0, v1, v2, v3);
}

void GPlatesOpenGL::OpenGLFunctions::glUniform4fv(GLint location, GLsizei count, const GLfloat* value)
{
	d_functions->glUniform4fv(location, count, value);
}

void GPlatesOpenGL::OpenGLFunctions::glUniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3)
{
	d_functions->glUniform4i(location, v0, v1, v2, v3);
}

void GPlatesOpenGL::OpenGLFunctions::glUniform4iv(GLint location, GLsizei count, const GLint* value)
{
	d_functions->glUniform4iv(location, count, value);
}

void GPlatesOpenGL::OpenGLFunctions::glUniform4ui(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3)
{
	d_functions->glUniform4ui(location, v0, v1, v2, v3);
}

void GPlatesOpenGL::OpenGLFunctions::glUniform4uiv(GLint location, GLsizei count, const GLuint* value)
{
	d_functions->glUniform4uiv(location, count, value);
}

void GPlatesOpenGL::OpenGLFunctions::glUniformBlockBinding(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding)
{
	d_functions->glUniformBlockBinding(program, uniformBlockIndex, uniformBlockBinding);
}

void GPlatesOpenGL::OpenGLFunctions::glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
	d_functions->glUniformMatrix2fv(location, count, transpose, value);
}

void GPlatesOpenGL::OpenGLFunctions::glUniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
	d_functions->glUniformMatrix2x3fv(location, count, transpose, value);
}

void GPlatesOpenGL::OpenGLFunctions::glUniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
	d_functions->glUniformMatrix2x4fv(location, count, transpose, value);
}

void GPlatesOpenGL::OpenGLFunctions::glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
	d_functions->glUniformMatrix3fv(location, count, transpose, value);
}

void GPlatesOpenGL::OpenGLFunctions::glUniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
	d_functions->glUniformMatrix3x2fv(location, count, transpose, value);
}

void GPlatesOpenGL::OpenGLFunctions::glUniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
	d_functions->glUniformMatrix3x4fv(location, count, transpose, value);
}

void GPlatesOpenGL::OpenGLFunctions::glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
	d_functions->glUniformMatrix4fv(location, count, transpose, value);
}

void GPlatesOpenGL::OpenGLFunctions::glUniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
	d_functions->glUniformMatrix4x2fv(location, count, transpose, value);
}

void GPlatesOpenGL::OpenGLFunctions::glUniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
	d_functions->glUniformMatrix4x3fv(location, count, transpose, value);
}

GLboolean GPlatesOpenGL::OpenGLFunctions::glUnmapBuffer(GLenum target)
{
	return d_functions->glUnmapBuffer(target);
}

void GPlatesOpenGL::OpenGLFunctions::glUseProgram(GLuint program)
{
	d_functions->glUseProgram(program);
}

void GPlatesOpenGL::OpenGLFunctions::glValidateProgram(GLuint program)
{
	d_functions->glValidateProgram(program);
}

void GPlatesOpenGL::OpenGLFunctions::glVertexAttribDivisor(GLuint index, GLuint divisor)
{
	d_functions->glVertexAttribDivisor(index, divisor);
}

void GPlatesOpenGL::OpenGLFunctions::glVertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const GLvoid* pointer)
{
	d_functions->glVertexAttribIPointer(index, size, type, stride, pointer);
}

void GPlatesOpenGL::OpenGLFunctions::glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* pointer)
{
	d_functions->glVertexAttribPointer(index, size, type, normalized, stride, pointer);
}

void GPlatesOpenGL::OpenGLFunctions::glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
	d_functions->glViewport(x, y, width, height);
}
