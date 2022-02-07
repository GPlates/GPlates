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
 
#include "ReconstructVisualLayerParams.h"

#include "gui/DrawStyleManager.h"


GPlatesPresentation::ReconstructVisualLayerParams::ReconstructVisualLayerParams(
		GPlatesAppLogic::LayerParams::non_null_ptr_type layer_params) :
	VisualLayerParams(
			layer_params,
			GPlatesGui::DrawStyleManager::instance()->default_style()),
	d_vgp_draw_circular_error(true),
	d_fill_polygons(false),
	d_fill_polylines(false),
	d_fill_opacity(1.0),
	d_fill_intensity(1.0),
	d_show_topology_reconstructed_feature_geometries(true),
	d_show_show_strain_accumulation(false),
	d_strain_accumulation_scale(1)
{
}


bool
GPlatesPresentation::ReconstructVisualLayerParams::get_vgp_draw_circular_error() const
{
	return d_vgp_draw_circular_error;
}


void
GPlatesPresentation::ReconstructVisualLayerParams::set_vgp_draw_circular_error(
		bool draw)
{
	d_vgp_draw_circular_error = draw;
	emit_modified();
}


void
GPlatesPresentation::ReconstructVisualLayerParams::set_fill_polygons(
		bool fill)
{
	d_fill_polygons = fill;
	emit_modified();
}


bool
GPlatesPresentation::ReconstructVisualLayerParams::get_fill_polygons() const
{
	return d_fill_polygons;
}


void
GPlatesPresentation::ReconstructVisualLayerParams::set_fill_polylines(
		bool fill)
{
	d_fill_polylines = fill;
	emit_modified();
}


bool
GPlatesPresentation::ReconstructVisualLayerParams::get_fill_polylines() const
{
	return d_fill_polylines;
}


void
GPlatesPresentation::ReconstructVisualLayerParams::set_fill_opacity(
		const double &opacity)
{
	d_fill_opacity = opacity;
	emit_modified();
}


void
GPlatesPresentation::ReconstructVisualLayerParams::set_fill_intensity(
		const double &intensity)
{
	d_fill_intensity = intensity;
	emit_modified();
}


GPlatesGui::Colour
GPlatesPresentation::ReconstructVisualLayerParams::get_fill_modulate_colour() const
{
	return GPlatesGui::Colour(d_fill_intensity, d_fill_intensity, d_fill_intensity, d_fill_opacity);
}


void
GPlatesPresentation::ReconstructVisualLayerParams::set_show_topology_reconstructed_feature_geometries(
		bool show_topology_reconstructed_feature_geometries)
{
	d_show_topology_reconstructed_feature_geometries = show_topology_reconstructed_feature_geometries;
	emit_modified();
}


bool 
GPlatesPresentation::ReconstructVisualLayerParams::get_show_topology_reconstructed_feature_geometries() const
{
	return d_show_topology_reconstructed_feature_geometries;
}


void
GPlatesPresentation::ReconstructVisualLayerParams::set_show_strain_accumulation(
		bool show_strain_accumulation)
{
	d_show_show_strain_accumulation = show_strain_accumulation;
	emit_modified();
}


bool 
GPlatesPresentation::ReconstructVisualLayerParams::get_show_strain_accumulation() const
{
	return d_show_show_strain_accumulation;
}


void
GPlatesPresentation::ReconstructVisualLayerParams::set_strain_accumulation_scale(
		const double &strain_accumulation_scale)
{
	d_strain_accumulation_scale = strain_accumulation_scale;
	emit_modified();
}


double
GPlatesPresentation::ReconstructVisualLayerParams::get_strain_accumulation_scale() const
{
	return d_strain_accumulation_scale;
}
