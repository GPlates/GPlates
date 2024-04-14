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
		const GPlatesViewOperations::RenderedGeometryParameters &rendered_geometry_parameters,
		const GPlatesGui::RenderSettings &render_settings,
		const std::set<GPlatesModel::FeatureId> &topological_sections,
		const GPlatesAppLogic::TopologyUtils::resolved_topological_boundaries_networks_to_shared_sub_segments_map_type &resolved_topological_shared_sub_segments_map,
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
	//
	// NOTE: We can get more than one matching ReconstructionGeometry for the the same
	// focused feature (and its focused geometry property) because it might be reconstructed in two
	// different layers. And since FeatureFocus arbitrarily picks the first match it might not pick
	// the one associated with the originally selected ReconstructionGeometry - which might manifest
	// as the user selecting one ReconstructionGeometry (from one layer) and finding that the other
	// ReconstructionGeometry (from another layer) gets highlighted.
	// So we highlight all ReconstructionGeometry's regardless of layer (instead of limiting to those
	// reconstructed by the same layer as the focused geometry - using reconstruct handles).
	GPlatesViewOperations::RenderedGeometryUtils::reconstruction_geom_seq_type reconstruction_geometries_observing_feature;
	if (!GPlatesViewOperations::RenderedGeometryUtils::get_unique_reconstruction_geometries_observing_feature(
			reconstruction_geometries_observing_feature,
			rendered_geom_collection,
			feature))
	{
		// Shouldn't really get here since there's a focused geometry (and associated focused feature)
		// so we should get at least one reconstruction geometry.
		return;
	}

	// FIXME: Probably should use the same styling params used to draw
	// the original geometries rather than use some of the defaults.
	GPlatesPresentation::ReconstructionGeometryRenderer::RenderParams render_style_params(
			rendered_geometry_parameters);
	render_style_params.reconstruction_line_width_hint =
			rendered_geometry_parameters.get_choose_feature_tool_line_width_hint();
	render_style_params.reconstruction_point_size_hint =
			rendered_geometry_parameters.get_choose_feature_tool_point_size_hint();;

	// Render the non-clicked (grey) focused geometries first so they don't occlude the clicked (white) geometries.
	BOOST_FOREACH(
			const GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type &reconstruction_geometry,
			reconstruction_geometries_observing_feature)
	{
		// Skip the clicked (white) geometries.
		if (reconstruction_geometry == focused_geometry.get())
		{
			continue;
		}

		// If the RG is the same as the focused geometry (the geometry that the
		// user clicked on) then highlight it in a different colour.
		const GPlatesGui::Colour &highlight_colour =
				rendered_geometry_parameters.get_choose_feature_tool_non_clicked_geometry_of_focused_feature_colour();

		// This creates the RenderedGeometry's using the highlight colour.
		GPlatesPresentation::ReconstructionGeometryRenderer highlighted_geometry_renderer(
				render_style_params,
				render_settings,
				topological_sections,
				resolved_topological_shared_sub_segments_map,
				highlight_colour, 
				boost::none,
				symbol_map);

		highlighted_geometry_renderer.begin_render(render_geom_layer);
		reconstruction_geometry->accept_visitor(highlighted_geometry_renderer);
		highlighted_geometry_renderer.end_render();
	}

	// Render the clicked (white) focused geometries last so they appear on top of the non-clicked (grey) geometries.
	BOOST_FOREACH(
			const GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type &reconstruction_geometry,
			reconstruction_geometries_observing_feature)
	{
		// Skip the non-clicked (grey) geometries.
		if (reconstruction_geometry != focused_geometry.get())
		{
			continue;
		}

		// If the RG is the same as the focused geometry (the geometry that the
		// user clicked on) then highlight it in a different colour.
		const GPlatesGui::Colour &highlight_colour =
				rendered_geometry_parameters.get_choose_feature_tool_clicked_geometry_of_focused_feature_colour();

		// This creates the RenderedGeometry's using the highlight colour.
		GPlatesPresentation::ReconstructionGeometryRenderer highlighted_geometry_renderer(
				render_style_params,
				render_settings,
				topological_sections,
				resolved_topological_shared_sub_segments_map,
				highlight_colour, 
				boost::none,
				symbol_map);

		highlighted_geometry_renderer.begin_render(render_geom_layer);
		reconstruction_geometry->accept_visitor(highlighted_geometry_renderer);
		highlighted_geometry_renderer.end_render();
	}
}
