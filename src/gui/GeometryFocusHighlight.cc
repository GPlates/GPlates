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
	} else {
		// No focused geometry, so nothing to draw.
	}
	emit canvas_should_update();
}
