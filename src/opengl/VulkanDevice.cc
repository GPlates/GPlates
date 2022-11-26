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

#include <limits>
#include <vector>
#include <boost//optional.hpp>

#include "VulkanDevice.h"
 // Enable VMA implementation. Only one '.cc' file should define this.
//
// Note: It's OK that this header is also included in our "VulkanDevice.h" because the
//       VMA_IMPLEMENTATION part of the header doesn't not have an include guard around it.
//
// NOTE: We include this AFTER "VulkanDevice.h" since that contains some VMA preprocessor defines
//       that determine the behaviour of VMA (and we don't want to repeat them here).
#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#include "VulkanException.h"
#include "VulkanHpp.h"

#include "global/GPlatesAssert.h"


GPlatesOpenGL::VulkanDevice::VulkanDevice(
		vk::Instance instance,
		boost::optional<const SurfaceInfo &> surface_info) :
	d_instance(instance),
	d_graphics_and_compute_queue_family(std::numeric_limits<std::uint32_t>::max())
{
	// Create the logical device.
	create_device(surface_info);

	// Initialise the VMA allocator (for allocating buffers and images).
	initialise_vma_allocator();
}


GPlatesOpenGL::VulkanDevice::~VulkanDevice()
{
	if (d_device)
	{
		// First make sure all commands in all queues have finished before we start destroying things.
		//
		// Note: It's OK to wait here since destroying a device is not a performance-critical part of the code.
		d_device.waitIdle();
	}

	if (d_vma_allocator)
	{
		// Destroy the VMA allocator.
		vmaDestroyAllocator(d_vma_allocator);
	}

	if (d_device)
	{
		// Destroy the logical device.
		d_device.destroy();
	}
}


void
GPlatesOpenGL::VulkanDevice::create_device(
		boost::optional<const SurfaceInfo &> surface_info)
{
	//
	// Select a physical device.
	//
	// Also initialise physical device properties/features and graphics/compute queue family.
	// And if an optional vk::SurfaceKHR is provided then return the present queue family.
	//
	select_physical_device(surface_info);

	//
	// Device queues.
	//
	const float queue_priority = 0;
	std::vector<vk::DeviceQueueCreateInfo> device_queue_infos;
	// The graphics+compute queue info.
	vk::DeviceQueueCreateInfo graphics_and_compute_device_queue_info;
	graphics_and_compute_device_queue_info
			.setQueueFamilyIndex(d_graphics_and_compute_queue_family)
			.setQueueCount(1)
			.setQueuePriorities(queue_priority);
	device_queue_infos.push_back(graphics_and_compute_device_queue_info);
	// The present queue info.
	if (surface_info)
	{
		// If the present queue family is not the graphics+compute queue family
		// then request creation of a new (present) queue.
		if (surface_info->present_queue_family != d_graphics_and_compute_queue_family)
		{
			vk::DeviceQueueCreateInfo present_device_queue_info;
			present_device_queue_info
					.setQueueFamilyIndex(surface_info->present_queue_family)
					.setQueueCount(1)
					.setQueuePriorities(queue_priority);
			device_queue_infos.push_back(present_device_queue_info);
		}
	}

	//
	// Device extensions.
	//
	std::vector<const char *> device_extensions;
	// If we have a surface then a swapchain will need to be created
	// (note that we don't create the swapchain, our caller is responsible for that).
	if (surface_info)
	{
		device_extensions.push_back("VK_KHR_swapchain");
	}

	//
	// Create the logical device.
	//
	vk::DeviceCreateInfo device_info;
	device_info
			.setQueueCreateInfos(device_queue_infos)
			.setPEnabledFeatures(&d_physical_device_features)
			.setPEnabledExtensionNames(device_extensions);
	d_device = d_physical_device.createDevice(device_info);

	// Get the graphics+compute queue from the logical device.
	//
	// Note: We don't retrieve the present queue (which could be same as graphics+compute queue), even if
	//       a vk::SurfaceKHR was provided, because that's the responsibility of whoever creates the swapchain.
	d_graphics_and_compute_queue = d_device.getQueue(d_graphics_and_compute_queue_family, 0/*queueIndex*/);
}


