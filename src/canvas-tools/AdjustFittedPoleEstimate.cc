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
#include "view-operations/RenderedGeometry.h"
#include "view-operations/RenderedGeometryFactory.h"
#include "view-operations/RenderedGeometryLayer.h"
#include "view-operations/RenderedGeometryProximity.h"
#include "AdjustFittedPoleEstimate.h"

const GPlatesGui::Colour VERTEX_COLOUR_POLE_12 = GPlatesGui::Colour::get_blue();
const GPlatesGui::Colour ARC_COLOUR_POLE_12 = GPlatesGui::Colour::get_blue();

const GPlatesGui::Colour VERTEX_COLOUR_POLE_13 = GPlatesGui::Colour::get_blue();
const GPlatesGui::Colour ARC_COLOUR_POLE_13 = GPlatesGui::Colour::get_blue();

// We can use the same highlight colour - only one pole will be selected at any one time.
const GPlatesGui::Colour VERTEX_HIGHLIGHT_COLOUR = GPlatesGui::Colour::get_yellow();
const GPlatesGui::Symbol POLE_SYMBOL = GPlatesGui::Symbol(GPlatesGui::Symbol::CIRCLE,1,true);
const GPlatesGui::Symbol END_POINT_SYMBOL = GPlatesGui::Symbol(GPlatesGui::Symbol::CROSS, 2, true);
const GPlatesGui::Symbol POLE_HIGHLIGHT_SYMBOL = GPlatesGui::Symbol(GPlatesGui::Symbol::CIRCLE, 2, true);
const GPlatesGui::Symbol END_POINT_HIGHLIGHT_SYMBOL = GPlatesGui::Symbol(GPlatesGui::Symbol::CROSS, 3, true);
const double INITIAL_ANGLE = 5.;


namespace
{
	/**
	 * Compare based on hit index.
	 */
	bool
	proximity_hit_index_compare(
			const GPlatesViewOperations::RenderedGeometryProximityHit &lhs,
			const GPlatesViewOperations::RenderedGeometryProximityHit &rhs)
	{
		return lhs.d_rendered_geom_index <
				rhs.d_rendered_geom_index;
	}

	/**
	 * Sorts proximity hits by index.
	 */
	void
	sort_proximity_by_index(
			GPlatesViewOperations::sorted_rendered_geometry_proximity_hits_type &sorted_proximity_seq)
	{
		// Sort results based on proximity closeness.
		std::sort(
					sorted_proximity_seq.begin(),
					sorted_proximity_seq.end(),
					proximity_hit_index_compare);
	}


	/**
	 * @brief generate_new_relative_end_point
	 * @param pole - the rotation pole
	 * @param reference_end_point - the point at the end of the arc which represents a baseline from which angles are measured
	 * @param relative_end_point - the point at the end of the arc which lies at angle @param angle from the baseline arc
	 * @param angle - the rotation angle in degrees
	 *
	 * The value of @param relative_end_point is changed in this function.
	 */
	void
	generate_new_relative_end_point(
			const GPlatesMaths::PointOnSphere &pole,
			const GPlatesMaths::PointOnSphere &reference_end_point,
			GPlatesMaths::PointOnSphere &relative_end_point,
			const double &angle)
	{
		using namespace GPlatesMaths;

		if (GPlatesMaths::are_almost_exactly_equal(angle,0.))
		{
			return;
		}

		try {
			// Project the relative_end_point onto the reference gca.
			GreatCircleArc gca = GreatCircleArc::create(pole,reference_end_point);
			UnitVector3D axis = gca.rotation_axis();
			double arc_angle = std::acos(dot(pole.position_vector(),relative_end_point.position_vector()).dval());

			if (GPlatesMaths::are_almost_exactly_equal(arc_angle,0.))
			{
				return;
			}

			Rotation r1 =  Rotation::create(axis,arc_angle);
			PointOnSphere projected_relative_end_point = r1*pole;

			Rotation r2 = GPlatesMaths::Rotation::create(pole.position_vector(),convert_deg_to_rad(angle));
			relative_end_point = r2*projected_relative_end_point;
		}
		catch(std::exception &exception)
		{
			qWarning() << "Error generating new end point in AdjustFittedPoleEstimate: " << exception.what();
		}
		catch(...)
		{
			qWarning() << "Unknown Error generating new end point in AdjustFittedPoleEstimate.";
		}
	}

