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

#ifndef GPLATES_CANVASTOOLS_GLOBECLICKGEOMETRY_H
#define GPLATES_CANVASTOOLS_GLOBECLICKGEOMETRY_H

#include "gui/FeatureFocus.h"
#include "gui/GlobeCanvasTool.h"
#include "gui/FeatureTableModel.h"


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
	class GlobeClickGeometry:
			public GPlatesGui::GlobeCanvasTool
	{

	public:
		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<ClickGeometry,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GlobeClickGeometry,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;

		virtual
		~GlobeClickGeometry()
		{  }

		/**
		 * Create a GlobeClickGeometry instance.
		 */
		static
		const non_null_ptr_type
		create(
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
				GPlatesGui::Globe &globe,
				GPlatesQtWidgets::GlobeCanvas &globe_canvas,
				const GPlatesQtWidgets::ViewportWindow &view_state,
				GPlatesGui::FeatureTableModel &clicked_table_model,
				GPlatesQtWidgets::FeaturePropertiesDialog &fp_dialog,
				GPlatesGui::FeatureFocus &feature_focus);

		
		virtual
		void
		handle_activation();

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

#if 0
		void
		no_hits_found();
#endif
	protected:
		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GlobeClickGeometry(
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
				GPlatesGui::Globe &globe,
				GPlatesQtWidgets::GlobeCanvas &globe_canvas,
				const GPlatesQtWidgets::ViewportWindow &view_state,
				GPlatesGui::FeatureTableModel &clicked_table_model,
				GPlatesQtWidgets::FeaturePropertiesDialog &fp_dialog,
				GPlatesGui::FeatureFocus &feature_focus);

		const GPlatesQtWidgets::ViewportWindow &
		view_state() const
		{
			return *d_view_state_ptr;
		}

		GPlatesGui::FeatureTableModel &
		clicked_table_model() const
		{
			return *d_clicked_table_model_ptr;
		}

		GPlatesQtWidgets::FeaturePropertiesDialog &
		fp_dialog() const
		{
			return *d_fp_dialog_ptr;
		}

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
	};
}

#endif  // GPLATES_CANVASTOOLS_GLOBECLICKGEOMETRY_H
