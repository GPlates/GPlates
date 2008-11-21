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
#include "utils/GeometryCreationUtils.h"
#include <vector>


void
GPlatesGui::GeometryFocusHighlight::set_focus(
		GPlatesModel::FeatureHandle::weak_ref feature_ref,
		GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type focused_geometry)
{
	if (d_focused_geometry == focused_geometry && d_feature == feature_ref) {
		// No change, so nothing to do.
		return;
	}
	// Else, presumably the focused geometry has changed.
	d_feature = feature_ref;
	d_focused_geometry = focused_geometry;
	draw_focused_geometry();
}


void
GPlatesGui::GeometryFocusHighlight::draw_focused_geometry()
{
	d_highlight_layer_ptr->clear();
	if (d_focused_geometry) {
		GPlatesGui::PlatesColourTable::const_iterator white = &GPlatesGui::Colour::WHITE;
		GPlatesGui::RenderedGeometry rendered_geometry =
				GPlatesGui::RenderedGeometry(d_focused_geometry->geometry(), white);
		d_highlight_layer_ptr->push_back(rendered_geometry);

		// add decorations as needed
		rendered_geometry.geometry()->accept_visitor(*this);
	
	} else {
		// No focused geometry, so nothing to draw.
	}
	emit canvas_should_update();
}

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