	void
	update_pole_and_angle_geometries(
			GPlatesCanvasTools::AdjustFittedPoleEstimate::child_layer_ptr_type &layer,
			const GPlatesMaths::PointOnSphere &pole,
			const GPlatesMaths::PointOnSphere &reference_arc_end_point,
			const GPlatesMaths::PointOnSphere &relative_arc_end_point)
	{
		GPlatesViewOperations::RenderedGeometry pole_geometry =
				GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
					pole.get_geometry_on_sphere(),
					VERTEX_COLOUR_POLE_12,
					2, /* point size */
					2, /* line thickness */
					false, /* fill polygon */
					false, /* fill polyline */
					GPlatesGui::Colour::get_white(), /* dummy colour */
					POLE_SYMBOL);

		layer->add_rendered_geometry(pole_geometry);

		GPlatesViewOperations::RenderedGeometry reference_end_point_geometry =
				GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
					reference_arc_end_point.get_geometry_on_sphere(),
					VERTEX_COLOUR_POLE_12,
					10, /* point size */
					2, /* line thickness */
					false, /* fill polygon */
					false, /* fill polyline */
					GPlatesGui::Colour::get_white(),/* dummy colour */
					END_POINT_SYMBOL);

		layer->add_rendered_geometry(reference_end_point_geometry);

		GPlatesViewOperations::RenderedGeometry relative_end_point_geometry =
				GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
					relative_arc_end_point.get_geometry_on_sphere(),
					VERTEX_COLOUR_POLE_12,
					10, /* point size */
					2, /* line thickness */
					false, /* fill polygon */
					false, /* fill polyline */
					GPlatesGui::Colour::get_white(),/* dummy colour */
					END_POINT_SYMBOL);


		layer->add_rendered_geometry(relative_end_point_geometry);

		std::vector<GPlatesMaths::PointOnSphere> points;

		try{

			GPlatesMaths::GreatCircleArc gca = GPlatesMaths::GreatCircleArc::create(pole, reference_arc_end_point);

			GPlatesMaths::tessellate(points, gca, GPlatesMaths::PI/1800.);

			GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline =
					GPlatesMaths::PolylineOnSphere::create(points);

			GPlatesViewOperations::RenderedGeometry gca_geometry =
					GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
						polyline,
						ARC_COLOUR_POLE_12);

			layer->add_rendered_geometry(gca_geometry);

			gca = GPlatesMaths::GreatCircleArc::create(pole, relative_arc_end_point);
			GPlatesMaths::tessellate(points,gca,GPlatesMaths::PI/1800.);
			polyline = GPlatesMaths::PolylineOnSphere::create(points);
			gca_geometry = GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
						polyline,
						ARC_COLOUR_POLE_12);

			layer->add_rendered_geometry(gca_geometry);

		}
		catch(std::exception &exception)
		{
			qWarning() << "Error updating pole and angle layer in AdjustFittedPoleEstimate: " << exception.what();
		}
		catch(...)
		{
			qWarning() << "Unknown error updating pole and angle layer in AdjustFittedPoleEstimate.";
		}

	}
}

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
	d_mouse_is_over_reference_arc(false),
	d_reference_arc_is_being_draggged(false),
	d_mouse_is_over_reference_arc_end_point(false),
	d_reference_arc_end_point_is_being_dragged(false),
	d_mouse_is_over_relative_arc(false),
	d_relative_arc_is_being_dragged(false),
	d_mouse_is_over_relative_arc_end_point(false),
	d_relative_arc_end_point_is_being_dragged(false),
	d_current_pole_12(GPlatesMaths::UnitVector3D(1,0,0)),
	d_current_angle_12(INITIAL_ANGLE),
	d_end_point_of_reference_arc_12(GPlatesMaths::UnitVector3D(1,0,0)),
	d_end_point_of_relative_arc_12(GPlatesMaths::UnitVector3D(1,0,0)),
	d_current_pole_13(GPlatesMaths::UnitVector3D(1,0,0)),
	d_current_angle_13(INITIAL_ANGLE),
	d_end_point_of_reference_arc_13(GPlatesMaths::UnitVector3D(1,0,0)),
	d_end_point_of_relative_arc_13(GPlatesMaths::UnitVector3D(1,0,0)),
	d_has_been_activated(false),
	d_active_pole_type(NO_ACTIVE_POLE_TYPE)
{
	d_current_pole_arrow_layer_ptr =
			d_rendered_geom_collection_ptr->create_child_rendered_layer_and_transfer_ownership(
				GPlatesViewOperations::RenderedGeometryCollection::HELLINGER_CANVAS_TOOL_WORKFLOW_LAYER);

	d_current_pole_and_angle_layer_ptr =
			d_rendered_geom_collection_ptr->create_child_rendered_layer_and_transfer_ownership(
				GPlatesViewOperations::RenderedGeometryCollection::HELLINGER_CANVAS_TOOL_WORKFLOW_LAYER);

	d_highlight_layer_ptr =
			d_rendered_geom_collection_ptr->create_child_rendered_layer_and_transfer_ownership(
				GPlatesViewOperations::RenderedGeometryCollection::HELLINGER_CANVAS_TOOL_WORKFLOW_LAYER);

	QObject::connect(d_hellinger_dialog_ptr,
					 SIGNAL(pole_estimate_12_lat_lon_changed(double,double)),
					 this,
					 SLOT(handle_pole_estimate_12_lat_lon_changed(double,double)));

	QObject::connect(d_hellinger_dialog_ptr,
					 SIGNAL(pole_estimate_12_angle_changed(double)),
					 this,
					 SLOT(handle_pole_estimate_12_angle_changed(double)));

	QObject::connect(d_hellinger_dialog_ptr,
					 SIGNAL(pole_estimate_13_lat_lon_changed(double,double)),
					 this,
					 SLOT(handle_pole_estimate_13_lat_lon_changed(double,double)));

	QObject::connect(d_hellinger_dialog_ptr,
					 SIGNAL(pole_estimate_13_angle_changed(double)),
					 this,
					 SLOT(handle_pole_estimate_13_angle_changed(double)));
}

