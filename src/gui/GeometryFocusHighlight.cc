/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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
#include "view-operations/RenderedGeometryCollection.h"
#include "view-operations/RenderedGeometryFactory.h"
#include "view-operations/RenderedGeometryLayer.h"


GPlatesGui::GeometryFocusHighlight::GeometryFocusHighlight(
		GPlatesViewOperations::RenderedGeometryFactory &rendered_geom_factory,
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection):
d_rendered_geom_factory(&rendered_geom_factory),
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

	if (d_focused_geometry)
	{
		const GPlatesGui::Colour &white = GPlatesGui::Colour::WHITE;

		GPlatesViewOperations::RenderedGeometry rendered_geometry =
				d_rendered_geom_factory->create_rendered_geometry_on_sphere(
						d_focused_geometry->geometry(), white);

		d_highlight_layer_ptr->add_rendered_geometry(rendered_geometry);
	}
	else
	{
		// No focused geometry, so nothing to draw.
	}
}
