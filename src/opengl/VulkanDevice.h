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

#ifndef GPLATES_OPENGL_VULKAN_DEVICE_H
#define GPLATES_OPENGL_VULKAN_DEVICE_H

#include <cstdint>
#include <vector>
#include <boost/optional.hpp>

//
// VMA memory allocator.
//
#define VMA_VULKAN_VERSION 1000000 // Vulkan 1.0
// Get VMA to fetch Vulkan function pointers dynamically using vkGetInstanceProcAddr/vkGetDeviceProcAddr.
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#include <vma/vk_mem_alloc.h>

//
// The Vulkan-Hpp C++ interface (around the Vulkan C interface).
//
#include "VulkanHpp.h"

#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	/**
	 * Higher-level interface to some of the more complex/verbose areas of Vulkan.
	 *
	 * This is designed to be used in combination with the Vulkan interface.
	 *
	 * Also provides access to higher-level Vulkan objects in one place
	 * (such as the Vulkan instance, physical device and logical device).
	 */
	class VulkanDevice :
			public GPlatesUtils::ReferenceCount<VulkanDevice>
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<VulkanDevice> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const VulkanDevice> non_null_ptr_to_const_type;


		/**
		 * Create a Vulkan logical device.
		 *
		 * NOTE: VulkanHpp::initialise() must have been called first.
		 */
		static
		non_null_ptr_type
		create(
				vk::Instance instance)
		{
			return non_null_ptr_type(new VulkanDevice(instance));
		}

		/**
		 * Create a Vulkan logical device supporting presentation to a Vulkan surface.
		 *
		 * The present queue family is returned in @a present_queue_family.
		 * This will be the graphics+compute queue family if it supports present (otherwise a different family).
		 *
		 * NOTE: VulkanHpp::initialise() must have been called first.
		 */
		static
		non_null_ptr_type
		create_for_surface(
				vk::Instance instance,
				vk::SurfaceKHR surface,
				std::uint32_t &present_queue_family)
		{
			const SurfaceInfo surface_info{ surface, present_queue_family };

			return non_null_ptr_type(new VulkanDevice(instance, surface_info));
		}


		~VulkanDevice();


		/**
		 * Return the Vulkan instance.
		 */
		vk::Instance
		get_instance()
		{
			return d_instance;
		}

		/**
		 * Return the Vulkan physical device (that the logical device was created from).
		 */
		vk::PhysicalDevice
		get_physical_device()
		{
			return d_physical_device;
		}

		/**
		 * Return the properties of the Vulkan physical device (that the logical device was created from).
		 */
		const vk::PhysicalDeviceProperties &
		get_physical_device_properties()
		{
			return d_physical_device_properties;
		}

		/**
		 * Return the enabled features of the Vulkan physical device (that the logical device was created from).
		 */
		const vk::PhysicalDeviceFeatures &
		get_physical_device_features()
		{
			return d_physical_device_features;
		}

		/**
		 * Return the Vulkan logical device.
		 */
		vk::Device
		get_device()
		{
			return d_device;
		}

		/**
		 * Return the graphics+compute queue family.
		 */
		std::uint32_t
		get_graphics_and_compute_queue_family() const
		{
			return d_graphics_and_compute_queue_family;
		}

		/**
		 * Return the graphics+compute queue.
		 */
		vk::Queue
		get_graphics_and_compute_queue()
		{
			return d_graphics_and_compute_queue;
		}

		/**
		 * Return the VMA allocator.
		 *
		 * Buffer and image allocations can go through this.
		 */
		VmaAllocator
		get_vma_allocator()
		{
			return d_vma_allocator;
		}

	private:
		// Instance.
		vk::Instance d_instance;

		// Physical device.
		vk::PhysicalDevice d_physical_device;
		vk::PhysicalDeviceProperties d_physical_device_properties;
		vk::PhysicalDeviceFeatures d_physical_device_features;
		std::uint32_t d_graphics_and_compute_queue_family;

		// Logical device.
		vk::Device d_device;
		vk::Queue d_graphics_and_compute_queue;


		struct SurfaceInfo
		{
			vk::SurfaceKHR surface;
			std::uint32_t &present_queue_family;  // Reference to caller's variable.
		};

		struct PhysicalDeviceInfo
		{
			std::uint32_t physical_device_index;
			std::uint32_t graphics_and_compute_queue_family;
			std::uint32_t present_queue_family;  // Only used if a vk::SurfaceKHR provided.
		};


		/**
		 * VMA allocator.
		 *
		 * Buffer and image allocations can go through this.
		 */
		VmaAllocator d_vma_allocator;


		VulkanDevice(
				vk::Instance instance,
				boost::optional<const SurfaceInfo &> surface_info = boost::none);


		void
		create_device(
				boost::optional<const SurfaceInfo &> surface_info);


		/**
		 * Select a physical device.
		 *
		 * Also initialise physical device properties/features and graphics/compute queue family.
		 *
		 * And if an optional vk::SurfaceKHR is provided then return the present queue family.
		 */
		void
		select_physical_device(
				boost::optional<const SurfaceInfo &> surface_info);

		bool
		check_physical_device_features(
				vk::PhysicalDevice physical_device,
				const vk::PhysicalDeviceFeatures &features) const;

		bool
		get_physical_device_graphics_and_compute_queue_family(
				vk::PhysicalDevice physical_device,
				const std::vector<vk::QueueFamilyProperties> &queue_family_properties,
				std::uint32_t &graphics_and_compute_queue_family) const;

		bool
		get_physical_device_present_queue_family(
				vk::PhysicalDevice physical_device,
				vk::SurfaceKHR surface,
				std::uint32_t num_queue_families,
				std::uint32_t graphics_and_compute_queue_family,
				std::uint32_t &present_queue_family) const;


		void
		initialise_vma_allocator();
	};
}

#endif // GPLATES_OPENGL_VULKAN_DEVICE_H
