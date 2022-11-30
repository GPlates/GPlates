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

#ifndef GPLATES_OPENGL_VULKANDEVICELIFETIME_H
#define GPLATES_OPENGL_VULKANDEVICELIFETIME_H

namespace GPlatesOpenGL
{
	class VulkanDevice;

	/**
	 * Interface for initialising Vulkan resources (objects) when the Vulkan device is created, and
	 * releasing resources when device is about to be destroyed.
	 *
	 * This is done using initialise/release methods instead of constructor/destructor since it's possible
	 * for Vulkan to have a lost device that we attempt to recover from by destroying and recreating the
	 * Vulkan device (which means the application needs to release and recreate its Vulkan resources).
	 * This also means that if an exception is thrown then resources are not cleaned up (but if an exception
	 * is thrown in rendering code then it's usually unrecoverable, ie, leads to aborting the application,
	 * and the operating system will then clean up the resources, including GPU resources/memory).
	 */
	class VulkanDeviceLifetime
	{
	public:

		virtual
		~VulkanDeviceLifetime()
		{  }


		/**
		 * The Vulkan device was just created.
		 */
		virtual
		void
		initialise_vulkan_resources(
				VulkanDevice &vulkan_device) = 0;

		/**
		 * The Vulkan device is about to be destroyed.
		 */
		virtual
		void
		release_vulkan_resources(
				VulkanDevice &vulkan_device) = 0;
	};
}

#endif // GPLATES_OPENGL_VULKANDEVICELIFETIME_H
