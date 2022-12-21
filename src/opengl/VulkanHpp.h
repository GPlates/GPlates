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

#ifndef GPLATES_OPENGL_VULKANHPP_H
#define GPLATES_OPENGL_VULKANHPP_H

//
// The Vulkan-Hpp C++ interface (around the Vulkan C interface).
//

// No prototypes since we're not linking directly to the Vulkan loader library.
#define VK_NO_PROTOTYPES
// Speed up compilation.
#define VULKAN_HPP_NO_SPACESHIP_OPERATOR  // removes the spaceship operator on structures (available with C++20)
#define VULKAN_HPP_NO_TO_STRING           // removes the various vk::to_string functions on enums and bitmasks

#if defined(_WIN32)
// Including "windows.h" before the Vulkan header appears to be necessary because "windows.h" defines
// (indirectly) the macro MemoryBarrier that conflicts with vk::MemoryBarrier, and if that macro is defined
// then "vulkan/vulkan.hpp" will un-define it, but if "windows.h" is included after "vulkan/vulkan.hpp" then
// this won't happen and there'll be a conflict.
//
// Note: Defining VK_USE_PLATFORM_WIN32_KHR also results in "windows.h" getting included but
//       doing that currently introduces other compiler errors in "vulkan/vulkan.hpp".
//
// However including "windows.h" currently generates a lot of extra compiler errors (and it's a fairly heavyweight header too).
// So instead of including it, any code that encounters this error can un-define "MemoryBarrier" just before using vk::MemoryBarrier.
// For example:
//
// #if defined(MemoryBarrier)
// #  undef MemoryBarrier
// #endif
// vk::MemoryBarrier memory_barrier;
//
// TODO: Find a better solution. This is a bit hacky and error prone.
#endif

#include <vulkan/vulkan.hpp>


namespace GPlatesOpenGL
{
	/**
	 * The Vulkan-Hpp C++ interface (around the Vulkan C interface).
	 */
	namespace VulkanHpp
	{
		/**
		 * Initialise the Vulkan-Hpp library.
		 *
		 * This involves connecting the Vulkan-Hpp C++ functions to the Vulkan C functions which
		 * are obtained dynamically via 'vkGetInstanceProcAddr()' and a 'VkInstance'.
		 *
		 * An example using QVulkanInstance is:
		 *
		 *   GPlatesOpenGL::VulkanHpp::initialise(
		 *       reinterpret_cast<PFN_vkGetInstanceProcAddr>(qvulkan_instance.getInstanceProcAddr("vkGetInstanceProcAddr")),
		 *       qvulkan_instance.vkInstance());
		 *
		 * Note: Subsequent calls to @a get_vkGetInstanceProcAddr will return this @a vkGetInstanceProcAddr_.
		 */
		void
		initialise(
				PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr_,
				VkInstance instance);


		/**
		 * Returns the function pointer 'vkGetInstanceProcAddr' passed into @a initialise.
		 *
		 * Throws @a VulkanException if @a initialise has not yet been called.
		 */
		PFN_vkGetInstanceProcAddr
		get_vkGetInstanceProcAddr();
	}
}

#endif // GPLATES_OPENGL_VULKANHPP_H
