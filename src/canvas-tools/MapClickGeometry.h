/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008 Geological Survey of Norway
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

#ifndef GPLATES_CANVASTOOLS_MAPCLICKGEOMETRY_H
#define GPLATES_CANVASTOOLS_MAPCLICKGEOMETRY_H

#include "gui/FeatureFocus.h"
#include "gui/FeatureTableModel.h"
#include "gui/MapCanvasTool.h"

namespace GPlatesGui
{
	class GeometryFocusHighlight;
}

namespace GPlatesQtWidgets
{
	class MapView;
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
	class MapClickGeometry:
			public GPlatesGui::MapCanvasTool
	{

	public:
		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<MapClickGeometry,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<MapClickGeometry,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;

		virtual
		~MapClickGeometry()
		{  }

		/**
		 * Create a MapClickGeometry instance.
		 */
		static
		const non_null_ptr_type
		create(
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
				GPlatesQtWidgets::MapCanvas &map_canvas_,
				GPlatesQtWidgets::MapView &map_view_,
				const GPlatesQtWidgets::ViewportWindow &view_state_,
				GPlatesGui::FeatureTableModel &clicked_table_model,
				GPlatesQtWidgets::FeaturePropertiesDialog &fp_dialog_,
				GPlatesGui::FeatureFocus &feature_focus,
				GPlatesGui::GeometryFocusHighlight &geometry_focus_highlight)
		{
			MapClickGeometry::non_null_ptr_type ptr(
					new MapClickGeometry(rendered_geom_collection,map_canvas_, map_view_, view_state_,
							clicked_table_model, fp_dialog_, feature_focus, geometry_focus_highlight),
					GPlatesUtils::NullIntrusivePointerHandler());
			return ptr;
		}

		
		virtual
		void
		handle_activation();

		virtual
		void
		handle_left_click(
				const QPointF &click_point_on_scene,
				bool is_on_surface);

		virtual
		void
		handle_shift_left_click(
				const QPointF &click_point_on_scene,
				bool is_on_surface);


	protected:
		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		MapClickGeometry(
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
				GPlatesQtWidgets::MapCanvas &map_canvas_,
				GPlatesQtWidgets::MapView &map_view_,
				const GPlatesQtWidgets::ViewportWindow &view_state_,
				GPlatesGui::FeatureTableModel &clicked_table_model_,
				GPlatesQtWidgets::FeaturePropertiesDialog &fp_dialog_,
				GPlatesGui::FeatureFocus &feature_focus,
				GPlatesGui::GeometryFocusHighlight &geometry_focus_highlight_):
			MapCanvasTool(map_canvas_, map_view_),
			d_rendered_geom_collection(&rendered_geom_collection),
			d_view_state_ptr(&view_state_),
			d_clicked_table_model_ptr(&clicked_table_model_),
			d_fp_dialog_ptr(&fp_dialog_),
			d_feature_focus_ptr(&feature_focus),
			d_geometry_focus_highlight(&geometry_focus_highlight_)
		{  }

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

		/**
		 * Used to draw the focused geometry explicitly (if currently in focus).
		 */
		GPlatesGui::GeometryFocusHighlight *d_geometry_focus_highlight;
		
		// This constructor should never be defined, because we don't want/need to allow
		// copy-construction.
		MapClickGeometry(
				const MapClickGeometry &);

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment.
		MapClickGeometry &
		operator=(
				const MapClickGeometry &);
	};
}

#endif  // GPLATES_CANVASTOOLS_MAPCLICKGEOMETRY_H
