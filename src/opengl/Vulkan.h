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

#ifndef GPLATES_OPENGL_VULKAN_H
#define GPLATES_OPENGL_VULKAN_H

// The Vulkan-Hpp C++ interface (around the Vulkan C interface).
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
	class Vulkan :
			public GPlatesUtils::ReferenceCount<Vulkan>
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<Vulkan> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const Vulkan> non_null_ptr_to_const_type;


		static
		non_null_ptr_type
		create(
				vk::Instance instance,
				vk::PhysicalDevice physical_device,
				vk::Device device)
		{
			return non_null_ptr_type(new Vulkan(instance, physical_device, device));
		}


		/**
		 * Return the Vulkan instance.
		 */
		vk::Instance
		instance()
		{
			return d_instance;
		}

		/**
		 * Return the Vulkan physical device (that the logical device was created from).
		 */
		vk::PhysicalDevice
		physical_device()
		{
			return d_physical_device;
		}

		/**
		 * Return the Vulkan logical device.
		 */
		vk::Device
		device()
		{
			return d_device;
		}

	private:
		vk::Instance d_instance;
		vk::PhysicalDevice d_physical_device;
		vk::Device d_device;


		Vulkan(
				vk::Instance instance,
				vk::PhysicalDevice physical_device,
				vk::Device device);
	};
}

#endif // GPLATES_OPENGL_VULKAN_H
