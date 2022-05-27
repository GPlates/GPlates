/* $Id$ */

/**
 * \file Derived @a CanvasTool to insert vertices into temporary or focused feature geometry.
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

#ifndef GPLATES_CANVASTOOLS_SPLITFEATURE_H
#define GPLATES_CANVASTOOLS_SPLITFEATURE_H

#include <boost/scoped_ptr.hpp>

#include "CanvasTool.h"

#include "model/FeatureHandle.h"
#include "model/ModelInterface.h"

#include "presentation/ViewState.h"

#include "view-operations/RenderedGeometryCollection.h"


namespace GPlatesCanvasTools
{
	class GeometryOperationState;
}

namespace GPlatesGui
{
	class CanvasToolWorkflows;
}

namespace GPlatesQtWidgets
{
	class ViewportWindow;
}

namespace GPlatesViewOperations
{
	class GeometryBuilder;
	class SplitFeatureGeometryOperation;
	class QueryProximityThreshold;
	class RenderedGeometryCollection;
}

namespace GPlatesCanvasTools
{
	/**
	 * This is the canvas tool used to insert vertices into geometry.
	 */
	class SplitFeature :
			public CanvasTool
	{
	public:

		/**
		 * Convenience typedef for GPlatesUtils::non_null_intrusive_ptr<SplitFeature>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<SplitFeature> non_null_ptr_type;

		static
		const non_null_ptr_type
		create(
				const status_bar_callback_type &status_bar_callback,
				GPlatesGui::FeatureFocus &feature_focus,
				GPlatesModel::ModelInterface model_interface,
				GPlatesViewOperations::GeometryBuilder &geometry_builder,
				GPlatesCanvasTools::GeometryOperationState &geometry_operation_state,
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
				GPlatesViewOperations::RenderedGeometryCollection::MainLayerType main_rendered_layer_type,
				GPlatesGui::CanvasToolWorkflows &canvas_tool_workflows,
				const GPlatesViewOperations::QueryProximityThreshold &query_proximity_threshold)
		{
			return new SplitFeature(
					status_bar_callback,
					feature_focus,
					model_interface,
					geometry_builder,
					geometry_operation_state,
					rendered_geometry_collection,
					main_rendered_layer_type,
					canvas_tool_workflows,
					query_proximity_threshold);
		}

		virtual
		~SplitFeature();
		
		virtual
		void
		handle_activation() override;

		virtual
		void
		handle_deactivation() override;

		virtual
		void
		handle_left_click(
				const GPlatesMaths::PointOnSphere &point_on_sphere,
				bool is_on_earth,
				double proximity_inclusion_threshold) override;

		virtual
		void
		handle_left_drag(
				const GPlatesMaths::PointOnSphere &initial_point_on_sphere,
				bool was_on_earth,
				double initial_proximity_inclusion_threshold,
				const GPlatesMaths::PointOnSphere &current_point_on_sphere,
				bool is_on_earth,
				double current_proximity_inclusion_threshold,
				const GPlatesMaths::PointOnSphere &centre_of_viewport) override;

		virtual
		void
		handle_move_without_drag(
				const GPlatesMaths::PointOnSphere &point_on_sphere,
				bool is_on_earth,
				double proximity_inclusion_threshold) override;

	private:

		/**
		 * Create a InsertVertex instance.
		 */
		SplitFeature(
				const status_bar_callback_type &status_bar_callback,
				GPlatesGui::FeatureFocus &feature_focus,
				GPlatesModel::ModelInterface model_interface,
				GPlatesViewOperations::GeometryBuilder &geometry_builder,
				GPlatesCanvasTools::GeometryOperationState &geometry_operation_state,
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
				GPlatesViewOperations::RenderedGeometryCollection::MainLayerType main_rendered_layer_type,
				GPlatesGui::CanvasToolWorkflows &canvas_tool_workflows,
				const GPlatesViewOperations::QueryProximityThreshold &query_proximity_threshold);

		/**
		 * Digitise operation for inserting a vertex into digitised or focused feature geometry.
		 */
		boost::scoped_ptr<GPlatesViewOperations::SplitFeatureGeometryOperation> d_split_feature_geometry_operation;
	};
}

#endif // GPLATES_CANVASTOOLS_SPLITFEATURE_H
