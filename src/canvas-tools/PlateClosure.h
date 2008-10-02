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

#ifndef GPLATES_CANVASTOOLS_PLATECLOSURE_H
#define GPLATES_CANVASTOOLS_PLATECLOSURE_H

#include <QObject>

#include "gui/FeatureFocus.h"
#include "gui/CanvasTool.h"
#include "gui/FeatureTableModel.h"
#include "qt-widgets/PlateClosureWidget.h"


namespace GPlatesQtWidgets
{
	class GlobeCanvas;
	class ViewportWindow;
}

namespace GPlatesGui
{
	class RenderedGeometryLayers;
}

namespace GPlatesCanvasTools
{
	/**
	 * This is the canvas tool used to define new geometry.
	 */
	class PlateClosure:
			public QObject,
			public GPlatesGui::CanvasTool
	{
		Q_OBJECT

	public:

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<PlateClosure,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<PlateClosure,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;

		virtual
		~PlateClosure()
		{  }

		/**
		 * Create a PlateClosure instance.
		 */
		static
		const non_null_ptr_type
		create(
				GPlatesGui::Globe &globe_,
				GPlatesQtWidgets::GlobeCanvas &globe_canvas_,
				GPlatesGui::RenderedGeometryLayers &layers,
				const GPlatesQtWidgets::ViewportWindow &view_state_,
				GPlatesGui::FeatureTableModel &clicked_table_model_,	
				GPlatesGui::FeatureTableModel &segments_table_model_,	
				GPlatesQtWidgets::PlateClosureWidget &plate_closure_widget_,
				GPlatesQtWidgets::PlateClosureWidget::GeometryType geom_type_,
				GPlatesGui::FeatureFocus &feature_focus_)
		{
			PlateClosure::non_null_ptr_type ptr(
					new PlateClosure(
							globe_, 
							globe_canvas_, 
							layers, 
							view_state_, 
							clicked_table_model_,
							segments_table_model_,
							plate_closure_widget_, 
							geom_type_, 
							feature_focus_),
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

		// FIXME do we need handle_left_shift_click ?

	signals:
		void
		sorted_hits_updated();

		void
		no_hits_found();

	protected:
		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		explicit
		PlateClosure(
				GPlatesGui::Globe &globe_,
				GPlatesQtWidgets::GlobeCanvas &globe_canvas_,
				GPlatesGui::RenderedGeometryLayers &layers,
				const GPlatesQtWidgets::ViewportWindow &view_state_,
				GPlatesGui::FeatureTableModel &clicked_table_model_,	
				GPlatesGui::FeatureTableModel &segments_table_model_,	
				GPlatesQtWidgets::PlateClosureWidget &plate_closure_widget_,
				GPlatesQtWidgets::PlateClosureWidget::GeometryType geom_type_,
				GPlatesGui::FeatureFocus &feature_focus):
			CanvasTool(globe_, globe_canvas_),
			d_layers_ptr(&layers),
			d_view_state_ptr(&view_state_),
			d_clicked_table_model_ptr(&clicked_table_model_),
			d_segments_table_model_ptr(&segments_table_model_),
			d_plate_closure_widget_ptr(&plate_closure_widget_),
 			d_default_geom_type(geom_type_),
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
		
		GPlatesGui::FeatureTableModel &
		segments_table_model() const
		{
			return *d_segments_table_model_ptr;
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
		 * This is the external table of selected features for boundary
		 */
		GPlatesGui::FeatureTableModel *d_segments_table_model_ptr;

		/**
		 * This is the PlateClosureWidget in the Task Panel.
		 * It accumulates points for us and handles the actual feature creation step.
		 */
		GPlatesQtWidgets::PlateClosureWidget *d_plate_closure_widget_ptr;
		
		/**
		 * This is the type of geometry this particular PlateClosure tool
		 * should default to.
		 */
		GPlatesQtWidgets::PlateClosureWidget::GeometryType d_default_geom_type;
	
		/**
		 * This is our reference to the Feature Focus, which we use to let the rest of the
		 * application know what the user just clicked on.
		 */
		GPlatesGui::FeatureFocus *d_feature_focus_ptr;
		
		// This constructor should never be defined, because we don't want/need to allow
		// copy-construction.
		PlateClosure(
				const PlateClosure &);

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment.
		PlateClosure &
		operator=(
				const PlateClosure &);
		
	};
}

#endif  // GPLATES_CANVASTOOLS_PLATECLOSURE_H
