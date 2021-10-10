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

#include <QObject>

#include "gui/FeatureFocus.h"
#include "gui/CanvasTool.h"
#include "gui/FeatureTableModel.h"


namespace GPlatesQtWidgets
{
	class GlobeCanvas;
	class ViewportWindow;
	class FeaturePropertiesDialog;
}

namespace GPlatesGui
{
	class RenderedGeometryLayers;
}

namespace GPlatesCanvasTools
{
	/**
	 * This is the canvas tool used to focus features by clicking on them.
	 */
	class ClickGeometry:
			// It seems that QObject must be the first base specified here...
			public QObject,
			public GPlatesGui::CanvasTool
	{
		Q_OBJECT

	public:
		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<ClickGeometry,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<ClickGeometry,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;

		virtual
		~ClickGeometry()
		{  }

		/**
		 * Create a ClickGeometry instance.
		 */
		static
		const non_null_ptr_type
		create(
				GPlatesGui::Globe &globe_,
				GPlatesQtWidgets::GlobeCanvas &globe_canvas_,
				GPlatesGui::RenderedGeometryLayers &layers,
				const GPlatesQtWidgets::ViewportWindow &view_state_,
				GPlatesGui::FeatureTableModel &clicked_table_model,
				GPlatesQtWidgets::FeaturePropertiesDialog &fp_dialog_,
				GPlatesGui::FeatureFocus &feature_focus)
		{
			ClickGeometry::non_null_ptr_type ptr(
					new ClickGeometry(globe_, globe_canvas_, layers, view_state_,
							clicked_table_model, fp_dialog_, feature_focus),
					GPlatesUtils::NullIntrusivePointerHandler());
			return ptr;
		}

		
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

	signals:
		void
		sorted_hits_updated();

		void
		no_hits_found();

	protected:
		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		explicit
		ClickGeometry(
				GPlatesGui::Globe &globe_,
				GPlatesQtWidgets::GlobeCanvas &globe_canvas_,
				GPlatesGui::RenderedGeometryLayers &layers,
				const GPlatesQtWidgets::ViewportWindow &view_state_,
				GPlatesGui::FeatureTableModel &clicked_table_model_,
				GPlatesQtWidgets::FeaturePropertiesDialog &fp_dialog_,
				GPlatesGui::FeatureFocus &feature_focus):
			CanvasTool(globe_, globe_canvas_),
			d_layers_ptr(&layers),
			d_view_state_ptr(&view_state_),
			d_clicked_table_model_ptr(&clicked_table_model_),
			d_fp_dialog_ptr(&fp_dialog_),
			d_feature_focus_ptr(&feature_focus)
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
		GPlatesGui::RenderedGeometryLayers *d_layers_ptr;

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
		
		// This constructor should never be defined, because we don't want/need to allow
		// copy-construction.
		ClickGeometry(
				const ClickGeometry &);

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment.
		ClickGeometry &
		operator=(
				const ClickGeometry &);
	};
}

#endif  // GPLATES_CANVASTOOLS_CLICKGEOMETRY_H
