/* $Id$ */

/**
 * @file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2014 Geological Survey of Norway
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
#include <vector>

#include <QObject>

#include "gui/Colour.h"
#include "gui/Symbol.h"
#include "qt-widgets/HellingerDialog.h"
#include "maths/GreatCircleArc.h"
#include "maths/ProximityHitDetail.h"
#include "view-operations/RenderedGeometry.h"
#include "view-operations/RenderedGeometryFactory.h"
#include "view-operations/RenderedGeometryLayer.h"
#include "view-operations/RenderedGeometryProximity.h"
#include "AdjustFittedPoleEstimate.h"



GPlatesCanvasTools::AdjustFittedPoleEstimate::AdjustFittedPoleEstimate(
		const status_bar_callback_type &status_bar_callback,
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
		GPlatesViewOperations::RenderedGeometryCollection::MainLayerType main_rendered_layer_type,
		GPlatesQtWidgets::HellingerDialog &hellinger_dialog) :
	CanvasTool(status_bar_callback),
	d_rendered_geom_collection_ptr(&rendered_geom_collection),
	d_hellinger_dialog_ptr(&hellinger_dialog),
	d_mouse_is_over_pole_estimate(false),
	d_pole_is_being_dragged(false),
	d_current_angle(0.),
	d_has_been_activated(false)
{
	d_pole_estimate_layer_ptr =
		d_rendered_geom_collection_ptr->create_child_rendered_layer_and_transfer_ownership(
				GPlatesViewOperations::RenderedGeometryCollection::HELLINGER_CANVAS_TOOL_WORKFLOW_LAYER);

	d_highlight_layer_ptr =
			d_rendered_geom_collection_ptr->create_child_rendered_layer_and_transfer_ownership(
				GPlatesViewOperations::RenderedGeometryCollection::HELLINGER_CANVAS_TOOL_WORKFLOW_LAYER);

	QObject::connect(d_hellinger_dialog_ptr,
					 SIGNAL(estimate_changed(double,double,double)),
					 this,
					 SLOT(handle_estimate_changed(double,double,double)));
}

void
GPlatesCanvasTools::AdjustFittedPoleEstimate::handle_activation()
{
	d_pole_estimate_layer_ptr->set_active(true);
	d_highlight_layer_ptr->set_active(true);

	set_status_bar_message(QT_TR_NOOP("Click and drag to adjust the pole estimate location."));
	d_hellinger_dialog_ptr->enable_pole_estimate_widgets(true);

	update_local_values_from_hellinger_dialog();
	update_pole_estimate_layer();

	d_has_been_activated = true;
}

void
GPlatesCanvasTools::AdjustFittedPoleEstimate::handle_deactivation()
{
	d_hellinger_dialog_ptr->enable_pole_estimate_widgets(false);
	d_hellinger_dialog_ptr->update_pole_estimate_spinboxes_and_layer(d_current_pole, d_current_angle);
	d_pole_estimate_layer_ptr->set_active(false);
	d_highlight_layer_ptr->set_active(false);
}


void
GPlatesCanvasTools::AdjustFittedPoleEstimate::handle_move_without_drag(
		const GPlatesMaths::PointOnSphere &point_on_sphere,
		bool is_on_earth,
		double proximity_inclusion_threshold)
{
	GPlatesMaths::ProximityCriteria proximity_criteria(
			point_on_sphere,
			proximity_inclusion_threshold);
	std::vector<GPlatesViewOperations::RenderedGeometryProximityHit> sorted_hits;

	d_mouse_is_over_pole_estimate = false;
	d_mouse_is_over_reference_arc = false;
	d_mouse_is_over_relative_arc = false;

	if (GPlatesViewOperations::test_proximity(
				sorted_hits,
				proximity_criteria,
				*d_pole_estimate_layer_ptr))
	{
		// highlight the vertex
		qDebug() << "Checking for geometry in MWD";
		GPlatesViewOperations::RenderedGeometryProximityHit hit = sorted_hits.front();
		GeometryFinder finder(hit.d_proximity_hit_detail->index());
		GPlatesViewOperations::RenderedGeometry rg =
				hit.d_rendered_geom_layer->get_rendered_geometry(
					hit.d_rendered_geom_index);
		rg.accept_visitor(finder);
		boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> geom =
				finder.get_geometry();
		if (geom)
		{
			// We have moved the mouse over one of the 3 geometries in the pole_estimate_layer.
			unsigned int index = hit.d_rendered_geom_index;

			switch(index){
			case POLE_GEOMETRY_INDEX:
				d_mouse_is_over_pole_estimate = true;
				break;
			case REFERENCE_ARC_GEOMETRY_INDEX:
				d_mouse_is_over_reference_arc = true;
				break;
			case RELATIVE_ARC_GEOMETRY_INDEX:
				d_mouse_is_over_relative_arc = true;
				break;
			}

			update_pole_estimate_highlight(*geom);
		}
	}
	else
	{
		d_highlight_layer_ptr->clear_rendered_geometries();
	}
}


void
GPlatesCanvasTools::AdjustFittedPoleEstimate::handle_left_press(
		const GPlatesMaths::PointOnSphere &point_on_sphere,
		bool is_on_earth,
		double proximity_inclusion_threshold)
{

	if (!mouse_is_over_a_highlight_geometry())
	{
		return;
	}


	GPlatesMaths::ProximityCriteria proximity_criteria(
			point_on_sphere,
			proximity_inclusion_threshold);
	std::vector<GPlatesViewOperations::RenderedGeometryProximityHit> sorted_hits;


	if (GPlatesViewOperations::test_proximity(
				sorted_hits,
				proximity_criteria,
				*d_highlight_layer_ptr))
	{

		d_pole_is_being_dragged = d_mouse_is_over_pole_estimate;
		d_reference_arc_is_being_draggged = d_mouse_is_over_reference_arc;
		d_relative_arc_is_being_dragged = d_mouse_is_over_relative_arc;
		d_pole_estimate_layer_ptr->set_active(false);
	}

}


void
GPlatesCanvasTools::AdjustFittedPoleEstimate::handle_estimate_changed(
		double lat,
		double lon,
		double rho)
{
	d_current_pole = GPlatesMaths::make_point_on_sphere(GPlatesMaths::LatLonPoint(lat,lon));
	update_pole_estimate_layer();
}



void
GPlatesCanvasTools::AdjustFittedPoleEstimate::handle_left_release_after_drag(
		const GPlatesMaths::PointOnSphere &initial_point_on_sphere,
		bool was_on_earth,
		double initial_proximity_inclusion_threshold,
		const GPlatesMaths::PointOnSphere &current_point_on_sphere,
		bool is_on_earth,
		double current_proximity_inclusion_threshold,
		const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport)
{
	d_pole_is_being_dragged = false;
	d_reference_arc_is_being_draggged = false;
	d_relative_arc_is_being_dragged = false;

	d_pole_estimate_layer_ptr->set_active(true);

	update_pole_estimate_layer();
}

void
GPlatesCanvasTools::AdjustFittedPoleEstimate::handle_left_drag(
		const GPlatesMaths::PointOnSphere &initial_point_on_sphere,
		bool was_on_earth,
		double initial_proximity_inclusion_threshold,
		const GPlatesMaths::PointOnSphere &current_point_on_sphere,
		bool is_on_earth,
		double current_proximity_inclusion_threshold,
		const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport)
{
	if (d_pole_is_being_dragged)
	{
		update_pole_estimate_highlight(current_point_on_sphere);
		d_current_pole = current_point_on_sphere;
		d_hellinger_dialog_ptr->update_pole_estimate_spinboxes(current_point_on_sphere,d_current_angle);
	}
	else if (d_relative_arc_is_being_dragged)
	{
		update_reference_arc_highlight(current_point_on_sphere);
	}
}

void
GPlatesCanvasTools::AdjustFittedPoleEstimate::paint()
{
#if 0
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;
#endif
}

bool
GPlatesCanvasTools::AdjustFittedPoleEstimate::mouse_is_over_a_highlight_geometry()
{
	return (d_mouse_is_over_pole_estimate || d_mouse_is_over_reference_arc || d_mouse_is_over_relative_arc);
}

void
GPlatesCanvasTools::AdjustFittedPoleEstimate::update_local_values_from_hellinger_dialog()
{
	d_current_pole = GPlatesMaths::make_point_on_sphere(d_hellinger_dialog_ptr->get_pole_estimate());

	if (!d_has_been_activated)
	{
		d_end_point_of_reference_arc = GPlatesMaths::PointOnSphere(GPlatesMaths::generate_perpendicular(d_current_pole.position_vector()));
	}
}

void
GPlatesCanvasTools::AdjustFittedPoleEstimate::update_pole_estimate_layer()
{
	d_pole_estimate_layer_ptr->clear_rendered_geometries();

	static const GPlatesGui::Symbol pole_estimate_symbol = GPlatesGui::Symbol(GPlatesGui::Symbol::CIRCLE, 1, true);

	GPlatesViewOperations::RenderedGeometry pole_geometry =
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
				d_current_pole.get_non_null_pointer(),
				GPlatesGui::Colour::get_yellow(),
				2, /* point size */
				2, /* line thickness */
				false, /* fill polygon */
				false, /* fill polyline */
				GPlatesGui::Colour::get_white(), /* dummy colour */
				pole_estimate_symbol);


	d_pole_estimate_layer_ptr->add_rendered_geometry(pole_geometry);

	GPlatesMaths::GreatCircleArc gca = GPlatesMaths::GreatCircleArc::create(d_current_pole,d_end_point_of_reference_arc);
	std::vector<GPlatesMaths::PointOnSphere> points;
	GPlatesMaths::tessellate(points,gca,GPlatesMaths::PI/1800.);

	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline =
			GPlatesMaths::PolylineOnSphere::create_on_heap(points);

	GPlatesViewOperations::RenderedGeometry gca_geometry =
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
				polyline,
				GPlatesGui::Colour::get_yellow());

	d_pole_estimate_layer_ptr->add_rendered_geometry(gca_geometry);
}

