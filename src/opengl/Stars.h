/**
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

#ifndef GPLATES_GUI_STARS_H
#define GPLATES_GUI_STARS_H

#include <cstdint>
#include <vector>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include "GLStreamPrimitives.h"
#include "GLVertexUtils.h"
#include "Vulkan.h"
#include "VulkanBuffer.h"

#include "gui/Colour.h"


namespace GPlatesOpenGL
{
	class GLViewProjection;
}

namespace GPlatesOpenGL
{
	/**
	 * Draws a random collection of stars in the background.
	 */
	class Stars
	{
	public:

		//! Default colour of the stars.
		static const GPlatesGui::Colour DEFAULT_COLOUR;


		explicit
		Stars(
				const GPlatesGui::Colour &colour = DEFAULT_COLOUR);


		/**
		 * The Vulkan device has been created.
		 */
		void
		initialise_vulkan_resources(
				GPlatesOpenGL::Vulkan &vulkan,
				vk::RenderPass default_render_pass,
				vk::CommandBuffer initialisation_command_buffer,
				vk::Fence initialisation_submit_fence);

		/**
		 * The Vulkan device is about to be destroyed.
		 */
		void
		release_vulkan_resources(
				GPlatesOpenGL::Vulkan &vulkan);


		/**
		 * Render the stars.
		 *
		 * Note: @a device_pixel_ratio is used on high-DPI displays where there are more pixels in the
		 *       same physical area on screen and so the point size of the stars is increased to compensate.
		 *
		 * Note: @a radius_multiplier is useful for the 2D map views to expand the positions of the stars radially
		 *       so that they're outside the map bounding sphere. The default of 1.0 works for the 3D globe view.
		 */
		void
		render(
				Vulkan &vulkan,
				vk::CommandBuffer command_buffer,
				const GLViewProjection &view_projection,
				int device_pixel_ratio,
				const double &radius_multiplier = 1.0);

	private:

		static constexpr float SMALL_STARS_SIZE = 1.4f;
		static constexpr float LARGE_STARS_SIZE = 2.4f;

		static const unsigned int NUM_SMALL_STARS = 4250;
		static const unsigned int NUM_LARGE_STARS = 3750;

		// Points sit on a sphere of this radius (note that the Earth globe has radius 1.0).
		// Ideally we'd have these points at infinity, but a large distance works well.
		static const constexpr double RADIUS = 7.0f;

		typedef GLVertexUtils::Vertex vertex_type;
		typedef std::uint32_t vertex_index_type;
		typedef GLDynamicStreamPrimitives<vertex_type, vertex_index_type> stream_primitives_type;

		//
		// layout (push_constant) uniform PushConstants
		// {
		//     mat4 view_projection;
		//     vec4 star_colour;
		//     float radius_multiplier;
		//     float point_size;
		// };
		//
		// NOTE: This fits within the minimum required size limit of 128 bytes for push constants.
		//       And push constants use the std430 layout.
		//
		struct PushConstants
		{
			float view_projection[16];
			float star_colour[4];
			float radius_multiplier;
			float point_size;
		};


		void
		create_graphics_pipeline(
				GPlatesOpenGL::Vulkan &vulkan,
				vk::RenderPass default_render_pass);

		void
		create_stars(
				std::vector<vertex_type> &vertices,
				std::vector<vertex_index_type> &vertex_indices);

		void
		stream_stars(
				stream_primitives_type &stream,
				boost::function< double () > &rand,
				unsigned int num_stars);

		void
		load_stars(
				GPlatesOpenGL::Vulkan &vulkan,
				vk::CommandBuffer initialisation_command_buffer,
				vk::Fence initialisation_submit_fence,
				const std::vector<vertex_type> &vertices,
				const std::vector<vertex_index_type> &vertex_indices);


		//! Colour of the stars.
		GPlatesGui::Colour d_colour;

		vk::PipelineLayout d_pipeline_layout;
		vk::Pipeline d_graphics_pipeline;

		// Vertex/index buffers (static buffers in device local memory).
		VulkanBuffer d_vertex_buffer;
		VulkanBuffer d_index_buffer;

		unsigned int d_num_small_star_vertices;
		unsigned int d_num_small_star_vertex_indices;
		unsigned int d_num_large_star_vertices;
		unsigned int d_num_large_star_vertex_indices;
	};
}

#endif  // GPLATES_GUI_STARS_H
