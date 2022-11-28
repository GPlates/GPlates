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

#ifndef GPLATES_QT_WIDGETS_VULKANWINDOW_H
#define GPLATES_QT_WIDGETS_VULKANWINDOW_H

#include <boost/optional.hpp>
#include <QWindow>

#include "opengl/VulkanDevice.h"
#include "opengl/VulkanHpp.h"
#include "opengl/VulkanSwapchain.h"


namespace GPlatesQtWidgets
{
	class VulkanWindow :
			public QWindow
	{
	public:

		explicit
		VulkanWindow(
				QWindow *parent_ = nullptr);

	protected:

		/**
		 * Notify subclass that Vulkan device was just created.
		 */
		virtual
		void
		initialise_vulkan_resources(
				GPlatesOpenGL::VulkanDevice &vulkan_device) = 0;

		/**
		 * Notify subclass that Vulkan device is about to be destroyed.
		 */
		virtual
		void
		release_vulkan_resources(
				GPlatesOpenGL::VulkanDevice &vulkan_device) = 0;

		/**
		 * Called when a frame should be rendered into the window by subclass.
		 */
		virtual
		void
		render_to_window(
				GPlatesOpenGL::VulkanDevice &vulkan_device,
				GPlatesOpenGL::VulkanSwapchain &vulkan_swapchain) = 0;


		/**
		 * The current size of the window in device pixels.
		 */
		vk::Extent2D
		get_swapchain_size() const
		{
			const QSize size_in_device_pixels = size() * devicePixelRatio();
			return vk::Extent2D(
					static_cast<std::uint32_t>(size_in_device_pixels.width()),
					static_cast<std::uint32_t>(size_in_device_pixels.height()));
		}

		/**
		 * Returns the Vulkan logical device.
		 *
		 * It is only available after this window is first exposed.
		 */
		GPlatesOpenGL::VulkanDevice &
		get_vulkan_device();

		/**
		 * Returns the Vulkan swapchain.
		 *
		 * It is only available after this window is first exposed.
		 */
		GPlatesOpenGL::VulkanSwapchain &
		get_vulkan_swapchain();

		/**
		 * Subclass should call this when the Vulkan logical device is lost (vk::Result::eErrorDeviceLost)
		 * and then request a window update (eg, call "requestUpdate()" on us).
		 *
		 * This is ensure a new logical device (and swapchain) is created when the window is next updated.
		 */
		void
		device_lost();

	protected:

		void
		exposeEvent(
				QExposeEvent *expose_event) override;

		bool
		event(
				QEvent *event) override;

	private:

		//! Called every time the window needs to be updated (rendered into).
		void
		update_window();

		void
		create_vulkan_device_and_swapchain();

		void
		destroy_vulkan_device_and_swapchain();


		struct VulkanDeviceAndSwapChain
		{
			GPlatesOpenGL::VulkanDevice::non_null_ptr_type device;
			GPlatesOpenGL::VulkanSwapchain::non_null_ptr_type swapchain;
		};

		/**
		 * The Vulkan logical device and swapchain.
		 *
		 * It is first created when this window is first exposed.
		 */
		boost::optional<VulkanDeviceAndSwapChain> d_vulkan_device_and_swapchain;
	};
}

#endif // GPLATES_QT_WIDGETS_VULKANWINDOW_H
