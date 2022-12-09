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

#include "GLMatrix.h"


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

		/**
		 * Matrix to pre-multiply OpenGL projection transforms before passing them to Vulkan.
		 *
		 * Vulkan changed the following (in relation to OpenGL):
		 * 1) The Vulkan viewport is top-to-bottom (instead of bottom-to-top in OpenGL).
		 * 2) The Vulkan viewport transformation converts the normalized device coordinate (NDC) z
		 *    in the range [0, 1] to framebuffer coordinate z in range [0, 1]
		 *    (using zf = zd, assuming a minDepth and maxDepth of 0 and 1).
		 *    Whereas OpenGL converts NDC z from [-1, 1] to window (framebuffer) z in [0, 1]
		 *    (using zw = 0.5 * zd + 0.5, assuming default glDepthRange n and f values of 0 and 1).
		 *
		 * For case (1), the returned matrix flips the 'y' direction.
		 * For case (2), the returned matrix also converts NDC 'z' from [-1, 1] to [0, 1] (which is what Vulkan expects).
		 * Well, it actually converts clip coordinates zc and wc (using zc' = 0.5 * zc + 0.5 * wc)
		 * such that zd' = zc'/wc = 0.5 * zc/wc + 0.5 = 0.5 * zd + 0.5 (and then Vulkan uses zf = zd').
		 */
		GLMatrix
		from_opengl_clip_space();
	}
}

#endif // GPLATES_OPENGL_VULKANUTILS_H
