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
	d_rendered_geometry = boost::none;
	render_focused_geometry();
	show_highlight();
}


void
GPlatesGui::GeometryFocusHighlight::render_focused_geometry()
{
	if ( ! d_focused_geometry) {
		// No focused geometry, so nothing to render.
		return;
	}
	GPlatesGui::PlatesColourTable::const_iterator white = &GPlatesGui::Colour::WHITE;
	d_rendered_geometry = GPlatesGui::RenderedGeometry(d_focused_geometry->geometry(), white);
}


void
GPlatesGui::GeometryFocusHighlight::hide_highlight()
{
	d_highlight_layer_ptr->clear();
	emit canvas_should_update();
}


void
GPlatesGui::GeometryFocusHighlight::show_highlight()
{
	d_highlight_layer_ptr->clear();
	if (d_rendered_geometry) {
		d_highlight_layer_ptr->push_back(*d_rendered_geometry);
	}
	emit canvas_should_update();
}
