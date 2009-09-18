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
#include <QDebug>

#include "MeasureDistance.h"
#include "MeasureDistanceState.h"
#include "gui/Colour.h"
#include "maths/PointOnSphere.h"
#include "maths/PolylineOnSphere.h"
#include "maths/ProximityCriteria.h"
#include "view-operations/GeometryBuilder.h"
#include "view-operations/GeometryType.h"
#include "view-operations/RenderedGeometryCollection.h"
#include "view-operations/RenderedGeometryFactory.h"
#include "view-operations/RenderedGeometryProximity.h"

const GPlatesGui::Colour
GPlatesCanvasTools::MeasureDistance::QUICK_MEASURE_LINE_COLOUR = GPlatesGui::Colour::get_white();

const GPlatesGui::Colour
GPlatesCanvasTools::MeasureDistance::FEATURE_MEASURE_LINE_COLOUR = GPlatesGui::Colour::get_grey();

const GPlatesGui::Colour
GPlatesCanvasTools::MeasureDistance::HIGHLIGHT_COLOUR = GPlatesGui::Colour::get_yellow();

const GPlatesGui::Colour
GPlatesCanvasTools::MeasureDistance::LABEL_COLOUR = GPlatesGui::Colour::get_yellow();

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
	d_label_layer_ptr(measure_distance_state.get_label_layer_ptr()),
	d_num_feature_measure_lines_rendered(0)
{
	make_signal_slot_connections();
	d_main_layer_ptr->set_active(true);
}

void
GPlatesCanvasTools::MeasureDistance::make_signal_slot_connections()
{
	QObject::connect(
			d_measure_distance_state_ptr,
			SIGNAL(feature_in_geometry_builder_changed()),
			this,
			SLOT(feature_changed()));
}

void
GPlatesCanvasTools::MeasureDistance::feature_changed()
{
	paint();
}

void
GPlatesCanvasTools::MeasureDistance::handle_activation()
{
	d_measure_distance_state_ptr->handle_activation();

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

	// deactivate rendered layer
	d_rendered_geom_collection_ptr->set_main_layer_active(
			GPlatesViewOperations::RenderedGeometryCollection::MEASURE_DISTANCE_LAYER,
			false);
}

void
GPlatesCanvasTools::MeasureDistance::paint()
{
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
	// we draw a point if there's only one point so as to
	// provide some visual feedback
	// for two points, we just draw the line
	boost::optional<GPlatesMaths::PointOnSphere> start =
		d_measure_distance_state_ptr->get_quick_measure_start();
	if (start)
	{
		boost::optional<GPlatesMaths::PointOnSphere> end =
			d_measure_distance_state_ptr->get_quick_measure_end();
		if (!end)
		{
			render_point_on_sphere(*start, QUICK_MEASURE_LINE_COLOUR, d_main_layer_ptr);
		}
		if (end)
		{
			render_line(*start, *end, QUICK_MEASURE_LINE_COLOUR, d_main_layer_ptr);
		}
	}
}

