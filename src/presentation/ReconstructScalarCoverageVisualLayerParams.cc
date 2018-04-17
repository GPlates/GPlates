/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
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

#include <algorithm>
#include <boost/foreach.hpp>

#include "ReconstructScalarCoverageVisualLayerParams.h"

#include "app-logic/ReconstructScalarCoverageLayerParams.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "gui/BuiltinColourPalettes.h"


GPlatesPresentation::ReconstructScalarCoverageVisualLayerParams::ReconstructScalarCoverageVisualLayerParams(
		GPlatesAppLogic::LayerParams::non_null_ptr_type layer_params) :
	VisualLayerParams(layer_params)
{
}


void
GPlatesPresentation::ReconstructScalarCoverageVisualLayerParams::handle_layer_modified(
		const GPlatesAppLogic::Layer &layer)
{
	bool modified_params = false;

	//
	// Assume that the scalar types have changed (ie, new types introduced or old types no longer exist).
	// Note that it doesn't matter if the coverage data (and associated statistics) have changed because the
	// colour palette parameters will only get changed if the user explicitly does so (eg, click min/max mapping button).
	//
	// We only remove any previously created colour palette parameters (associated with scalar types that no longer exist).
	// We don't also create any colour palette parameters that don't yet exist (for any scalar types).
	// This is because we'd prefer to delay creation as long as possible. Basically colour palette parameters are only
	// created when the client asks for them. This delays creation until its actually needed since it can be very expensive
	// (eg, when the full reconstructed history of a scalar coverage is needed to gather the statistics used to generate
	// the colour palette). An example of delaying: Initially the layer might be invisible and layer options not expanded,
	// and so the colour palette does not need to be generated yet. The creation is then delayed either to when the layer
	// is made visible or to when the layer options are expanded.
	//

	std::vector<GPlatesPropertyValues::ValueObjectType> scalar_types;
	get_scalar_types(scalar_types);

	// Remove any colour palettes associated with scalar types that don't exist anymore.
	colour_palette_parameters_map_type::iterator colour_palette_parameters_iter = d_colour_palette_parameters_map.begin();
	while (colour_palette_parameters_iter != d_colour_palette_parameters_map.end())
	{
		const GPlatesPropertyValues::ValueObjectType &scalar_type = colour_palette_parameters_iter->first;

		if (std::find(scalar_types.begin(), scalar_types.end(), scalar_type) == scalar_types.end())
		{
			// Increment iterator before erasing.
			colour_palette_parameters_map_type::iterator erase_iter = colour_palette_parameters_iter;
			++colour_palette_parameters_iter;

			d_colour_palette_parameters_map.erase(erase_iter);
			modified_params = true;
		}
		else
		{
			++colour_palette_parameters_iter;
		}
	}

	if (modified_params)
	{
		emit_modified();
	}
}


const GPlatesPresentation::RemappedColourPaletteParameters &
GPlatesPresentation::ReconstructScalarCoverageVisualLayerParams::get_current_colour_palette_parameters() const
{
	return get_colour_palette_parameters(get_current_scalar_type());
}


void
GPlatesPresentation::ReconstructScalarCoverageVisualLayerParams::set_current_colour_palette_parameters(
		const RemappedColourPaletteParameters &colour_palette_parameters)
{
	set_colour_palette_parameters(
			get_current_scalar_type(),
			colour_palette_parameters);
}


GPlatesPresentation::RemappedColourPaletteParameters
GPlatesPresentation::ReconstructScalarCoverageVisualLayerParams::create_default_colour_palette_parameters()
{
	return RemappedColourPaletteParameters(
			GPlatesGui::RasterColourPalette::create<double>(
					GPlatesGui::BuiltinColourPalettes::create_scalar_colour_palette()));
}


