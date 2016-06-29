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

#include <QDebug>

#include "ScalarField3DRenderParameters.h"

#include "gui/ColourPaletteRangeRemapper.h"
#include "gui/ColourPaletteUtils.h"
#include "gui/DefaultColourPalettes.h"

#include "scribe/Scribe.h"
#include "scribe/TranscribeEnumProtocol.h"


GPlatesViewOperations::ScalarField3DRenderParameters::ScalarField3DRenderParameters() :
	d_render_mode(RENDER_MODE_ISOSURFACE),
	d_isosurface_deviation_window_mode(ISOSURFACE_DEVIATION_WINDOW_MODE_NONE),
	d_isosurface_colour_mode(ISOSURFACE_COLOUR_MODE_DEPTH),
	d_cross_section_colour_mode(CROSS_SECTION_COLOUR_MODE_SCALAR),
	d_scalar_colour_palette_parameters(
			GPlatesGui::RasterColourPalette::create<double>(
					GPlatesGui::DefaultColourPalettes::create_scalar_colour_palette())),
	d_gradient_colour_palette_parameters(
			GPlatesGui::RasterColourPalette::create<double>(
					GPlatesGui::DefaultColourPalettes::create_gradient_colour_palette()))
{
}


GPlatesViewOperations::ScalarField3DRenderParameters::ScalarField3DRenderParameters(
		RenderMode render_mode,
		IsosurfaceDeviationWindowMode isosurface_deviation_window_mode,
		IsosurfaceColourMode isosurface_colour_mode,
		CrossSectionColourMode cross_section_colour_mode,
		const GPlatesPresentation::RemappedColourPaletteParameters &scalar_colour_palette_parameters,
		const GPlatesPresentation::RemappedColourPaletteParameters &gradient_colour_palette_parameters,
		const IsovalueParameters &isovalue_parameters,
		const DeviationWindowRenderOptions &deviation_window_render_options,
		const SurfacePolygonsMask &surface_polygons_mask,
		const DepthRestriction &depth_restriction,
		const QualityPerformance &quality_performance,
		const std::vector<float> &shader_test_variables) :
	d_render_mode(render_mode),
	d_isosurface_deviation_window_mode(isosurface_deviation_window_mode),
	d_isosurface_colour_mode(isosurface_colour_mode),
	d_cross_section_colour_mode(cross_section_colour_mode),
	d_scalar_colour_palette_parameters(scalar_colour_palette_parameters),
	d_gradient_colour_palette_parameters(gradient_colour_palette_parameters),
	d_isovalue_parameters(isovalue_parameters),
	d_deviation_window_render_options(deviation_window_render_options),
	d_surface_polygons_mask(surface_polygons_mask),
	d_depth_restriction(depth_restriction),
	d_quality_performance(quality_performance),
	d_shader_test_variables(shader_test_variables)
{
}


GPlatesViewOperations::ScalarField3DRenderParameters::IsovalueParameters::IsovalueParameters(
		float isovalue_) :
	isovalue1(isovalue_),
	lower_deviation1(0),
	upper_deviation1(0),
	isovalue2(isovalue_),
	lower_deviation2(0),
	upper_deviation2(0),
	symmetric_deviation(true)
{
}


