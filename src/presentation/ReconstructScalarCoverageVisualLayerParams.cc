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
	// Assume that the scalar types have changed, so:
	//  (1) Add any new scalar types that weren't there before, and
	//  (2) Remove any scalar types that aren't there anymore.
	//

	// Add any new scalar types that weren't there before.
	const std::vector<GPlatesPropertyValues::ValueObjectType> &scalar_types = get_scalar_types();
	BOOST_FOREACH(const GPlatesPropertyValues::ValueObjectType &scalar_type, scalar_types)
	{
		// See if a colour palette already exists for the scalar type.
		if (!get_colour_palette_parameters(scalar_type))
		{
			// Ensure a colour palette exists for the scalar type.
			create_colour_palette_parameters(scalar_type);
			modified_params = true;
		}
	}

	// Remove any scalar types that aren't there anymore.
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
	return get_or_create_colour_palette_parameters(get_current_scalar_type());
}


void
GPlatesPresentation::ReconstructScalarCoverageVisualLayerParams::set_current_colour_palette_parameters(
		const RemappedColourPaletteParameters &colour_palette_parameters)
{
	set_colour_palette_parameters(
			get_current_scalar_type(),
			colour_palette_parameters);
}


boost::optional<const GPlatesPresentation::RemappedColourPaletteParameters &>
GPlatesPresentation::ReconstructScalarCoverageVisualLayerParams::get_colour_palette_parameters(
		const GPlatesPropertyValues::ValueObjectType &scalar_type) const
{
	// Look for the colour palette parameters associated with the scalar type.
	colour_palette_parameters_map_type::const_iterator colour_palette_parameters_iter =
			d_colour_palette_parameters_map.find(scalar_type);

	if (colour_palette_parameters_iter == d_colour_palette_parameters_map.end())
	{
		return boost::none;
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


const std::vector<GPlatesPropertyValues::ValueObjectType> &
GPlatesPresentation::ReconstructScalarCoverageVisualLayerParams::get_scalar_types() const
{
	GPlatesAppLogic::ReconstructScalarCoverageLayerParams *layer_params =
		dynamic_cast<GPlatesAppLogic::ReconstructScalarCoverageLayerParams *>(
				get_layer_params().get());

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			layer_params,
			GPLATES_ASSERTION_SOURCE);

	return layer_params->get_scalar_types();

}


const GPlatesPresentation::RemappedColourPaletteParameters &
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
	RemappedColourPaletteParameters colour_palette_parameters(
			GPlatesGui::RasterColourPalette::create<double>(
					GPlatesGui::BuiltinColourPalettes::create_scalar_colour_palette()));

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

	return d_colour_palette_parameters_map.insert(
			colour_palette_parameters_map_type::value_type(
					scalar_type,
					colour_palette_parameters)).first->second;
}


const GPlatesPresentation::RemappedColourPaletteParameters &
GPlatesPresentation::ReconstructScalarCoverageVisualLayerParams::get_or_create_colour_palette_parameters(
		const GPlatesPropertyValues::ValueObjectType &scalar_type) const
{
	// See if a colour palette already exists for the scalar type.
	boost::optional<const RemappedColourPaletteParameters &> scalar_type_colour_palette_parameters =
			get_colour_palette_parameters(scalar_type);
	if (!scalar_type_colour_palette_parameters)
	{
		// Ensure a colour palette exists for the scalar type.
		scalar_type_colour_palette_parameters = create_colour_palette_parameters(scalar_type);
	}

	return scalar_type_colour_palette_parameters.get();
}