void
GPlatesCanvasTools::AdjustFittedPoleEstimate::handle_activation()
{
	d_current_pole_arrow_layer_ptr->set_active(true);
	d_current_pole_and_angle_layer_ptr->set_active(true);
	d_highlight_layer_ptr->set_active(true);

	set_status_bar_message(QT_TR_NOOP("Click and drag to adjust the pole estimate(s) and angle(s)."));

	d_hellinger_dialog_ptr->set_state_for_pole_adjustment_tool(true);

	update_local_values_from_hellinger_dialog();
	update_current_pole_arrow_layer();
	update_current_pole_and_angle_layer();

	d_has_been_activated = true;
}

void
GPlatesCanvasTools::AdjustFittedPoleEstimate::handle_deactivation()
{
	update_hellinger_dialog_from_local_values();

	d_hellinger_dialog_ptr->set_state_for_pole_adjustment_tool(false);

	d_current_pole_arrow_layer_ptr->set_active(false);
	d_current_pole_and_angle_layer_ptr->set_active(false);
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
	d_mouse_is_over_reference_arc_end_point = false;
	d_mouse_is_over_relative_arc_end_point = false;
	d_active_pole_type = NO_ACTIVE_POLE_TYPE;

	if (GPlatesViewOperations::test_proximity(
				sorted_hits,
				proximity_criteria,
				*d_current_pole_and_angle_layer_ptr))
	{
		// The hits are sorted by closeness, but here we want to sort by index to be sure we get
		// one of the vertices - which are rendered before the arcs, and so have lower geometry index.
		sort_proximity_by_index(sorted_hits);

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

			// We have moved the mouse over one of the 4 geometries in the current_pole_and_angle_layer.
			unsigned int index = hit.d_rendered_geom_index;
			switch(index){
			case POLE_12_GEOMETRY_INDEX:
				d_mouse_is_over_pole_estimate = true;
				d_active_pole_type = PLATES_1_2_POLE_TYPE;
				update_current_pole_arrow_layer();
				update_current_pole_and_angle_layer();
				update_pole_estimate_and_arc_highlight(d_current_pole_12,
													   d_end_point_of_reference_arc_12,
													   d_end_point_of_relative_arc_12);
				break;
			case REFERENCE_ARC_ENDPOINT_12_GEOMETRY_INDEX:
				d_active_pole_type = PLATES_1_2_POLE_TYPE;
				d_mouse_is_over_reference_arc_end_point = true;
				update_arc_and_end_point_highlight(d_end_point_of_reference_arc_12,
												   d_current_pole_12);
				break;
			case RELATIVE_ARC_ENDPOINT_12_GEOMETRY_INDEX:
				d_mouse_is_over_relative_arc_end_point = true;
				d_active_pole_type = PLATES_1_2_POLE_TYPE;
				update_arc_and_end_point_highlight(d_end_point_of_relative_arc_12,
												   d_current_pole_12);
				break;
			case REFERENCE_ARC_12_GEOMETRY_INDEX:
				// Ignore this geometry for now. It's simpler to control the movement by the end point.
				break;
			case POLE_13_GEOMETRY_INDEX:
				d_mouse_is_over_pole_estimate = true;
				d_active_pole_type = PLATES_1_3_POLE_TYPE;
				update_current_pole_arrow_layer();
				update_current_pole_and_angle_layer();
				update_pole_estimate_and_arc_highlight(d_current_pole_13,
													   d_end_point_of_reference_arc_13,
													   d_end_point_of_relative_arc_13);
				break;
			case REFERENCE_ARC_ENDPOINT_13_GEOMETRY_INDEX:
				d_mouse_is_over_reference_arc_end_point = true;
				d_active_pole_type = PLATES_1_3_POLE_TYPE;
				update_arc_and_end_point_highlight(d_end_point_of_reference_arc_13,
												   d_current_pole_13);
				break;
			case RELATIVE_ARC_ENDPOINT_13_GEOMETRY_INDEX:
				d_mouse_is_over_relative_arc_end_point = true;
				d_active_pole_type = PLATES_1_3_POLE_TYPE;
				update_arc_and_end_point_highlight(d_end_point_of_relative_arc_13,
												   d_current_pole_13);
				break;
			case REFERENCE_ARC_13_GEOMETRY_INDEX:
				// Ignore this geometry for now. It's simpler to control the movement by the end point.
				break;


			}
		}
	}
	else
	{
		d_highlight_layer_ptr->clear_rendered_geometries();
	}

	update_current_pole_arrow_layer();
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
		d_reference_arc_end_point_is_being_dragged = d_mouse_is_over_reference_arc_end_point;
		d_relative_arc_end_point_is_being_dragged = d_mouse_is_over_relative_arc_end_point;
	}

}


