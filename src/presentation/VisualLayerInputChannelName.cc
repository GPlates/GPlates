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

#include "VisualLayerInputChannelName.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"


QString
GPlatesPresentation::VisualLayerInputChannelName::get_input_channel_name(
		GPlatesAppLogic::LayerInputChannelName::Type layer_input_channel_name)
{
	switch (layer_input_channel_name)
	{
	case GPlatesAppLogic::LayerInputChannelName::RECONSTRUCTION_FEATURES:
		return "Reconstruction features";

	case GPlatesAppLogic::LayerInputChannelName::RECONSTRUCTION_TREE:
		return "Reconstruction tree";

	case GPlatesAppLogic::LayerInputChannelName::RECONSTRUCTABLE_FEATURES:
		return "Reconstructable features";

	case GPlatesAppLogic::LayerInputChannelName::TOPOLOGY_SURFACES:
		return "Topology surfaces";

	case GPlatesAppLogic::LayerInputChannelName::TOPOLOGICAL_GEOMETRY_FEATURES:
		return "Topological geometry features";

	case GPlatesAppLogic::LayerInputChannelName::TOPOLOGICAL_SECTION_LAYERS:
		return "Topological sections";

	case GPlatesAppLogic::LayerInputChannelName::TOPOLOGICAL_NETWORK_FEATURES:
		return "Topological network features";

	case GPlatesAppLogic::LayerInputChannelName::VELOCITY_DOMAIN_LAYERS:
		return "Velocity domains (points/multi-points/polylines/polygons)";

	case GPlatesAppLogic::LayerInputChannelName::VELOCITY_SURFACE_LAYERS:
		return "Velocity surfaces (static/dynamic polygons/networks)";

	case GPlatesAppLogic::LayerInputChannelName::RASTER_FEATURE:
		return "Raster feature";

	case GPlatesAppLogic::LayerInputChannelName::RECONSTRUCTED_POLYGONS:
		return "Reconstructed polygons";

	case GPlatesAppLogic::LayerInputChannelName::AGE_GRID_RASTER:
		return "Age grid raster";

	case GPlatesAppLogic::LayerInputChannelName::NORMAL_MAP_RASTER:
		return "Surface relief raster";

	case GPlatesAppLogic::LayerInputChannelName::SCALAR_FIELD_FEATURE:
		return "Scalar field feature";

	case GPlatesAppLogic::LayerInputChannelName::CROSS_SECTIONS:
		return "Cross sections";

	case GPlatesAppLogic::LayerInputChannelName::SURFACE_POLYGONS_MASK:
		return "Surface polygons mask";

	case GPlatesAppLogic::LayerInputChannelName::CO_REGISTRATION_SEED_GEOMETRIES:
		return "Reconstructed seed geometries";

	case GPlatesAppLogic::LayerInputChannelName::CO_REGISTRATION_TARGET_GEOMETRIES:
		return "Reconstructed target geometries/rasters";

	case GPlatesAppLogic::LayerInputChannelName::RECONSTRUCTED_SCALAR_COVERAGE_DOMAINS:
		return "Reconstructed coverage domains";

	default:
		break;
	}

	// Shouldn't be able to get here.
	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
	return ""; // Keep compiler happy.
}
