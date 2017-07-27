/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#include <utility>
#include <boost/optional.hpp>
#include <boost/foreach.hpp>
 
#include "RasterVisualLayerParams.h"

#include "app-logic/ExtractRasterFeatureProperties.h"
#include "app-logic/Layer.h"
#include "app-logic/RasterLayerParams.h"

#include "gui/BuiltinColourPalettes.h"

#include "maths/MathsUtils.h"


GPlatesPresentation::RasterVisualLayerParams::RasterVisualLayerParams(
		GPlatesAppLogic::LayerParams::non_null_ptr_type layer_params) :
	VisualLayerParams(layer_params),
	d_colour_palette_parameters_initialised_from_raster(false),
	d_colour_palette_parameters(
			GPlatesGui::RasterColourPalette::create<double>(
					GPlatesGui::BuiltinColourPalettes::create_scalar_colour_palette())),
	d_raster_type(GPlatesPropertyValues::RasterType::UNKNOWN),
	d_opacity(1.0),
	d_intensity(1.0),
	d_surface_relief_scale(1)
{
}


void
GPlatesPresentation::RasterVisualLayerParams::handle_layer_modified(
		const GPlatesAppLogic::Layer &layer)
{
	GPlatesAppLogic::RasterLayerParams *raster_layer_params =
		dynamic_cast<GPlatesAppLogic::RasterLayerParams *>(
				get_layer_params().get());

	if (raster_layer_params &&
		raster_layer_params->get_raster_feature())
	{
		//
		// Need to initialise some parameters that depend on the raster data (eg, mean/std_dev).
		// This needs to be done only once but the raster data needs to be ready/setup, so we just
		// choose here to initialise since we know the raster data should have been setup by now.
		//

		if (!d_colour_palette_parameters_initialised_from_raster)
		{
			// We have a raster feature so map the palette range to the scalar mean +/- deviation.

			// Default values result in clearing the colour scale widget.
			double raster_scalar_mean = 0;
			double raster_scalar_std_dev = 0;

			const GPlatesPropertyValues::RasterStatistics raster_statistic =
					raster_layer_params->get_band_statistic();
			if (raster_statistic.mean &&
				raster_statistic.standard_deviation)
			{
				raster_scalar_mean = raster_statistic.mean.get();
				raster_scalar_std_dev = raster_statistic.standard_deviation.get();
			}

			d_colour_palette_parameters.map_palette_range(
					raster_scalar_mean - d_colour_palette_parameters.get_deviation_from_mean() * raster_scalar_std_dev,
					raster_scalar_mean + d_colour_palette_parameters.get_deviation_from_mean() * raster_scalar_std_dev);

			d_colour_palette_parameters_initialised_from_raster = true;
		}

		d_raster_type = raster_layer_params->get_raster_type();
	}
	// ...else there's no raster feature...

	emit_modified();
}


void
GPlatesPresentation::RasterVisualLayerParams::set_colour_palette_parameters(
		const GPlatesPresentation::RemappedColourPaletteParameters &colour_palette_parameters)
{
	d_colour_palette_parameters = colour_palette_parameters;
	d_colour_palette_parameters_initialised_from_raster = true;
	emit_modified();
}


GPlatesPropertyValues::RasterType::Type
GPlatesPresentation::RasterVisualLayerParams::get_raster_type() const
{
	return d_raster_type;
}


void
GPlatesPresentation::RasterVisualLayerParams::set_opacity(
		const double &opacity)
{
	d_opacity = opacity;
	emit_modified();
}


void
GPlatesPresentation::RasterVisualLayerParams::set_intensity(
		const double &intensity)
{
	d_intensity = intensity;
	emit_modified();
}


GPlatesGui::Colour
GPlatesPresentation::RasterVisualLayerParams::get_modulate_colour() const
{
	return GPlatesGui::Colour(d_intensity, d_intensity, d_intensity, d_opacity);
}


void
GPlatesPresentation::RasterVisualLayerParams::set_surface_relief_scale(
		float surface_relief_scale)
{
	d_surface_relief_scale = surface_relief_scale;
	emit_modified();
}
