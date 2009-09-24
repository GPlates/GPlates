/* $Id$ */

/**
 * @file 
 * Contains implementation of MeasureDistance
 *
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#include <iterator>
#include <QObject>

#include "MeasureDistance.h"
#include "MeasureDistanceState.h"
#include "gui/Colour.h"
#include "maths/GreatCircleArc.h"
#include "maths/PointOnSphere.h"
#include "maths/PolylineOnSphere.h"
#include "maths/ProximityCriteria.h"
#include "utils/GeometryCreationUtils.h"
#include "view-operations/GeometryType.h"
#include "view-operations/RenderedGeometryCollection.h"
#include "view-operations/RenderedGeometryFactory.h"
#include "view-operations/RenderedGeometryProximity.h"

const GPlatesGui::Colour
GPlatesCanvasTools::MeasureDistance::QUICK_MEASURE_LINE_COLOUR = GPlatesGui::Colour::get_grey();

const GPlatesGui::Colour
GPlatesCanvasTools::MeasureDistance::FEATURE_MEASURE_LINE_COLOUR = GPlatesGui::Colour::get_white();

const GPlatesGui::Colour
GPlatesCanvasTools::MeasureDistance::HIGHLIGHT_COLOUR = GPlatesGui::Colour::get_yellow();

const GPlatesGui::Colour
GPlatesCanvasTools::MeasureDistance::LABEL_COLOUR = GPlatesGui::Colour::get_yellow();

const GPlatesGui::Colour
GPlatesCanvasTools::MeasureDistance::LABEL_SHADOW_COLOUR = GPlatesGui::Colour::get_black();

const float
GPlatesCanvasTools::MeasureDistance::POINT_SIZE = 2.0f * GPlatesViewOperations::RenderedGeometryFactory::DEFAULT_POINT_SIZE_HINT;

const float
GPlatesCanvasTools::MeasureDistance::LINE_WIDTH = 2.0f * GPlatesViewOperations::RenderedGeometryFactory::DEFAULT_LINE_WIDTH_HINT;

const int
GPlatesCanvasTools::MeasureDistance::LABEL_PRECISION = 4;

const int
GPlatesCanvasTools::MeasureDistance::LABEL_X_OFFSET = 3;

const int
GPlatesCanvasTools::MeasureDistance::LABEL_Y_OFFSET = 5;

GPlatesCanvasTools::MeasureDistance::MeasureDistance(
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
				GPlatesCanvasTools::MeasureDistanceState &measure_distance_state):
	d_rendered_geom_collection_ptr(&rendered_geom_collection),
	d_measure_distance_state_ptr(&measure_distance_state),
	d_main_layer_ptr(measure_distance_state.get_main_layer_ptr()),
	d_highlight_layer_ptr(measure_distance_state.get_highlight_layer_ptr()),
	d_label_layer_ptr(measure_distance_state.get_label_layer_ptr())
{
	make_signal_slot_connections();
	d_main_layer_ptr->set_active(true);
}

void
GPlatesCanvasTools::MeasureDistance::make_signal_slot_connections()
{
	// listen to state object
	QObject::connect(
			d_measure_distance_state_ptr,
			SIGNAL(feature_in_geometry_builder_changed()),
			this,
			SLOT(feature_changed()));
}

void
GPlatesCanvasTools::MeasureDistance::feature_changed()
{
	// repaint if feature changed
	paint();
}

void
GPlatesCanvasTools::MeasureDistance::handle_activation()
{
	d_measure_distance_state_ptr->handle_activation();

	// set status bar message
	switch (get_view())
	{
		case GLOBE_VIEW:
			set_status_bar_message(QObject::tr("Click to measure distance between points. Ctrl+drag to rotate the globe."));
			break;
	
		case MAP_VIEW:
			set_status_bar_message(QObject::tr("Click to measure distance between points. Ctrl+drag to pan the map."));
			break;

		default:
			break;
	}

	// activate rendered layer
	d_rendered_geom_collection_ptr->set_main_layer_active(
			GPlatesViewOperations::RenderedGeometryCollection::MEASURE_DISTANCE_LAYER);

	// redraw everything
	paint();
}

void
GPlatesCanvasTools::MeasureDistance::handle_deactivation()
{
	d_measure_distance_state_ptr->handle_deactivation();
}

void
GPlatesCanvasTools::MeasureDistance::paint()
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;

	// clear the rendered geometries
	d_main_layer_ptr->clear_rendered_geometries();
	d_highlight_layer_ptr->clear_rendered_geometries();
	d_label_layer_ptr->clear_rendered_geometries();

	// must be painted in this order because the proximity test code
	// assumes that the Feature Measure lines are painted first
	paint_feature_measure();
	paint_quick_measure();

	// paint highlight and label into their own child layers
	paint_highlight();
	paint_label();
}

void
GPlatesCanvasTools::MeasureDistance::paint_quick_measure()
{
	boost::optional<GPlatesMaths::PointOnSphere> start =
		d_measure_distance_state_ptr->get_quick_measure_start();
	if (start)
	{
		boost::optional<GPlatesMaths::PointOnSphere> end =
			d_measure_distance_state_ptr->get_quick_measure_end();
		if (!end)
		{
			// just draw a point if only one point, to provide some visual feedback
			render_point_on_sphere(*start, QUICK_MEASURE_LINE_COLOUR, d_main_layer_ptr);
		}
		if (end)
		{
			// for two points, draw the line and no points
			render_line(*start, *end, QUICK_MEASURE_LINE_COLOUR, d_main_layer_ptr);
		}
	}
}

void
GPlatesCanvasTools::MeasureDistance::paint_feature_measure()
{
	// clear line to point mapping
	d_line_to_point_mapping.clear();

	GPlatesViewOperations::GeometryBuilder *geometry_builder =
		d_measure_distance_state_ptr->get_current_geometry_builder_ptr();
	if (geometry_builder &&
			geometry_builder->get_num_geometries() &&
			geometry_builder->get_num_points_in_current_geometry() > 1 &&
			(geometry_builder->get_geometry_build_type() == GPlatesViewOperations::GeometryType::POLYLINE ||
			 geometry_builder->get_geometry_build_type() == GPlatesViewOperations::GeometryType::POLYGON))
				// Feature Measure tool does not apply to multipoints
	{
		GPlatesViewOperations::GeometryBuilder::GeometryIndex current_index =
			geometry_builder->get_current_geometry_index();

		// get iterators to points in geometry builder
		typedef GPlatesViewOperations::GeometryBuilder::point_const_iterator_type iterator_type;
		iterator_type begin = geometry_builder->get_geometry_point_begin(current_index);
		iterator_type end = geometry_builder->get_geometry_point_end(current_index);

		// draw lines
		bool is_polygon = geometry_builder->get_geometry_build_type() == GPlatesViewOperations::GeometryType::POLYGON;
		render_multiple_line_segments(begin, end, FEATURE_MEASURE_LINE_COLOUR, is_polygon, d_main_layer_ptr);
	}
}

void
GPlatesCanvasTools::MeasureDistance::paint_highlight()
{
	if (d_highlight_start && d_highlight_end)
	{
		render_line(
				*d_highlight_start,
				*d_highlight_end,
				HIGHLIGHT_COLOUR,
				d_highlight_layer_ptr);
	}
}

void
GPlatesCanvasTools::MeasureDistance::paint_label()
{
	if (d_label_text && d_label_position)
	{
		// now paint main label on top
		GPlatesViewOperations::RenderedGeometry main_label =
			GPlatesViewOperations::create_rendered_string(
					*d_label_position,
					*d_label_text,
					LABEL_COLOUR,
					LABEL_SHADOW_COLOUR,
					LABEL_X_OFFSET,
					LABEL_Y_OFFSET);
		d_label_layer_ptr->add_rendered_geometry(main_label);
	}
}

void
GPlatesCanvasTools::MeasureDistance::handle_left_click(
		const GPlatesMaths::PointOnSphere &point_on_sphere,
		bool is_on_earth,
		double proximity_inclusion_threshold)
{
	/* left-click adds points to the Quick Measure tool,
	 * which remembers the last two points clicked and calculates
	 * the distance between them */

	if (d_measure_distance_state_ptr->is_active())
	{
		// add the point to the state object
		d_measure_distance_state_ptr->quick_measure_add_point(point_on_sphere);

		// remove the label and highlighting in case the user clicked while doing mouse-over
		remove_distance_label_and_highlight();

		// redraw everything
		paint();
	}
}

