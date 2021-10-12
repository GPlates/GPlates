/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008, 2009 The University of Sydney, Australia
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

#include <boost/none.hpp>

#include "GeometryFocusHighlight.h"
#include "gui/ColourTable.h"
#include "model/ReconstructedFeatureGeometryFinder.h"
#include "view-operations/RenderedGeometryCollection.h"
#include "view-operations/RenderedGeometryFactory.h"
#include "view-operations/RenderedGeometryLayer.h"
#include "view-operations/RenderedGeometryParameters.h"


GPlatesGui::GeometryFocusHighlight::GeometryFocusHighlight(
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection):
d_rendered_geom_collection(&rendered_geom_collection),
d_highlight_layer_ptr(
		rendered_geom_collection.get_main_rendered_layer(
				GPlatesViewOperations::RenderedGeometryCollection::GEOMETRY_FOCUS_HIGHLIGHT_LAYER))
{
}

void
GPlatesGui::GeometryFocusHighlight::set_focus(
		GPlatesModel::FeatureHandle::weak_ref feature_ref,
		GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type focused_geometry)
{
	if (d_focused_geometry == focused_geometry && d_feature == feature_ref) {
		// No change, so nothing to do.
		return;
	}

	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block.
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;

	// Else, presumably the focused geometry has changed.
	d_feature = feature_ref;
	d_focused_geometry = focused_geometry;
	draw_focused_geometry();
}


void
GPlatesGui::GeometryFocusHighlight::draw_focused_geometry()
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block.
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;

	// Activate our layer.
	d_highlight_layer_ptr->set_active();

	// Clear all geometries from layer before adding them.
	d_highlight_layer_ptr->clear_rendered_geometries();

	if (!d_focused_geometry)
	{
		// There's no focused geometry so there's nothing to draw.
		// NOTE: we do this after clearing the rendered geometries so that
		// any previously focused geometry is no longer highlighted.
		return;
	}

	//
	// Since a feature can have multiple geometry properties we need
	// to highlight them all even though only one geometry was clicked by the user.
	//

	// The reconstruction that the focused geometry came from.
	const GPlatesModel::Reconstruction *reconstruction = d_focused_geometry->reconstruction();

	// Iterate through the RFGs belonging to the same 'reconstruction' that the
	// clicked geometry came from and that belong to the focused feature.
	GPlatesModel::ReconstructedFeatureGeometryFinder rfgFinder(reconstruction);
	rfgFinder.find_rfgs_of_feature(d_feature);

	GPlatesModel::ReconstructedFeatureGeometryFinder::rfg_container_type::const_iterator rfgIter;
	for (rfgIter = rfgFinder.found_rfgs_begin();
		rfgIter != rfgFinder.found_rfgs_end();
		++rfgIter)
	{
		GPlatesModel::ReconstructedFeatureGeometry *rfg = rfgIter->get();

		// If the RFG is the same as the focused geometry (the geometry that the
		// user clicked on) then highlight it in a different colour.
		const GPlatesGui::Colour &highlight_colour = (rfg == d_focused_geometry.get())
			? GPlatesViewOperations::FocusedFeatureParameters::CLICKED_GEOMETRY_OF_FOCUSED_FEATURE_COLOUR
			: GPlatesViewOperations::FocusedFeatureParameters::NON_CLICKED_GEOMETRY_OF_FOCUSED_FEATURE_COLOUR;

		GPlatesViewOperations::RenderedGeometry rendered_geometry =
				GPlatesViewOperations::create_rendered_geometry_on_sphere(
						rfg->geometry(),
						highlight_colour,
						GPlatesViewOperations::RenderedLayerParameters::GEOMETRY_FOCUS_POINT_SIZE_HINT,
						GPlatesViewOperations::RenderedLayerParameters::GEOMETRY_FOCUS_LINE_WIDTH_HINT);

		d_highlight_layer_ptr->add_rendered_geometry(rendered_geometry);
	}
}
