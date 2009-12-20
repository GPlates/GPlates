/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#include <cstddef> // For std::size_t

#include "RenderReconstructionGeometries.h"

#include "RenderedGeometryCollection.h"
#include "RenderedGeometryLayer.h"
#include "RenderedGeometryFactory.h"
#include "RenderedGeometryParameters.h"

#include "app-logic/PlateVelocityUtils.h"
#include "app-logic/ReconstructionGeometryUtils.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "gui/Colour.h"
#include "gui/ColourTable.h"

#include "model/Reconstruction.h"
#include "model/ResolvedTopologicalNetwork.h"


namespace
{
	/**
	 * Render velocities for the points in resolved topological networks.
	 *
	 * FIXME: Putting this here is just temporary - soon I'll be setting up
	 * a framework to handle this sort of thing better.
	 */
	void
	render_resolved_topological_network_velocities(
			const GPlatesModel::Reconstruction &reconstruction,
			GPlatesViewOperations::RenderedGeometryLayer *reconstruction_layer)
	{
		const GPlatesGui::Colour &velocity_colour = GPlatesGui::Colour::get_white();

		GPlatesAppLogic::TopologyUtils::resolved_topological_network_impl_seq_type
				resolved_topological_network_impl_seq;

		GPlatesAppLogic::TopologyUtils::find_resolved_topological_network_impls(
				resolved_topological_network_impl_seq,
				reconstruction);

		// Iterate through the unique topological networks.
		GPlatesAppLogic::TopologyUtils::resolved_topological_network_impl_seq_type::const_iterator
				resolved_network_iter = resolved_topological_network_impl_seq.begin();
		GPlatesAppLogic::TopologyUtils::resolved_topological_network_impl_seq_type::const_iterator
				resolved_network_end = resolved_topological_network_impl_seq.end();
		for ( ; resolved_network_iter != resolved_network_end; ++resolved_network_iter)
		{
			const GPlatesModel::ResolvedTopologicalNetworkImpl *resolved_network_impl =
					*resolved_network_iter;

			const GPlatesAppLogic::PlateVelocityUtils::TopologicalNetworkVelocities &network_velocites =
					resolved_network_impl->get_network_velocities();
			if (!network_velocites.contains_velocities())
			{
				continue;
			}

			// Get the velocities at the network points.
			std::vector<GPlatesMaths::PointOnSphere> network_points;
			std::vector<GPlatesMaths::VectorColatitudeLongitude> network_velocities;
			network_velocites.get_network_velocities(
					network_points, network_velocities);

			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					network_points.size() == network_velocities.size(),
					GPLATES_ASSERTION_SOURCE);

			// Render each velocity in the current network.
			for (std::size_t velocity_index = 0;
				velocity_index < network_velocities.size();
				++velocity_index)
			{
				const GPlatesMaths::PointOnSphere &point = network_points[velocity_index];
				const GPlatesMaths::Vector3D velocity_vector =
						GPlatesMaths::convert_vector_from_colat_lon_to_xyz(
								point, network_velocities[velocity_index]);

				// Create a RenderedGeometry using the velocity vector.
				const GPlatesViewOperations::RenderedGeometry rendered_vector =
					GPlatesViewOperations::RenderedGeometryFactory::create_rendered_direction_arrow(
						point,
						velocity_vector,
						0.05f,
						velocity_colour);

				reconstruction_layer->add_rendered_geometry(rendered_vector);
			}
		}
	}
}


void
GPlatesViewOperations::render_reconstruction_geometries(
		GPlatesModel::Reconstruction &reconstruction,
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
		const GPlatesGui::ColourTable &colour_table)
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block. This is so we can do multiple changes
	// without redrawing canvas after each change.
	// This should ideally be located at the highest level to capture one
	// user GUI interaction - the user performs an action and we update canvas once.
	// But since these guards can be nested it's probably a good idea to have it here too.
	RenderedGeometryCollection::UpdateGuard update_guard;

	// Get the reconstruction rendered layer.
	RenderedGeometryLayer *reconstruction_layer =
		rendered_geom_collection.get_main_rendered_layer(
				RenderedGeometryCollection::RECONSTRUCTION_LAYER);

	// Activate the layer.
	reconstruction_layer->set_active();

	// Clear all RenderedGeometry's before adding new ones.
	reconstruction_layer->clear_rendered_geometries();

	// Get the reconstruction geometries that are resolved topological networks and
	// draw the velocities at the network points if there are any.
	//
	// FIXME: Putting this here is just temporary - soon I'll be setting up
	// a framework to handle this sort of thing better.
	render_resolved_topological_network_velocities(reconstruction, reconstruction_layer);

	GPlatesModel::Reconstruction::geometry_collection_type::const_iterator iter =
			reconstruction.geometries().begin();
	GPlatesModel::Reconstruction::geometry_collection_type::const_iterator end =
			reconstruction.geometries().end();

	for ( ; iter != end; ++iter)
	{
		const GPlatesModel::ReconstructionGeometry *reconstruction_geom = iter->get();

		GPlatesGui::ColourTable::const_iterator colour =
				colour_table.lookup(*reconstruction_geom);

		if (colour == colour_table.end())
		{
			// Anything not in the table uses the 'Olive' colour.
			colour = &GPlatesGui::Colour::get_olive();
		}

		// Create a RenderedGeometry for drawing the reconstructed geometry.
		RenderedGeometry rendered_geom =
				GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
						reconstruction_geom->geometry(),
						*colour,
						RenderedLayerParameters::RECONSTRUCTION_POINT_SIZE_HINT,
						RenderedLayerParameters::RECONSTRUCTION_LINE_WIDTH_HINT);

		// Create a RenderedGeometry for storing the reconstructed geometry
		// and the RenderedGeometry used for drawing it.
		RenderedGeometry rendered_reconstruction_geom =
				GPlatesViewOperations::RenderedGeometryFactory::create_rendered_reconstruction_geometry(
						*iter, rendered_geom);

		// Add to the reconstruction rendered layer.
		// Updates to the canvas will be taken care of since canvas listens
		// to the update signal of RenderedGeometryCollection which in turn
		// listens to its rendered layers.
		reconstruction_layer->add_rendered_geometry(rendered_reconstruction_geom);
	}
}
