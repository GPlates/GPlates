/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2007, 2008 The University of Sydney, Australia
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

#ifndef GPLATES_CANVASTOOLS_CLICKGEOMETRY_H
#define GPLATES_CANVASTOOLS_CLICKGEOMETRY_H

#include <vector>

#include "CanvasTool.h"

#include "app-logic/ReconstructionGeometry.h"

#include "gui/AddClickedGeometriesToFeatureTable.h"
#include "gui/FeatureFocus.h"
#include "gui/FeatureTableModel.h"

#include "view-operations/RenderedGeometryCollection.h"


namespace GPlatesAppLogic
{
	class ApplicationState;
	class ReconstructGraph;
}

namespace GPlatesQtWidgets
{
	class GlobeCanvas;
	class ViewportWindow;
	class FeaturePropertiesDialog;
}

namespace GPlatesViewOperations
{
	class GeometryBuilder;
}

namespace GPlatesCanvasTools
{
	/**
	 * This is the canvas tool used to focus features by clicking on them.
	 */
	class ClickGeometry :
			public CanvasTool
	{
	public:

		/**
		 * Convenience typedef for GPlatesUtils::non_null_intrusive_ptr<ClickGeometry>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<ClickGeometry> non_null_ptr_type;

		static
		const non_null_ptr_type
		create(
				const status_bar_callback_type &status_bar_callback,
				GPlatesViewOperations::GeometryBuilder &focused_feature_geometry_builder,
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
				GPlatesViewOperations::RenderedGeometryCollection::MainLayerType main_rendered_layer_type,
				GPlatesQtWidgets::ViewportWindow &view_state,
				GPlatesGui::FeatureTableModel &clicked_table_model,
				GPlatesQtWidgets::FeaturePropertiesDialog &fp_dialog,
				GPlatesGui::FeatureFocus &feature_focus,
				GPlatesAppLogic::ApplicationState &application_state)
		{
			return new ClickGeometry(
					status_bar_callback,
					focused_feature_geometry_builder,
					rendered_geom_collection,
					main_rendered_layer_type,
					view_state,
					clicked_table_model,
					fp_dialog,
					feature_focus,
					application_state);
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
		handle_shift_left_click(
				const GPlatesMaths::PointOnSphere &point_on_sphere,
				bool is_on_earth,
				double proximity_inclusion_threshold);


		/**
		 * Returns the sequence of geometries last clicked by the user (if any).
		 */
		const std::vector<GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type> &
		get_clicked_geom_seq() const
		{
			return d_clicked_geom_seq;
		}

	private:

		/**
		 * Create a ClickGeometry instance.
		 */
		ClickGeometry(
				const status_bar_callback_type &status_bar_callback,
				GPlatesViewOperations::GeometryBuilder &focused_feature_geometry_builder,
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
				GPlatesViewOperations::RenderedGeometryCollection::MainLayerType main_rendered_layer_type,
				GPlatesQtWidgets::ViewportWindow &view_state,
				GPlatesGui::FeatureTableModel &clicked_table_model,
				GPlatesQtWidgets::FeaturePropertiesDialog &fp_dialog,
				GPlatesGui::FeatureFocus &feature_focus,
				GPlatesAppLogic::ApplicationState &application_state);

		/**
		 * The focused feature geometry builder.
		 */
		GPlatesViewOperations::GeometryBuilder &d_focused_feature_geometry_builder;

		/**
		 * We need to change which canvas-tool layer is shown when this canvas-tool is
		 * activated.
		 */
		GPlatesViewOperations::RenderedGeometryCollection &d_rendered_geom_collection;

		/**
		 * The main rendered layer we're currently rendering into.
		 */
		GPlatesViewOperations::RenderedGeometryCollection::MainLayerType d_main_rendered_layer_type;

		/**
		 * This is the view state which is used to obtain the reconstruction root.
		 *
		 * Since the view state is also the ViewportWindow, it is currently used to
		 * pass messages to the status bar.
		 */
		GPlatesQtWidgets::ViewportWindow &d_view_state_ptr;

		/**
		 * This is the external table of hits which will be updated in the event that
		 * the test point hits one or more geometries.
		 */
		GPlatesGui::FeatureTableModel &d_clicked_table_model_ptr;

		/**
		 * This is the dialog box which we will be populating in response to a feature
		 * query.
		 */
		GPlatesQtWidgets::FeaturePropertiesDialog &d_fp_dialog_ptr;

		/**
		 * This is our reference to the Feature Focus, which we use to let the rest of the
		 * application know what the user just clicked on.
		 */
		GPlatesGui::FeatureFocus &d_feature_focus_ptr;

		/**
		 * Used when adding reconstruction geometries to the clicked feature table.
		 */
		const GPlatesAppLogic::ReconstructGraph &d_reconstruct_graph;

		/**
		 * Used to filter clicked geometries before adding to the feature table.
		 */
		GPlatesGui::filter_reconstruction_geometry_predicate_type d_filter_reconstruction_geometry_predicate;

		/**
		 * The sequence of clicked geometries from the last user click.
		 */
		std::vector<GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type> d_clicked_geom_seq;

		/**
		 * The focused feature (if any) from the last user click.
		 */
		GPlatesModel::FeatureHandle::weak_ref d_save_restore_focused_feature;

		/**
		 * The focused feature geometry property (if any) to restore when this canvas tool workflow re-activates.
		 */
		GPlatesModel::FeatureHandle::iterator d_save_restore_focused_feature_geometry_property;


		GPlatesQtWidgets::FeaturePropertiesDialog &
		fp_dialog() const
		{
			return d_fp_dialog_ptr;
		}

	};
}

#endif  // GPLATES_CANVASTOOLS_CLICKGEOMETRY_H
