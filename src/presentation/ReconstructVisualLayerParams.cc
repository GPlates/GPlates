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

#include "scribe/Scribe.h"
#include "scribe/TranscribeEnumProtocol.h"


const double
GPlatesPresentation::ReconstructVisualLayerParams::INITIAL_VGP_DELTA_T = 5.0;


GPlatesPresentation::ReconstructVisualLayerParams::ReconstructVisualLayerParams(
		GPlatesAppLogic::LayerParams::non_null_ptr_type layer_params,
		ViewState &view_state) :
	VisualLayerParams(
			layer_params,
			view_state,
			GPlatesGui::DrawStyleManager::instance()->default_style()),
	d_vgp_visibility_setting(DELTA_T_AROUND_AGE),
	d_vgp_earliest_time(GPlatesPropertyValues::GeoTimeInstant::create_distant_past()),
	d_vgp_latest_time(GPlatesPropertyValues::GeoTimeInstant::create_distant_future()),
	d_vgp_delta_t(INITIAL_VGP_DELTA_T),
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


GPlatesPresentation::ReconstructVisualLayerParams::VGPVisibilitySetting
GPlatesPresentation::ReconstructVisualLayerParams::get_vgp_visibility_setting() const
{
	return d_vgp_visibility_setting;
}


void
GPlatesPresentation::ReconstructVisualLayerParams::set_vgp_visibility_setting(
		VGPVisibilitySetting setting)
{
	d_vgp_visibility_setting = setting;
	emit_modified();
}


const GPlatesPropertyValues::GeoTimeInstant &
GPlatesPresentation::ReconstructVisualLayerParams::get_vgp_earliest_time() const
{
	return d_vgp_earliest_time;	
}


void
GPlatesPresentation::ReconstructVisualLayerParams::set_vgp_earliest_time(
		const GPlatesPropertyValues::GeoTimeInstant &earliest_time)
{	
	d_vgp_earliest_time = earliest_time;
	emit_modified();
}


const GPlatesPropertyValues::GeoTimeInstant &
GPlatesPresentation::ReconstructVisualLayerParams::get_vgp_latest_time() const
{
	return d_vgp_latest_time;	
}


void
GPlatesPresentation::ReconstructVisualLayerParams::set_vgp_latest_time(
		const GPlatesPropertyValues::GeoTimeInstant &latest_time)
{	
	d_vgp_latest_time = latest_time;
	emit_modified();
}


double
GPlatesPresentation::ReconstructVisualLayerParams::get_vgp_delta_t() const
{
	return d_vgp_delta_t.dval();
}


void
GPlatesPresentation::ReconstructVisualLayerParams::set_vgp_delta_t(
		double vgp_delta_t)
{	
	d_vgp_delta_t = vgp_delta_t;
	emit_modified();
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


bool
GPlatesPresentation::ReconstructVisualLayerParams::show_vgp(
		double current_time,
		const boost::optional<double> &age) const
{
	// Check if the VGP should be drawn for the current time.
	
	const GPlatesPropertyValues::GeoTimeInstant geo_time(current_time);

	switch (d_vgp_visibility_setting)
	{
		case ALWAYS_VISIBLE:
			return true;

		case TIME_WINDOW:
			if ( geo_time.is_later_than_or_coincident_with(d_vgp_earliest_time) && 
				 geo_time.is_earlier_than_or_coincident_with(d_vgp_latest_time) )
			{
				return true;
			}
			break;

		case DELTA_T_AROUND_AGE:
			if (age)
			{
				const GPlatesPropertyValues::GeoTimeInstant earliest_time(age.get() + d_vgp_delta_t.dval());
				const GPlatesPropertyValues::GeoTimeInstant latest_time(age.get() - d_vgp_delta_t.dval());
				
				if ((geo_time.is_later_than_or_coincident_with(earliest_time)) &&
					(geo_time.is_earlier_than_or_coincident_with(latest_time)))
				{
					return true;
				}
			}
			break;			
	}

	return false;
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


GPlatesScribe::TranscribeResult
GPlatesPresentation::transcribe(
		GPlatesScribe::Scribe &scribe,
		ReconstructVisualLayerParams::VGPVisibilitySetting &vgp_visibility_setting,
		bool transcribed_construct_data)
{
	// WARNING: Changing the string ids will break backward/forward compatibility.
	//          So don't change the string ids even if the enum name changes.
	static const GPlatesScribe::EnumValue enum_values[] =
	{
		GPlatesScribe::EnumValue("ALWAYS_VISIBLE", ReconstructVisualLayerParams::ALWAYS_VISIBLE),
		GPlatesScribe::EnumValue("TIME_WINDOW", ReconstructVisualLayerParams::TIME_WINDOW),
		GPlatesScribe::EnumValue("DELTA_T_AROUND_AGE", ReconstructVisualLayerParams::DELTA_T_AROUND_AGE)
	};

	return GPlatesScribe::transcribe_enum_protocol(
			TRANSCRIBE_SOURCE,
			scribe,
			vgp_visibility_setting,
			enum_values,
			enum_values + sizeof(enum_values) / sizeof(enum_values[0]));
}
