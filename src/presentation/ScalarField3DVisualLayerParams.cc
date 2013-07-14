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

#include "app-logic/ScalarField3DLayerTask.h"

#include "file-io/ScalarField3DFileFormatReader.h"

#include "gui/RasterColourPalette.h"


GPlatesPresentation::ScalarField3DVisualLayerParams::ScalarField3DVisualLayerParams(
		GPlatesAppLogic::LayerTaskParams &layer_task_params) :
	VisualLayerParams(layer_task_params),
	d_render_mode(GPlatesViewOperations::ScalarField3DRenderParameters::RENDER_MODE_ISOSURFACE),
	d_isosurface_deviation_window_mode(GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_DEVIATION_WINDOW_MODE_NONE),
	d_isosurface_colour_mode(GPlatesViewOperations::ScalarField3DRenderParameters::ISOSURFACE_COLOUR_MODE_DEPTH),
	d_cross_section_colour_mode(GPlatesViewOperations::ScalarField3DRenderParameters::CROSS_SECTION_COLOUR_MODE_SCALAR),
	d_scalar_colour_palette(GPlatesViewOperations::ScalarField3DRenderParameters::ColourPalette::SCALAR),
	d_initialised_scalar_colour_palette_range_mapping(false),
	d_gradient_colour_palette(GPlatesViewOperations::ScalarField3DRenderParameters::ColourPalette::GRADIENT),
	d_initialised_gradient_colour_palette_range_mapping(false)
{
}


void
GPlatesPresentation::ScalarField3DVisualLayerParams::handle_layer_modified(
		const GPlatesAppLogic::Layer &layer)
{
	GPlatesAppLogic::ScalarField3DLayerTask::Params *layer_task_params =
		dynamic_cast<GPlatesAppLogic::ScalarField3DLayerTask::Params *>(
				&get_layer_task_params());

	if (layer_task_params && layer_task_params->get_scalar_field_feature())
	{
		update(true);
		return;
	}
	// ...there's no scalar field feature...

	emit_modified();
}


GPlatesViewOperations::ScalarField3DRenderParameters
GPlatesPresentation::ScalarField3DVisualLayerParams::get_scalar_field_3d_render_parameters() const
{
	GPlatesViewOperations::ScalarField3DRenderParameters::IsovalueParameters isovalue_parameters;
	if (get_isovalue_parameters())
	{
		isovalue_parameters = get_isovalue_parameters().get();
	}

	GPlatesViewOperations::ScalarField3DRenderParameters::DepthRestriction depth_restriction;
	if (get_depth_restriction())
	{
		depth_restriction = get_depth_restriction().get();
	}

	return GPlatesViewOperations::ScalarField3DRenderParameters(
			get_render_mode(),
			get_isosurface_deviation_window_mode(),
			get_isosurface_colour_mode(),
			get_cross_section_colour_mode(),
			get_scalar_colour_palette(),
			get_gradient_colour_palette(),
			isovalue_parameters,
			get_deviation_window_render_options(),
			get_surface_polygons_mask(),
			depth_restriction,
			get_quality_performance(),
			get_shader_test_variables());
}


void
GPlatesPresentation::ScalarField3DVisualLayerParams::update(
		bool always_emit_modified_signal)
{
	bool emit_modified_signal = always_emit_modified_signal;

	// Once we know the scalar field's scalar mean/std-dev we can set up the initial palette range mapping.
	if (!d_initialised_scalar_colour_palette_range_mapping)
	{
		GPlatesAppLogic::ScalarField3DLayerTask::Params *layer_task_params =
			dynamic_cast<GPlatesAppLogic::ScalarField3DLayerTask::Params *>(
					&get_layer_task_params());

		// If we have a scalar field then map the palette range to the scalar mean +/- deviation.
		if (layer_task_params &&
			layer_task_params->get_scalar_mean() &&
			layer_task_params->get_scalar_standard_deviation())
		{
			d_scalar_colour_palette.map_palette_range(
					layer_task_params->get_scalar_mean().get() -
							d_scalar_colour_palette.get_deviation_from_mean() *
									layer_task_params->get_scalar_standard_deviation().get(),
					layer_task_params->get_scalar_mean().get() +
							d_scalar_colour_palette.get_deviation_from_mean() *
									layer_task_params->get_scalar_standard_deviation().get());

			d_initialised_scalar_colour_palette_range_mapping = true;

			emit_modified_signal = true;
		}
	}

	// Once we know the scalar field's gradient mean/std-dev we can set up the initial palette range mapping.
	if (!d_initialised_gradient_colour_palette_range_mapping)
	{
		GPlatesAppLogic::ScalarField3DLayerTask::Params *layer_task_params =
			dynamic_cast<GPlatesAppLogic::ScalarField3DLayerTask::Params *>(
					&get_layer_task_params());

		// If we have a scalar field then map the palette range to the gradient +/- (mean + deviation).
		if (layer_task_params &&
			layer_task_params->get_gradient_magnitude_mean() &&
			layer_task_params->get_gradient_magnitude_standard_deviation())
		{
			d_gradient_colour_palette.map_palette_range(
					-layer_task_params->get_gradient_magnitude_mean().get() -
							d_gradient_colour_palette.get_deviation_from_mean() *
									layer_task_params->get_gradient_magnitude_standard_deviation().get(),
					layer_task_params->get_gradient_magnitude_mean().get() +
							d_gradient_colour_palette.get_deviation_from_mean() *
									layer_task_params->get_gradient_magnitude_standard_deviation().get());

			d_initialised_gradient_colour_palette_range_mapping = true;

			emit_modified_signal = true;
		}
	}

	// If the isovalue parameters have not yet been set then initialise them with the scalar field mean value.
	if (!d_isovalue_parameters)
	{
		GPlatesAppLogic::ScalarField3DLayerTask::Params *layer_task_params =
			dynamic_cast<GPlatesAppLogic::ScalarField3DLayerTask::Params *>(
					&get_layer_task_params());

		// If we have a scalar field then set the isovalue.
		if (layer_task_params && layer_task_params->get_scalar_mean())
		{
			d_isovalue_parameters = GPlatesViewOperations::ScalarField3DRenderParameters::IsovalueParameters(
					layer_task_params->get_scalar_mean().get());

			emit_modified_signal = true;
		}
	}

	// If the depth restriction range has not yet been set then initialise it with the
	// scalar field depth range.
	if (!d_depth_restriction)
	{
		GPlatesAppLogic::ScalarField3DLayerTask::Params *layer_task_params =
			dynamic_cast<GPlatesAppLogic::ScalarField3DLayerTask::Params *>(
					&get_layer_task_params());

		// If we have a scalar field then set the depth restriction range.
		if (layer_task_params &&
			layer_task_params->get_minimum_depth_layer_radius() &&
			layer_task_params->get_maximum_depth_layer_radius())
		{
			d_depth_restriction =
					GPlatesViewOperations::ScalarField3DRenderParameters::DepthRestriction(
							layer_task_params->get_minimum_depth_layer_radius().get(),
							layer_task_params->get_maximum_depth_layer_radius().get());

			emit_modified_signal = true;
		}
	}

	if (emit_modified_signal)
	{
		emit_modified();
	}
}
