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

#include "RenderReconstructionGeometries.h"

#include "RenderedGeometryCollection.h"
#include "RenderedGeometryLayer.h"
#include "RenderedGeometryFactory.h"
#include "RenderedGeometryParameters.h"

#include "app-logic/ReconstructionGeometryUtils.h"

#include "gui/ColourTable.h"

#include "model/Reconstruction.h"


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

	GPlatesModel::Reconstruction::geometry_collection_type::iterator iter =
			reconstruction.geometries().begin();
	GPlatesModel::Reconstruction::geometry_collection_type::iterator end =
			reconstruction.geometries().end();

	for ( ; iter != end; ++iter)
	{
		GPlatesModel::ReconstructionGeometry *reconstruction_geom = iter->get();

		GPlatesGui::ColourTable::const_iterator colour =
				colour_table.lookup(*reconstruction_geom);

		if (colour == colour_table.end())
		{
			// Anything not in the table uses the 'Olive' colour.
			colour = &GPlatesGui::Colour::get_olive();
		}

		// Create a RenderedGeometry for drawing the reconstructed geometry.
		RenderedGeometry rendered_geom =
			create_rendered_geometry_on_sphere(
					reconstruction_geom->geometry(),
					*colour,
					RenderedLayerParameters::RECONSTRUCTION_POINT_SIZE_HINT,
					RenderedLayerParameters::RECONSTRUCTION_LINE_WIDTH_HINT);

		// Create a RenderedGeometry for storing the reconstructed geometry
		// and the RenderedGeometry used for drawing it.
		RenderedGeometry rendered_reconstruction_geom =
			GPlatesViewOperations::create_rendered_reconstruction_geometry(
					*iter, rendered_geom);

		// Add to the reconstruction rendered layer.
		// Updates to the canvas will be taken care of since canvas listens
		// to the update signal of RenderedGeometryCollection which in turn
		// listens to its rendered layers.
		reconstruction_layer->add_rendered_geometry(rendered_reconstruction_geom);
	}
}
