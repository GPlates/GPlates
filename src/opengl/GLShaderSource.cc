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
	d_shader_version(shader_version),
	d_initial_code_segment(SHADER_VERSION_STRINGS[shader_version])
{
}


GPlatesOpenGL::GLShaderSource::GLShaderSource(
		const char *shader_source,
		ShaderVersion shader_version) :
	d_shader_version(shader_version),
	d_initial_code_segment(SHADER_VERSION_STRINGS[shader_version])
{
	add_code_segment(shader_source);
}


GPlatesOpenGL::GLShaderSource::GLShaderSource(
		const QByteArray &shader_source,
		ShaderVersion shader_version) :
	d_shader_version(shader_version),
	d_initial_code_segment(SHADER_VERSION_STRINGS[shader_version])
{
	add_code_segment(shader_source);
}


void
GPlatesOpenGL::GLShaderSource::add_code_segment(
		const char *shader_source)
{
	add_processed_code_segment(QByteArray(shader_source));
}


void
GPlatesOpenGL::GLShaderSource::add_code_segment(
		const QByteArray &shader_source)
{
	add_processed_code_segment(shader_source);
}


void
GPlatesOpenGL::GLShaderSource::add_code_segment_from_file(
		const QString& shader_source_file_name)
{
	QFile shader_source_file(shader_source_file_name);
	// The QIODevice::Text flag tells 'open()' to convert "\r\n" into "\n".
	if (!shader_source_file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		throw GPlatesFileIO::ErrorOpeningFileForReadingException(
				GPLATES_EXCEPTION_SOURCE,
				shader_source_file.fileName());
	}

	// Read the entire file.
	const QByteArray shader_source = shader_source_file.readAll();

	add_processed_code_segment(shader_source, shader_source_file_name);
}


void
GPlatesOpenGL::GLShaderSource::add_processed_code_segment(
		QByteArray source_code,
		boost::optional<QString> source_file_name)
{
	// Add a new line character to the last line if it doesn't end with one.
	if (!source_code.endsWith('\n'))
	{
		source_code.append('\n');
	}

	// Search for lines beginning with "#extension".
	//
	// We'll copy these to the initial code segment and comment then out in the current code segment.
	// This is because #extension must not occur *after* any non-preprocessor source code.
	int extension_index = 0;
	while (true)
	{
		extension_index = source_code.indexOf("#extension", extension_index);
		if (extension_index < 0)
		{
			break;
		}

		// Find start of the line containing "#extension".
		const int prev_newline_index = source_code.lastIndexOf('\n', extension_index);
		const int line_start_index = (prev_newline_index >= 0)
				? prev_newline_index + 1  // Skip the previous newline.
				: 0;  // We're on the first line of source code.

		// Find newline ending the current line.
		int next_newline_index = source_code.indexOf('\n', extension_index);
		// Should always find a newline since we ensured source code ended with a newline.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				next_newline_index >= 0,
				GPLATES_ASSERTION_SOURCE);

		// If there's only whitespace characters before "#extension" on the current line then
		// we'll assume the extension hasn't been commented out.
		//
		// TODO: It's still possible that a multi-line /**/ style comment could comment out the extension.
		// But would need more advanced parsing to handle this.
		if (source_code.mid(line_start_index, extension_index - line_start_index).trimmed().isEmpty())
		{
			// Extract the "#extension" line, including the ending newline.
			const QByteArray extension_line = source_code.mid(
					line_start_index,
					next_newline_index - line_start_index + 1);  // Include the newline.

			// Append the extension line to the initial code segment.
			//
			// This also puts it after the "#version" line and any "#extension" lines added so far.
			d_initial_code_segment.source_code.append(extension_line);
			++d_initial_code_segment.num_lines;

			// Comment out the current line now that we've copied it.
			source_code.insert(line_start_index, "//");
			// Account for inserting the comment.
			next_newline_index += 2;
			extension_index += 2;
		}

		// Continue searching, starting at the next line.
		extension_index = next_newline_index + 1;
	}

	// Add processed shader source code as a new code segment.
	d_added_code_segments.push_back(
			CodeSegment(source_code, source_file_name));
}


std::vector<GPlatesOpenGL::GLShaderSource::CodeSegment>
GPlatesOpenGL::GLShaderSource::get_code_segments() const
{
	std::vector<GPlatesOpenGL::GLShaderSource::CodeSegment> code_segments;
	code_segments.reserve(1 + d_added_code_segments.size());

	// Add initial code segment followed by the client's code segments.
	//
	// We don't need to worry about expensive copying of code segment source strings
	// because we're using QByteArray for that and it uses implicit sharing for copies.
	code_segments.push_back(d_initial_code_segment);
	code_segments.insert(
			code_segments.end(),
			d_added_code_segments.begin(),
			d_added_code_segments.end());

	return code_segments;
}


GPlatesOpenGL::GLShaderSource::CodeSegment::CodeSegment(
		const QByteArray &source_code_,
		boost::optional<QString> source_file_name_) :
	source_code(source_code_),
	num_lines(source_code_.count('\n')),  // Count number of lines.
	source_file_name(source_file_name_)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			num_lines >= 1,
			GPLATES_ASSERTION_SOURCE);
}
