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

#include "RenderedGeometryRenderer.h"

#include "presentation/ViewState.h"

#include "view-operations/RenderedGeometryCollection.h"
#include "view-operations/RenderedGeometryLayer.h"


namespace GPlatesOpenGL
{
	namespace
	{
		/**
		 * Visits a @a RenderedGeometryCollection to determine if any rendered layers contain sub-surface geometries.
		 */
		class HasSubSurfaceLayers :
				public GPlatesViewOperations::ConstRenderedGeometryCollectionVisitor<>
		{
		public:

			HasSubSurfaceLayers() :
				d_has_sub_surface_layers(false)
			{  }

			bool
			has_sub_surface_layers() const
			{
				return d_has_sub_surface_layers;
			}

			virtual
			bool
			visit_rendered_geometry_layer(
					const GPlatesViewOperations::RenderedGeometryLayer &rendered_geometry_layer)
			{
				// Only visit if layer is active and we haven't found a sub-surface geometry yet.
				return rendered_geometry_layer.is_active() && !d_has_sub_surface_layers;
			}

			virtual
			void
			visit_rendered_resolved_scalar_field_3d(
					const GPlatesViewOperations::RenderedResolvedScalarField3D &rrsf)
			{
				d_has_sub_surface_layers = true;
			}

		private:

			bool d_has_sub_surface_layers;
		};
	}
}


GPlatesOpenGL::RenderedGeometryRenderer::RenderedGeometryRenderer(
		GPlatesPresentation::ViewState &view_state) :
	d_rendered_geometry_collection(view_state.get_rendered_geometry_collection()),
	d_gl_visual_layers(GLVisualLayers::create(view_state.get_application_state()))
{
}


void
GPlatesOpenGL::RenderedGeometryRenderer::initialise_vulkan_resources(
		Vulkan &vulkan,
		vk::RenderPass default_render_pass,
		vk::CommandBuffer initialisation_command_buffer)
{
}


void
GPlatesOpenGL::RenderedGeometryRenderer::release_vulkan_resources(
		Vulkan &vulkan)
{
}


GPlatesOpenGL::RenderedGeometryRenderer::cache_handle_type
GPlatesOpenGL::RenderedGeometryRenderer::render(
		Vulkan &vulkan,
		const GLViewProjection &view_projection,
		const double &viewport_zoom_factor,
		bool is_globe_active,
		bool improve_performance_reduce_quality_of_sub_surfaces_hint)
{
	// Initialise our visitation parameters so our visit methods can access them.
	d_visitation_params = VisitationParams(
			vulkan,
			view_projection,
			viewport_zoom_factor,
			is_globe_active,
			improve_performance_reduce_quality_of_sub_surfaces_hint);

	// Render the rendered geometry layers.
	d_rendered_geometry_collection.accept_visitor(*this);

	// Get the cache handle for all the rendered geometry layers.
	const cache_handle_type cache_handle = d_visitation_params->d_cache_handle;

	// These parameters are only used for the duration of this 'render()' method.
	d_visitation_params = boost::none;

	return cache_handle;
}


bool
GPlatesOpenGL::RenderedGeometryRenderer::has_sub_surface_geometries() const
{
	HasSubSurfaceLayers visitor;
	d_rendered_geometry_collection.accept_visitor(visitor);

	return visitor.has_sub_surface_layers();
}
