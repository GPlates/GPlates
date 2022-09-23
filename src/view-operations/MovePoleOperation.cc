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

#include "MovePoleOperation.h"

#include "RenderedGeometryFactory.h"
#include "RenderedGeometryParameters.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "gui/MapProjection.h"
#include "gui/ViewportZoom.h"

#include "maths/types.h"

#include "qt-widgets/MovePoleWidget.h"


// Highlight arrow in yellow with some transparency.
const GPlatesGui::Colour GPlatesViewOperations::MovePoleOperation::ARROW_HIGHLIGHT_COLOUR = GPlatesGui::Colour(1, 1, 0, 0.5f);

// Unhighlight arrow in white.
const GPlatesGui::Colour GPlatesViewOperations::MovePoleOperation::ARROW_UNHIGHLIGHT_COLOUR = GPlatesGui::Colour::get_white();

const float GPlatesViewOperations::MovePoleOperation::ARROW_LENGTH = 0.3f;
const float GPlatesViewOperations::MovePoleOperation::ARROWHEAD_SIZE = 0.12f;
const float GPlatesViewOperations::MovePoleOperation::ARROW_BODY_WIDTH = 0.5f * GPlatesViewOperations::MovePoleOperation::ARROWHEAD_SIZE;


GPlatesViewOperations::MovePoleOperation::MovePoleOperation(
		const GPlatesGui::MapProjection &map_projection,
		GPlatesGui::ViewportZoom &viewport_zoom,
		RenderedGeometryCollection &rendered_geometry_collection,
		RenderedGeometryCollection::MainLayerType main_rendered_layer_type,
		GPlatesQtWidgets::MovePoleWidget &move_pole_widget) :
	d_map_projection(map_projection),
	d_viewport_zoom(viewport_zoom),
	d_rendered_geometry_collection(rendered_geometry_collection),
	d_main_rendered_layer_type(main_rendered_layer_type),
	d_move_pole_widget(move_pole_widget),
	d_is_dragging_pole(false)
{
}


void
GPlatesViewOperations::MovePoleOperation::activate()
{
	// Activate the Move Pole widget.
	d_move_pole_widget.activate();

	// Listen for pole changes due to the Move Pole widget (eg, text entry).
	QObject::connect(
			&d_move_pole_widget, SIGNAL(pole_changed(boost::optional<GPlatesMaths::PointOnSphere>)),
			this, SLOT(react_pole_changed()));

	// Create the rendered geometry layers.
	create_rendered_geometry_layers();

	// Activate our render layer so it becomes visible.
	d_pole_layer_ptr->set_active(true);

	// Render pole as unhighlighted.
	render_pole(false/*highlight*/);
}


void
GPlatesViewOperations::MovePoleOperation::deactivate()
{
	// Get rid of all render layers even if switching to drag or zoom tool
	// (which normally previously would display the most recent tool's layers).
	// This is because once we are deactivated we won't be able to update the render layers.
	// This means the user won't see this tool's render layers while in the drag or zoom tool.
	d_pole_layer_ptr->set_active(false);
	d_pole_layer_ptr->clear_rendered_geometries();
	d_is_dragging_pole = false;

	// Stop listening for pole changes due to the Move Pole widget.
	QObject::disconnect(
			&d_move_pole_widget, SIGNAL(pole_changed(boost::optional<GPlatesMaths::PointOnSphere>)),
			this, SLOT(react_pole_changed()));

	// Deactivate the Move Pole widget.
	d_move_pole_widget.deactivate();
}


void
GPlatesViewOperations::MovePoleOperation::mouse_move_on_globe(
		const GPlatesMaths::PointOnSphere &current_pos_on_globe,
		const double &closeness_inclusion_threshold)
{
	// Render pole as either highlighted if mouse hovering near pole or unhighlighted.
	const bool highlight =
			test_proximity_to_pole_on_globe(
					current_pos_on_globe,
					// Increase closeness inclusion so easier to select arrow instead of point...
					adjust_closeness_inclusion_threshold(closeness_inclusion_threshold));

	render_pole(highlight);
}


void
GPlatesViewOperations::MovePoleOperation::mouse_move_on_map(
		const QPointF &current_point_on_scene,
		const GPlatesMaths::PointOnSphere &current_point_on_sphere)
{
	// Render pole as either highlighted if mouse hovering near pole or unhighlighted.
	const bool highlight =test_proximity_to_pole_on_map(current_point_on_scene, current_point_on_sphere);

	render_pole(highlight);
}


bool
GPlatesViewOperations::MovePoleOperation::start_drag_on_globe(
		const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
		const double &closeness_inclusion_threshold)
{
	if (!d_move_pole_widget.can_change_pole())
	{
		return false;
	}

	if (!test_proximity_to_pole_on_globe(
		initial_pos_on_globe,
		// Increase closeness inclusion so easier to select arrow instead of point...
		adjust_closeness_inclusion_threshold(closeness_inclusion_threshold)))
	{
		return false;
	}

	d_is_dragging_pole = true;

	render_pole(true/*highlight*/);

	return true;
}


