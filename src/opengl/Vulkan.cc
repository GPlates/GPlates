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

#include "Vulkan.h"


GPlatesOpenGL::Vulkan::Vulkan(
		VulkanDevice &vulkan_device,
		VulkanFrame &vulkan_frame,
		vk::RenderPass default_render_pass) :
	d_vulkan_device(vulkan_device),
	d_vulkan_frame(vulkan_frame),
	d_default_render_pass(default_render_pass)
{
}
