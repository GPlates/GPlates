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
#include "presentation/VisualLayer.h"
#include "presentation/VisualLayerParams.h"
#include "presentation/VisualLayers.h"

#include "view-operations/RenderedGeometryCollection.h"
#include "view-operations/RenderedGeometryFactory.h"
#include "view-operations/RenderedGeometryLayer.h"
#include "view-operations/RenderedGeometryParameters.h"
#include "view-operations/RenderedGeometryUtils.h"


namespace GPlatesGui
{
	namespace GeometryFocusHighlight
	{
		namespace
		{
			/**
			 * Draw the focused feature's geometries.
			 *
			 * Either draw the unclicked geometries or the clicked geometry depending on @a render_clicked_geometry.
			 * The unclicked geometries are drawn first so they don't occlude the clicked geometry.
			 */
			void
			draw_focused_geometry(
					GPlatesAppLogic::ReconstructionGeometry::maybe_null_ptr_to_const_type focused_geometry,
					const GPlatesViewOperations::RenderedGeometryUtils::child_rendered_geometry_layer_reconstruction_geom_map_type &
							reconstruction_geometries_observing_feature,
					GPlatesViewOperations::RenderedGeometryLayer &render_geom_layer,
					GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
					const GPlatesViewOperations::RenderedGeometryParameters &rendered_geometry_parameters,
					const RenderSettings &render_settings,
					const GPlatesPresentation::VisualLayers &visual_layers,
					const std::set<GPlatesModel::FeatureId> &topological_sections,
					const symbol_map_type &symbol_map,
					bool render_clicked_geometry)
			{
				// Iterate over the child rendered geometry layers in the main rendered RECONSTRUCTION layer.
				GPlatesViewOperations::RenderedGeometryUtils::child_rendered_geometry_layer_reconstruction_geom_map_type::const_iterator
						child_rendered_geometry_layer_iter = reconstruction_geometries_observing_feature.begin();
				GPlatesViewOperations::RenderedGeometryUtils::child_rendered_geometry_layer_reconstruction_geom_map_type::const_iterator
						child_rendered_geometry_layer_end = reconstruction_geometries_observing_feature.end();
				for ( ;
					child_rendered_geometry_layer_iter != child_rendered_geometry_layer_end;
					++child_rendered_geometry_layer_iter)
				{
					const GPlatesViewOperations::RenderedGeometryCollection::child_layer_index_type
							child_rendered_geometry_layer_index = child_rendered_geometry_layer_iter->first;

					// Find the visual layer associated with the current child layer index.
					boost::shared_ptr<const GPlatesPresentation::VisualLayer> visual_layer =
							visual_layers.get_visual_layer_at_child_layer_index(child_rendered_geometry_layer_index).lock();
					if (!visual_layer)
					{
						// Visual layer no longer exists for some reason, so ignore it.
						continue;
					}

					const GPlatesPresentation::VisualLayerParams::non_null_ptr_to_const_type visual_layer_params =
							visual_layer->get_visual_layer_params();
					const GPlatesPresentation::ReconstructionGeometrySymboliser &reconstruction_geometry_symboliser =
							visual_layer_params->get_reconstruction_geometry_symboliser();

					GPlatesPresentation::ReconstructionGeometryRenderer::RenderParamsPopulator
							render_params_populator(rendered_geometry_parameters);
					visual_layer_params->accept_visitor(render_params_populator);

					GPlatesPresentation::ReconstructionGeometryRenderer::RenderParams render_params =
							render_params_populator.get_render_params();
					render_params.reconstruction_line_width_hint =
							rendered_geometry_parameters.get_choose_feature_tool_line_width_hint();
					render_params.reconstruction_point_size_hint =
							rendered_geometry_parameters.get_choose_feature_tool_point_size_hint();
					// Ensure filled polygons are fully opaque (it's possible the layer has set a translucent opacity).
					render_params.fill_modulate_colour = Colour::get_white();

					// The ReconstructionGeometry objects in the current rendered geometry layer.
					const GPlatesViewOperations::RenderedGeometryUtils::reconstruction_geom_seq_type &
							reconstruction_geometries = child_rendered_geometry_layer_iter->second;

					BOOST_FOREACH(
							const GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type &reconstruction_geometry,
							reconstruction_geometries)
					{
						// Skip the current reconstruction geometry if it's the clicked geometry and
						// we're only supposed to rendered unclicked geometries, or vice versa.
						if (render_clicked_geometry !=
							(reconstruction_geometry == focused_geometry.get()))
						{
							continue;
						}

						// Choose the highlight colour based on whether we're rendering the click geometry or unclicked geometries.
						const Colour &highlight_colour = render_clicked_geometry
							? rendered_geometry_parameters.get_choose_feature_tool_clicked_geometry_of_focused_feature_colour()
							: rendered_geometry_parameters.get_choose_feature_tool_non_clicked_geometry_of_focused_feature_colour();

						// This creates the RenderedGeometry's using the highlight colour.
						GPlatesPresentation::ReconstructionGeometryRenderer highlighted_geometry_renderer(
								render_params,
								render_settings,
								reconstruction_geometry_symboliser,
								topological_sections,
								highlight_colour, 
								boost::none,
								symbol_map);

						highlighted_geometry_renderer.begin_render(render_geom_layer);
						reconstruction_geometry->accept_visitor(highlighted_geometry_renderer);
						highlighted_geometry_renderer.end_render();
					}
				}
			}
		}
	}
}


