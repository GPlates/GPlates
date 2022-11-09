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
