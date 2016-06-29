/**
 * \file 
 * $Revision: 12398 $
 * $Date: 2011-10-07 04:02:38 -0700 (Fri, 07 Oct 2011) $
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
 
#include "TopologyNetworkVisualLayerParams.h"

#include "app-logic/Layer.h"

#include "gui/DefaultColourPalettes.h"

#include "maths/MathsUtils.h"


//
// NOTE: these are the defaults for a RTN; and they control the GUI defaults
//
GPlatesPresentation::TopologyNetworkVisualLayerParams::TopologyNetworkVisualLayerParams(
		GPlatesAppLogic::LayerParams::non_null_ptr_type layer_params) :
	VisualLayerParams(layer_params),
	d_show_delaunay_triangulation(      true),
	d_show_constrained_triangulation(   false),
	d_show_mesh_triangulation(          false),
	d_show_total_triangulation(         false),
	d_show_fill(                        false),
	d_show_segment_velocity(            false),
	d_color_index(                      1), // default colouring; not a derrived value 
	d_range1_max(						17.0),
	d_range1_min(						13.0),
	d_range2_max(						-13.0),
	d_range2_min(						-17.0),
	d_fg_colour( 						GPlatesGui::Colour(1, 1, 1) ),
	d_max_colour( 						GPlatesGui::Colour(1, 0, 0) ),
	d_mid_colour( 						GPlatesGui::Colour(1, 1, 1) ),
	d_min_colour(						GPlatesGui::Colour(0, 0, 1) ),
	d_bg_colour(						GPlatesGui::Colour(1, 1, 1) ),
	d_colour_palette_filename(QString())
{
}


void
GPlatesPresentation::TopologyNetworkVisualLayerParams::handle_layer_modified(
		const GPlatesAppLogic::Layer &layer)
{
	if (d_colour_palette_filename.isEmpty()) // i.e. colour palette auto-generated
	{
		d_colour_palette = GPlatesGui::ColourPalette<double>::non_null_ptr_type(
			GPlatesGui::DefaultColourPalettes::create_deformation_strain_colour_palette(
				d_range1_max, 
				d_range1_min, 
				d_range2_max, 
				d_range2_min, 
				d_fg_colour, 
				d_max_colour, 
				d_mid_colour, 
				d_min_colour,
				d_bg_colour
			));
	}
	emit_modified();
}


void
GPlatesPresentation::TopologyNetworkVisualLayerParams::set_colour_palette(
		const QString &filename,
		const GPlatesGui::ColourPalette<double>::non_null_ptr_type &colour_palette)
{
	d_colour_palette_filename = filename;
	d_colour_palette = colour_palette;
	emit_modified();
}


void
GPlatesPresentation::TopologyNetworkVisualLayerParams::user_generated_colour_palette()
{
	d_colour_palette_filename = QString();

	// Create a default colour palette.
	d_colour_palette = GPlatesGui::ColourPalette<double>::non_null_ptr_type(
			GPlatesGui::DefaultColourPalettes::create_deformation_strain_colour_palette(
					d_range1_max, d_range1_min,
					d_range2_max, d_range2_min,
					d_fg_colour, d_max_colour, d_mid_colour, d_min_colour, d_bg_colour));

	emit_modified();
}
