/* $Id$ */

/**
 * @file 
 * Contains the definition of the MeasureDistanceState class.
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

#ifndef GPLATES_CANVASTOOLS_MEASUREDISTANCESTATE_H
#define GPLATES_CANVASTOOLS_MEASUREDISTANCESTATE_H

#include <QObject>
#include <boost/optional.hpp>

#include "maths/PointOnSphere.h"
#include "maths/Real.h"
#include "model/FeatureHandle.h"
#include "model/ReconstructionGeometry.h"
#include "view-operations/RenderedGeometryCollection.h"

namespace GPlatesViewOperations
{
	class GeometryBuilder;
	class GeometryOperationTarget;
	class RenderedGeometryLayer;
}

namespace GPlatesCanvasTools
{

	/**
	 * Stores the state for the distance measuring tool, shared between globe and map
	 */
	class MeasureDistanceState :
		public QObject
	{

		Q_OBJECT
	
	private:

		typedef GPlatesMaths::Real real_t;
		typedef GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type child_layer_ptr_type;

	public:

		//! Constructor
		explicit
		MeasureDistanceState(
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
				GPlatesViewOperations::GeometryOperationTarget &geometry_operation_target);
		
		//! Destructor
		~MeasureDistanceState()
		{
		}

		//! Add a new point for the Quick Measure tool
		void
		quick_measure_add_point(
				const GPlatesMaths::PointOnSphere &point);

		//! Get the first Quick Measure point, if any
		const boost::optional<GPlatesMaths::PointOnSphere> &
		get_quick_measure_start()
		{
			return d_quick_measure_start;
		}

		//! Get the second Quick Measure point, if any
		const boost::optional<GPlatesMaths::PointOnSphere> &
		get_quick_measure_end()
		{
			return d_quick_measure_end;
		}
		
		//! Get the distance between the two Quick Measure points, if there are two such points
		boost::optional<double>
		get_quick_measure_distance();

		//! Set the start and end points for the Feature Measure tool
		void
		set_feature_segment_points(
				const boost::optional<GPlatesMaths::PointOnSphere> &start,
				const boost::optional<GPlatesMaths::PointOnSphere> &end);

		//! Get the first Feature Measure segment point, if any
		const boost::optional<GPlatesMaths::PointOnSphere> &
		get_feature_segment_start()
		{
			return d_feature_segment_start;
		}

		//! Get the second Feature Measure segment point, if any
		const boost::optional<GPlatesMaths::PointOnSphere> &
		get_feature_segment_end()
		{
			return d_feature_segment_end;
		}
		
		//! Get the distance between the two Feature Measure points, if there are two such points
		boost::optional<double>
		get_feature_segment_distance();

		/**
		 * Sets the radius of the earth and emits updated()
		 * if the new radius is different from the old radius
		 */
		void
		set_radius(real_t radius);

		//! Gets the radius of the earth used by the measure distance tool
		real_t
		get_radius()
		{
			return d_radius;
		}
		
		//! Call this when the Measure Distance tool is activated
		void
		handle_activation();

		//! Call this when the Measure Distance tool is deactivated
		void
		handle_deactivation();

		//! Returns whether the Measure Distance canvas tool is active or not
		bool
		is_active()
		{
			return d_is_active;
		}

		//! Returns pointer to current geometry builder; NULL if none
		GPlatesViewOperations::GeometryBuilder *
		get_current_geometry_builder_ptr()
		{
			return d_current_geometry_builder_ptr;
		}

		GPlatesViewOperations::RenderedGeometryLayer *
		get_main_layer_ptr()
		{
			return d_main_layer_ptr;
		}

		child_layer_ptr_type
		get_highlight_layer_ptr()
		{
			return d_highlight_layer_ptr;
		}

		child_layer_ptr_type
		get_label_layer_ptr()
		{
			return d_label_layer_ptr;
		}

		void
		set_quick_measure_highlight(
				bool is_highlighted);

		void
		set_feature_measure_highlight(
				bool is_highlighted);

	private:
		
		//! Main layer on which to render lines
		GPlatesViewOperations::RenderedGeometryLayer *d_main_layer_ptr;

		//! Layer for mouse-over line highlight
		child_layer_ptr_type d_highlight_layer_ptr;

		//! Layer for mouse-over label
		child_layer_ptr_type d_label_layer_ptr;

		//! The radius of the earth in kilometres
		real_t d_radius;

		//! Quick measure tool start point
		boost::optional<GPlatesMaths::PointOnSphere> d_quick_measure_start;

		//! Quick measure tool end point
		boost::optional<GPlatesMaths::PointOnSphere> d_quick_measure_end;

		//! Determines which GeometryBuilder to get points from
		GPlatesViewOperations::GeometryOperationTarget *d_geometry_operation_target_ptr;

		//! The current geometry builder as returned by GeometryOperationTarget
		GPlatesViewOperations::GeometryBuilder *d_current_geometry_builder_ptr;

		//! The calculated total distance for Feature Measure tool; boost::none if no feature
		boost::optional<double> d_feature_total_distance;

		//! The start point of the feature segment that is highlighted
		boost::optional<GPlatesMaths::PointOnSphere> d_feature_segment_start;

		//! The end point of the feature segment that is highlighted
		boost::optional<GPlatesMaths::PointOnSphere> d_feature_segment_end;

		//! Whether the Measure Distance canvas tool is currently active
		bool d_is_active;

		//! Whether the Quick Measure distance field in the TaskPanel is highlighted
		bool d_is_quick_measure_highlighted;

		//! Whether the Feature Measure segment distance field in the TaskPanel is highlighted
		bool d_is_feature_measure_highlighted;

		//! The default radius value
		static
		const double
		DEFAULT_RADIUS_OF_EARTH;

		void
		make_signal_slot_connections_for_geometry_operation_target();

		void
		make_signal_slot_connections_for_geometry_builder();
		
		void
		disconnect_signal_slot_connections_for_geometry_builder();

		void
		emit_quick_measure_updated();

		void
		emit_feature_measure_updated();

		void
		process_geometry_builder(
				const GPlatesViewOperations::GeometryBuilder *geometry_builder);

	private slots:

		void
		switch_geometry_builder(
				GPlatesViewOperations::GeometryOperationTarget &,
				GPlatesViewOperations::GeometryBuilder *);

		void
		reexamine_geometry_builder();

	signals:
		
		//! Emitted when the Quick Measure state is changed
		void
		quick_measure_updated(
				boost::optional<GPlatesMaths::PointOnSphere> start,
				boost::optional<GPlatesMaths::PointOnSphere> end,
				boost::optional<double> distance);

		//! Emitted when New/Selected Measure state is changed (and there is a feature)
		void
		feature_measure_updated(
				double total_distance,
				boost::optional<GPlatesMaths::PointOnSphere> segment_start,
				boost::optional<GPlatesMaths::PointOnSphere> segment_end,
				boost::optional<double> segment_distance);

		//! Emitted when New/Selected Measure state is changed (and there is NO feature)
		void
		feature_measure_updated();

		//! Emitted when the canvas tool needs to redraw the displayed feature
		void
		feature_in_geometry_builder_changed();

		//! Emitted when the Quick Measure distance field highlight is changed
		void
		quick_measure_highlight_changed(
				bool is_highlighted);

		//! Emitted when the Feature Measure segment distance field highlight is changed
		void
		feature_measure_highlight_changed(
				bool is_highlighted);
	};

}

#endif  // GPLATES_CANVASTOOLS_MEASUREDISTANCESTATE_H
