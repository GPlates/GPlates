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

#include <QFile>

#include "VulkanUtils.h"

#include "file-io/ErrorOpeningFileForReadingException.h"


std::vector<std::uint32_t>
GPlatesOpenGL::VulkanUtils::load_shader_code(
		QString shader_filename)
{
	QFile shader_file(shader_filename);
	if (!shader_file.open(QIODevice::ReadOnly))
	{
		throw GPlatesFileIO::ErrorOpeningFileForReadingException(
				GPLATES_EXCEPTION_SOURCE,
				shader_file.fileName());
	}

	// Read the entire file.
	const QByteArray shader_source_as_bytes = shader_file.readAll();

	// SPIR-V binary files should be a multiple of 4 bytes.
	const unsigned int num_ints_in_shader_source = shader_source_as_bytes.size() / sizeof(std::uint32_t);
	const std::uint32_t *shader_source_as_ints = reinterpret_cast<const std::uint32_t *>(shader_source_as_bytes.constData());

	// Copy bytes into ints.
	const std::vector<std::uint32_t> shader_code(
			shader_source_as_ints,
			shader_source_as_ints + num_ints_in_shader_source);

	return shader_code;
}


GPlatesOpenGL::GLMatrix
GPlatesOpenGL::VulkanUtils::from_opengl_clip_space()
{
	// Matrix used to pre-multiply OpenGL projection transforms before passing them to Vulkan.
	static const double FROM_OPENGL_CLIP_SPACE[16] =
	{
		1.0,  0.0,  0.0,  0.0,
		// Flip the 'y' direction since the Vulkan viewport is top-to-bottom (instead of bottom-to-top in OpenGL)...
		0.0, -1.0,  0.0,  0.0,
		// Convert NDC 'z' from [-1, 1] to [0, 1] since Vulkan's viewport transform expects NDC 'z' to be in range [0, 1]...
		0.0,  0.0,  0.5,  0.5,
		0.0,  0.0,  0.0,  1.0
	};
	static const GLMatrix FROM_OPENGL_CLIP_SPACE_MATRIX(FROM_OPENGL_CLIP_SPACE);

	return FROM_OPENGL_CLIP_SPACE_MATRIX;
}