void
GPlatesCanvasTools::AdjustFittedPoleEstimate::update_pole_estimate_highlight(
		const GPlatesMaths::PointOnSphere &geom)
{
	d_highlight_layer_ptr->clear_rendered_geometries();

	static const GPlatesGui::Symbol highlight_symbol = GPlatesGui::Symbol(GPlatesGui::Symbol::CIRCLE, 2, true);

	GPlatesViewOperations::RenderedGeometry pole_geometry =
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
				geom.get_non_null_pointer(),
				GPlatesGui::Colour::get_yellow(),
				2, /* point size */
				2, /* line thickness */
				false, /* fill polygon */
				false, /* fill polyline */
				GPlatesGui::Colour::get_white(), /* dummy colour */
				highlight_symbol);


	d_highlight_layer_ptr->add_rendered_geometry(pole_geometry);
}

void
GPlatesCanvasTools::AdjustFittedPoleEstimate::update_pole_estimate_highlight(
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geom)
{
	d_highlight_layer_ptr->clear_rendered_geometries();

	static const GPlatesGui::Symbol highlight_symbol = GPlatesGui::Symbol(GPlatesGui::Symbol::CIRCLE, 2, true);

	GPlatesViewOperations::RenderedGeometry pole_geometry =
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
				geom,
				GPlatesGui::Colour::get_yellow(),
				2, /* point size */
				2, /* line thickness */
				false, /* fill polygon */
				false, /* fill polyline */
				GPlatesGui::Colour::get_white(), /* dummy colour */
				highlight_symbol);


	d_highlight_layer_ptr->add_rendered_geometry(pole_geometry);
}

void
GPlatesCanvasTools::AdjustFittedPoleEstimate::update_reference_arc_highlight(
		const GPlatesMaths::PointOnSphere &current_pos)
{


}

void
GPlatesCanvasTools::AdjustFittedPoleEstimate::update_relative_arc_highlight(
		const GPlatesMaths::PointOnSphere &current_pos)
{

}

