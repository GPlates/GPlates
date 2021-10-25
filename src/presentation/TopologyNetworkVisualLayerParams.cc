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

#include "gui/BuiltinColourPalettes.h"
#include "gui/DrawStyleManager.h"

#include "maths/MathsUtils.h"

#include "scribe/Scribe.h"
#include "scribe/TranscribeEnumProtocol.h"


GPlatesPresentation::TopologyNetworkVisualLayerParams::TopologyNetworkVisualLayerParams(
		GPlatesAppLogic::LayerParams::non_null_ptr_type layer_params) :
	VisualLayerParams(
			layer_params,
			GPlatesGui::DrawStyleManager::instance()->default_style()),
	d_colour_mode(COLOUR_DRAW_STYLE),
	d_min_abs_dilatation(1e-17),
	d_max_abs_dilatation(1e-13),
	d_dilatation_colour_palette_filename(QString()),
	d_min_abs_second_invariant(1e-17),
	d_max_abs_second_invariant(1e-13),
	d_second_invariant_colour_palette_filename(QString()),
	d_show_segment_velocity(false),
	d_fill_triangulation(false),
	d_fill_rigid_blocks(false),
	d_fill_opacity(1.0),
	d_fill_intensity(1.0)
{
}


void
GPlatesPresentation::TopologyNetworkVisualLayerParams::handle_layer_modified(
		const GPlatesAppLogic::Layer &layer)
{
	if (d_dilatation_colour_palette_filename.isEmpty()) // i.e. colour palette auto-generated
	{
		create_default_dilatation_colour_palette();
	}

	if (d_second_invariant_colour_palette_filename.isEmpty()) // i.e. colour palette auto-generated
	{
		create_default_second_invariant_colour_palette();
	}

	emit_modified();
}


void
GPlatesPresentation::TopologyNetworkVisualLayerParams::set_min_abs_dilatation(
		const double &min_abs_dilatation)
{
	d_min_abs_dilatation = min_abs_dilatation;

	if (d_dilatation_colour_palette_filename.isEmpty()) // i.e. colour palette auto-generated
	{
		create_default_dilatation_colour_palette();
	}

	emit_modified();
}


void
GPlatesPresentation::TopologyNetworkVisualLayerParams::set_max_abs_dilatation(
		const double &max_abs_dilatation)
{
	d_max_abs_dilatation = max_abs_dilatation;

	if (d_dilatation_colour_palette_filename.isEmpty()) // i.e. colour palette auto-generated
	{
		create_default_dilatation_colour_palette();
	}

	emit_modified();
}


void
GPlatesPresentation::TopologyNetworkVisualLayerParams::set_dilatation_colour_palette(
		const QString &filename,
		const GPlatesGui::ColourPalette<double>::non_null_ptr_type &colour_palette)
{
	d_dilatation_colour_palette_filename = filename;
	d_dilatation_colour_palette = colour_palette;

	emit_modified();
}


void
GPlatesPresentation::TopologyNetworkVisualLayerParams::use_default_dilatation_colour_palette()
{
	d_dilatation_colour_palette_filename = QString();
	create_default_dilatation_colour_palette();

	emit_modified();
}


void
GPlatesPresentation::TopologyNetworkVisualLayerParams::set_min_abs_second_invariant(
		const double &min_abs_second_invariant)
{
	d_min_abs_second_invariant = min_abs_second_invariant;

	if (d_second_invariant_colour_palette_filename.isEmpty()) // i.e. colour palette auto-generated
	{
		create_default_second_invariant_colour_palette();
	}

	emit_modified();
}


void
GPlatesPresentation::TopologyNetworkVisualLayerParams::set_max_abs_second_invariant(
		const double &max_abs_second_invariant)
{
	d_max_abs_second_invariant = max_abs_second_invariant;

	if (d_second_invariant_colour_palette_filename.isEmpty()) // i.e. colour palette auto-generated
	{
		create_default_second_invariant_colour_palette();
	}

	emit_modified();
}


void
GPlatesPresentation::TopologyNetworkVisualLayerParams::set_second_invariant_colour_palette(
		const QString &filename,
		const GPlatesGui::ColourPalette<double>::non_null_ptr_type &colour_palette)
{
	d_second_invariant_colour_palette_filename = filename;
	d_second_invariant_colour_palette = colour_palette;

	emit_modified();
}


void
GPlatesPresentation::TopologyNetworkVisualLayerParams::use_default_second_invariant_colour_palette()
{
	d_second_invariant_colour_palette_filename = QString();
	create_default_second_invariant_colour_palette();

	emit_modified();
}


void
GPlatesPresentation::TopologyNetworkVisualLayerParams::create_default_dilatation_colour_palette()
{
	d_dilatation_colour_palette = GPlatesGui::ColourPalette<double>::non_null_ptr_type(
			GPlatesGui::BuiltinColourPalettes::create_strain_colour_palette(
					d_min_abs_dilatation,
					d_max_abs_dilatation));
}


void
GPlatesPresentation::TopologyNetworkVisualLayerParams::create_default_second_invariant_colour_palette()
{
	d_second_invariant_colour_palette = GPlatesGui::ColourPalette<double>::non_null_ptr_type(
			GPlatesGui::BuiltinColourPalettes::create_strain_colour_palette(
					d_min_abs_second_invariant,
					d_max_abs_second_invariant));
}


GPlatesScribe::TranscribeResult
GPlatesPresentation::transcribe(
		GPlatesScribe::Scribe &scribe,
		TopologyNetworkVisualLayerParams::ColourMode &colour_mode,
		bool transcribed_construct_data)
{
	// WARNING: Changing the string ids will break backward/forward compatibility.
	//          So don't change the string ids even if the enum name changes.
	static const GPlatesScribe::EnumValue enum_values[] =
	{
		GPlatesScribe::EnumValue("COLOUR_DRAW_STYLE", TopologyNetworkVisualLayerParams::COLOUR_DRAW_STYLE),
		GPlatesScribe::EnumValue("COLOUR_DILATATION_STRAIN_RATE", TopologyNetworkVisualLayerParams::COLOUR_DILATATION_STRAIN_RATE),
		GPlatesScribe::EnumValue("COLOUR_SECOND_INVARIANT_STRAIN_RATE", TopologyNetworkVisualLayerParams::COLOUR_SECOND_INVARIANT_STRAIN_RATE)
	};

	return GPlatesScribe::transcribe_enum_protocol(
			TRANSCRIBE_SOURCE,
			scribe,
			colour_mode,
			enum_values,
			enum_values + sizeof(enum_values) / sizeof(enum_values[0]));
}
