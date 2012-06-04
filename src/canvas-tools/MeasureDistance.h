/* $Id$ */

/**
 * @file 
 * Contains definition of MeasureDistance
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

#ifndef GPLATES_CANVASTOOLS_MEASUREDISTANCE_H
#define GPLATES_CANVASTOOLS_MEASUREDISTANCE_H

#include <boost/scoped_ptr.hpp>
#include <vector>
#include <QString>
#include <QObject>

#include "CanvasTool.h"

#include "view-operations/GeometryBuilder.h"
#include "view-operations/RenderedGeometryCollection.h"


namespace GPlatesGui
{
	class Colour;
}

namespace GPlatesMaths
{
	class PointOnSphere;
}

namespace GPlatesViewOperations
{
	class GeometryBuilder;
	class RenderedGeometryLayer;
}

namespace GPlatesCanvasTools
{
	class GeometryOperationState;
	class MeasureDistanceState;

	/**
	 * Canvas tool used to measuring distances on the globe and map
	 */
	class MeasureDistance :
			public QObject,
			public CanvasTool
	{
		Q_OBJECT

	public:

		/**
		 * Convenience typedef for GPlatesUtils::non_null_intrusive_ptr<MeasureDistance>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<MeasureDistance> non_null_ptr_type;

		static
		const non_null_ptr_type
		create(
				const status_bar_callback_type &status_bar_callback,
				GPlatesViewOperations::GeometryBuilder &geometry_builder,
				GeometryOperationState &geometry_operation_state,
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
				GPlatesViewOperations::RenderedGeometryCollection::MainLayerType main_rendered_layer_type,
				GPlatesCanvasTools::MeasureDistanceState &measure_distance_state)
		{
			return new MeasureDistance(
					status_bar_callback,
					geometry_builder,
					geometry_operation_state,
					rendered_geom_collection,
					main_rendered_layer_type,
					measure_distance_state);
		}

		virtual
		void
		handle_activation();


		virtual
		void
		handle_deactivation();

		virtual
		void
		handle_left_click(
				const GPlatesMaths::PointOnSphere &point_on_sphere,
				bool is_on_earth,
				double proximity_inclusion_threshold);

		virtual
		void
		handle_move_without_drag(	
				const GPlatesMaths::PointOnSphere &point_on_sphere,
				bool is_on_earth,
				double proximity_inclusion_threshold);
		
	private:

		//! Convenience typedef for GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type
		typedef GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type child_layer_ptr_type;


		MeasureDistance(
				const status_bar_callback_type &status_bar_callback,
				GPlatesViewOperations::GeometryBuilder &geometry_builder,
				GeometryOperationState &geometry_operation_state,
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
				GPlatesViewOperations::RenderedGeometryCollection::MainLayerType main_rendered_layer_type,
				GPlatesCanvasTools::MeasureDistanceState &measure_distance_state);

		/**
		 * The geometry builder (either digitised geometry or focused feature geometry) to measure.
		 */
		GPlatesViewOperations::GeometryBuilder &d_geometry_builder;

		/**
		 * Lets others know which geometry builder we are targeting.
		 */
		GeometryOperationState &d_geometry_operation_state;

		//! For rendering purposes
		GPlatesViewOperations::RenderedGeometryCollection *d_rendered_geom_collection_ptr;

		//! A pointer to the state of the measure distance tool
		GPlatesCanvasTools::MeasureDistanceState *d_measure_distance_state_ptr;

		/**
		 * A mapping from rendered line segment indices to point indices, such that the
		 * i-th element of this vector is the index of the point at the beginning of the
		 * i-th rendered line segment. This is because a line segment is not rendered
		 * between two points if they are too close together.
		 */
		std::vector<GPlatesViewOperations::GeometryBuilder::PointIndex> d_line_to_point_mapping;

		/**
		 * The main rendered layer we're currently rendering into.
		 */
		GPlatesViewOperations::RenderedGeometryCollection::MainLayerType d_main_rendered_layer_type;

		//! Rendered geometry layer for drawing geometry
		child_layer_ptr_type d_geometry_layer_ptr;

		//! Rendered geometry layer for mouse-over highlighting
		child_layer_ptr_type d_highlight_layer_ptr;

		//! Rendered geometry layer for the text label
		child_layer_ptr_type d_label_layer_ptr;

		//! Text of label to display, if any
		boost::optional<QString> d_label_text;

		//! Position of label to display, if any
		boost::optional<GPlatesMaths::PointOnSphere> d_label_position;

		//! Start point of mouse-over highlight, if any
		boost::optional<GPlatesMaths::PointOnSphere> d_highlight_start;
		
		//! End point of mouse-over highlight, if any
		boost::optional<GPlatesMaths::PointOnSphere> d_highlight_end;

		//! The colour in which Quick Measure points and lines are rendered
		static
		const GPlatesGui::Colour
		QUICK_MEASURE_LINE_COLOUR;

		//! The colour in which Feature Measure points and lines are rendered
		static
		const GPlatesGui::Colour
		FEATURE_MEASURE_LINE_COLOUR;

		//! The colour in which to render the mouse-over line highlight
		static
		const GPlatesGui::Colour
		HIGHLIGHT_COLOUR;

		//! The colour in which to render the label
		static
		const GPlatesGui::Colour
		LABEL_COLOUR;

		//! The colour in which to render the shadow under the label
		static
		const GPlatesGui::Colour
		LABEL_SHADOW_COLOUR;

		//! The size of points
		static
		const float
		POINT_SIZE;

		//! The thickness of lines
		static
		const float
		LINE_WIDTH;

		//! Number of decimal places for distance labels
		static
		const int
		LABEL_PRECISION;

		//! Horizontal offset from mouse cursor (pixels)
		static
		const int
		LABEL_X_OFFSET;

		//! Vertial offset from mouse cursor (pixels)
		static
		const int
		LABEL_Y_OFFSET;

		//! Creates signal/slot connections
		void
		make_signal_slot_connections();

		//! Does the drawing for this canvas tool
		void
		paint();

		//! Does drawing for Quick Measure
		void
		paint_quick_measure();

		//! Does drawing for Feature Measure
		void
		paint_feature_measure();

		//! Does drawing for mouse-over highlight
		void
		paint_highlight();

		//! Does drawing of text label if there is currently one
		void
		paint_label();

		//! Places a point into a RenderedGeometryLayer.
		template <typename LayerPointerType>
		void
		render_point_on_sphere(
				const GPlatesMaths::PointOnSphere &point_on_sphere,
				const GPlatesGui::Colour &colour,
				LayerPointerType layer_ptr);

		/**
		 * Places a line into a RenderedGeometryLayer.
		 * Returns true iff line created and added to layer.
		 */
		template <typename LayerPointerType>
		bool
		render_line(
				const GPlatesMaths::PointOnSphere &start,
				const GPlatesMaths::PointOnSphere &end,
				const GPlatesGui::Colour &colour,
				LayerPointerType layer_ptr);

		//! Places multiple line segments into a RenderedGeometryLayer, assumes two or more points.
		template <typename LayerPointerType>
		void
		render_multiple_line_segments(
				GPlatesViewOperations::GeometryBuilder::point_const_iterator_type begin,
				GPlatesViewOperations::GeometryBuilder::point_const_iterator_type end,
				const GPlatesGui::Colour &colour,
				bool is_polygon,
				LayerPointerType layer_ptr);

		//! Adds distance label and mouse-over highlight, and (always) repaints
		void
		add_distance_label_and_highlight(
				double distance,
				const GPlatesMaths::PointOnSphere &label_position,
				const GPlatesMaths::PointOnSphere &highlight_start,
				const GPlatesMaths::PointOnSphere &highlight_end,
				bool is_quick_measure);

		//! Removes distance label and mouse-over highlight, and repaints if necessary
		void
		remove_distance_label_and_highlight();

	public slots:

		void
		feature_changed();

	private slots:
	
		void
		handle_quick_measure_cleared();
	};
}

#endif  // GPLATES_CANVASTOOLS_MEASUREDISTANCE_H
