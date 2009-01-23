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
#include "view-operations/RenderedGeometryParameters.h"


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
						d_focused_geometry->geometry(),
						white,
						GPlatesViewOperations::RenderedLayerParameters::GEOMETRY_FOCUS_POINT_SIZE_HINT,
						GPlatesViewOperations::RenderedLayerParameters::GEOMETRY_FOCUS_LINE_WIDTH_HINT);

		d_highlight_layer_ptr->add_rendered_geometry(rendered_geometry);
	}
	else
	{
		// No focused geometry, so nothing to draw.
	}
}

#if 0
void
GPlatesGui::GeometryFocusHighlight::visit_polyline_on_sphere(
	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline)
{
		GPlatesGui::PlatesColourTable::const_iterator colour = &GPlatesGui::Colour::SILVER;

		GPlatesUtils::GeometryConstruction::GeometryConstructionValidity validity;
		std::vector<GPlatesMaths::PointOnSphere> points;

		// get the begin and end points ; NOTE the -- on end
		GPlatesMaths::PolylineOnSphere::vertex_const_iterator v_begin= polyline->vertex_begin();
		GPlatesMaths::PolylineOnSphere::vertex_const_iterator v_end= --(polyline->vertex_end());

		// add the first point of the line to the layer
		points.clear();
		points.push_back( *v_begin );

		boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> g_start = 
				GPlatesUtils::create_point_on_sphere(points, validity);

		GPlatesGui::RenderedGeometry rg_start = 
			GPlatesGui::RenderedGeometry( *g_start, colour, 7.0f );

		d_highlight_layer_ptr->push_back( rg_start );

		// add the last point of the line to the layer
		points.clear();
		points.push_back( *v_end );
		boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> g_end = 
				GPlatesUtils::create_point_on_sphere(points, validity);

		GPlatesGui::RenderedGeometry rg_end = 
			GPlatesGui::RenderedGeometry( *g_end, colour, 5.0f);

		d_highlight_layer_ptr->push_back( rg_end );
}
#endif