void
GPlatesCanvasTools::AdjustFittedPoleEstimate::handle_pole_estimate_12_lat_lon_changed(
		double lat,
		double lon)
{
	d_current_pole_12 = GPlatesMaths::make_point_on_sphere(GPlatesMaths::LatLonPoint(lat,lon));
	update_angle();
	update_current_pole_arrow_layer();
	update_current_pole_and_angle_layer();
	update_hellinger_dialog_from_local_values();
}

void
GPlatesCanvasTools::AdjustFittedPoleEstimate::handle_pole_estimate_12_angle_changed(
		double angle)
{
	generate_new_relative_end_point(d_current_pole_12,d_end_point_of_reference_arc_12,d_end_point_of_relative_arc_12,angle);
	update_current_pole_arrow_layer();
	update_current_pole_and_angle_layer();
}

void
GPlatesCanvasTools::AdjustFittedPoleEstimate::handle_pole_estimate_13_lat_lon_changed(
		double lat,
		double lon)
{
	d_current_pole_13 = GPlatesMaths::make_point_on_sphere(GPlatesMaths::LatLonPoint(lat,lon));
	update_angle();
	update_current_pole_arrow_layer();
	update_current_pole_and_angle_layer();
	update_hellinger_dialog_from_local_values();
}

void
GPlatesCanvasTools::AdjustFittedPoleEstimate::handle_pole_estimate_13_angle_changed(
		double angle)
{
	generate_new_relative_end_point(d_current_pole_13,d_end_point_of_reference_arc_13,d_end_point_of_relative_arc_13,angle);
	update_current_pole_arrow_layer();
	update_current_pole_and_angle_layer();
}



