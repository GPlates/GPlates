/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#ifndef GPLATES_CANVASTOOLS_MOVEVERTEX_H
#define GPLATES_CANVASTOOLS_MOVEVERTEX_H

#include <boost/scoped_ptr.hpp>

#include "CanvasTool.h"

#include "model/FeatureHandle.h"

#include "view-operations/RenderedGeometryCollection.h"


namespace GPlatesCanvasTools
{
	class GeometryOperationState;
	class ModifyGeometryState;
}

namespace GPlatesGui
{
	class CanvasToolWorkflows;
	class FeatureFocus;
}

namespace GPlatesViewOperations
{
	class GeometryBuilder;
	class MoveVertexGeometryOperation;
	class QueryProximityThreshold;
}

namespace GPlatesCanvasTools
{
	/**
	 * This is the canvas tool used to move individual vertices of geometry.
	 */
	class MoveVertex :
			public CanvasTool
	{
	public:

		/**
		 * Convenience typedef for GPlatesUtils::non_null_intrusive_ptr<MoveVertex>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<MoveVertex> non_null_ptr_type;

		static
		const non_null_ptr_type
		create(
				const status_bar_callback_type &status_bar_callback,
				GPlatesViewOperations::GeometryBuilder &geometry_builder,
				GPlatesCanvasTools::GeometryOperationState &geometry_operation_state,
				GPlatesCanvasTools::ModifyGeometryState &modify_geometry_state,
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
				GPlatesViewOperations::RenderedGeometryCollection::MainLayerType main_rendered_layer_type,
				GPlatesGui::CanvasToolWorkflows &canvas_tool_workflows,
				const GPlatesViewOperations::QueryProximityThreshold &query_proximity_threshold,
				GPlatesGui::FeatureFocus &feature_focus)
		{
			return new MoveVertex(
					status_bar_callback,
					geometry_builder,
					geometry_operation_state,
					modify_geometry_state,
					rendered_geometry_collection,
					main_rendered_layer_type,
					canvas_tool_workflows,
					query_proximity_threshold,
					feature_focus);
		}

		virtual
		~MoveVertex();
		
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
		handle_left_press(
				const GPlatesMaths::PointOnSphere &point_on_sphere,
				bool is_on_earth,
				double proximity_inclusion_threshold);

		virtual
		void
		handle_left_drag(
				const GPlatesMaths::PointOnSphere &initial_point_on_sphere,
				bool was_on_earth,
				double initial_proximity_inclusion_threshold,
				const GPlatesMaths::PointOnSphere &current_point_on_sphere,
				bool is_on_earth,
				double current_proximity_inclusion_threshold,
				const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport);

		virtual
		void
		handle_left_release_after_drag(
				const GPlatesMaths::PointOnSphere &initial_point_on_sphere,
				bool was_on_earth,
				double initial_proximity_inclusion_threshold,
				const GPlatesMaths::PointOnSphere &current_point_on_sphere,
				bool is_on_earth,
				double current_proximity_inclusion_threshold,
				const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport);
				
		void
		handle_move_without_drag(
				const GPlatesMaths::PointOnSphere &point_on_sphere,
				bool is_on_earth,
				double proximity_inclusion_threshold);

	private:

		/**
		 * Create a MoveVertex instance.
		 */
		MoveVertex(
				const status_bar_callback_type &status_bar_callback,
				GPlatesViewOperations::GeometryBuilder &geometry_builder,
				GPlatesCanvasTools::GeometryOperationState &geometry_operation_state,
				GPlatesCanvasTools::ModifyGeometryState &modify_geometry_state,
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
				GPlatesViewOperations::RenderedGeometryCollection::MainLayerType main_rendered_layer_type,
				GPlatesGui::CanvasToolWorkflows &canvas_tool_workflows,
				const GPlatesViewOperations::QueryProximityThreshold &query_proximity_threshold,
				GPlatesGui::FeatureFocus &feature_focus);

		/**
		 * Digitise operation for moving a vertex in digitised geometry.
		 */
		boost::scoped_ptr<GPlatesViewOperations::MoveVertexGeometryOperation> d_move_vertex_geometry_operation;

		/**
		 * Whether or not this tool is currently in the midst of a drag.
		 */
		bool d_is_in_drag;

		void
		handle_left_drag(
				bool &is_in_drag,
				GPlatesViewOperations::MoveVertexGeometryOperation *move_vertex_geometry_operation,
				const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
				const double &closeness_inclusion_threshold,
				const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe);
	};
}

#endif  // GPLATES_CANVASTOOLS_MOVEVERTEX_H
