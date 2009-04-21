/* $Id$ */

/**
 * \file 
 * Adds points to a digitised geometry as the user clicks a position on the globe.
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

#ifndef GPLATES_VIEWOPERATIONS_ADDPOINTGEOMETRYOPERATION_H
#define GPLATES_VIEWOPERATIONS_ADDPOINTGEOMETRYOPERATION_H

#include <boost/noncopyable.hpp>
#include <QObject>

#include "GeometryBuilder.h"
#include "GeometryOperation.h"
#include "GeometryType.h"
#include "RenderedGeometryCollection.h"


namespace GPlatesMaths
{
	class PointOnSphere;
}

namespace GPlatesGui
{
	class ChooseCanvasTool;
}

namespace GPlatesViewOperations
{
	class ActiveGeometryOperation;
	class GeometryBuilder;
	class GeometryOperationTarget;
	class QueryProximityThreshold;
	class RenderedGeometryCollection;
	class RenderedGeometryLayer;

	/**
	 * Adds a point to @a GeometryBuilder and adds @a RenderedGeometry objects
	 * to @a RenderedGeometryCollection.
	 */
	class AddPointGeometryOperation :
		public GeometryOperation,
		private boost::noncopyable
	{
		Q_OBJECT

	public:
		AddPointGeometryOperation(
				GeometryType::Value build_geom_type,
				GeometryOperationTarget &geometry_operation_target,
				ActiveGeometryOperation &active_geometry_operation,
				RenderedGeometryCollection *rendered_geometry_collection,
				GPlatesGui::ChooseCanvasTool &choose_canvas_tool,
				const QueryProximityThreshold &query_proximity_threshold);

		/**
		 * Activate this operation and attach to specified @a GeometryBuilder
		 * and render into specified main rendered layer.
		 */
		virtual
		void
		activate(
				GeometryBuilder *,
				RenderedGeometryCollection::MainLayerType main_layer_type);

		//! Deactivate this operation.
		virtual
		void
		deactivate();

		/**
		 * Add a point to the curent geometry builder at the specified position on sphere.
		 * This can fail to add a point if it is too close to an existing point.
		 */
		void
		add_point(
				const GPlatesMaths::PointOnSphere &clicked_pos_on_sphere,
				const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere);

	public slots:
		// NOTE: all signals/slots should use namespace scope for all arguments
		//       otherwise differences between signals and slots will cause Qt
		//       to not be able to connect them at runtime.

		void
		geometry_builder_stopped_updating_geometry();

	private:
		/**
		 * The type of geometry we are attempting to build.
		 */
		GeometryType::Value d_build_geom_type;

		/**
		 * Used by undo/redo.
		 */
		GeometryOperationTarget *d_geometry_operation_target;

		/**
		 * We call this when we activate/deactivate.
		 */
		ActiveGeometryOperation *d_active_geometry_operation;

		/**
		 * This is used to build geometry. We add points to it.
		 */
		GeometryBuilder *d_geometry_builder;

		/**
		 * This is where we render our geometries and activate our render layer.
		 */
		RenderedGeometryCollection *d_rendered_geometry_collection;

		/**
		 * The main rendered layer we're currently rendering into.
		 */
		RenderedGeometryCollection::MainLayerType d_main_layer_type;

		/**
		 * Rendered geometry layer used for lines.
		 */
		RenderedGeometryCollection::child_layer_owner_ptr_type d_lines_layer_ptr;

		/**
		 * Rendered geometry layer used for points.
		 */
		RenderedGeometryCollection::child_layer_owner_ptr_type d_points_layer_ptr;

		/**
		 * Used by undo/redo to make sure appropriate tool is active
		 * when the undo/redo happens.
		 */
		GPlatesGui::ChooseCanvasTool *d_choose_canvas_tool;

		/**
		 * Used to query the proximity threshold based on position on globe.
		 */
		const QueryProximityThreshold *d_query_proximity_threshold;

		/**
		 * Returns true if specified point is too close to existing points.
		 */
		bool
		too_close_to_existing_points(
				const GPlatesMaths::PointOnSphere &clicked_pos_on_sphere,
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
		update_rendered_point_on_sphere(
				GeometryBuilder::GeometryIndex geom_index);

		void
		update_rendered_multipoint_on_sphere(
				GeometryBuilder::GeometryIndex geom_index);

		void
		update_rendered_polyline_on_sphere(
				GeometryBuilder::GeometryIndex geom_index);

		void
		update_rendered_polygon_on_sphere(
				GeometryBuilder::GeometryIndex geom_index);
	};
}

#endif // GPLATES_VIEWOPERATIONS_ADDPOINTGEOMETRYOPERATION_H