void
GPlatesCanvasTools::AdjustFittedPoleEstimate::handle_left_release_after_drag(
		const GPlatesMaths::PointOnSphere &initial_point_on_sphere,
		bool was_on_earth,
		double initial_proximity_inclusion_threshold,
		const GPlatesMaths::PointOnSphere &current_point_on_sphere,
		bool is_on_earth,
		double current_proximity_inclusion_threshold,
		const GPlatesMaths::PointOnSphere &centre_of_viewport)
{
	if (d_pole_is_being_dragged)
	{
		d_pole_is_being_dragged = false;
		d_mouse_is_over_pole_estimate = false;
	}
	d_reference_arc_end_point_is_being_dragged = false;
	d_relative_arc_end_point_is_being_dragged = false;
	d_relative_arc_is_being_dragged = false;
	d_reference_arc_is_being_draggged = false;

	d_highlight_layer_ptr->clear_rendered_geometries();

	update_current_pole_arrow_layer();
	update_current_pole_and_angle_layer();
}

void
GPlatesCanvasTools::AdjustFittedPoleEstimate::handle_left_drag(
		const GPlatesMaths::PointOnSphere &initial_point_on_sphere,
		bool was_on_earth,
		double initial_proximity_inclusion_threshold,
		const GPlatesMaths::PointOnSphere &current_point_on_sphere,
		bool is_on_earth,
		double current_proximity_inclusion_threshold,
		const GPlatesMaths::PointOnSphere &centre_of_viewport)
{
	if (d_pole_is_being_dragged)
	{
		if (d_active_pole_type == PLATES_1_2_POLE_TYPE)
		{
			d_current_pole_12 = current_point_on_sphere;
			update_pole_estimate_and_arc_highlight(current_point_on_sphere,
												   d_end_point_of_reference_arc_12,
												   d_end_point_of_relative_arc_12);
		}
		else
		{
			d_current_pole_13 = current_point_on_sphere;
			update_pole_estimate_and_arc_highlight(current_point_on_sphere,
												   d_end_point_of_reference_arc_13,
												   d_end_point_of_relative_arc_13);
		}
		update_angle();
		update_hellinger_dialog_from_local_values();
	}
	else if (d_reference_arc_end_point_is_being_dragged)
	{
		if (d_active_pole_type == PLATES_1_2_POLE_TYPE)
		{
			d_end_point_of_reference_arc_12 = current_point_on_sphere;
			update_arc_and_end_point_highlight(current_point_on_sphere,
											   d_current_pole_12);
		}
		else
		{
			d_end_point_of_reference_arc_13 = current_point_on_sphere;
			update_arc_and_end_point_highlight(current_point_on_sphere,
											   d_current_pole_13);
		}
		update_angle();
		update_hellinger_dialog_from_local_values();
	}
	else if (d_relative_arc_end_point_is_being_dragged)
	{
		if (d_active_pole_type == PLATES_1_2_POLE_TYPE)
		{
			d_end_point_of_relative_arc_12 = current_point_on_sphere;
			update_arc_and_end_point_highlight(current_point_on_sphere,
											   d_current_pole_12);
		}
		else
		{
			d_end_point_of_relative_arc_13 = current_point_on_sphere;
			update_arc_and_end_point_highlight(current_point_on_sphere,
											   d_current_pole_13);
		}
		update_angle();
		update_hellinger_dialog_from_local_values();
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
	return (d_mouse_is_over_pole_estimate ||
			d_mouse_is_over_reference_arc ||
			d_mouse_is_over_relative_arc  ||
			d_mouse_is_over_reference_arc_end_point ||
			d_mouse_is_over_relative_arc_end_point);
}

void
GPlatesCanvasTools::AdjustFittedPoleEstimate::update_local_values_from_hellinger_dialog()
{
	d_current_pole_12 = GPlatesMaths::make_point_on_sphere(d_hellinger_dialog_ptr->get_pole_estimate_12_lat_lon());
	d_current_angle_12 = d_hellinger_dialog_ptr->get_pole_estimate_12_angle();

	d_current_pole_13 = GPlatesMaths::make_point_on_sphere(d_hellinger_dialog_ptr->get_pole_estimate_13_lat_lon());
	d_current_angle_13 = d_hellinger_dialog_ptr->get_pole_estimate_13_angle();

	if (!d_has_been_activated)
	{
		// Set an (arbitrary) initial direction for the reference arc relative to the pole, with arc-angle 30 degrees.
		GPlatesMaths::UnitVector3D perpendicular = GPlatesMaths::generate_perpendicular(d_current_pole_12.position_vector());
		GPlatesMaths::Rotation rotation_1 = GPlatesMaths::Rotation::create(perpendicular,GPlatesMaths::convert_deg_to_rad(30.));
		d_end_point_of_reference_arc_12 = rotation_1*d_current_pole_12;

		GPlatesMaths::Rotation rotation_2 = GPlatesMaths::Rotation::create(d_current_pole_12.position_vector(),GPlatesMaths::convert_deg_to_rad(d_current_angle_12));
		d_end_point_of_relative_arc_12 = rotation_2*d_end_point_of_reference_arc_12;

		d_end_point_of_reference_arc_13 = rotation_1*d_current_pole_13;
		d_end_point_of_relative_arc_13 = rotation_2*d_end_point_of_reference_arc_13;
	}
}

void
GPlatesCanvasTools::AdjustFittedPoleEstimate::update_hellinger_dialog_from_local_values()
{
	d_hellinger_dialog_ptr->update_pole_estimates(
				d_current_pole_12,
				d_current_angle_12,
				d_current_pole_13,
				d_current_angle_13);
}



void
GPlatesCanvasTools::AdjustFittedPoleEstimate::update_current_pole_arrow_layer()
{
	d_current_pole_arrow_layer_ptr->clear_rendered_geometries();

	if (!d_mouse_is_over_pole_estimate || (d_active_pole_type != PLATES_1_2_POLE_TYPE))
	{
		GPlatesViewOperations::RenderedGeometry pole_12_geometry_arrow =
				GPlatesViewOperations::RenderedGeometryFactory::create_rendered_radial_arrow(
					d_current_pole_12,
					d_hellinger_dialog_ptr->configuration().d_pole_arrow_height/*arrow_projected_length*/,
					d_hellinger_dialog_ptr->configuration().d_pole_arrow_radius/*arrowhead_projected_size*/,
					0.5f/*ratio_arrowline_width_to_arrowhead_size*/,
					GPlatesGui::Colour(0, 0, 1, 0.5f)/*arrow_colour*/,
					GPlatesViewOperations::RenderedRadialArrow::SYMBOL_CIRCLE_WITH_CROSS/*symbol_type*/,
					10.0f/*symbol_size*/,
					GPlatesGui::Colour::get_blue()/*symbol_colour*/);

		d_current_pole_arrow_layer_ptr->add_rendered_geometry(pole_12_geometry_arrow);
	}

	if (d_hellinger_dialog_ptr->get_fit_type() == GPlatesQtWidgets::THREE_PLATE_FIT_TYPE)
	{
		if (!d_mouse_is_over_pole_estimate || (d_active_pole_type != PLATES_1_3_POLE_TYPE))
		{
			GPlatesViewOperations::RenderedGeometry pole_13_geometry_arrow =
					GPlatesViewOperations::RenderedGeometryFactory::create_rendered_radial_arrow(
						d_current_pole_13,
						d_hellinger_dialog_ptr->configuration().d_pole_arrow_height/*arrow_projected_length*/,
						d_hellinger_dialog_ptr->configuration().d_pole_arrow_radius/*arrowhead_projected_size*/,
						0.5f/*ratio_arrowline_width_to_arrowhead_size*/,
						GPlatesGui::Colour(0, 0, 1, 0.5f)/*arrow_colour*/,
						GPlatesViewOperations::RenderedRadialArrow::SYMBOL_CIRCLE_WITH_CROSS/*symbol_type*/,
						10.0f/*symbol_size*/,
						GPlatesGui::Colour::get_blue()/*symbol_colour*/);

			d_current_pole_arrow_layer_ptr->add_rendered_geometry(pole_13_geometry_arrow);
		}
	}
}


void GPlatesCanvasTools::AdjustFittedPoleEstimate::update_current_pole_and_angle_layer()
{
	/**
	 * These geometries should be added in the same order as the GeometryTypeIndex so that we
	 * can tell which type of geometry we are hovering over.
	 *
	 * From the header file:
	 * enum GeometryTypeIndex
	 * {
	 * POLE_GEOMETRY_INDEX,
	 * REFERENCE_ARC_ENDPOINT_GEOMETRY_INDEX,
	 * RELATIVE_ARC_ENDPOINT_GEOMETRY_INDEX,
	 * REFERENCE_ARC_GEOMETRY_INDEX,
	 * RELATIVE_ARC_GEOMETRY_INDEX
	 * };
	 *
	 */

	d_current_pole_and_angle_layer_ptr->clear_rendered_geometries();


	update_pole_and_angle_geometries(d_current_pole_and_angle_layer_ptr,
									 d_current_pole_12,
									 d_end_point_of_reference_arc_12,
									 d_end_point_of_relative_arc_12);

	if (d_hellinger_dialog_ptr->get_fit_type() == GPlatesQtWidgets::THREE_PLATE_FIT_TYPE)
	{
		update_pole_and_angle_geometries(d_current_pole_and_angle_layer_ptr,
										 d_current_pole_13,
										 d_end_point_of_reference_arc_13,
										 d_end_point_of_relative_arc_13);
	}
}

void
GPlatesCanvasTools::AdjustFittedPoleEstimate::update_pole_estimate_and_arc_highlight(
		const GPlatesMaths::PointOnSphere &pole,
		const GPlatesMaths::PointOnSphere &reference_arc_end_point,
		const GPlatesMaths::PointOnSphere &relative_arc_end_point)
{
	d_highlight_layer_ptr->clear_rendered_geometries();


	try{

		GPlatesViewOperations::RenderedGeometry pole_geometry =
				GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
					pole.get_geometry_on_sphere(),
					VERTEX_COLOUR_POLE_12,
					2, /* point size */
					2, /* line thickness */
					false, /* fill polygon */
					false, /* fill polyline */
					GPlatesGui::Colour::get_white(), /* dummy colour */
					POLE_SYMBOL);

		GPlatesViewOperations::RenderedGeometry pole_arrow_geometry =
				GPlatesViewOperations::RenderedGeometryFactory::create_rendered_radial_arrow(
					pole,
					d_hellinger_dialog_ptr->configuration().d_pole_arrow_height/*arrow_projected_length*/,
					d_hellinger_dialog_ptr->configuration().d_pole_arrow_radius/*arrowhead_projected_size*/,
					0.5f/*ratio_arrowline_width_to_arrowhead_size*/,
					GPlatesGui::Colour::get_yellow()/*arrow_colour*/,
					GPlatesViewOperations::RenderedRadialArrow::SYMBOL_CIRCLE_WITH_CROSS/*symbol_type*/,
					10.0f/*symbol_size*/,
					GPlatesGui::Colour::get_yellow()/*symbol_colour*/);

		GPlatesMaths::GreatCircleArc gca_reference = GPlatesMaths::GreatCircleArc::create(pole, reference_arc_end_point);
		std::vector<GPlatesMaths::PointOnSphere> points;
		GPlatesMaths::tessellate(points,gca_reference,GPlatesMaths::PI/1800.);

		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type reference_polyline =
				GPlatesMaths::PolylineOnSphere::create(points);

		GPlatesViewOperations::RenderedGeometry gca_reference_geometry =
				GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
					reference_polyline,
					GPlatesGui::Colour::get_yellow(),
					3,
					3);

		GPlatesMaths::GreatCircleArc gca_relative = GPlatesMaths::GreatCircleArc::create(pole, relative_arc_end_point);

		GPlatesMaths::tessellate(points,gca_relative,GPlatesMaths::PI/1800.);


		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type relative_polyline =
				GPlatesMaths::PolylineOnSphere::create(points);

		GPlatesViewOperations::RenderedGeometry gca_relative_geometry =
				GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
					relative_polyline,
					GPlatesGui::Colour::get_yellow(),
					3,
					3);

		d_highlight_layer_ptr->add_rendered_geometry(pole_arrow_geometry);
		d_highlight_layer_ptr->add_rendered_geometry(pole_geometry);
		d_highlight_layer_ptr->add_rendered_geometry(gca_reference_geometry);
		d_highlight_layer_ptr->add_rendered_geometry(gca_relative_geometry);
	}
	catch(std::exception &exception)
	{
		qWarning() << "Error updating pole estimate and arc highlight in AdjustFittedPoleEstimate: " << exception.what();
	}
	catch(...)
	{
		qWarning() << "Unknown error updating pole estimate and arc highlight in AdjustFittedPoleEstimate.";
	}

}

