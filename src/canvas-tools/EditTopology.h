/**
 * \file 
 * $Revision: 4572 $
 * $Date: 2009-01-13 01:17:30 -0800 (Tue, 13 Jan 2009) $ 
 * 
 * Copyright (C) 2008, 2010 The University of Sydney, Australia
 * Copyright (C) 2008, 2009 California Institute of Technology 
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

#ifndef GPLATES_CANVASTOOLS_EDIT_TOPOLOGY_H
#define GPLATES_CANVASTOOLS_EDIT_TOPOLOGY_H

#include <QObject>

#include "CanvasTool.h"

#include "gui/FeatureFocus.h"
#include "gui/FeatureTableModel.h"
#include "gui/TopologySectionsContainer.h"
#include "gui/TopologyTools.h"


namespace GPlatesAppLogic
{
	class ApplicationState;
	class ReconstructGraph;
}

namespace GPlatesQtWidgets
{
	class GlobeCanvas;
	class ViewportWindow;
	class TopologyToolsWidget;
}

namespace GPlatesViewOperations
{
	class RenderedGeometryCollection;
}

namespace GPlatesCanvasTools
{
	/**
	 * This is the canvas tool used to define new geometry.
	 */
	class EditTopology :
			public QObject,
			public CanvasTool
	{
		Q_OBJECT

	public:

		/**
		 * Convenience typedef for GPlatesUtils::non_null_intrusive_ptr<EditTopology>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<EditTopology> non_null_ptr_type;

		static
		const non_null_ptr_type
		create(
				const status_bar_callback_type &status_bar_callback,
				GPlatesPresentation::ViewState &view_state,
				GPlatesQtWidgets::ViewportWindow &viewport_window,
				GPlatesGui::FeatureTableModel &clicked_table_model_,	
				GPlatesGui::TopologySectionsContainer &topology_sections_container,
				GPlatesQtWidgets::TopologyToolsWidget &topology_tools_widget,
				GPlatesAppLogic::ApplicationState &application_state)
		{
			return new EditTopology(
					status_bar_callback,
					view_state,
					viewport_window,
					clicked_table_model_,
					topology_sections_container,
					topology_tools_widget,
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

	private:

		EditTopology(
				const status_bar_callback_type &status_bar_callback,
				GPlatesPresentation::ViewState &view_state,
				GPlatesQtWidgets::ViewportWindow &viewport_window,
				GPlatesGui::FeatureTableModel &clicked_table_model_,	
				GPlatesGui::TopologySectionsContainer &topology_sections_container,
				GPlatesQtWidgets::TopologyToolsWidget &topology_tools_widget,
				GPlatesAppLogic::ApplicationState &application_state);

		/**
		 * We need to change which canvas-tool layer is shown when this canvas-tool is
		 * activated.
		 */
		GPlatesViewOperations::RenderedGeometryCollection *d_rendered_geom_collection;

		/**
		 *
		 * This is currently used to pass messages to the status bar.
		 */
		GPlatesQtWidgets::ViewportWindow *d_viewport_window_ptr;

		/**
		 * This is the external table of hits which will be updated in the event that
		 * the test point hits one or more geometries.
		 */
		GPlatesGui::FeatureTableModel *d_clicked_table_model_ptr;

		/**
		 * This is the external table of selected features for the closed boundary
		 */
		GPlatesGui::TopologySectionsContainer *d_topology_sections_container_ptr;

		/**
		 * This is the TopologyToolsWidget in the Task Panel.
		 */
		GPlatesQtWidgets::TopologyToolsWidget *d_topology_tools_widget_ptr;

		
		/**
		 * This is our reference to the Feature Focus, which we use to let the rest of the
		 * application know what the user just clicked on.
		 */
		GPlatesGui::FeatureFocus *d_feature_focus_ptr;

		/**
		 * Used when adding reconstruction geometries to the clicked feature table.
		 */
		const GPlatesAppLogic::ReconstructGraph &d_reconstruct_graph;

	};
}

#endif  // GPLATES_CANVASTOOLS_EDIT_TOPOLOGY_H
