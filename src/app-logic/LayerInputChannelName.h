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

#ifndef GPLATES_APP_LOGIC_LAYERINPUTCHANNELNAME_H
#define GPLATES_APP_LOGIC_LAYERINPUTCHANNELNAME_H

#include "scribe/Transcribe.h"


namespace GPlatesAppLogic
{
	namespace LayerInputChannelName
	{
		/**
		 * The layer input channel names.
		 *
		 * These are enumerations rather than strings in app-logic code here.
		 * In GUI code these can be mapped to display strings.
		 *
		 * So using enumerations avoids breaking backward/forward compatibility of sessions/projects
		 * whenever the 'display' string is changed.
		 */
		enum Type
		{
			RECONSTRUCTION_FEATURES,
			RECONSTRUCTABLE_FEATURES,
			TOPOLOGICAL_GEOMETRY_FEATURES,
			TOPOLOGICAL_NETWORK_FEATURES,
			RASTER_FEATURE,
			SCALAR_FIELD_FEATURE,
			RECONSTRUCTION_TREE,
			DEFORMATION_SURFACES,
			TOPOLOGICAL_SECTION_LAYERS,
			VELOCITY_DOMAIN_LAYERS,
			VELOCITY_SURFACE_LAYERS,
			RECONSTRUCTED_POLYGONS,
			AGE_GRID_RASTER,
			NORMAL_MAP_RASTER,
			CROSS_SECTIONS,
			SURFACE_POLYGONS_MASK,
			CO_REGISTRATION_SEED_GEOMETRIES,
			CO_REGISTRATION_TARGET_GEOMETRIES,
			RECONSTRUCTED_SCALAR_COVERAGE_DOMAINS,

			UNUSED
		};


		/**
		 * Transcribe for sessions/projects.
		 */
		GPlatesScribe::TranscribeResult
		transcribe(
				GPlatesScribe::Scribe &scribe,
				Type &layer_input_channel_name,
				bool transcribed_construct_data);
	}
}

#endif // GPLATES_APP_LOGIC_LAYERINPUTCHANNELNAME_H
