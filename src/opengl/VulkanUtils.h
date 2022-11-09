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

#ifndef GPLATES_OPENGL_VULKANUTILS_H
#define GPLATES_OPENGL_VULKANUTILS_H

#include <cstdint>
#include <vector>
#include <QString>


namespace GPlatesOpenGL
{
	namespace VulkanUtils
	{
		/**
		 * Load shader code from a Vulkan SPIR-V binary file.
		 */
		std::vector<std::uint32_t>
		load_shader_code(
				QString shader_filename);
	}
}

#endif // GPLATES_OPENGL_VULKANUTILS_H
