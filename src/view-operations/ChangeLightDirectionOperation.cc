/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2014 The University of Sydney, Australia
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

#include "ChangeLightDirectionOperation.h"

#include "RenderedGeometryFactory.h"
#include "RenderedGeometryParameters.h"

#include "gui/SceneLightingParameters.h"
#include "gui/SimpleGlobeOrientation.h"
#include "gui/ViewportZoom.h"

#include "maths/types.h"


// Highlight arrow in yellow with some transparency.
const GPlatesGui::Colour GPlatesViewOperations::ChangeLightDirectionOperation::ARROW_HIGHLIGHT_COLOUR =
		GPlatesGui::Colour(1, 1, 0, 0.8f);
// Highlight symbol in red.
const GPlatesGui::Colour GPlatesViewOperations::ChangeLightDirectionOperation::SYMBOL_HIGHLIGHT_COLOUR =
		GPlatesGui::Colour(1, 0, 0, 1);

// Unhighlight arrow in white.
const GPlatesGui::Colour GPlatesViewOperations::ChangeLightDirectionOperation::ARROW_UNHIGHLIGHT_COLOUR =
		GPlatesGui::Colour::get_white();
// Unhighlight symbol in white.
const GPlatesGui::Colour GPlatesViewOperations::ChangeLightDirectionOperation::SYMBOL_UNHIGHLIGHT_COLOUR =
		GPlatesGui::Colour::get_white();

const float GPlatesViewOperations::ChangeLightDirectionOperation::ARROW_PROJECTED_LENGTH = 0.3f;
const float GPlatesViewOperations::ChangeLightDirectionOperation::ARROW_HEAD_PROJECTED_SIZE = 0.1f;
const float GPlatesViewOperations::ChangeLightDirectionOperation::RATIO_ARROW_LINE_WIDTH_TO_ARROW_HEAD_SIZE = 0.5f;
const GPlatesViewOperations::RenderedRadialArrow::SymbolType GPlatesViewOperations::ChangeLightDirectionOperation::SYMBOL_TYPE =
		GPlatesViewOperations::RenderedRadialArrow::SYMBOL_FILLED_CIRCLE;


GPlatesViewOperations::ChangeLightDirectionOperation::ChangeLightDirectionOperation(
		GPlatesGui::SceneLightingParameters &scene_lighting_parameters,
		GPlatesGui::SimpleGlobeOrientation &globe_orientation,
		GPlatesGui::ViewportZoom &viewport_zoom,
		RenderedGeometryCollection &rendered_geometry_collection,
		RenderedGeometryCollection::MainLayerType main_rendered_layer_type) :
	d_scene_lighting_parameters(scene_lighting_parameters),
	d_globe_orientation(globe_orientation),
	d_viewport_zoom(viewport_zoom),
	d_rendered_geometry_collection(rendered_geometry_collection),
	d_main_rendered_layer_type(main_rendered_layer_type),
	d_is_dragging_light_direction(false)
{
}


void
GPlatesViewOperations::ChangeLightDirectionOperation::activate()
{
	// Create the rendered geometry layers.
	create_rendered_geometry_layers();

	// Activate our render layer so it becomes visible.
	d_light_direction_layer_ptr->set_active(true);

	// Render light direction as unhighlighted.
	render_light_direction(false/*highlight*/);
}


void
GPlatesViewOperations::ChangeLightDirectionOperation::deactivate()
{
	// Get rid of all render layers even if switching to drag or zoom tool
	// (which normally previously would display the most recent tool's layers).
	// This is because once we are deactivated we won't be able to update the render layers.
	// This means the user won't see this tool's render layers while in the drag or zoom tool.
	d_light_direction_layer_ptr->set_active(false);
	d_light_direction_layer_ptr->clear_rendered_geometries();
}


void
GPlatesViewOperations::ChangeLightDirectionOperation::mouse_move(
		const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere,
		const double &closeness_inclusion_threshold)
{
	// Render light direction as either highlighted if mouse hovering near light direction or unhighlighted.
	const bool highlight =
			test_proximity_to_light_direction(
					oriented_pos_on_sphere,
					// Increase closeness inclusion so easier to select arrow instead of point...
					adjust_closeness_inclusion_threshold(closeness_inclusion_threshold));

	render_light_direction(highlight);
}


void
GPlatesViewOperations::ChangeLightDirectionOperation::start_drag(
		const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere,
		const double &closeness_inclusion_threshold)
{
	if (test_proximity_to_light_direction(
		oriented_pos_on_sphere,
		// Increase closeness inclusion so easier to select arrow instead of point...
		adjust_closeness_inclusion_threshold(closeness_inclusion_threshold)))
	{
		d_is_dragging_light_direction = true;

		render_light_direction(true/*highlight*/);
	}
}


void
GPlatesViewOperations::ChangeLightDirectionOperation::update_drag(
		const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere)
{
	if (d_is_dragging_light_direction)
	{
		move_light_direction(oriented_pos_on_sphere.position_vector());

		render_light_direction(true/*highlight*/);
	}
}


