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

#include "ScalarField3DRenderParameters.h"

#include "gui/RasterColourPalette.h"


GPlatesViewOperations::ScalarField3DRenderParameters::ScalarField3DRenderParameters() :
	d_render_mode(RENDER_MODE_ISOSURFACE),
	d_colour_mode(COLOUR_MODE_DEPTH),
	d_colour_palette(GPlatesGui::DefaultNormalisedRasterColourPalette::create())
{
}


GPlatesViewOperations::ScalarField3DRenderParameters::ScalarField3DRenderParameters(
		RenderMode render_mode,
		ColourMode colour_mode,
		const GPlatesGui::ColourPalette<double>::non_null_ptr_to_const_type &colour_palette,
		const IsovalueParameters &isovalue_parameters,
		const RenderOptions &render_options,
		const SurfacePolygonsMask &surface_polygons_mask,
		const DepthRestriction &depth_restriction,
		const QualityPerformance &quality_performance,
		const std::vector<float> &shader_test_variables) :
	d_render_mode(render_mode),
	d_colour_mode(colour_mode),
	d_colour_palette(colour_palette),
	d_isovalue_parameters(isovalue_parameters),
	d_render_options(render_options),
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


GPlatesViewOperations::ScalarField3DRenderParameters::RenderOptions::RenderOptions(
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


GPlatesViewOperations::ScalarField3DRenderParameters::SurfacePolygonsMask::SurfacePolygonsMask(
		bool treat_polylines_as_polygons_,
		bool show_polygon_walls_,
		bool only_show_boundary_walls_) :
	treat_polylines_as_polygons(treat_polylines_as_polygons_),
	show_polygon_walls(show_polygon_walls_),
	only_show_boundary_walls(only_show_boundary_walls_)
{
}


GPlatesViewOperations::ScalarField3DRenderParameters::DepthRestriction::DepthRestriction(
		float min_depth_radius_restriction_,
		float max_depth_radius_restriction_) :
	min_depth_radius_restriction(min_depth_radius_restriction_),
	max_depth_radius_restriction(max_depth_radius_restriction_)
{
}


GPlatesViewOperations::ScalarField3DRenderParameters::QualityPerformance::QualityPerformance(
		unsigned int sampling_rate_,
		unsigned int bisection_iterations_) :
	sampling_rate(sampling_rate_),
	bisection_iterations(bisection_iterations_)
{
}

