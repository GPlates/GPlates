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
		GPlatesAppLogic::LayerTaskParams &layer_task_params) :
	VisualLayerParams(
			layer_task_params,
			GPlatesGui::DrawStyleManager::instance()->default_style()),
	d_vgp_draw_circular_error(true),
	d_fill_polygons(false),
	d_show_deformed_feature_geometries(true),
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
GPlatesPresentation::ReconstructVisualLayerParams::set_show_deformed_feature_geometries(
		bool show_deformed_feature_geometries)
{
	d_show_deformed_feature_geometries = show_deformed_feature_geometries;
	emit_modified();
}


bool 
GPlatesPresentation::ReconstructVisualLayerParams::get_show_deformed_feature_geometries() const
{
	return d_show_deformed_feature_geometries;
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