GPlatesScribe::TranscribeResult
GPlatesViewOperations::ScalarField3DRenderParameters::IsovalueParameters::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	// Provide default values for failed parameters instead of returning failure.
	// This way a future version of GPlates can add or remove parameters and still be backward/forward compatible.
	static const IsovalueParameters DEFAULT_PARAMS;

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, isovalue1, "isovalue1"))
	{
		isovalue1 = DEFAULT_PARAMS.isovalue1;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, lower_deviation1, "lower_deviation1"))
	{
		lower_deviation1 = DEFAULT_PARAMS.lower_deviation1;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, upper_deviation1, "upper_deviation1"))
	{
		upper_deviation1 = DEFAULT_PARAMS.upper_deviation1;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, isovalue2, "isovalue2"))
	{
		isovalue2 = DEFAULT_PARAMS.isovalue2;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, lower_deviation2, "lower_deviation2"))
	{
		lower_deviation2 = DEFAULT_PARAMS.lower_deviation2;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, upper_deviation2, "upper_deviation2"))
	{
		upper_deviation2 = DEFAULT_PARAMS.upper_deviation2;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, symmetric_deviation, "symmetric_deviation"))
	{
		symmetric_deviation = DEFAULT_PARAMS.symmetric_deviation;
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesViewOperations::ScalarField3DRenderParameters::DeviationWindowRenderOptions::DeviationWindowRenderOptions(
		float opacity_deviation_surfaces_,
		bool deviation_window_volume_rendering_,
		float opacity_deviation_window_volume_rendering_,
		bool surface_deviation_window_,
		unsigned int surface_deviation_window_isoline_frequency_) :
	opacity_deviation_surfaces(opacity_deviation_surfaces_),
	deviation_window_volume_rendering(deviation_window_volume_rendering_),
	opacity_deviation_window_volume_rendering(opacity_deviation_window_volume_rendering_),
	surface_deviation_window(surface_deviation_window_),
	surface_deviation_window_isoline_frequency(surface_deviation_window_isoline_frequency_)
{
}


GPlatesScribe::TranscribeResult
GPlatesViewOperations::ScalarField3DRenderParameters::DeviationWindowRenderOptions::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	// Provide default values for failed parameters instead of returning failure.
	// This way a future version of GPlates can add or remove parameters and still be backward/forward compatible.
	static const DeviationWindowRenderOptions DEFAULT_PARAMS;

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, opacity_deviation_surfaces, "opacity_deviation_surfaces"))
	{
		opacity_deviation_surfaces = DEFAULT_PARAMS.opacity_deviation_surfaces;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, deviation_window_volume_rendering, "deviation_window_volume_rendering"))
	{
		deviation_window_volume_rendering = DEFAULT_PARAMS.deviation_window_volume_rendering;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, opacity_deviation_window_volume_rendering, "opacity_deviation_window_volume_rendering"))
	{
		opacity_deviation_window_volume_rendering = DEFAULT_PARAMS.opacity_deviation_window_volume_rendering;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, surface_deviation_window, "surface_deviation_window"))
	{
		surface_deviation_window = DEFAULT_PARAMS.surface_deviation_window;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, surface_deviation_window_isoline_frequency, "surface_deviation_window_isoline_frequency"))
	{
		surface_deviation_window_isoline_frequency = DEFAULT_PARAMS.surface_deviation_window_isoline_frequency;
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesViewOperations::ScalarField3DRenderParameters::SurfacePolygonsMask::SurfacePolygonsMask(
		bool enable_surface_polygons_mask_,
		bool treat_polylines_as_polygons_,
		bool show_polygon_walls_,
		bool only_show_boundary_walls_) :
	enable_surface_polygons_mask(enable_surface_polygons_mask_),
	treat_polylines_as_polygons(treat_polylines_as_polygons_),
	show_polygon_walls(show_polygon_walls_),
	only_show_boundary_walls(only_show_boundary_walls_)
{
}


GPlatesScribe::TranscribeResult
GPlatesViewOperations::ScalarField3DRenderParameters::SurfacePolygonsMask::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	// Provide default values for failed parameters instead of returning failure.
	// This way a future version of GPlates can add or remove parameters and still be backward/forward compatible.
	static const SurfacePolygonsMask DEFAULT_PARAMS;

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, enable_surface_polygons_mask, "enable_surface_polygons_mask"))
	{
		enable_surface_polygons_mask = DEFAULT_PARAMS.enable_surface_polygons_mask;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, treat_polylines_as_polygons, "treat_polylines_as_polygons"))
	{
		treat_polylines_as_polygons = DEFAULT_PARAMS.treat_polylines_as_polygons;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, show_polygon_walls, "show_polygon_walls"))
	{
		show_polygon_walls = DEFAULT_PARAMS.show_polygon_walls;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, only_show_boundary_walls, "only_show_boundary_walls"))
	{
		only_show_boundary_walls = DEFAULT_PARAMS.only_show_boundary_walls;
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesViewOperations::ScalarField3DRenderParameters::DepthRestriction::DepthRestriction(
		float min_depth_radius_restriction_,
		float max_depth_radius_restriction_) :
	min_depth_radius_restriction(min_depth_radius_restriction_),
	max_depth_radius_restriction(max_depth_radius_restriction_)
{
}


GPlatesScribe::TranscribeResult
GPlatesViewOperations::ScalarField3DRenderParameters::DepthRestriction::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	// Provide default values for failed parameters instead of returning failure.
	// This way a future version of GPlates can add or remove parameters and still be backward/forward compatible.
	static const DepthRestriction DEFAULT_PARAMS;

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, min_depth_radius_restriction, "min_depth_radius_restriction"))
	{
		min_depth_radius_restriction = DEFAULT_PARAMS.min_depth_radius_restriction;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, max_depth_radius_restriction, "max_depth_radius_restriction"))
	{
		max_depth_radius_restriction = DEFAULT_PARAMS.max_depth_radius_restriction;
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesViewOperations::ScalarField3DRenderParameters::QualityPerformance::QualityPerformance(
		unsigned int sampling_rate_,
		unsigned int bisection_iterations_,
		bool enable_reduce_rate_during_drag_globe_,
		unsigned int reduce_rate_during_drag_globe_) :
	sampling_rate(sampling_rate_),
	bisection_iterations(bisection_iterations_),
	enable_reduce_rate_during_drag_globe(enable_reduce_rate_during_drag_globe_),
	reduce_rate_during_drag_globe(reduce_rate_during_drag_globe_)
{
}


GPlatesScribe::TranscribeResult
GPlatesViewOperations::ScalarField3DRenderParameters::QualityPerformance::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	// Provide default values for failed parameters instead of returning failure.
	// This way a future version of GPlates can add or remove parameters and still be backward/forward compatible.
	static const QualityPerformance DEFAULT_PARAMS;

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, sampling_rate, "sampling_rate"))
	{
		sampling_rate = DEFAULT_PARAMS.sampling_rate;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, bisection_iterations, "bisection_iterations"))
	{
		bisection_iterations = DEFAULT_PARAMS.bisection_iterations;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, enable_reduce_rate_during_drag_globe, "enable_reduce_rate_during_drag_globe"))
	{
		enable_reduce_rate_during_drag_globe = DEFAULT_PARAMS.enable_reduce_rate_during_drag_globe;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, reduce_rate_during_drag_globe, "reduce_rate_during_drag_globe"))
	{
		reduce_rate_during_drag_globe = DEFAULT_PARAMS.reduce_rate_during_drag_globe;
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesViewOperations::transcribe(
		GPlatesScribe::Scribe &scribe,
		ScalarField3DRenderParameters::RenderMode &render_mode,
		bool transcribed_construct_data)
{
	// WARNING: Changing the string ids will break backward/forward compatibility.
	//          So don't change the string ids even if the enum name changes.
	static const GPlatesScribe::EnumValue enum_values[] =
	{
		GPlatesScribe::EnumValue("RENDER_MODE_ISOSURFACE", ScalarField3DRenderParameters::RENDER_MODE_ISOSURFACE),
		GPlatesScribe::EnumValue("RENDER_MODE_CROSS_SECTIONS", ScalarField3DRenderParameters::RENDER_MODE_CROSS_SECTIONS)
	};

	return GPlatesScribe::transcribe_enum_protocol(
			TRANSCRIBE_SOURCE,
			scribe,
			render_mode,
			enum_values,
			enum_values + sizeof(enum_values) / sizeof(enum_values[0]));
}


GPlatesScribe::TranscribeResult
GPlatesViewOperations::transcribe(
		GPlatesScribe::Scribe &scribe,
		ScalarField3DRenderParameters::IsosurfaceDeviationWindowMode &isosurface_deviation_window_mode,
		bool transcribed_construct_data)
{
	// WARNING: Changing the string ids will break backward/forward compatibility.
	//          So don't change the string ids even if the enum name changes.
	static const GPlatesScribe::EnumValue enum_values[] =
	{
		GPlatesScribe::EnumValue("ISOSURFACE_DEVIATION_WINDOW_MODE_NONE", ScalarField3DRenderParameters::ISOSURFACE_DEVIATION_WINDOW_MODE_NONE),
		GPlatesScribe::EnumValue("ISOSURFACE_DEVIATION_WINDOW_MODE_SINGLE", ScalarField3DRenderParameters::ISOSURFACE_DEVIATION_WINDOW_MODE_SINGLE),
		GPlatesScribe::EnumValue("ISOSURFACE_DEVIATION_WINDOW_MODE_DOUBLE", ScalarField3DRenderParameters::ISOSURFACE_DEVIATION_WINDOW_MODE_DOUBLE)
	};

	return GPlatesScribe::transcribe_enum_protocol(
			TRANSCRIBE_SOURCE,
			scribe,
			isosurface_deviation_window_mode,
			enum_values,
			enum_values + sizeof(enum_values) / sizeof(enum_values[0]));
}


GPlatesScribe::TranscribeResult
GPlatesViewOperations::transcribe(
		GPlatesScribe::Scribe &scribe,
		ScalarField3DRenderParameters::IsosurfaceColourMode &isosurface_colour_mode,
		bool transcribed_construct_data)
{
	// WARNING: Changing the string ids will break backward/forward compatibility.
	//          So don't change the string ids even if the enum name changes.
	static const GPlatesScribe::EnumValue enum_values[] =
	{
		GPlatesScribe::EnumValue("ISOSURFACE_COLOUR_MODE_DEPTH", ScalarField3DRenderParameters::ISOSURFACE_COLOUR_MODE_DEPTH),
		GPlatesScribe::EnumValue("ISOSURFACE_COLOUR_MODE_SCALAR", ScalarField3DRenderParameters::ISOSURFACE_COLOUR_MODE_SCALAR),
		GPlatesScribe::EnumValue("ISOSURFACE_COLOUR_MODE_GRADIENT", ScalarField3DRenderParameters::ISOSURFACE_COLOUR_MODE_GRADIENT)
	};

	return GPlatesScribe::transcribe_enum_protocol(
			TRANSCRIBE_SOURCE,
			scribe,
			isosurface_colour_mode,
			enum_values,
			enum_values + sizeof(enum_values) / sizeof(enum_values[0]));
}


GPlatesScribe::TranscribeResult
GPlatesViewOperations::transcribe(
		GPlatesScribe::Scribe &scribe,
		ScalarField3DRenderParameters::CrossSectionColourMode &cross_section_colour_mode,
		bool transcribed_construct_data)
{
	// WARNING: Changing the string ids will break backward/forward compatibility.
	//          So don't change the string ids even if the enum name changes.
	static const GPlatesScribe::EnumValue enum_values[] =
	{
		GPlatesScribe::EnumValue("CROSS_SECTION_COLOUR_MODE_SCALAR", ScalarField3DRenderParameters::CROSS_SECTION_COLOUR_MODE_SCALAR),
		GPlatesScribe::EnumValue("CROSS_SECTION_COLOUR_MODE_GRADIENT", ScalarField3DRenderParameters::CROSS_SECTION_COLOUR_MODE_GRADIENT)
	};

	return GPlatesScribe::transcribe_enum_protocol(
			TRANSCRIBE_SOURCE,
			scribe,
			cross_section_colour_mode,
			enum_values,
			enum_values + sizeof(enum_values) / sizeof(enum_values[0]));
}
