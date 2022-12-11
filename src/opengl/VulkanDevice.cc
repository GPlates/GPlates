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

#include <vector>
#include <boost//optional.hpp>

#include "VulkanDevice.h"

#include "VulkanException.h"
#include "VulkanHpp.h"

#include "global/GPlatesAssert.h"


GPlatesOpenGL::VulkanDevice::VulkanDevice(
		vk::Instance instance) :
	d_instance(instance),
	d_graphics_and_compute_queue_family(UINT32_MAX),
	d_vma_allocator{}
{
}


void
GPlatesOpenGL::VulkanDevice::create()
{
	create_internal();
}


void
GPlatesOpenGL::VulkanDevice::create_for_surface(
		vk::SurfaceKHR surface,
		std::uint32_t &present_queue_family)
{
	const SurfaceInfo surface_info{ surface, present_queue_family };
	create_internal(surface_info);
}


void
GPlatesOpenGL::VulkanDevice::create_internal(
		boost::optional<const SurfaceInfo &> surface_info)
{
	// Our VMA allocator can use the VK_KHR_dedicated_allocation extension (if available and we've enabled it).
	bool use_KHR_dedicated_allocation = false;

	// Create the logical device.
	create_device(use_KHR_dedicated_allocation, surface_info);

	// Create the VMA allocator (for allocating buffers and images).
	create_vma_allocator(use_KHR_dedicated_allocation);
}


void
GPlatesOpenGL::VulkanDevice::destroy()
{
	GPlatesGlobal::Assert<VulkanException>(
			d_device,
			GPLATES_ASSERTION_SOURCE,
			"Attempted to destroy Vulkan device without first creating it.");

	// First make sure all commands in all queues have finished before we start destroying things.
	//
	// Note: It's OK to wait here since destroying a device is not a performance-critical part of the code.
	d_device.waitIdle();

	// Destroy the VMA allocator.
	vmaDestroyAllocator(d_vma_allocator);
	d_vma_allocator = {};

	// Destroy the logical device.
	d_device.destroy();
	d_device = nullptr;

	// Reset some members.
	d_physical_device = nullptr;  // Owned by Vulkan instance (which we're not destroying).
	d_graphics_and_compute_queue = nullptr;  // Owned by device (which was destroyed when device was destroyed).
}


void
GPlatesOpenGL::VulkanDevice::create_device(
		bool &use_KHR_dedicated_allocation,
		boost::optional<const SurfaceInfo &> surface_info)
{
	GPlatesGlobal::Assert<VulkanException>(
			!d_device,
			GPLATES_ASSERTION_SOURCE,
			"Attempted to create Vulkan device without first destroying it.");

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

	// Device extensions that we'll enable (and that are available).
	std::vector<const char *> enabled_device_extensions;

	// Get the available device extensions.
	const std::vector<vk::ExtensionProperties> available_device_extension_properties =
			d_physical_device.enumerateDeviceExtensionProperties();

	// If we have a surface then a swapchain will need to be created
	// (note that we don't create the swapchain, our caller is responsible for that).
	if (surface_info)
	{
		// We need the VK_KHR_swapchain device extension to render to a window/surface.
		// It should be available on systems with a display.
		GPlatesGlobal::Assert<VulkanException>(
				is_device_extension_available("VK_KHR_swapchain", available_device_extension_properties),
				GPLATES_ASSERTION_SOURCE,
				"The Vulkan extension VK_KHR_swapchain must be supported (for rendering to windows/surfaces).");
		enabled_device_extensions.push_back("VK_KHR_swapchain");
	}

	// Our VMA allocator will automatically use the following extensions if they're available and enabled.

	const bool have_KHR_dedicated_allocation = is_device_extension_available(
			"VK_KHR_dedicated_allocation", available_device_extension_properties);
	if (have_KHR_dedicated_allocation) enabled_device_extensions.push_back("VK_KHR_dedicated_allocation");

	const bool have_KHR_get_memory_requirements2 = is_device_extension_available(
			"VK_KHR_get_memory_requirements2", available_device_extension_properties);
	if (have_KHR_get_memory_requirements2) enabled_device_extensions.push_back("VK_KHR_get_memory_requirements2");

	// Let the VMA allocator know whether it can use 'VK_KHR_dedicated_allocation'.
	use_KHR_dedicated_allocation = have_KHR_dedicated_allocation && have_KHR_get_memory_requirements2;


	//
	// Create the logical device.
	//
	vk::DeviceCreateInfo device_info;
	device_info
			.setQueueCreateInfos(device_queue_infos)
			.setPEnabledFeatures(&d_physical_device_features)
			.setPEnabledExtensionNames(enabled_device_extensions);
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
		std::uint32_t present_queue_family = UINT32_MAX;
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
	d_physical_device_memory_properties = d_physical_device.getMemoryProperties();
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
		// Note that a queue supporting graphics or compute operations also supports transfer operations
		// (the Vulkan spec states that reporting 'vk::QueueFlagBits::eTransfer' is not need in this case).
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


bool
GPlatesOpenGL::VulkanDevice::is_device_extension_available(
		const char *device_extension,
		const std::vector<vk::ExtensionProperties> &available_device_extension_properties)
{
	for (const auto &available_device_extension_property : available_device_extension_properties)
	{
		const char *available_device_extension = available_device_extension_property.extensionName;
		if (QString::fromUtf8(device_extension) == QString::fromUtf8(available_device_extension))
		{
			return true;
		}
	}

	return false;
}


void
GPlatesOpenGL::VulkanDevice::create_vma_allocator(
		bool use_KHR_dedicated_allocation)
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
	// Get the VMA allocator to use the VK_KHR_dedicated_allocation extension (if available and we've enabled it).
	// This allows dedicated allocations for buffer/image resources when the driver decides its more efficient.
	if (use_KHR_dedicated_allocation)
	{
		// Note that the VMA docs tell us the following validation error can be ignored:
		//
		//   "vkBindBufferMemory(): Binding memory to buffer 0x2d but vkGetBufferMemoryRequirements() has not been called on that buffer. "
		//
		allocator_create_info.flags |= VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT;
	}
	vmaCreateAllocator(&allocator_create_info, &d_vma_allocator);
}