void
GPlatesCanvasTools::MeasureDistance::handle_move_without_drag(
		const GPlatesMaths::PointOnSphere &point_on_sphere,
		bool is_on_earth,
		double proximity_inclusion_threshold)
{
	/* mouse move over a line segment will display the length
	 * of that line segment */

	if (d_measure_distance_state_ptr->is_active() && is_on_earth)
	{
		// test if any line segments are near the cursor
		GPlatesMaths::ProximityCriteria proximity_criteria(
				point_on_sphere,
				proximity_inclusion_threshold);
		std::vector<GPlatesViewOperations::RenderedGeometryProximityHit> sorted_hits;
		if (GPlatesViewOperations::test_proximity(
					sorted_hits,
					*d_main_layer_ptr,
					proximity_criteria))
		{
			// a close hit found
			const GPlatesViewOperations::RenderedGeometryProximityHit &closest_hit = sorted_hits.front();
			const unsigned int rendered_geom_index = closest_hit.d_rendered_geom_index;

			// work out if the rendered geometry belongs to Quick Measure or Feature Measure
			// note that the Quick Measure line is always rendered immediately after the Feature Meaure lines
			if (rendered_geom_index == d_line_to_point_mapping.size())
			{
				// Quick Measure
				if (d_measure_distance_state_ptr->get_quick_measure_distance())
					// true only if there are two quick measure points
				{
					add_distance_label_and_highlight(
							*(d_measure_distance_state_ptr->get_quick_measure_distance()),
							point_on_sphere,
							*d_measure_distance_state_ptr->get_quick_measure_start(),
							*d_measure_distance_state_ptr->get_quick_measure_end(),
							true);
				}

				// mouse not on a line segment belonging to a feature
				d_measure_distance_state_ptr->set_feature_segment_points(boost::none, boost::none);
			}
			else
			{
				// Feature Measure
				GPlatesViewOperations::GeometryBuilder *geometry_builder =
					d_measure_distance_state_ptr->get_current_geometry_builder_ptr();
				if (geometry_builder)
				{
					bool is_polygon = geometry_builder->get_geometry_build_type() == GPlatesViewOperations::GeometryType::POLYGON;
					GPlatesViewOperations::GeometryBuilder::GeometryIndex geom_index =
						geometry_builder->get_current_geometry_index();
					GPlatesViewOperations::GeometryBuilder::PointIndex start_point_index =
						d_line_to_point_mapping[rendered_geom_index];
					typedef GPlatesViewOperations::GeometryBuilder::point_const_iterator_type iterator_type;

					// work out the start and end points of the line segment
					// if it is a polygon, the last line rendered will be the line joining end to start
					iterator_type start_point, end_point;
					if (is_polygon && start_point_index == geometry_builder->get_num_points_in_current_geometry() - 1)
					{
						end_point = geometry_builder->get_geometry_point_begin(geom_index);
						start_point = end_point;
						std::advance(start_point, start_point_index);
					}
					else
					{
						start_point = geometry_builder->get_geometry_point_begin(geom_index);
						std::advance(start_point, start_point_index);
						end_point = start_point;
						++end_point;
					}

					d_measure_distance_state_ptr->set_feature_segment_points(
							boost::optional<GPlatesMaths::PointOnSphere>(*start_point),
							boost::optional<GPlatesMaths::PointOnSphere>(*end_point));
					if (d_measure_distance_state_ptr->get_feature_segment_distance())
					{
						add_distance_label_and_highlight(
								*(d_measure_distance_state_ptr->get_feature_segment_distance()),
								point_on_sphere,
								*start_point,
								*end_point,
								false);
					}
				}
			}
		}
		else
		{
			// no close hit found
			remove_distance_label_and_highlight();

			// mouse not on a line segment belonging to a feature
			d_measure_distance_state_ptr->set_feature_segment_points(boost::none, boost::none);
		}
	}
}