void
GPlatesOpenGL::VulkanDevice::select_physical_device(
		boost::optional<const SurfaceInfo &> surface_info)
{
	// Get the physical devices.
	const std::vector<vk::PhysicalDevice> physical_devices = d_instance.enumeratePhysicalDevices();
	GPlatesGlobal::Assert<VulkanException>(
			!physical_devices.empty(),
			GPLATES_ASSERTION_SOURCE,
			"No physical devices present.");

	// Find candidate physical devices with a queue family supporting both graphics and compute.
	//
	// According to the Vulkan spec...
	//   "If an implementation exposes any queue family that supports graphics operations, at least one queue family of
	//    at least one physical device exposed by the implementation must support both graphics and compute operations."
	//
	// In the case of multi-vendor on the desktop (eg, a computer with a discrete graphics card and graphics integrated into the CPU),
	// each vendor (ie, each physical device) pretty much has to provide a queue family supporting both graphics and compute
	// because that vendor has to assume it might be the only physical device (GPU) on the system:
	//   see https://www.reddit.com/r/vulkan/comments/hbauoz/comment/fv8rnt7/?utm_source=share&utm_medium=web2x&context=3
	// So if there's a discrete graphics card then it will likely support graphics and therefore a graphics+compute queue family.
	//
	std::vector<PhysicalDeviceInfo> candidate_physical_device_infos;
	for (unsigned int physical_device_index = 0; physical_device_index < physical_devices.size(); ++physical_device_index)
	{
		vk::PhysicalDevice physical_device = physical_devices[physical_device_index];

		// Get the features of the current physical device.
		const vk::PhysicalDeviceFeatures features = physical_device.getFeatures();

		// Check that the current physical device supports the features we require.
		if (!check_physical_device_features(physical_device, features))
		{
			continue;
		}

		// Get the queue family properties of current physical device.
		const std::vector<vk::QueueFamilyProperties> queue_family_properties = physical_device.getQueueFamilyProperties();

		// See if any queue family supports both graphics and compute.
		std::uint32_t graphics_and_compute_queue_family;
		if (!get_physical_device_graphics_and_compute_queue_family(
				physical_device,
				queue_family_properties,
				graphics_and_compute_queue_family))
		{
			continue;
		}

		// If a vk::SurfaceKHR was provided then see if any queue family supports present
		// (preferring the graphics+compute queue family, if it supports present).
		std::uint32_t present_queue_family = std::numeric_limits<std::uint32_t>::max();
		if (surface_info)
		{
			if (!get_physical_device_present_queue_family(
					physical_device,
					surface_info->surface,
					queue_family_properties.size()/*num_queue_families*/,
					graphics_and_compute_queue_family,
					present_queue_family))
			{
				continue;
			}
		}

		candidate_physical_device_infos.push_back(
				{ physical_device_index, graphics_and_compute_queue_family, present_queue_family });
	}

	// If we couldn't find a suitable physical device then throw an exception.
	GPlatesGlobal::Assert<VulkanException>(
			!candidate_physical_device_infos.empty(),
			GPLATES_ASSERTION_SOURCE,
			"Failed to find a suitable Vulkan physical device.");

	// Choose a 'discrete' GPU if found.
	boost::optional<PhysicalDeviceInfo> selected_physical_device_info;
	for (const auto &physical_device_info : candidate_physical_device_infos)
	{
		// Get the properties of candidate physical device.
		const vk::PhysicalDeviceProperties physical_device_properties =
				physical_devices[physical_device_info.physical_device_index].getProperties();

		if (physical_device_properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
		{
			selected_physical_device_info = physical_device_info;
			break;
		}
	}

	// Discrete GPU not found, so just choose the first candidate physical device.
	if (!selected_physical_device_info)
	{
		selected_physical_device_info = candidate_physical_device_infos.front();
	}

	//
	// Initialise physical device and its properties/features, and the graphics/compute queue family.
	//

	d_physical_device = physical_devices[selected_physical_device_info->physical_device_index];
	d_physical_device_properties = d_physical_device.getProperties();
	d_physical_device_features = d_physical_device.getFeatures();
	// Disable robust buffer access in release builds (for improved performance).
#if !defined(GPLATES_DEBUG)
	d_physical_device_features.setRobustBufferAccess(false);
#endif
	d_graphics_and_compute_queue_family = selected_physical_device_info->graphics_and_compute_queue_family;

	// If a vk::SurfaceKHR was provided then also return the present queue family to the caller.
	if (surface_info)
	{
		surface_info->present_queue_family = selected_physical_device_info->present_queue_family;
	}
}


bool
GPlatesOpenGL::VulkanDevice::check_physical_device_features(
		vk::PhysicalDevice physical_device,
		const vk::PhysicalDeviceFeatures &features) const
{
	//
	// For feature support on different platforms/systems see http://vulkan.gpuinfo.org/listfeaturescore10.php
	//
	// We currently only use features that are commonly available on desktop Windows, Linux and macOS.
	//
	// Note that wide lines and geometry shaders are not typically supported on macOS (so we don't use them).
	//
	return
			// Rendering stars disables the near and far clip planes (and clamps depth values outside)...
			features.depthClamp &&
			// Order-independent transparency writes to memory (and uses atomics) in fragment shaders...
			features.fragmentStoresAndAtomics &&
			// Rendering stars uses point sizes greater than 1.0...
			features.largePoints &&
			// We use anisotropic filtering in many textures...
			features.samplerAnisotropy &&
			// Clip distances are used in some shaders (eg, rendering stars)...
			features.shaderClipDistance;
}


bool
GPlatesOpenGL::VulkanDevice::get_physical_device_graphics_and_compute_queue_family(
		vk::PhysicalDevice physical_device,
		const std::vector<vk::QueueFamilyProperties> &queue_family_properties,
		std::uint32_t &graphics_and_compute_queue_family) const
{
	//
	// See if the physical device has a queue family supporting both graphics and compute.
	//
	// According to the Vulkan spec...
	//   "If an implementation exposes any queue family that supports graphics operations, at least one queue family of
	//    at least one physical device exposed by the implementation must support both graphics and compute operations."
	//
	// In the case of multi-vendor on the desktop (eg, a computer with a discrete graphics card and graphics integrated into the CPU),
	// each vendor (ie, each physical device) pretty much has to provide a queue family supporting both graphics and compute
	// because that vendor has to assume it might be the only physical device (GPU) on the system:
	//   see https://www.reddit.com/r/vulkan/comments/hbauoz/comment/fv8rnt7/?utm_source=share&utm_medium=web2x&context=3
	//

	// See if any queue family supports both graphics and compute.
	for (std::uint32_t queue_family_index = 0; queue_family_index < queue_family_properties.size(); ++queue_family_index)
	{
		if ((queue_family_properties[queue_family_index].queueFlags & vk::QueueFlagBits::eGraphics) &&
			(queue_family_properties[queue_family_index].queueFlags & vk::QueueFlagBits::eCompute))
		{
			graphics_and_compute_queue_family = queue_family_index;
			return true;
		}
	}

	return false;
}


bool
GPlatesOpenGL::VulkanDevice::get_physical_device_present_queue_family(
		vk::PhysicalDevice physical_device,
		vk::SurfaceKHR surface,
		std::uint32_t num_queue_families,
		std::uint32_t graphics_and_compute_queue_family,
		std::uint32_t &present_queue_family) const
{
	// First see if the graphics+compute queue family supports present.
	if (physical_device.getSurfaceSupportKHR(graphics_and_compute_queue_family, surface))
	{
		present_queue_family = graphics_and_compute_queue_family;
		return true;
	}

	// See if any other queue family supports present.
	for (std::uint32_t queue_family_index = 0; queue_family_index < num_queue_families; ++queue_family_index)
	{
		if (queue_family_index == graphics_and_compute_queue_family)
		{
			// Already checked the graphics+compute queue family.
			continue;
		}

		// See if current queue family supports present.
		if (physical_device.getSurfaceSupportKHR(queue_family_index, surface))
		{
			present_queue_family = queue_family_index;
			return true;
		}
	}

	return false;
}


void
GPlatesOpenGL::VulkanDevice::initialise_vma_allocator()
{
	// Get the function pointer 'vkGetInstanceProcAddr'.
	PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr_ = VulkanHpp::get_vkGetInstanceProcAddr();

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
