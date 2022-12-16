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

#ifndef GPLATES_OPENGL_RENDERED_GEOMETRY_RENDERER_H
#define GPLATES_OPENGL_RENDERED_GEOMETRY_RENDERER_H

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

#include "GLVisualLayers.h"
#include "Vulkan.h"

#include "gui/Colour.h"

#include "view-operations/RenderedGeometryCollectionVisitor.h"


namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesViewOperations
{
	class RenderedGeometryCollection;
}

namespace GPlatesOpenGL
{
	class GLViewProjection;

	/**
	 * Draw the rendered geometries in the layers of a rendered geometry collection.
	 */
	class RenderedGeometryRenderer :
			private GPlatesViewOperations::ConstRenderedGeometryCollectionVisitor<>,
			private boost::noncopyable
	{
	public:
		/**
		 * Typedef for an opaque object that caches a particular rendering.
		 */
		typedef boost::shared_ptr<void> cache_handle_type;


		explicit
		RenderedGeometryRenderer(
				GPlatesPresentation::ViewState &view_state);


		/**
		 * The Vulkan device was just created.
		 *
		 * The initialisation command buffer will be submitted to the graphics+compute queue upon return.
		 */
		void
		initialise_vulkan_resources(
				Vulkan &vulkan,
				vk::RenderPass default_render_pass,
				vk::CommandBuffer initialisation_command_buffer);

		/**
		 * The Vulkan device is about to be destroyed.
		 */
		void
		release_vulkan_resources(
				Vulkan &vulkan);

		/**
		 * Draw the rendered geometries in the layers of our rendered geometry collection.
		 *
		 * @param viewport_zoom_factor is used for rendering view-dependent geometries (like rendered arrows).
		 * @param improve_performance_reduce_quality_of_sub_surfaces_hint a hint to improve performance of sub-surfaces
		 *        by presumably reducing quality - this is a temporary hint usually during camera mouse drags.
		 */
		cache_handle_type
		render(
				Vulkan &vulkan,
				const GLViewProjection &view_projection,
				const double &viewport_zoom_factor,
				bool is_globe_active,
				bool improve_performance_reduce_quality_of_sub_surfaces_hint = false);

		/**
		 * Returns the OpenGL layers used to filled polygons, render rasters and scalar fields.
		 */
		GLVisualLayers::non_null_ptr_type
		get_gl_visual_layers()
		{
			return d_gl_visual_layers;
		}

	private:

		/**
		 * Returns true if any rendered geometry layer has sub-surface geometries.
		 */
		bool
		has_sub_surface_geometries() const;


		/**
		 * Parameters that are only available when a @a RenderedGeometryCollection is visiting us.
		 */
		struct VisitationParams
		{
			VisitationParams(
					GPlatesOpenGL::Vulkan &vulkan,
					const GPlatesOpenGL::GLViewProjection &view_projection,
					const double &viewport_zoom_factor,
					bool is_globe_active,
					// Used for sub-surfaces...
					bool improve_performance_reduce_quality_of_sub_surfaces_hint = false) :
				d_vulkan(&vulkan),
				d_view_projection(view_projection),
				d_inverse_viewport_zoom_factor(1.0 / viewport_zoom_factor),
				d_is_globe_active(is_globe_active),
				d_improve_performance_reduce_quality_of_sub_surfaces_hint(improve_performance_reduce_quality_of_sub_surfaces_hint),
				d_cache_handle(new std::vector<cache_handle_type>())
			{  }

			GPlatesOpenGL::Vulkan *d_vulkan;
			GPlatesOpenGL::GLViewProjection d_view_projection;
			double d_inverse_viewport_zoom_factor;
			bool d_is_globe_active;

			// Used for sub-surfaces...
			bool d_improve_performance_reduce_quality_of_sub_surfaces_hint;

			// Cache of rendered geometry layers.
			boost::shared_ptr<std::vector<cache_handle_type> > d_cache_handle;
		};


		/**
		 * The collection of rendered geometries that we render.
		 */
		const GPlatesViewOperations::RenderedGeometryCollection &d_rendered_geometry_collection;

		/**
		 * Keeps track of OpenGL-related objects that persist from one render to the next.
		 */
		GPlatesOpenGL::GLVisualLayers::non_null_ptr_type d_gl_visual_layers;

		/**
		 * Parameters that are only available when a @a RenderedGeometryCollection is visiting us.
		 */
		boost::optional<VisitationParams> d_visitation_params;
	};
}

#endif // GPLATES_OPENGL_RENDERED_GEOMETRY_RENDERER_H