void
GPlatesCanvasTools::MeasureDistance::add_distance_label_and_highlight(
		double distance,
		const GPlatesMaths::PointOnSphere &label_position,
		const GPlatesMaths::PointOnSphere &highlight_start,
		const GPlatesMaths::PointOnSphere &highlight_end,
		bool is_quick_measure)
{
	d_label_text = boost::optional<QString>(QString("%1").arg(distance, 0, 'f', LABEL_PRECISION).append(" km"));
	d_highlight_start = boost::optional<GPlatesMaths::PointOnSphere>(highlight_start);
	d_highlight_end = boost::optional<GPlatesMaths::PointOnSphere>(highlight_end);

	// snap the label to a point on the line (it looks neater)
	GPlatesMaths::GreatCircleArc gca = GPlatesMaths::GreatCircleArc::create(highlight_start, highlight_end);
	d_label_position = boost::optional<GPlatesMaths::PointOnSphere>(
			*(gca.get_closest_point(label_position)));

	// redraw since we just changed the label
	paint();

	// set highlighting in widget
	d_measure_distance_state_ptr->set_quick_measure_highlight(is_quick_measure);
	d_measure_distance_state_ptr->set_feature_measure_highlight(!is_quick_measure);
}

void
GPlatesCanvasTools::MeasureDistance::remove_distance_label_and_highlight()
{
	if (d_label_text || d_label_position || d_highlight_start || d_highlight_end)
	{
		d_label_text = boost::none;
		d_label_position = boost::none;
		d_highlight_start = boost::none;
		d_highlight_end = boost::none;

		// redraw since we just got rid of the label
		paint();

		// remove the highlighting in widget
		d_measure_distance_state_ptr->set_quick_measure_highlight(false);
		d_measure_distance_state_ptr->set_feature_measure_highlight(false);
	}
}

