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

#ifndef GPLATES_OPENGL_VULKANSWAPCHAIN_H
#define GPLATES_OPENGL_VULKANSWAPCHAIN_H

#include <cstdint>
#include <QSize>

#include "VulkanHpp.h"

#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	class VulkanDevice;

	class VulkanSwapchain :
			public GPlatesUtils::ReferenceCount<VulkanSwapchain>
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<VulkanSwapchain> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const VulkanSwapchain> non_null_ptr_to_const_type;


		/**
		 * Create a Vulkan swapchain.
		 *
		 * NOTE: The lifetime of the returned @a VulkanSwapchain must not be longer
		 *       than the lifetime of @a vulkan_device (because it's used in our destructor).
		 */
		static
		non_null_ptr_type
		create(
				VulkanDevice &vulkan_device,
				vk::SurfaceKHR surface,
				std::uint32_t present_queue_family,
				const QSize &window_size)
		{
			return non_null_ptr_type(new VulkanSwapchain(vulkan_device, surface, present_queue_family, window_size));
		}


		~VulkanSwapchain();


		/**
		 * Returns the current swapchain size (in device pixels).
		 */
		QSize
		get_swapchain_size() const
		{
			return d_swapchain_size;
		}

		/**
		 * Re-create the vk::SwapchainKHR.
		 *
		 * This is useful when the surface window is resized.
		 */
		void
		recreate_swapchain(
				const QSize &window_size);

	private:

		VulkanDevice &d_vulkan_device;

		vk::SurfaceKHR d_surface;
		std::uint32_t d_present_queue_family;

		vk::SwapchainKHR d_swapchain;
		QSize d_swapchain_size;


		VulkanSwapchain(
				VulkanDevice &vulkan_device,
				vk::SurfaceKHR surface,
				std::uint32_t present_queue_family,
				const QSize &window_size);
	};
}

#endif // GPLATES_OPENGL_VULKANSWAPCHAIN_H
