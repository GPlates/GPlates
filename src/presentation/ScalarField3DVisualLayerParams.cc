/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#include "ScalarField3DVisualLayerParams.h"

#include "ViewState.h"

#include "app-logic/ScalarField3DLayerTask.h"

#include "file-io/ScalarField3DFileFormatReader.h"

#include "gui/RasterColourPalette.h"

#include "opengl/GLContext.h"
#include "opengl/GLRenderer.h"
#include "opengl/GLScalarField3D.h"

#include "qt-widgets/GlobeAndMapWidget.h"
#include "qt-widgets/ReconstructionViewWidget.h"
#include "qt-widgets/ViewportWindow.h"


namespace GPlatesPresentation
{
	namespace
	{
		/**
		 * Returns true if 3D scalar fields support 'surface polygons mask'.
		 */
		bool
		determine_if_surface_polygons_mask_supported(
				ViewState &view_state)
		{
			// Get an OpenGL context.
			GPlatesOpenGL::GLContext::non_null_ptr_type gl_context =
					view_state.get_other_view_state().reconstruction_view_widget().globe_and_map_widget()
							.get_active_gl_context();

			// Make sure the context is currently active.
			gl_context->make_current();

			// We need an OpenGL renderer before we can query support.
			GPlatesOpenGL::GLRenderer::non_null_ptr_type renderer = gl_context->create_renderer();

			// Start a begin_render/end_render scope.
			GPlatesOpenGL::GLRenderer::RenderScope render_scope(*renderer);

			return GPlatesOpenGL::GLScalarField3D::supports_surface_fill_mask(*renderer);
		}
	}
}


GPlatesPresentation::ScalarField3DVisualLayerParams::ScalarField3DVisualLayerParams(
		GPlatesAppLogic::LayerTaskParams &layer_task_params,
		GPlatesPresentation::ViewState &view_state,
		const GPlatesViewOperations::ScalarField3DRenderParameters &default_render_parameters) :
	VisualLayerParams(layer_task_params),
	d_render_mode(default_render_parameters.get_render_mode()),
	d_isosurface_deviation_window_mode(default_render_parameters.get_isosurface_deviation_window_mode()),
	d_isosurface_colour_mode(default_render_parameters.get_isosurface_colour_mode()),
	d_cross_section_colour_mode(default_render_parameters.get_cross_section_colour_mode()),
	d_scalar_colour_palette_parameters_initialised_from_scalar_field(false),
	d_scalar_colour_palette_parameters(default_render_parameters.get_scalar_colour_palette_parameters()),
	d_gradient_colour_palette_parameters_initialised_from_scalar_field(false),
	d_gradient_colour_palette_parameters(default_render_parameters.get_gradient_colour_palette_parameters()),
	d_isovalue_parameters_initialised_from_scalar_field(false),
	d_isovalue_parameters(default_render_parameters.get_isovalue_parameters()),
	d_deviation_window_render_options(default_render_parameters.get_deviation_window_render_options()),
	d_is_surface_polygons_mask_supported(determine_if_surface_polygons_mask_supported(view_state)),
	d_surface_polygons_mask(default_render_parameters.get_surface_polygons_mask()),
	d_depth_restriction_initialised_from_scalar_field(false),
	d_depth_restriction(default_render_parameters.get_depth_restriction()),
	d_quality_performance()
{
	// If surface polygons mask not supported then disable it.
	if (!d_is_surface_polygons_mask_supported)
	{
		d_surface_polygons_mask.enable_surface_polygons_mask = false;
	}
}


