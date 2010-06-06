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

#include "gui/FeatureFocus.h"
#include "CanvasTool.h"
#include "gui/FeatureTableModel.h"


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
	class RenderedGeometryCollection;
}

namespace GPlatesCanvasTools
{
	/**
	 * This is the canvas tool used to focus features by clicking on them.
	 */
	class ClickGeometry:
			public CanvasTool
	{

	public:

		virtual
		~ClickGeometry()
		{  }

		/**
		 * Create a ClickGeometry instance.
		 */
		ClickGeometry(
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
				const GPlatesQtWidgets::ViewportWindow &view_state,
				GPlatesGui::FeatureTableModel &clicked_table_model,
				GPlatesQtWidgets::FeaturePropertiesDialog &fp_dialog,
				GPlatesGui::FeatureFocus &feature_focus,
				GPlatesAppLogic::ApplicationState &application_state);
		
		virtual
		void
		handle_activation();

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

	private:

		/**
		 * We need to change which canvas-tool layer is shown when this canvas-tool is
		 * activated.
		 */
		GPlatesViewOperations::RenderedGeometryCollection *d_rendered_geom_collection;

		/**
		 * This is the view state which is used to obtain the reconstruction root.
		 *
		 * Since the view state is also the ViewportWindow, it is currently used to
		 * pass messages to the status bar.
		 */
		const GPlatesQtWidgets::ViewportWindow *d_view_state_ptr;

		/**
		 * This is the external table of hits which will be updated in the event that
		 * the test point hits one or more geometries.
		 */
		GPlatesGui::FeatureTableModel *d_clicked_table_model_ptr;

		/**
		 * This is the dialog box which we will be populating in response to a feature
		 * query.
		 */
		GPlatesQtWidgets::FeaturePropertiesDialog *d_fp_dialog_ptr;

		/**
		 * This is our reference to the Feature Focus, which we use to let the rest of the
		 * application know what the user just clicked on.
		 */
		GPlatesGui::FeatureFocus *d_feature_focus_ptr;

		/**
		 * Used when adding reconstruction geometries to the clicked feature table.
		 */
		const GPlatesAppLogic::ReconstructGraph &d_reconstruct_graph;


		GPlatesQtWidgets::FeaturePropertiesDialog &
		fp_dialog() const
		{
			return *d_fp_dialog_ptr;
		}

	};
}

#endif  // GPLATES_CANVASTOOLS_CLICKGEOMETRY_H