void
GPlatesCanvasTools::MeasureDistance::paint_feature_measure()
{
	GPlatesViewOperations::GeometryBuilder *geometry_builder =
		d_measure_distance_state_ptr->get_current_geometry_builder_ptr();
	if (geometry_builder &&
			(geometry_builder->get_geometry_build_type() == GPlatesViewOperations::GeometryType::POLYLINE ||
			 geometry_builder->get_geometry_build_type() == GPlatesViewOperations::GeometryType::POLYGON))
				// Feature Measure tool does not apply to points
	{
		GPlatesViewOperations::GeometryBuilder::GeometryIndex current_index =
			geometry_builder->get_current_geometry_index();
		if (geometry_builder->get_num_points_in_current_geometry() > 1)
		{
			// how many lines rendered? store for later use
			bool is_polygon = geometry_builder->get_geometry_build_type() == GPlatesViewOperations::GeometryType::POLYGON;
			d_num_feature_measure_lines_rendered = geometry_builder->get_num_points_in_current_geometry();
			if (!is_polygon) // for n points, polygon has n lines, polyline has n - 1 lines
			{
				--d_num_feature_measure_lines_rendered;
			}

			// get iterators to points in geometry builder
			typedef GPlatesViewOperations::GeometryBuilder::point_const_iterator_type iterator_type;
			iterator_type begin = geometry_builder->get_geometry_point_begin(current_index);
			iterator_type end = geometry_builder->get_geometry_point_end(current_index);

			// draw lines
			render_multiple_line_segments(begin, end, FEATURE_MEASURE_LINE_COLOUR, is_polygon, d_main_layer_ptr);
		}
	}
	else
	{
		d_num_feature_measure_lines_rendered = 0;
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
		GPlatesViewOperations::RenderedGeometry rendered =
			GPlatesViewOperations::create_rendered_string(
					*d_label_position,
					*d_label_text,
					LABEL_COLOUR,
					LABEL_X_OFFSET,
					LABEL_Y_OFFSET);
		d_label_layer_ptr->add_rendered_geometry(rendered);
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
			if (rendered_geom_index == d_num_feature_measure_lines_rendered)
			{
				// Quick Measure (since it's the last line drawn)
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

				// no current feature segment
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
					typedef GPlatesViewOperations::GeometryBuilder::point_const_iterator_type iterator_type;

					// if it is a polygon, the last line rendered will be the line joining end to start
					iterator_type line_segment_begin, line_segment_end;
					if (is_polygon && rendered_geom_index == d_num_feature_measure_lines_rendered - 1)
					{
						line_segment_end = geometry_builder->get_geometry_point_begin(geom_index);
						line_segment_begin = line_segment_end;
						std::advance(line_segment_begin, rendered_geom_index);
					}
					else
					{
						line_segment_begin = geometry_builder->get_geometry_point_begin(geom_index);
						std::advance(line_segment_begin, rendered_geom_index);
						line_segment_end = line_segment_begin;
						++line_segment_end;
					}

					d_measure_distance_state_ptr->set_feature_segment_points(
							boost::optional<GPlatesMaths::PointOnSphere>(*line_segment_begin),
							boost::optional<GPlatesMaths::PointOnSphere>(*line_segment_end));
					if (d_measure_distance_state_ptr->get_feature_segment_distance())
					{
						add_distance_label_and_highlight(
								*(d_measure_distance_state_ptr->get_feature_segment_distance()),
								point_on_sphere,
								*line_segment_begin,
								*line_segment_end,
								false);
					}
				}
			}
		}
		else
		{
			// no close geometry
			remove_distance_label_and_highlight();

			// no current feature segment
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
	d_label_position = boost::optional<GPlatesMaths::PointOnSphere>(label_position);
	d_highlight_start = boost::optional<GPlatesMaths::PointOnSphere>(highlight_start);
	d_highlight_end = boost::optional<GPlatesMaths::PointOnSphere>(highlight_end);

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
void
GPlatesCanvasTools::MeasureDistance::render_line(
		const GPlatesMaths::PointOnSphere &start,
		const GPlatesMaths::PointOnSphere &end,
		const GPlatesGui::Colour &colour,
		LayerPointerType layer_ptr)
{
	const GPlatesMaths::PointOnSphere points[] = { start, end };
	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline =
		GPlatesMaths::PolylineOnSphere::create_on_heap(
				points,
				points + sizeof(points) / sizeof(GPlatesMaths::PointOnSphere));
	GPlatesViewOperations::RenderedGeometry rendered =
		GPlatesViewOperations::create_rendered_polyline_on_sphere(
				polyline,
				colour,
				LINE_WIDTH);
	layer_ptr->add_rendered_geometry(rendered);
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
	++begin;
	for (iterator_type iter = begin; iter != end; ++iter)
	{
		render_line(*previous, *iter, colour, layer_ptr);
		previous = iter;
	}
	if (is_polygon)
	{
		render_line(*previous, *first, colour, layer_ptr);
	}
}

