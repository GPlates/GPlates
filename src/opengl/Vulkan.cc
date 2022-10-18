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

// Enable VMA implementation. Only one '.cc' file should define this.
//
// Note: It's OK that this header is also included in our "Vulkan.h" because the
//       VMA_IMPLEMENTATION part of the header doesn't not have an include guard around it.
//
// NOTE: We include this AFTER "Vulkan.h" since that contains some VMA preprocessor defines
//       that determine the behaviour of VMA (and we don't want to repeat them here).
#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>


GPlatesOpenGL::Vulkan::Vulkan(
		PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr_,
		vk::Instance instance,
		vk::PhysicalDevice physical_device,
		vk::Device device) :
	d_instance(instance),
	d_physical_device(physical_device),
	d_device(device)
{
	initialise_vma_allocator(vkGetInstanceProcAddr_);
}


GPlatesOpenGL::Vulkan::~Vulkan()
{
	destroy_vma_allocator();
}


void
GPlatesOpenGL::Vulkan::initialise_vma_allocator(
		PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr_)
{
	//
	// Get VMA to fetch its needed Vulkan function pointers dynamically using vkGetInstanceProcAddr/vkGetDeviceProcAddr.
	//
	// We told it to do this with "#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1".
	//
	VmaVulkanFunctions vulkan_functions = {};
	vulkan_functions.vkGetInstanceProcAddr = vkGetInstanceProcAddr_;
	vulkan_functions.vkGetDeviceProcAddr = reinterpret_cast<PFN_vkGetDeviceProcAddr>(
			vkGetInstanceProcAddr_(d_instance, "vkGetDeviceProcAddr"));

	// Create the VMA allocator.
	VmaAllocatorCreateInfo allocator_create_info = {};
	allocator_create_info.vulkanApiVersion = VK_API_VERSION_1_0;
	allocator_create_info.instance = d_instance;
	allocator_create_info.physicalDevice = d_physical_device;
	allocator_create_info.device = d_device;
	allocator_create_info.pVulkanFunctions = &vulkan_functions;
	vmaCreateAllocator(&allocator_create_info, &d_vma_allocator);
}


void
GPlatesOpenGL::Vulkan::destroy_vma_allocator()
{
	// Destroy the VMA allocator.
	vmaDestroyAllocator(d_vma_allocator);
}
