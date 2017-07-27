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

#include "LayerInputChannelName.h"

#include "scribe/TranscribeEnumProtocol.h"


GPlatesScribe::TranscribeResult
GPlatesAppLogic::LayerInputChannelName::transcribe(
		GPlatesScribe::Scribe &scribe,
		Type &layer_input_channel_name,
		bool transcribed_construct_data)
{
	// WARNING: Changing the string ids will break backward/forward compatibility.
	//          So don't change the string ids even if the enum name changes.
	static const GPlatesScribe::EnumValue enum_values[] =
	{
		GPlatesScribe::EnumValue("RECONSTRUCTION_FEATURES", RECONSTRUCTION_FEATURES),
		GPlatesScribe::EnumValue("RECONSTRUCTION_TREE", RECONSTRUCTION_TREE),
		GPlatesScribe::EnumValue("RECONSTRUCTABLE_FEATURES", RECONSTRUCTABLE_FEATURES),
		// Note: We didn't change the string to "TOPOLOGY_SURFACES" since that would have broken
		// backward backward/forward compatibility...
		GPlatesScribe::EnumValue("DEFORMATION_SURFACES", TOPOLOGY_SURFACES),
		GPlatesScribe::EnumValue("TOPOLOGICAL_GEOMETRY_FEATURES", TOPOLOGICAL_GEOMETRY_FEATURES),
		GPlatesScribe::EnumValue("TOPOLOGICAL_SECTION_LAYERS", TOPOLOGICAL_SECTION_LAYERS),
		GPlatesScribe::EnumValue("TOPOLOGICAL_NETWORK_FEATURES", TOPOLOGICAL_NETWORK_FEATURES),
		GPlatesScribe::EnumValue("VELOCITY_DOMAIN_LAYERS", VELOCITY_DOMAIN_LAYERS),
		GPlatesScribe::EnumValue("VELOCITY_SURFACE_LAYERS", VELOCITY_SURFACE_LAYERS),
		GPlatesScribe::EnumValue("RASTER_FEATURE", RASTER_FEATURE),
		GPlatesScribe::EnumValue("RECONSTRUCTED_POLYGONS", RECONSTRUCTED_POLYGONS),
		GPlatesScribe::EnumValue("AGE_GRID_RASTER", AGE_GRID_RASTER),
		GPlatesScribe::EnumValue("NORMAL_MAP_RASTER", NORMAL_MAP_RASTER),
		GPlatesScribe::EnumValue("SCALAR_FIELD_FEATURE", SCALAR_FIELD_FEATURE),
		GPlatesScribe::EnumValue("CROSS_SECTIONS", CROSS_SECTIONS),
		GPlatesScribe::EnumValue("SURFACE_POLYGONS_MASK", SURFACE_POLYGONS_MASK),
		GPlatesScribe::EnumValue("CO_REGISTRATION_SEED_GEOMETRIES", CO_REGISTRATION_SEED_GEOMETRIES),
		GPlatesScribe::EnumValue("CO_REGISTRATION_TARGET_GEOMETRIES", CO_REGISTRATION_TARGET_GEOMETRIES),
		GPlatesScribe::EnumValue("RECONSTRUCTED_SCALAR_COVERAGE_DOMAINS", RECONSTRUCTED_SCALAR_COVERAGE_DOMAINS),
		GPlatesScribe::EnumValue("UNUSED", UNUSED)
	};

	return GPlatesScribe::transcribe_enum_protocol(
			TRANSCRIBE_SOURCE,
			scribe,
			layer_input_channel_name,
			enum_values,
			enum_values + sizeof(enum_values) / sizeof(enum_values[0]));
}
