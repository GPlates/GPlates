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

#include "VulkanHpp.h"


// This must be used exactly once in our source code to provide storage for the default dynamic dispatcher.
// This is needed since we're indirectly defining VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE via defining VK_NO_PROTOTYPES.
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE


void
GPlatesOpenGL::VulkanHpp::initialise(
		PFN_vkGetInstanceProcAddr get_instance_proc_addr,
		VkInstance instance)
{
	// Initialise the Vulkan-Hpp global function pointers (like 'vkCreateInstance').
	VULKAN_HPP_DEFAULT_DISPATCHER.init(get_instance_proc_addr);

	// Initialise all remaining Vulkan-Hpp function pointers for instance.
	VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);

	// NOTE: We could also specialise Vulkan-Hpp function pointers for a particular device (VkDevice)
	//       to get a little extra efficiency by avoiding an internal dispatch that looks up the VkDevice
	//       passed in (instead going directly to the VkDevice used when 'vkGetDeviceProcAddr()' was called).
	//       However this means only that particular 'VkDevice' can be used with those function pointers and
	//       we'd rather not have to re-fetch those specialised function pointers with each VkDevice
	//       (for example QVulkanWindow will destroy and create a new VkDevice on a lost device).
	//       In any case, such a speed improvement is not really necessary for our application since
	//       we're not making a lot of Vulkan calls (compared to a game for example).
}
