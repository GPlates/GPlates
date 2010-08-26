/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
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

#include "LayerTaskTypeInfo.h"


const QString &
GPlatesGui::LayerTaskTypeInfo::get_name(
		GPlatesAppLogic::LayerTaskType::Type layer_type)
{
	static const QString RECONSTRUCTION_NAME = "Reconstruction Tree";
	static const QString RECONSTRUCT_NAME = "Reconstructed Geometries";
	static const QString RASTER_NAME = "Reconstructed Raster";
	static const QString AGE_GRID_NAME = "Age Grid";
	static const QString TOPOLOGY_BOUNDARY_RESOLVER_NAME = "Resolved Topological Closed Plate Boundaries";
	static const QString TOPOLOGY_NETWORK_RESOLVER_NAME = "Resolved Topological Networks";
	static const QString VELOCITY_FIELD_CALCULATOR_NAME = "Calculated Velocity Fields";

	static const QString DEFAULT_NAME = "";

	using namespace GPlatesAppLogic::LayerTaskType;

	switch (layer_type)
	{
		case RECONSTRUCTION:
			return RECONSTRUCTION_NAME;

		case RECONSTRUCT:
			return RECONSTRUCT_NAME;

		case RASTER:
			return RASTER_NAME;

		case AGE_GRID:
			return AGE_GRID_NAME;

		case TOPOLOGY_BOUNDARY_RESOLVER:
			return TOPOLOGY_BOUNDARY_RESOLVER_NAME;

		case TOPOLOGY_NETWORK_RESOLVER:
			return TOPOLOGY_NETWORK_RESOLVER_NAME;

		case VELOCITY_FIELD_CALCULATOR:
			return VELOCITY_FIELD_CALCULATOR_NAME;

		default:
			return DEFAULT_NAME;
	}
}


const QString &
GPlatesGui::LayerTaskTypeInfo::get_description(
		GPlatesAppLogic::LayerTaskType::Type layer_type)
{
	static const QString RECONSTRUCTION_DESCRIPTION =
		"A plate-reconstruction hierarchy of total reconstruction poles "
		"which can be used to reconstruct geometries in other layers";

	static const QString RECONSTRUCT_DESCRIPTION =
		"Geometries in this layer will be reconstructed when "
		"this layer is connected to a reconstruction tree layer";

	static const QString RASTER_DESCRIPTION =
		"A raster in this layer will be reconstructed when "
		"this layer is connected to a static plate polygon feature collection and "
		"to a reconstruction tree layer";

	static const QString AGE_GRID_DESCRIPTION =
		"An age grid can be attached to a reconstructed raster layer to provide "
		"smoother raster reconstructions";

	static const QString TOPOLOGY_BOUNDARY_RESOLVER_DESCRIPTION =
		"Plate boundaries will be generated dynamically by referencing topological section "
		"features, that have been reconstructed to a geological time, and joining them to "
		"form a closed polygon boundary";

	static const QString TOPOLOGY_NETWORK_RESOLVER_DESCRIPTION =
		"Deforming regions will be simulated dynamically by referencing topological section "
		"features, that have been reconstructed to a geological time, and triangulating "
		"the convex hull region defined by these reconstructed sections while excluding "
		"any micro-block sections from the triangulation";

	static const QString VELOCITY_FIELD_CALCULATOR_DESCRIPTION =
		"Lithosphere-motion velocity vectors will be calculated dynamically at mesh points "
		"that within resolved topological boundaries or topological networks";

	static const QString DEFAULT_DESCRIPTION = "";

	using namespace GPlatesAppLogic::LayerTaskType;

	switch (layer_type)
	{
		case RECONSTRUCTION:
			return RECONSTRUCTION_DESCRIPTION;

		case RECONSTRUCT:
			return RECONSTRUCT_DESCRIPTION;

		case RASTER:
			return RASTER_DESCRIPTION;

		case AGE_GRID:
			return AGE_GRID_DESCRIPTION;

		case TOPOLOGY_BOUNDARY_RESOLVER:
			return TOPOLOGY_BOUNDARY_RESOLVER_DESCRIPTION;

		case TOPOLOGY_NETWORK_RESOLVER:
			return TOPOLOGY_NETWORK_RESOLVER_DESCRIPTION;

		case VELOCITY_FIELD_CALCULATOR:
			return VELOCITY_FIELD_CALCULATOR_DESCRIPTION;

		default:
			return DEFAULT_DESCRIPTION;
	}
}

