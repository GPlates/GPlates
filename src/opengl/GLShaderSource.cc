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

#include <opengl/OpenGL.h>

#include "GLShaderSource.h"

#include "file-io/ErrorOpeningFileForReadingException.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"


const char *GPlatesOpenGL::GLShaderSource::SHADER_VERSION_STRINGS[GPlatesOpenGL::GLShaderSource::NUM_SHADER_VERSIONS] =
{
	"#version 110\n",
	"#version 120\n",
	"#version 130\n",
	"#version 140\n",
	"#version 150 compatibility\n",
	"#version 330 compatibility\n",
	"#version 400 compatibility\n",
	"#version 410 compatibility\n",
	"#version 420 compatibility\n"
};


GPlatesOpenGL::GLShaderSource
GPlatesOpenGL::GLShaderSource::create_shader_source_from_file(
		const QString& shader_source_file_name,
		ShaderVersion shader_version)
{
	GLShaderSource shader_source(shader_version);
	shader_source.add_code_segment_from_file(shader_source_file_name);
	return shader_source;
}


GPlatesOpenGL::GLShaderSource::GLShaderSource(
		ShaderVersion shader_version) :
	d_shader_version(shader_version)
{
	// Add the shader version string first (it needs to come before any non-commented source code).
	add_code_segment(SHADER_VERSION_STRINGS[shader_version]);
}


GPlatesOpenGL::GLShaderSource::GLShaderSource(
		const char *shader_source,
		ShaderVersion shader_version) :
	d_shader_version(shader_version)
{
	// Add the shader version string first (it needs to come before any non-commented source code).
	add_code_segment(SHADER_VERSION_STRINGS[shader_version]);

	add_code_segment(shader_source);
}


GPlatesOpenGL::GLShaderSource::GLShaderSource(
		const QByteArray &shader_source,
		ShaderVersion shader_version) :
	d_shader_version(shader_version)
{
	// Add the shader version string first (it needs to come before any non-commented source code).
	add_code_segment(SHADER_VERSION_STRINGS[shader_version]);

	add_code_segment(shader_source);
}


void
GPlatesOpenGL::GLShaderSource::add_code_segment(
		const char *shader_source)
{
	add_code_segment(
			CodeSegment(
					QByteArray::fromRawData(shader_source, qstrlen(shader_source))));
}


void
GPlatesOpenGL::GLShaderSource::add_code_segment(
		const QByteArray &shader_source)
{
	add_code_segment(CodeSegment(shader_source));
}


void
GPlatesOpenGL::GLShaderSource::add_code_segment_from_file(
		const QString& shader_source_file_name)
{
	QFile shader_source_file(shader_source_file_name);
	if (!shader_source_file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		throw GPlatesFileIO::ErrorOpeningFileForReadingException(
				GPLATES_EXCEPTION_SOURCE,
				shader_source_file.fileName());
	}

	// Read the entire file.
	const QByteArray shader_source = shader_source_file.readAll();

	add_code_segment(CodeSegment(shader_source, shader_source_file_name));
}


void
GPlatesOpenGL::GLShaderSource::add_code_segment(
		const CodeSegment &code_segment)
{
	d_code_segments.push_back(code_segment);
}


GPlatesOpenGL::GLShaderSource::CodeSegment::CodeSegment(
		const QByteArray &source_code_,
		boost::optional<QString> source_file_name_) :
	source_code(source_code_),
	num_lines(0),
	source_file_name(source_file_name_)
{
	// Add a new line character to the last line if it doesn't end with one.
	if (!source_code.endsWith('\n'))
	{
		source_code.append('\n');
	}

	// Count the number of lines in the shader source segment.
	num_lines = source_code.count('\n');

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			num_lines >= 1,
			GPLATES_ASSERTION_SOURCE);
}