void
GPlatesPresentation::ScalarField3DVisualLayerParams::handle_layer_modified(
		const GPlatesAppLogic::Layer &layer)
{
	GPlatesAppLogic::ScalarField3DLayerTask::Params *layer_task_params =
		dynamic_cast<GPlatesAppLogic::ScalarField3DLayerTask::Params *>(
				&get_layer_task_params());

	if (layer_task_params &&
		layer_task_params->get_scalar_field_feature())
	{
		//
		// Need to initialise some parameters that depend on the scalar data (eg, min/max/mean/std_dev).
		// This needs to be done only once but the scalar data needs to be ready/setup, so we just
		// choose here to initialise since we know the scalar data should have been setup by now.
		//

		if (!d_scalar_colour_palette_parameters_initialised_from_scalar_field)
		{
			// If we have a scalar field then map the palette range to the scalar mean +/- deviation.
			if (layer_task_params->get_scalar_mean() &&
				layer_task_params->get_scalar_standard_deviation())
			{
				d_scalar_colour_palette_parameters.map_palette_range(
						layer_task_params->get_scalar_mean().get() -
								d_scalar_colour_palette_parameters.get_deviation_from_mean() *
										layer_task_params->get_scalar_standard_deviation().get(),
						layer_task_params->get_scalar_mean().get() +
								d_scalar_colour_palette_parameters.get_deviation_from_mean() *
										layer_task_params->get_scalar_standard_deviation().get());

				d_scalar_colour_palette_parameters_initialised_from_scalar_field = true;
			}
		}

		if (!d_gradient_colour_palette_parameters_initialised_from_scalar_field)
		{
			// If we have a scalar field then map the palette range to the gradient +/- (mean + deviation).
			if (layer_task_params->get_gradient_magnitude_mean() &&
				layer_task_params->get_gradient_magnitude_standard_deviation())
			{
				d_gradient_colour_palette_parameters.map_palette_range(
						-layer_task_params->get_gradient_magnitude_mean().get() -
								d_gradient_colour_palette_parameters.get_deviation_from_mean() *
										layer_task_params->get_gradient_magnitude_standard_deviation().get(),
						layer_task_params->get_gradient_magnitude_mean().get() +
								d_gradient_colour_palette_parameters.get_deviation_from_mean() *
										layer_task_params->get_gradient_magnitude_standard_deviation().get());

				d_gradient_colour_palette_parameters_initialised_from_scalar_field = true;
			}
		}

		if (!d_isovalue_parameters_initialised_from_scalar_field)
		{
			// If we have a scalar field then set the isovalue.
			if (layer_task_params->get_scalar_mean())
			{
				d_isovalue_parameters = GPlatesViewOperations::ScalarField3DRenderParameters::IsovalueParameters(
						layer_task_params->get_scalar_mean().get());

				d_isovalue_parameters_initialised_from_scalar_field = true;
			}
		}

		if (!d_depth_restriction_initialised_from_scalar_field)
		{
			// If we have a scalar field then set the depth restriction range.
			if (layer_task_params->get_minimum_depth_layer_radius() &&
				layer_task_params->get_maximum_depth_layer_radius())
			{
				d_depth_restriction =
						GPlatesViewOperations::ScalarField3DRenderParameters::DepthRestriction(
								layer_task_params->get_minimum_depth_layer_radius().get(),
								layer_task_params->get_maximum_depth_layer_radius().get());

				d_depth_restriction_initialised_from_scalar_field = true;
			}
		}
	}
	// ...else there's no scalar field feature...

	emit_modified();
}


GPlatesViewOperations::ScalarField3DRenderParameters
GPlatesPresentation::ScalarField3DVisualLayerParams::get_scalar_field_3d_render_parameters() const
{
	return GPlatesViewOperations::ScalarField3DRenderParameters(
			get_render_mode(),
			get_isosurface_deviation_window_mode(),
			get_isosurface_colour_mode(),
			get_cross_section_colour_mode(),
			get_scalar_colour_palette_parameters(),
			get_gradient_colour_palette_parameters(),
			get_isovalue_parameters(),
			get_deviation_window_render_options(),
			get_surface_polygons_mask(),
			get_depth_restriction(),
			get_quality_performance(),
			get_shader_test_variables());
}