template <typename LayerPointerType>
void
GPlatesCanvasTools::MeasureDistance::render_point_on_sphere(
		const GPlatesMaths::PointOnSphere &point_on_sphere,
		const GPlatesGui::Colour &colour,
		LayerPointerType layer_ptr)
{
	GPlatesViewOperations::RenderedGeometry rendered =
		GPlatesViewOperations::create_rendered_point_on_sphere(
				point_on_sphere,
				colour,
				POINT_SIZE);
	layer_ptr->add_rendered_geometry(rendered);
}

template <typename LayerPointerType>
bool
GPlatesCanvasTools::MeasureDistance::render_line(
		const GPlatesMaths::PointOnSphere &start,
		const GPlatesMaths::PointOnSphere &end,
		const GPlatesGui::Colour &colour,
		LayerPointerType layer_ptr)
{
	const GPlatesMaths::PointOnSphere points[] = { start, end };
	GPlatesUtils::GeometryConstruction::GeometryConstructionValidity validity;
	boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> polyline =
		GPlatesUtils::create_polyline_on_sphere(
				points,
				points + sizeof(points) / sizeof(GPlatesMaths::PointOnSphere),
				validity);

	// need to check for validity; it is invalid if the points are too close together
	if (validity == GPlatesUtils::GeometryConstruction::VALID)
	{
		GPlatesViewOperations::RenderedGeometry rendered =
			GPlatesViewOperations::create_rendered_polyline_on_sphere(
					*polyline,
					colour,
					LINE_WIDTH);
		layer_ptr->add_rendered_geometry(rendered);
		return true;
	}
	else
	{
		return false;
	}
}

template <typename LayerPointerType>
void
GPlatesCanvasTools::MeasureDistance::render_multiple_line_segments(
		GPlatesViewOperations::GeometryBuilder::point_const_iterator_type begin,
		GPlatesViewOperations::GeometryBuilder::point_const_iterator_type end,
		const GPlatesGui::Colour &colour,
		bool is_polygon,
		LayerPointerType layer_ptr)
{
	typedef GPlatesViewOperations::GeometryBuilder::point_const_iterator_type iterator_type;
	iterator_type previous = begin;
	iterator_type first = begin;
	for (iterator_type iter = begin + 1; iter != end; ++iter)
	{
		if (render_line(*previous, *iter, colour, layer_ptr))
		{
			d_line_to_point_mapping.push_back(previous - begin);
		}
		previous = iter;
	}

	// close off polygon if we are rendering a polygon
	if (is_polygon)
	{
		if (render_line(*previous, *first, colour, layer_ptr))
		{
			d_line_to_point_mapping.push_back(previous - begin);
		}
	}
}