bool
GPlatesViewOperations::MovePoleOperation::start_drag_on_map(
		const QPointF &initial_point_on_scene,
		const GPlatesMaths::PointOnSphere &initial_point_on_sphere)
{
	if (!d_move_pole_widget.can_change_pole())
	{
		return false;
	}

	if (!test_proximity_to_pole_on_map(initial_point_on_scene, initial_point_on_sphere))
	{
		return false;
	}

	d_is_dragging_pole = true;

	render_pole(true/*highlight*/);

	return true;
}


void
GPlatesViewOperations::MovePoleOperation::update_drag(
		const GPlatesMaths::PointOnSphere &pos_on_sphere)
{
	if (d_is_dragging_pole)
	{
		move_pole(pos_on_sphere);

		render_pole(true/*highlight*/);
	}
}


void
GPlatesViewOperations::MovePoleOperation::end_drag(
		const GPlatesMaths::PointOnSphere &pos_on_sphere)
{
	if (d_is_dragging_pole)
	{
		move_pole(pos_on_sphere);

		render_pole(true/*highlight*/);

		d_is_dragging_pole = false;
	}
}


void
GPlatesViewOperations::MovePoleOperation::react_pole_changed()
{
	// We assume the pole is unhighlighted since mouse cursor is not hovering over the pole location
	// but instead is in the task panel (of the Move Pole widget).
	render_pole(false/*highlight*/);
}


void
GPlatesViewOperations::MovePoleOperation::create_rendered_geometry_layers()
{
	// Create a rendered layer to draw the pole.
	d_pole_layer_ptr =
		d_rendered_geometry_collection.create_child_rendered_layer_and_transfer_ownership(
				d_main_rendered_layer_type);

	// Above, we store the returned object as a data member and it automatically destroys the
	// created layer for us when 'this' object is destroyed.
}


double
GPlatesViewOperations::MovePoleOperation::adjust_closeness_inclusion_threshold(
		const double &closeness_inclusion_threshold)
{
	// Expand the closeness inclusion threshold by the radius of the arrow head (half its diameter).
	// This enables the user to easily select the arrow when it's pointing towards the camera.
	//
	// We're assuming that "arcsin(size) ~ size" for small enough arrow sizes/extents.
	// And we also adjust for viewport zoom since the rendered arrow is scaled by zoom factor.
	return GPlatesMaths::cos(
			0.5 * ARROWHEAD_SIZE / d_viewport_zoom.zoom_factor() +
				GPlatesMaths::acos(closeness_inclusion_threshold)).dval();
}


bool
GPlatesViewOperations::MovePoleOperation::test_proximity_to_pole_on_globe(
		const GPlatesMaths::PointOnSphere &pos_on_sphere,
		const double &closeness_inclusion_threshold)
{
	// If pole is not enabled then we cannot be close to it.
	if (!d_move_pole_widget.get_pole())
	{
		return false;
	}

	const GPlatesMaths::real_t closeness = dot(
			pos_on_sphere.position_vector(),
			d_move_pole_widget.get_pole()->position_vector());

	return closeness.is_precisely_greater_than(closeness_inclusion_threshold);
}


bool
GPlatesViewOperations::MovePoleOperation::test_proximity_to_pole_on_map(
		const QPointF &point_on_scene,
		const GPlatesMaths::PointOnSphere &point_on_sphere)
{
	// If pole is not enabled then we cannot be close to it.
	if (!d_move_pole_widget.get_pole())
	{
		return false;
	}

	// Find the pole location in map *scene* coordinates.
	const QPointF pole_on_scene = d_map_projection.forward_transform(d_move_pole_widget.get_pole().get());

	// Calculate distance between pole and point in scene coordinates.
	const QPointF difference = point_on_scene - pole_on_scene;
	const qreal distance = sqrt(difference.x() * difference.x() + difference.y() * difference.y());

	// Choose a threshold distance in map *scene* coordinates.
	// Also take into account the zoom factor since we want to reduce the threshold as we zoom in.
	return distance <= 10.0 / d_viewport_zoom.zoom_factor();
}


void
GPlatesViewOperations::MovePoleOperation::move_pole(
		const GPlatesMaths::PointOnSphere &pole)
{
	d_move_pole_widget.set_pole(pole);
}


void
GPlatesViewOperations::MovePoleOperation::render_pole(
		bool highlight)
{
	// Clear current pole rendered geometry first.
	d_pole_layer_ptr->clear_rendered_geometries();

	// We should only be rendering the pole if it's currently enabled.
	if (d_move_pole_widget.get_pole())
	{
		const GPlatesMaths::Vector3D pole_arrow_vector = ARROW_LENGTH *
				GPlatesMaths::Vector3D(d_move_pole_widget.get_pole()->position_vector());

		// Render the pole as an arrow.
		const RenderedGeometry pole_arrow_rendered_geom =
				RenderedGeometryFactory::create_rendered_arrow(
						d_move_pole_widget.get_pole().get(),  // start position
						pole_arrow_vector,
						highlight ? ARROW_HIGHLIGHT_COLOUR : ARROW_UNHIGHLIGHT_COLOUR,
						ARROWHEAD_SIZE,
						ARROW_BODY_WIDTH);
		d_pole_layer_ptr->add_rendered_geometry(pole_arrow_rendered_geom);
	}
}