void
GPlatesCanvasTools::AdjustFittedPoleEstimate::update_arc_and_end_point_highlight(
		const GPlatesMaths::PointOnSphere &end_point,
		const GPlatesMaths::PointOnSphere &pole)
{
	d_highlight_layer_ptr->clear_rendered_geometries();

	GPlatesViewOperations::RenderedGeometry end_point_geometry =
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
				end_point.get_geometry_on_sphere(),
				GPlatesGui::Colour::get_yellow(),
				2, /* point size */
				2, /* line thickness */
				false, /* fill polygon */
				false, /* fill polyline */
				GPlatesGui::Colour::get_white(), /* dummy colour */
				END_POINT_HIGHLIGHT_SYMBOL);


	d_highlight_layer_ptr->add_rendered_geometry(end_point_geometry);

	try {
		GPlatesMaths::GreatCircleArc gca = GPlatesMaths::GreatCircleArc::create(pole, end_point);
		std::vector<GPlatesMaths::PointOnSphere> points;
		GPlatesMaths::tessellate(points,gca,GPlatesMaths::PI/1800.);

		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline =
				GPlatesMaths::PolylineOnSphere::create(points);

		GPlatesViewOperations::RenderedGeometry gca_geometry =
				GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
					polyline,
					GPlatesGui::Colour::get_yellow(),
					3,
					3);

		d_highlight_layer_ptr->add_rendered_geometry(gca_geometry);
	}
	catch(std::exception &exception)
	{
		qWarning() << "Error updating arc and endpoint in AdjustFittedPoleEstimate: " << exception.what();
	}
	catch(...)
	{
		qWarning() << "Unknown error updating arc and endpoint in AdjustFittedPoleEstimate.";
	}

}