const GPlatesPresentation::RemappedColourPaletteParameters &
GPlatesPresentation::ReconstructScalarCoverageVisualLayerParams::get_colour_palette_parameters(
		const GPlatesPropertyValues::ValueObjectType &scalar_type) const
{
	// See if a colour palette already exists for the scalar type.
	colour_palette_parameters_map_type::const_iterator colour_palette_parameters_iter =
			d_colour_palette_parameters_map.find(scalar_type);
	if (colour_palette_parameters_iter == d_colour_palette_parameters_map.end())
	{
		// Create a colour palette for the scalar type.
		const RemappedColourPaletteParameters colour_palette_parameters =
				create_colour_palette_parameters(scalar_type);

		colour_palette_parameters_iter =
				d_colour_palette_parameters_map.insert(
						colour_palette_parameters_map_type::value_type(
								scalar_type,
								colour_palette_parameters)).first;
	}

	return colour_palette_parameters_iter->second;
}


void
GPlatesPresentation::ReconstructScalarCoverageVisualLayerParams::set_colour_palette_parameters(
		const GPlatesPropertyValues::ValueObjectType &scalar_type,
		const RemappedColourPaletteParameters &colour_palette_parameters)
{
	// See if a colour palette already exists for the specified scalar type.
	colour_palette_parameters_map_type::iterator colour_palette_parameters_iter =
			d_colour_palette_parameters_map.find(scalar_type);

	if (colour_palette_parameters_iter != d_colour_palette_parameters_map.end())
	{
		// Overwrite the existing palette map entry.
		colour_palette_parameters_iter->second = colour_palette_parameters;
	}
	else
	{
		// Add the palette as a new map entry.
		d_colour_palette_parameters_map.insert(
				colour_palette_parameters_map_type::value_type(
						scalar_type,
						colour_palette_parameters));
	}

	emit_modified();
}


const GPlatesPropertyValues::ValueObjectType &
GPlatesPresentation::ReconstructScalarCoverageVisualLayerParams::get_current_scalar_type() const
{
	GPlatesAppLogic::ReconstructScalarCoverageLayerParams *layer_params =
		dynamic_cast<GPlatesAppLogic::ReconstructScalarCoverageLayerParams *>(
				get_layer_params().get());

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			layer_params,
			GPLATES_ASSERTION_SOURCE);

	return layer_params->get_scalar_type();
}


void
GPlatesPresentation::ReconstructScalarCoverageVisualLayerParams::get_scalar_types(
		std::vector<GPlatesPropertyValues::ValueObjectType> &scalar_types) const
{
	GPlatesAppLogic::ReconstructScalarCoverageLayerParams *layer_params =
		dynamic_cast<GPlatesAppLogic::ReconstructScalarCoverageLayerParams *>(
				get_layer_params().get());

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			layer_params,
			GPLATES_ASSERTION_SOURCE);

	layer_params->get_scalar_types(scalar_types);
}


GPlatesPresentation::RemappedColourPaletteParameters
GPlatesPresentation::ReconstructScalarCoverageVisualLayerParams::create_colour_palette_parameters(
		const GPlatesPropertyValues::ValueObjectType &scalar_type) const
{
	GPlatesAppLogic::ReconstructScalarCoverageLayerParams *layer_params =
		dynamic_cast<GPlatesAppLogic::ReconstructScalarCoverageLayerParams *>(
				get_layer_params().get());

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			layer_params,
			GPLATES_ASSERTION_SOURCE);

	// Create a new colour palette parameters.
	RemappedColourPaletteParameters colour_palette_parameters = create_default_colour_palette_parameters();

	// If we have scalar data then initialise the palette range to the scalar mean +/- deviation.
	boost::optional<GPlatesPropertyValues::ScalarCoverageStatistics> statistics =
			layer_params->get_scalar_statistics(scalar_type);
	if (statistics)
	{
		const double scalar_mean = statistics->mean;
		const double scalar_std_dev = statistics->standard_deviation;

		colour_palette_parameters.map_palette_range(
				scalar_mean - colour_palette_parameters.get_deviation_from_mean() * scalar_std_dev,
				scalar_mean + colour_palette_parameters.get_deviation_from_mean() * scalar_std_dev);
	}

	return colour_palette_parameters;
}
