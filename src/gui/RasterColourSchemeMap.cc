/* $Id$ */

/**
 * @file 
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

#include "RasterColourSchemeMap.h"

#include "app-logic/ReconstructGraph.h"


GPlatesGui::RasterColourSchemeMap::RasterColourSchemeMap(
		GPlatesAppLogic::ReconstructGraph &reconstruct_graph)
{
	QObject::connect(
			&reconstruct_graph,
			SIGNAL(layer_about_to_be_removed(
					GPlatesAppLogic::ReconstructGraph &,
					GPlatesAppLogic::Layer)),
			this,
			SLOT(handle_layer_about_to_be_removed(
					GPlatesAppLogic::ReconstructGraph &,
					GPlatesAppLogic::Layer)));
}


void
GPlatesGui::RasterColourSchemeMap::set_colour_scheme(
		const GPlatesAppLogic::Layer &layer,
		const RasterColourScheme::non_null_ptr_type &colour_scheme,
		const QString &palette_file_name)
{
	map_type::iterator iter = d_map.find(layer);
	LayerInfo layer_info(colour_scheme, palette_file_name);
	if (iter == d_map.end())
	{
		d_map.insert(std::make_pair(layer, layer_info));
	}
	else
	{
		iter->second = layer_info;
	}
}


boost::optional<GPlatesGui::RasterColourScheme::non_null_ptr_type>
GPlatesGui::RasterColourSchemeMap::get_colour_scheme(
		const GPlatesAppLogic::Layer &layer)
{
	map_type::const_iterator iter = d_map.find(layer);
	if (iter == d_map.end())
	{
		return boost::none;
	}
	else
	{
		const LayerInfo &layer_info = iter->second;
		return layer_info.colour_scheme;
	}
}


boost::optional<GPlatesGui::RasterColourSchemeMap::layer_info_type>
GPlatesGui::RasterColourSchemeMap::get_layer_info(
		const GPlatesAppLogic::Layer &layer)
{
	map_type::const_iterator iter = d_map.find(layer);
	if (iter == d_map.end())
	{
		return boost::none;
	}
	else
	{
		return iter->second;
	}
}


void
GPlatesGui::RasterColourSchemeMap::handle_layer_about_to_be_removed(
		GPlatesAppLogic::ReconstructGraph &reconstruct_graph,
		GPlatesAppLogic::Layer layer)
{
	d_map.erase(layer);
}

