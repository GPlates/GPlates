/* $Id$ */

/**
 * @file
 * Contains implementation of MeasureDistanceState
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

#include "MeasureDistanceState.h"
#include "view-operations/GeometryBuilder.h"
#include "view-operations/GeometryOperationTarget.h"
#include "view-operations/GeometryType.h"
#include "view-operations/RenderedGeometryLayer.h"
#include "utils/MathUtils.h"

namespace
{
	boost::optional<double>
	calculate_distance_between_optional_points(
			boost::optional<GPlatesMaths::PointOnSphere> start,
			boost::optional<GPlatesMaths::PointOnSphere> end,
			GPlatesMaths::real_t radius)
	{
		if (start && end)
		{
			return boost::optional<double>(
					calculate_distance_on_surface_of_sphere(
						*start, *end, radius).dval());
		}
		else
		{
			return boost::none;
		}
	}
}

const double
GPlatesCanvasTools::MeasureDistanceState::DEFAULT_RADIUS_OF_EARTH = 6378.1;
	// from Google Calculator

GPlatesCanvasTools::MeasureDistanceState::MeasureDistanceState(
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
		GPlatesViewOperations::GeometryOperationTarget &geometry_operation_target) :
	d_main_layer_ptr(rendered_geom_collection.get_main_rendered_layer(
				GPlatesViewOperations::RenderedGeometryCollection::MEASURE_DISTANCE_LAYER)),
	d_highlight_layer_ptr(rendered_geom_collection.create_child_rendered_layer_and_transfer_ownership(
				GPlatesViewOperations::RenderedGeometryCollection::MEASURE_DISTANCE_LAYER)),
	d_label_layer_ptr(rendered_geom_collection.create_child_rendered_layer_and_transfer_ownership(
				GPlatesViewOperations::RenderedGeometryCollection::MEASURE_DISTANCE_LAYER)),
	d_radius(GPlatesCanvasTools::MeasureDistanceState::DEFAULT_RADIUS_OF_EARTH),
	d_geometry_operation_target_ptr(&geometry_operation_target),
	d_current_geometry_builder_ptr(NULL),
	d_is_active(false),
	d_is_quick_measure_highlighted(false),
	d_is_feature_measure_highlighted(false)
{
	make_signal_slot_connections_for_geometry_operation_target();
	d_highlight_layer_ptr->set_active(true);
	d_label_layer_ptr->set_active(true);
}

void
GPlatesCanvasTools::MeasureDistanceState::make_signal_slot_connections_for_geometry_operation_target()
{
	// listen to GeometryOperationTarget for changes to which GeometryBuilder we are using
	QObject::connect(
			d_geometry_operation_target_ptr,
			SIGNAL(switched_geometry_builder(
					GPlatesViewOperations::GeometryOperationTarget &,
					GPlatesViewOperations::GeometryBuilder *)),
			this,
			SLOT(switch_geometry_builder(
					GPlatesViewOperations::GeometryOperationTarget &,
					GPlatesViewOperations::GeometryBuilder *)));
}

void
GPlatesCanvasTools::MeasureDistanceState::make_signal_slot_connections_for_geometry_builder()
{
	if (d_current_geometry_builder_ptr)
	{
		// listen to the current GeometryBuilder for changes to the geometry
		QObject::connect(
				d_current_geometry_builder_ptr,
				SIGNAL(stopped_updating_geometry_excluding_intermediate_moves()),
				this,
				SLOT(reexamine_geometry_builder()));
	}
}

void
GPlatesCanvasTools::MeasureDistanceState::disconnect_signal_slot_connections_for_geometry_builder()
{
	if (d_current_geometry_builder_ptr)
	{
		// listen to the current GeometryBuilder for changes to the geometry
		QObject::disconnect(
				d_current_geometry_builder_ptr,
				SIGNAL(stopped_updating_geometry_excluding_intermediate_moves()),
				this,
				SLOT(reexamine_geometry_builder()));
	}
}

void
GPlatesCanvasTools::MeasureDistanceState::switch_geometry_builder(
		GPlatesViewOperations::GeometryOperationTarget &,
		GPlatesViewOperations::GeometryBuilder *geometry_builder)
{
	disconnect_signal_slot_connections_for_geometry_builder();
	d_current_geometry_builder_ptr = geometry_builder;
	make_signal_slot_connections_for_geometry_builder();
}

void
GPlatesCanvasTools::MeasureDistanceState::reexamine_geometry_builder()
{
	process_geometry_builder(d_current_geometry_builder_ptr);
	emit feature_in_geometry_builder_changed();
}

void
GPlatesCanvasTools::MeasureDistanceState::set_feature_segment_points(
		const boost::optional<GPlatesMaths::PointOnSphere> &start,
		const boost::optional<GPlatesMaths::PointOnSphere> &end)
{
	d_feature_segment_start = start;
	d_feature_segment_end = end;

	emit_feature_measure_updated();
}

boost::optional<double>
GPlatesCanvasTools::MeasureDistanceState::get_quick_measure_distance()
{
	return calculate_distance_between_optional_points(
			d_quick_measure_start,
			d_quick_measure_end,
			d_radius);
}

boost::optional<double>
GPlatesCanvasTools::MeasureDistanceState::get_feature_segment_distance()
{
	return calculate_distance_between_optional_points(
			d_feature_segment_start,
			d_feature_segment_end,
			d_radius);
}

void
GPlatesCanvasTools::MeasureDistanceState::emit_quick_measure_updated()
{
	emit quick_measure_updated(
			d_quick_measure_start,
			d_quick_measure_end,
			get_quick_measure_distance());
}

void
GPlatesCanvasTools::MeasureDistanceState::quick_measure_add_point(
		const GPlatesMaths::PointOnSphere &point)
{
	if (d_is_active)
	{
		if (!d_quick_measure_start) // 0 points
		{
			d_quick_measure_start = boost::optional<GPlatesMaths::PointOnSphere>(point);
		}
		else if (!d_quick_measure_end) // 1 point
		{
			if (*d_quick_measure_start != point) // the two points cannot be the same
			{
				d_quick_measure_end = boost::optional<GPlatesMaths::PointOnSphere>(point);
			}
		}
		else // 2 points
		{
			if (*d_quick_measure_end != point) // the two points cannot be the same
			{
				d_quick_measure_start = d_quick_measure_end;
				d_quick_measure_end = boost::optional<GPlatesMaths::PointOnSphere>(point);
			}
		}
		
		emit_quick_measure_updated();
	}
}

void
GPlatesCanvasTools::MeasureDistanceState::handle_activation()
{
	d_is_active = true;

	// get and process the current geometry builder
	d_current_geometry_builder_ptr =
		d_geometry_operation_target_ptr->get_and_set_current_geometry_builder_for_newly_activated_tool(
				GPlatesCanvasTools::CanvasToolType::MEASURE_DISTANCE);
	make_signal_slot_connections_for_geometry_builder();
	process_geometry_builder(d_current_geometry_builder_ptr);

	// update all our listeners
	emit_quick_measure_updated();
	emit_feature_measure_updated();
}

void
GPlatesCanvasTools::MeasureDistanceState::emit_feature_measure_updated()
{
	if (d_feature_total_distance)
	{
		emit feature_measure_updated(
				*d_feature_total_distance,
				d_feature_segment_start,
				d_feature_segment_end,
				get_feature_segment_distance());
	}
	else
	{
		emit feature_measure_updated();
	}
}

void
GPlatesCanvasTools::MeasureDistanceState::handle_deactivation()
{
	d_is_active = false;
}

void
GPlatesCanvasTools::MeasureDistanceState::process_geometry_builder(
		const GPlatesViewOperations::GeometryBuilder *geometry_builder)
{
	if (d_is_active)
	{
		if (geometry_builder &&
				geometry_builder->get_num_points_in_current_geometry() &&
					// we treat a geometry builder with fewer than two points as no selection
				(geometry_builder->get_geometry_build_type() == GPlatesViewOperations::GeometryType::POLYLINE ||
				 geometry_builder->get_geometry_build_type() == GPlatesViewOperations::GeometryType::POLYGON))
					// we do not measure distances between a set of points
		{
			if (geometry_builder->get_num_points_in_current_geometry() > 1)
			{
				real_t total_distance = 0.0;
				typedef GPlatesViewOperations::GeometryBuilder::point_const_iterator_type iterator_type;

				GPlatesViewOperations::GeometryBuilder::GeometryIndex geom_index =
					geometry_builder->get_current_geometry_index();
				iterator_type iter = geometry_builder->get_geometry_point_begin(geom_index);
				iterator_type end = geometry_builder->get_geometry_point_end(geom_index);
				iterator_type previous = iter;
				++iter;

				// loop through points
				for (; iter != end; ++iter, ++previous)
				{
					total_distance += calculate_distance_on_surface_of_sphere(
							*previous,
							*iter,
							d_radius);
				}

				// if polygon, add distance between first and last points
				if (geometry_builder->get_geometry_build_type() == GPlatesViewOperations::GeometryType::POLYGON)
				{
					total_distance += calculate_distance_on_surface_of_sphere(
							*previous,
							*(geometry_builder->get_geometry_point_begin(geom_index)),
							d_radius);
				}
				
				d_feature_total_distance = boost::optional<double>(total_distance.dval());
			}
			else
			{
				d_feature_total_distance = boost::optional<double>(0.0);
			}
		}
		else
		{
			d_feature_total_distance = boost::none;
			d_feature_segment_start = boost::none;
			d_feature_segment_end = boost::none;
		}
	}
}

void
GPlatesCanvasTools::MeasureDistanceState::set_radius(
		real_t radius)
{
	if (!GPlatesUtils::are_almost_exactly_equal(radius.dval(), d_radius.dval()))
	{
		d_radius = radius;
		emit_quick_measure_updated();

		// update the total feature distance
		process_geometry_builder(d_current_geometry_builder_ptr);

		emit_feature_measure_updated();
	}
}

void
GPlatesCanvasTools::MeasureDistanceState::set_quick_measure_highlight(
		bool is_highlighted)
{
	if (d_is_quick_measure_highlighted != is_highlighted)
	{
		d_is_quick_measure_highlighted = is_highlighted;
		emit quick_measure_highlight_changed(is_highlighted);
	}
}

void
GPlatesCanvasTools::MeasureDistanceState::set_feature_measure_highlight(
		bool is_highlighted)
{
	if (d_is_feature_measure_highlighted != is_highlighted)
	{
		d_is_feature_measure_highlighted = is_highlighted;
		emit feature_measure_highlight_changed(is_highlighted);
	}
}

