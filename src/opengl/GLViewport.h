/* $Id$ */


/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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
 
#ifndef GPLATES_OPENGL_GLVIEWPORT_H
#define GPLATES_OPENGL_GLVIEWPORT_H

#include <cstdint>
#include <boost/operators.hpp>

#include "VulkanHpp.h"


namespace GPlatesOpenGL
{
	/**
	 * Vulkan viewport parameters.
	 */
	class GLViewport :
			public boost::equality_comparable<GLViewport>
	{
	public:
		//! Default constructor.
		GLViewport()
		{
			set_viewport(0, 0, 0, 0);
		}


		//! Constructor.
		GLViewport(
				std::int32_t x_,
				std::int32_t y_,
				std::uint32_t width_,
				std::uint32_t height_)
		{
			set_viewport(x_, y_, width_, height_);
		}


		/**
		 * Sets the viewport parameters.
		 */
		void
		set_viewport(
				std::int32_t x_,
				std::int32_t y_,
				std::uint32_t width_,
				std::uint32_t height_)
		{
			d_x = x_;
			d_y = y_;
			d_width = width_;
			d_height = height_;
		}


		std::int32_t
		x() const
		{
			return d_x;
		}

		std::int32_t
		y() const
		{
			return d_y;
		}

		std::uint32_t
		width() const
		{
			return d_width;
		}

		std::uint32_t
		height() const
		{
			return d_height;
		}

		vk::Viewport
		get_vulkan_viewport(
				float min_depth = 0,
				float max_depth = 1) const
		{
			return {
					static_cast<float>(d_x), static_cast<float>(d_y),
					static_cast<float>(d_width), static_cast<float>(d_height),
					min_depth, max_depth };
		}

		vk::Rect2D
		get_vulkan_rect_2D() const
		{
			return { {d_x, d_y}, {d_width, d_height} };
		}


		//! Equality operator - and operator!= provided by boost::equality_comparable.
		bool
		operator==(
				const GLViewport &other) const
		{
			return d_x == other.d_x &&
				d_y == other.d_y &&
				d_width == other.d_width &&
				d_height == other.d_height;
		}

	private:
		std::int32_t d_x;
		std::int32_t d_y;
		std::uint32_t d_width;
		std::uint32_t d_height;
	};
}

#endif // GPLATES_OPENGL_GLVIEWPORT_H