void
GPlatesViewOperations::ChangeLightDirectionOperation::end_drag(
		const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere)
{
	if (d_is_dragging_light_direction)
	{
		move_light_direction(oriented_pos_on_sphere.position_vector());

		render_light_direction(true/*highlight*/);

		d_is_dragging_light_direction = false;
	}
}


void
GPlatesViewOperations::ChangeLightDirectionOperation::create_rendered_geometry_layers()
{
	// Create a rendered layer to draw the light direction arrow.
	d_light_direction_layer_ptr =
		d_rendered_geometry_collection.create_child_rendered_layer_and_transfer_ownership(
				d_main_rendered_layer_type);

	// Above, we store the returned object as a data member and it automatically destroys the
	// created layer for us when 'this' object is destroyed.
}


GPlatesMaths::UnitVector3D
GPlatesViewOperations::ChangeLightDirectionOperation::get_world_space_light_direction() const
{
	// Convert light direction to world-space (from view-space) if necessary.
	return d_scene_lighting_parameters.is_light_direction_attached_to_view_frame()
			? GPlatesGui::transform_globe_view_space_light_direction_to_world_space(
					d_scene_lighting_parameters.get_globe_view_light_direction(),
					d_globe_orientation.rotation())
			: d_scene_lighting_parameters.get_globe_view_light_direction();
}


double
GPlatesViewOperations::ChangeLightDirectionOperation::adjust_closeness_inclusion_threshold(
		const double &closeness_inclusion_threshold)
{
	// Expand the closeness inclusion threshold by the radius of the arrow head (half its diameter).
	// This enables the user to easily select the arrow when it's pointing towards the camera.
	//
	// We're assuming that "arcsin(size) ~ size" for small enough arrow sizes/extents.
	// And we also adjust for viewport zoom since the rendered arrow is scaled by zoom factor.
	return GPlatesMaths::cos(
			0.5 * ARROW_HEAD_PROJECTED_SIZE / d_viewport_zoom.zoom_factor() +
				GPlatesMaths::acos(closeness_inclusion_threshold)).dval();
}


bool
GPlatesViewOperations::ChangeLightDirectionOperation::test_proximity_to_light_direction(
		const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere,
		const double &closeness_inclusion_threshold)
{
	// Convert light direction to world-space (from view-space) if necessary.
	const GPlatesMaths::UnitVector3D world_space_light_direction = get_world_space_light_direction();

	const GPlatesMaths::real_t closeness = dot(
			oriented_pos_on_sphere.position_vector(),
			world_space_light_direction);

	return closeness.is_precisely_greater_than(closeness_inclusion_threshold);
}


void
GPlatesViewOperations::ChangeLightDirectionOperation::move_light_direction(
		const GPlatesMaths::UnitVector3D &world_space_light_direction)
{
	if (d_scene_lighting_parameters.is_light_direction_attached_to_view_frame())
	{
		// Convert light direction to view-space (from world-space).
		const GPlatesMaths::UnitVector3D view_space_light_direction =
				GPlatesGui::transform_globe_world_space_light_direction_to_view_space(
						world_space_light_direction,
						d_globe_orientation.rotation());
		d_scene_lighting_parameters.set_globe_view_light_direction(view_space_light_direction);
	}
	else
	{
		d_scene_lighting_parameters.set_globe_view_light_direction(world_space_light_direction);
	}
}


void
GPlatesViewOperations::ChangeLightDirectionOperation::render_light_direction(
		bool highlight)
{
	// Clear current light direction rendered geometry first.
	d_light_direction_layer_ptr->clear_rendered_geometries();

	// Convert light direction to world-space (from view-space) if necessary.
	const GPlatesMaths::UnitVector3D world_space_light_direction = get_world_space_light_direction();

	// Render the light direction as an arrow.
	// Render symbol in map view with a symbol size of zero so that it doesn't show up because
	// we don't currently support changing light direction in the map view.
	const RenderedGeometry light_direction_arrow_rendered_geom =
			RenderedGeometryFactory::create_rendered_radial_arrow(
					GPlatesMaths::PointOnSphere(world_space_light_direction),
					ARROW_PROJECTED_LENGTH,
					ARROW_HEAD_PROJECTED_SIZE,
					RATIO_ARROW_LINE_WIDTH_TO_ARROW_HEAD_SIZE,
					highlight ? ARROW_HIGHLIGHT_COLOUR : ARROW_UNHIGHLIGHT_COLOUR,
					SYMBOL_TYPE,
					// Map symbol size set to zero...
					0,
					highlight ? SYMBOL_HIGHLIGHT_COLOUR : SYMBOL_UNHIGHLIGHT_COLOUR);
	d_light_direction_layer_ptr->add_rendered_geometry(light_direction_arrow_rendered_geom);
}
