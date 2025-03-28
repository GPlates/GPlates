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
	d_triangulation_colour_mode(TRIANGULATION_COLOUR_DRAW_STYLE),
	// Display as mesh by default (instead of just a boundary) since it's a good visual indicator
	// of where deforming regions are. The default colour mode is still by draw style (ie, flat coloured).
	// This seemed like a good compromise between no mesh (ie, boundary - which is fastest) and
	// blue/red strain rate (which is slowest).
	d_triangulation_draw_mode(TRIANGULATION_DRAW_MESH),
	d_min_abs_dilatation(1e-17),
	d_max_abs_dilatation(3e-14),
	d_dilatation_colour_palette_filename(QString()),
	d_min_abs_second_invariant(1e-17),
	d_max_abs_second_invariant(3e-14),
	d_second_invariant_colour_palette_filename(QString()),
	d_min_strain_rate_style(-1.0),
	d_max_strain_rate_style(1.0),
	d_strain_rate_style_colour_palette_filename(QString()),
	d_show_segment_velocity(false),
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
GPlatesPresentation::TopologyNetworkVisualLayerParams::set_min_strain_rate_style(
		const double &min_strain_rate_style)
{
	d_min_strain_rate_style = min_strain_rate_style;

	if (d_strain_rate_style_colour_palette_filename.isEmpty()) // i.e. colour palette auto-generated
	{
		create_default_strain_rate_style_colour_palette();
	}

	emit_modified();
}


void
GPlatesPresentation::TopologyNetworkVisualLayerParams::set_max_strain_rate_style(
		const double &max_strain_rate_style)
{
	d_max_strain_rate_style = max_strain_rate_style;

	if (d_strain_rate_style_colour_palette_filename.isEmpty()) // i.e. colour palette auto-generated
	{
		create_default_strain_rate_style_colour_palette();
	}

	emit_modified();
}


void
GPlatesPresentation::TopologyNetworkVisualLayerParams::set_strain_rate_style_colour_palette(
		const QString &filename,
		const GPlatesGui::ColourPalette<double>::non_null_ptr_type &colour_palette)
{
	d_strain_rate_style_colour_palette_filename = filename;
	d_strain_rate_style_colour_palette = colour_palette;

	emit_modified();
}


void
GPlatesPresentation::TopologyNetworkVisualLayerParams::use_default_strain_rate_style_colour_palette()
{
	d_strain_rate_style_colour_palette_filename = QString();
	create_default_strain_rate_style_colour_palette();

	emit_modified();
}


void
GPlatesPresentation::TopologyNetworkVisualLayerParams::create_default_dilatation_colour_palette()
{
	d_dilatation_colour_palette = GPlatesGui::ColourPalette<double>::non_null_ptr_type(
			GPlatesGui::BuiltinColourPalettes::create_strain_rate_dilatation_colour_palette(
					d_min_abs_dilatation,
					d_max_abs_dilatation));
}


void
GPlatesPresentation::TopologyNetworkVisualLayerParams::create_default_second_invariant_colour_palette()
{
	d_second_invariant_colour_palette = GPlatesGui::ColourPalette<double>::non_null_ptr_type(
			GPlatesGui::BuiltinColourPalettes::create_strain_rate_second_invariant_colour_palette(
					d_min_abs_second_invariant,
					d_max_abs_second_invariant));
}


void
GPlatesPresentation::TopologyNetworkVisualLayerParams::create_default_strain_rate_style_colour_palette()
{
	d_strain_rate_style_colour_palette = GPlatesGui::ColourPalette<double>::non_null_ptr_type(
			GPlatesGui::BuiltinColourPalettes::create_strain_rate_strain_rate_style_colour_palette(
					d_min_strain_rate_style,
					d_max_strain_rate_style));
}


GPlatesScribe::TranscribeResult
GPlatesPresentation::transcribe(
		GPlatesScribe::Scribe &scribe,
		TopologyNetworkVisualLayerParams::TriangulationColourMode &triangulation_colour_mode,
		bool transcribed_construct_data)
{
	// WARNING: Changing the string ids will break backward/forward compatibility.
	//          So don't change the string ids even if the enum name changes.
	static const GPlatesScribe::EnumValue enum_values[] =
	{
		GPlatesScribe::EnumValue("COLOUR_DRAW_STYLE", TopologyNetworkVisualLayerParams::TRIANGULATION_COLOUR_DRAW_STYLE),
		GPlatesScribe::EnumValue("COLOUR_DILATATION_STRAIN_RATE", TopologyNetworkVisualLayerParams::TRIANGULATION_COLOUR_DILATATION_STRAIN_RATE),
		GPlatesScribe::EnumValue("COLOUR_SECOND_INVARIANT_STRAIN_RATE", TopologyNetworkVisualLayerParams::TRIANGULATION_COLOUR_SECOND_INVARIANT_STRAIN_RATE),
		GPlatesScribe::EnumValue("COLOUR_COLOUR_STRAIN_RATE_STYLE", TopologyNetworkVisualLayerParams::TRIANGULATION_COLOUR_STRAIN_RATE_STYLE)
	};

	return GPlatesScribe::transcribe_enum_protocol(
			TRANSCRIBE_SOURCE,
			scribe,
			triangulation_colour_mode,
			enum_values,
			enum_values + sizeof(enum_values) / sizeof(enum_values[0]));
}


GPlatesScribe::TranscribeResult
GPlatesPresentation::transcribe(
		GPlatesScribe::Scribe &scribe,
		TopologyNetworkVisualLayerParams::TriangulationDrawMode &triangulation_draw_mode,
		bool transcribed_construct_data)
{
	// WARNING: Changing the string ids will break backward/forward compatibility.
	//          So don't change the string ids even if the enum name changes.
	static const GPlatesScribe::EnumValue enum_values[] =
	{
		// Note: Changed enum from TRIANGULATION_DRAW_BOUNDARY to TRIANGULATION_DRAW_NONE,
		//       but we keep the string id "DRAW_BOUNDARY" for compatibility...
		GPlatesScribe::EnumValue("DRAW_BOUNDARY", TopologyNetworkVisualLayerParams::TRIANGULATION_DRAW_NONE),
		GPlatesScribe::EnumValue("DRAW_MESH", TopologyNetworkVisualLayerParams::TRIANGULATION_DRAW_MESH),
		GPlatesScribe::EnumValue("DRAW_FILL", TopologyNetworkVisualLayerParams::TRIANGULATION_DRAW_FILL)
	};

	return GPlatesScribe::transcribe_enum_protocol(
			TRANSCRIBE_SOURCE,
			scribe,
			triangulation_draw_mode,
			enum_values,
			enum_values + sizeof(enum_values) / sizeof(enum_values[0]));
}
