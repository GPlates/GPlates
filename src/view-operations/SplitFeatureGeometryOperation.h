/* $Id$ */

/**
 * \file 
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

#ifndef GPLATES_VIEWOPERATIONS_SPLITFEATUREGEOMETRYOPERATION_H
#define GPLATES_VIEWOPERATIONS_SPLITFEATUREGEOMETRYOPERATION_H

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <QObject>
#include <vector>

#include "GeometryBuilder.h"
#include "GeometryOperation.h"
#include "RenderedGeometryCollection.h"
#include "UndoRedo.h"

#include "gui/FeatureFocus.h"

#include "model/ModelInterface.h"

#include "presentation/ViewState.h"


namespace GPlatesCanvasTools
{
	class GeometryOperationState;
}

namespace GPlatesMaths
{
	class PointOnSphere;
}

namespace GPlatesModel
{
	class Gpgim;
}

namespace GPlatesGui
{
	class CanvasToolWorkflows;
}

namespace GPlatesViewOperations
{
	class GeometryBuilder;
	class QueryProximityThreshold;
	class RenderedGeometryLayer;
	struct RenderedGeometryProximityHit;

	/**
	 * 
	 */
	class SplitFeatureGeometryOperation :
		public GeometryOperation,
		private boost::noncopyable
	{
		Q_OBJECT

	public:
		SplitFeatureGeometryOperation(
				GPlatesGui::FeatureFocus &feature_focus,
				const GPlatesModel::Gpgim &gpgim,
				GPlatesModel::ModelInterface model_interface,
				GeometryBuilder &geometry_builder,
				GPlatesCanvasTools::GeometryOperationState &geometry_operation_state,
				RenderedGeometryCollection &rendered_geometry_collection,
				RenderedGeometryCollection::MainLayerType main_rendered_layer_type,
				GPlatesGui::CanvasToolWorkflows &canvas_tool_workflows,
				const QueryProximityThreshold &query_proximity_threshold);

		/**
		 * Activate this operation.
		 */
		virtual
		void
		activate();

		//! Deactivate this operation.
		virtual
		void
		deactivate();

		//! User has just clicked on the sphere.
		void
		left_click(
			const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere,
			const double &closeness_inclusion_threshold);

		//! The mouse has moved but it is not a drag because mouse button is not pressed.
		void
		mouse_move(
				const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere,
				const double &closeness_inclusion_threshold);

	public Q_SLOTS:
		// NOTE: all signals/slots should use namespace scope for all arguments
		//       otherwise differences between signals and slots will cause Qt
		//       to not be able to connect them at runtime.

		void
		geometry_builder_stopped_updating_geometry();

	private:

		GPlatesGui::FeatureFocus &d_feature_focus;

		const GPlatesModel::Gpgim &d_gpgim;

		GPlatesModel::ModelInterface d_model_interface;

		/**
		 * This is used to build geometry. We delete vertices with it.
		 */
		GeometryBuilder &d_geometry_builder;

		/**
		 * We call this when we activate/deactivate.
		 */
		GPlatesCanvasTools::GeometryOperationState &d_geometry_operation_state;

		/**
		 * This is where we render our geometries and activate our render layer.
		 */
		RenderedGeometryCollection &d_rendered_geometry_collection;

		/**
		 * The main rendered layer we're currently rendering into.
		 */
		RenderedGeometryCollection::MainLayerType d_main_rendered_layer_type;

		/**
		 * Rendered geometry layer used for line segments.
		 */
		RenderedGeometryCollection::child_layer_owner_ptr_type d_line_segments_layer_ptr;

		/**
		 * Rendered geometry layer used for points.
		 */
		RenderedGeometryCollection::child_layer_owner_ptr_type d_points_layer_ptr;

		/**
		 * Rendered geometry layer used for the single highlighted point (the point
		 * that the mouse cursor is currently hovering over if any).
		 */
		RenderedGeometryCollection::child_layer_owner_ptr_type d_highlight_layer_ptr;

		/**
		 * A mapping from rendered line segment indices to point indices, such that the
		 * i-th element of this vector is the index of the point at the beginning of the
		 * i-th rendered line segment. This is because a line segment is not rendered
		 * between two points if they are too close together.
		 */
		std::vector<GeometryBuilder::PointIndex> d_line_to_point_mapping;

		/**
		 * Used by undo/redo to make sure appropriate tool is active
		 * when the undo/redo happens.
		 */
		GPlatesGui::CanvasToolWorkflows &d_canvas_tool_workflows;

		/**
		 * Used to query the proximity threshold based on position on globe.
		 */
		const QueryProximityThreshold &d_query_proximity_threshold;

		/**
		* .
		*/
		void
		split_feature(
				const unsigned int line_segment_index,
				const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere,
				const double &closeness_inclusion_threshold);

		
		/**
		 * .
		 */
		void 
		split_feature(
				const GeometryBuilder::PointIndex insert_vertex_index,
				boost::optional<const GPlatesMaths::PointOnSphere> insert_pos_on_sphere);

		/**
		 * Projects the specified point onto the specified line segment.
		 * Note: specified point must satisfy proximity test with
		 * specified line segment and must fail proximity test with the
		 * line segment's end points - this ensures we can successfully
		 * perform the projection.
		 */
		GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type
		project_point_onto_line_segment(
				const unsigned int line_segment_index,
				const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere);

		/**
		* Tests proximity of specified point to the rendered geometries
		* in the specified rendered geometry layer.
		* Returns the closest rendered geometry if any otherwise returns false.
		*/
		boost::optional<GPlatesViewOperations::RenderedGeometryProximityHit>
			test_proximity_to_rendered_geom_layer(
			const RenderedGeometryLayer &rendered_geom_layer,
			const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere,
			const double &closeness_inclusion_threshold);

		/**
		 * Returns point index of closest point (in geometry contained in our
		 * geometry builder) to the specified point.
		 * If there are no points in the geometry then returns false.
		 */
		boost::optional<const GeometryBuilder::PointIndex>
		get_closest_geometry_point_to(
				const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere);

		void
		connect_to_geometry_builder_signals();

		void
		disconnect_from_geometry_builder_signals();

		void
		create_rendered_geometry_layers();

		/**
		 * Update all @a RenderedGeometry objects.
		 */
		void
		update_rendered_geometries();

		void
		update_rendered_geometry(
				GeometryBuilder::GeometryIndex geom_index);

		void
		update_rendered_polyline_on_sphere(
				GeometryBuilder::GeometryIndex geom_index);

		void
		update_rendered_polygon_on_sphere(
				GeometryBuilder::GeometryIndex geom_index);

		void
		add_rendered_points(
				GeometryBuilder::GeometryIndex geom_index);

		void
		add_rendered_lines(
				GeometryBuilder::GeometryIndex geom_index,
				const GPlatesMaths::GeometryType::Value actual_geom_type);

		void
		update_highlight_rendered_layer(
			const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere,
			const double &closeness_inclusion_threshold);

		void
			add_rendered_highlight_on_line_segment(
			const unsigned int line_segment_index,
			const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere,
			const double &closeness_inclusion_threshold);

		void
		add_rendered_highlight_off_line_segment(
				const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere);
		void
		add_rendered_highlight_line_segment(
				const unsigned int highlight_line_segment_index);

		void
		add_rendered_highlight_line_segment(
				const GPlatesMaths::PointOnSphere &start_point,
				const GPlatesMaths::PointOnSphere &end_point);

		template <typename ForwardIterPointOnSphere>
		void
		add_rendered_highlight_line_segment(
				ForwardIterPointOnSphere begin_point_on_sphere,
				ForwardIterPointOnSphere end_point_on_sphere);
	};
}

#endif // GPLATES_VIEWOPERATIONS_SPLITFEATUREGEOMETRYOPERATION_H
