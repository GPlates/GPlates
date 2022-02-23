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

#include "app-logic/ScalarField3DLayerParams.h"

#include "file-io/ScalarField3DFileFormatReader.h"

#include "gui/RasterColourPalette.h"

#include "qt-widgets/GlobeAndMapWidget.h"
#include "qt-widgets/ReconstructionViewWidget.h"
#include "qt-widgets/ViewportWindow.h"


GPlatesPresentation::ScalarField3DVisualLayerParams::ScalarField3DVisualLayerParams(
		GPlatesAppLogic::LayerParams::non_null_ptr_type layer_params,
		GPlatesPresentation::ViewState &view_state) :
	VisualLayerParams(layer_params, view_state),
	d_scalar_colour_palette_parameters_initialised_from_scalar_field(false),
	d_gradient_colour_palette_parameters_initialised_from_scalar_field(false),
	d_isovalue_parameters_initialised_from_scalar_field(false),
	d_depth_restriction_initialised_from_scalar_field(false)
{
}


void
GPlatesPresentation::ScalarField3DVisualLayerParams::handle_layer_modified(
		const GPlatesAppLogic::Layer &layer)
{
	GPlatesAppLogic::ScalarField3DLayerParams *layer_params =
		dynamic_cast<GPlatesAppLogic::ScalarField3DLayerParams *>(
				get_layer_params().get());

	if (layer_params &&
		layer_params->get_scalar_field_feature())
	{
		//
		// Need to initialise some parameters that depend on the scalar data (eg, min/max/mean/std_dev).
		// This needs to be done only once but the scalar data needs to be ready/setup, so we just
		// choose here to initialise since we know the scalar data should have been setup by now.
		//

		if (!d_scalar_colour_palette_parameters_initialised_from_scalar_field)
		{
			// If we have a scalar field then map the palette range to the scalar mean +/- deviation.
			if (layer_params->get_scalar_mean() &&
				layer_params->get_scalar_standard_deviation())
			{
				GPlatesPresentation::RemappedColourPaletteParameters scalar_colour_palette_parameters =
						d_scalar_field_3d_render_parameters.get_scalar_colour_palette_parameters();

				scalar_colour_palette_parameters.map_palette_range(
						layer_params->get_scalar_mean().get() -
								scalar_colour_palette_parameters.get_deviation_from_mean() *
										layer_params->get_scalar_standard_deviation().get(),
						layer_params->get_scalar_mean().get() +
								scalar_colour_palette_parameters.get_deviation_from_mean() *
										layer_params->get_scalar_standard_deviation().get());

				d_scalar_field_3d_render_parameters.set_scalar_colour_palette_parameters(scalar_colour_palette_parameters);

				d_scalar_colour_palette_parameters_initialised_from_scalar_field = true;
			}
		}

		if (!d_gradient_colour_palette_parameters_initialised_from_scalar_field)
		{
			// If we have a scalar field then map the palette range to the gradient +/- (mean + deviation).
			if (layer_params->get_gradient_magnitude_mean() &&
				layer_params->get_gradient_magnitude_standard_deviation())
			{
				GPlatesPresentation::RemappedColourPaletteParameters gradient_colour_palette_parameters =
						d_scalar_field_3d_render_parameters.get_gradient_colour_palette_parameters();

				gradient_colour_palette_parameters.map_palette_range(
						-layer_params->get_gradient_magnitude_mean().get() -
								gradient_colour_palette_parameters.get_deviation_from_mean() *
										layer_params->get_gradient_magnitude_standard_deviation().get(),
						layer_params->get_gradient_magnitude_mean().get() +
								gradient_colour_palette_parameters.get_deviation_from_mean() *
										layer_params->get_gradient_magnitude_standard_deviation().get());

				d_scalar_field_3d_render_parameters.set_gradient_colour_palette_parameters(gradient_colour_palette_parameters);

				d_gradient_colour_palette_parameters_initialised_from_scalar_field = true;
			}
		}

		if (!d_isovalue_parameters_initialised_from_scalar_field)
		{
			// If we have a scalar field then set the isovalue.
			if (layer_params->get_scalar_mean())
			{
				d_scalar_field_3d_render_parameters.set_isovalue_parameters(
						GPlatesViewOperations::ScalarField3DRenderParameters::IsovalueParameters(
								layer_params->get_scalar_mean().get()));

				d_isovalue_parameters_initialised_from_scalar_field = true;
			}
		}

		if (!d_depth_restriction_initialised_from_scalar_field)
		{
			// If we have a scalar field then set the depth restriction range.
			if (layer_params->get_minimum_depth_layer_radius() &&
				layer_params->get_maximum_depth_layer_radius())
			{
				d_scalar_field_3d_render_parameters.set_depth_restriction(
						GPlatesViewOperations::ScalarField3DRenderParameters::DepthRestriction(
								layer_params->get_minimum_depth_layer_radius().get(),
								layer_params->get_maximum_depth_layer_radius().get()));

				d_depth_restriction_initialised_from_scalar_field = true;
			}
		}
	}
	// ...else there's no scalar field feature...

	emit_modified();
}