void GPlatesCanvasTools::AdjustFittedPoleEstimate::update_angle()
{
	try{

		if (d_active_pole_type == PLATES_1_2_POLE_TYPE)
		{
			GPlatesMaths::GreatCircleArc gca_1 = GPlatesMaths::GreatCircleArc::create(d_end_point_of_reference_arc_12,d_current_pole_12);
			GPlatesMaths::GreatCircleArc gca_2 = GPlatesMaths::GreatCircleArc::create(d_current_pole_12,d_end_point_of_relative_arc_12);

			double angle = GPlatesMaths::convert_rad_to_deg(GPlatesMaths::calculate_angle_between_adjacent_non_zero_length_arcs(gca_1,gca_2));
			d_current_angle_12 = (angle > 180.) ? (angle - 360.) : angle;
		}
		else if (d_active_pole_type == PLATES_1_3_POLE_TYPE)
		{
			GPlatesMaths::GreatCircleArc gca_1 = GPlatesMaths::GreatCircleArc::create(d_end_point_of_reference_arc_13,d_current_pole_13);
			GPlatesMaths::GreatCircleArc gca_2 = GPlatesMaths::GreatCircleArc::create(d_current_pole_13,d_end_point_of_relative_arc_13);

			double angle = GPlatesMaths::convert_rad_to_deg(GPlatesMaths::calculate_angle_between_adjacent_non_zero_length_arcs(gca_1,gca_2));
			d_current_angle_13 = (angle > 180.) ? (angle - 360.) : angle;
		}
	}
	catch(std::exception &exception)
	{
		qWarning() << "Error updating angle in AdjustFittedPoleEstimate: " << exception.what();
	}
	catch(...)
	{
		qWarning() << "Unknown error updating angle in AdjustFittedPoleEstimate.";
	}

}



