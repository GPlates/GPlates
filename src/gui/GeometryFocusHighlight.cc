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

#include <boost/foreach.hpp>
#include <boost/none.hpp>

#include "Colour.h"
#include "GeometryFocusHighlight.h"

#include "gui/DrawStyleManager.h"
#include "gui/FeatureFocus.h"
#include "gui/Symbol.h"

#include "model/FeatureHandle.h"

#include "presentation/ReconstructionGeometryRenderer.h"

#include "view-operations/RenderedGeometryCollection.h"
#include "view-operations/RenderedGeometryFactory.h"
#include "view-operations/RenderedGeometryLayer.h"
#include "view-operations/RenderedGeometryParameters.h"
#include "view-operations/RenderedGeometryUtils.h"

void
GPlatesGui::GeometryFocusHighlight::draw_focused_geometry(
		FeatureFocus &feature_focus,
		GPlatesViewOperations::RenderedGeometryLayer &render_geom_layer,
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
		const symbol_map_type &symbol_map)
{
	// Clear all geometries from layer before adding them.
	render_geom_layer.clear_rendered_geometries();

	const GPlatesModel::FeatureHandle::weak_ref feature = feature_focus.focused_feature();
	GPlatesAppLogic::ReconstructionGeometry::maybe_null_ptr_to_const_type focused_geometry =
			feature_focus.associated_reconstruction_geometry();

	if (!feature.is_valid() || !focused_geometry)
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

	// Find all reconstruction geometries, of all geometry properties, of the focused feature.
	// NOTE: We get these from the rendered geometry collection since that represents the visible
	// geometries and also represents the latest reconstruction.
	GPlatesViewOperations::RenderedGeometryUtils::reconstruction_geom_seq_type reconstruction_geometries_observing_feature;
	if (!GPlatesViewOperations::RenderedGeometryUtils::get_unique_reconstruction_geometries_observing_feature(
			reconstruction_geometries_observing_feature,
			rendered_geom_collection,
			feature,
			// Restrict the found RGs to those reconstructed by same tree as focused geometry.
			// They all come from current reconstruction time so should have the same reconstruction tree...
			focused_geometry->reconstruction_tree()))
	{
		// Shouldn't really get here since there's a focused geometry (and associated focused feature)
		// so we should get at least one reconstruction geometry.
		return;
	}

	// FIXME: Probably should use the same styling params used to draw
	// the original geometries rather than use some of the defaults.
	GPlatesPresentation::ReconstructionGeometryRenderer::RenderParams render_style_params;
	render_style_params.reconstruction_line_width_hint =
			GPlatesViewOperations::RenderedLayerParameters::GEOMETRY_FOCUS_LINE_WIDTH_HINT;
	render_style_params.reconstruction_point_size_hint =
			GPlatesViewOperations::RenderedLayerParameters::GEOMETRY_FOCUS_POINT_SIZE_HINT;

	// Iterate over the reconstruction geometries of the focused features.
	BOOST_FOREACH(
			const GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type &reconstruction_geometry,
			reconstruction_geometries_observing_feature)
	{
		// If the RG is the same as the focused geometry (the geometry that the
		// user clicked on) then highlight it in a different colour.
		const GPlatesGui::Colour &highlight_colour =
				(reconstruction_geometry == focused_geometry.get())
				? GPlatesViewOperations::FocusedFeatureParameters::CLICKED_GEOMETRY_OF_FOCUSED_FEATURE_COLOUR
				: GPlatesViewOperations::FocusedFeatureParameters::NON_CLICKED_GEOMETRY_OF_FOCUSED_FEATURE_COLOUR;

		// This creates the RenderedGeometry's using the highlight colour.
		GPlatesPresentation::ReconstructionGeometryRenderer highlighted_geometry_renderer(
				render_style_params,
				highlight_colour, 
				boost::none,
				symbol_map);

		highlighted_geometry_renderer.begin_render(render_geom_layer);

		reconstruction_geometry->accept_visitor(highlighted_geometry_renderer);

		highlighted_geometry_renderer.end_render();
	}
}
