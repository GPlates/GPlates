/**
 * \file 
 * $Revision: 4572 $
 * $Date: 2009-01-13 01:17:30 -0800 (Tue, 13 Jan 2009) $ 
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#ifndef GPLATES_CANVASTOOLS_BUILD_TOPOLOGY_H
#define GPLATES_CANVASTOOLS_BUILD_TOPOLOGY_H

#include <QObject>

#include "gui/FeatureFocus.h"
#include "gui/GlobeCanvasTool.h"
#include "gui/FeatureTableModel.h"
#include "gui/TopologySectionsContainer.h"
#include "gui/TopologyTools.h"


namespace GPlatesAppLogic
{
	class Reconstruct;
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
	class BuildTopology:
			public QObject,
			public GPlatesGui::GlobeCanvasTool
	{
		Q_OBJECT

	public:

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<BuildTopology,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<BuildTopology,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;

		virtual
		~BuildTopology()
		{  }

		/**
		 * Create a BuildTopology instance.
		 */
		static
		const non_null_ptr_type
		create(
				GPlatesGui::Globe &globe,
				GPlatesQtWidgets::GlobeCanvas &globe_canvas,
				GPlatesPresentation::ViewState &view_state,
				const GPlatesQtWidgets::ViewportWindow &viewport_window,
				GPlatesGui::FeatureTableModel &clicked_table_model,	
				GPlatesGui::TopologySectionsContainer &topology_sections_container,
				GPlatesQtWidgets::TopologyToolsWidget &topology_tools_widget)
		{
			BuildTopology::non_null_ptr_type ptr(
					new BuildTopology(
							globe, 
							globe_canvas,
							view_state,
							viewport_window, 
							clicked_table_model,
							topology_sections_container,
							topology_tools_widget),
					GPlatesUtils::NullIntrusivePointerHandler());
			return ptr;
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
				const GPlatesMaths::PointOnSphere &click_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_click_pos_on_globe,
				bool is_on_globe);

		virtual
		void
		handle_shift_left_click(
				const GPlatesMaths::PointOnSphere &click_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_click_pos_on_globe,
				bool is_on_globe);

	protected:
		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		BuildTopology(
				GPlatesGui::Globe &globe,
				GPlatesQtWidgets::GlobeCanvas &globe_canvas,
				GPlatesPresentation::ViewState &view_state,
				const GPlatesQtWidgets::ViewportWindow &viewport_window,
				GPlatesGui::FeatureTableModel &clicked_table_model,	
				GPlatesGui::TopologySectionsContainer &topology_sections_container,
				GPlatesQtWidgets::TopologyToolsWidget &topology_tools_widget);


		GPlatesGui::FeatureTableModel &
		clicked_table_model() const
		{
			return *d_clicked_table_model_ptr;
		}
		
	private:

		/**
		 * We need to change which canvas-tool layer is shown when this canvas-tool is
		 * activated.
		 */
		GPlatesViewOperations::RenderedGeometryCollection *d_rendered_geom_collection;

		/**
		 * For querying the reconstruction.
		 */
		GPlatesAppLogic::Reconstruct *d_reconstruct_ptr;

		/**
		 * This is currently used to pass messages to the status bar.
		 */
		const GPlatesQtWidgets::ViewportWindow *d_viewport_window_ptr;

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
		
		// This constructor should never be defined, because we don't want/need to allow
		// copy-construction.
		BuildTopology(
				const BuildTopology &);

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment.
		BuildTopology &
		operator=(
				const BuildTopology &);
	};
}

#endif  // GPLATES_CANVASTOOLS_BUILD_TOPOLOGY_H