void
GPlatesGui::GeometryFocusHighlight::draw_focused_geometry(
		FeatureFocus &feature_focus,
		GPlatesViewOperations::RenderedGeometryLayer &render_geom_layer,
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
		const GPlatesViewOperations::RenderedGeometryParameters &rendered_geometry_parameters,
		const RenderSettings &render_settings,
		const GPlatesPresentation::VisualLayers &visual_layers,
		const std::set<GPlatesModel::FeatureId> &topological_sections,
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
	// NOTE: We can get more than one matching ReconstructionGeometry for the same
	// focused feature (and its focused geometry property) because it might be reconstructed in two
	// different layers. And since FeatureFocus arbitrarily picks the first match it might not pick
	// the one associated with the originally selected ReconstructionGeometry - which might manifest
	// as the user selecting one ReconstructionGeometry (from one layer) and finding that the other
	// ReconstructionGeometry (from another layer) gets highlighted.
	// So we highlight all ReconstructionGeometry's regardless of layer (instead of limiting to those
	// reconstructed by the same layer as the focused geometry - using reconstruct handles).
	//
	// However we still need to know which layer each ReconstructionGeometry came from since each
	// visual layer has its own symbology, and we want to render the RFGs using the same symbols as before
	// (but with a different colour) so that they overlap nicely (and are recognisable by their expected symbology).
	GPlatesViewOperations::RenderedGeometryUtils::child_rendered_geometry_layer_reconstruction_geom_map_type
			reconstruction_geometries_observing_feature;
	if (!GPlatesViewOperations::RenderedGeometryUtils::get_unique_reconstruction_geometries_observing_feature_in_reconstruction_child_layers(
			reconstruction_geometries_observing_feature,
			rendered_geom_collection,
			feature))
	{
		// Shouldn't really get here since there's a focused geometry (and associated focused feature)
		// so we should get at least one reconstruction geometry.
		return;
	}

	// Render the non-clicked (grey) focused geometries first so they don't occlude the clicked (white) geometries.
	draw_focused_geometry(
			focused_geometry,
			reconstruction_geometries_observing_feature,
			render_geom_layer,
			rendered_geom_collection,
			rendered_geometry_parameters,
			render_settings,
			visual_layers,
			topological_sections,
			symbol_map,
			false/*render_clicked_geometry*/);

	// Render the clicked (white) focused geometry last so it appears on top of the non-clicked (grey) geometries.
	draw_focused_geometry(
			focused_geometry,
			reconstruction_geometries_observing_feature,
			render_geom_layer,
			rendered_geom_collection,
			rendered_geometry_parameters,
			render_settings,
			visual_layers,
			topological_sections,
			symbol_map,
			true/*render_clicked_geometry*/);
}
