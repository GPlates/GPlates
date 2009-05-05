/**
 * \file 
 * $Revision: 4572 $
 * $Date: 2009-01-13 01:17:30 -0800 (Tue, 13 Jan 2009) $ 
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

#ifndef GPLATES_CANVASTOOLS_EDIT_TOPLOGY_H
#define GPLATES_CANVASTOOLS_EDIT_TOPLOGY_H

#include <QObject>

#include "gui/FeatureFocus.h"
#include "gui/CanvasTool.h"
#include "gui/FeatureTableModel.h"
#include "gui/TopologySectionsContainer.h"
#include "qt-widgets/EditTopologyWidget.h"


namespace GPlatesQtWidgets
{
	class GlobeCanvas;
	class ViewportWindow;
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
	class EditTopology:
			public QObject,
			public GPlatesGui::CanvasTool
	{
		Q_OBJECT

	public:

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<EditTopology,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<EditTopology,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;

		virtual
		~EditTopology()
		{  }

		/**
		 * Create a EditTopology instance.
		 */
		static
		const non_null_ptr_type
		create(
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
				GPlatesGui::Globe &globe,
				GPlatesQtWidgets::GlobeCanvas &globe_canvas,
				const GPlatesQtWidgets::ViewportWindow &view_state,
				GPlatesGui::FeatureTableModel &clicked_table_model,	
				GPlatesGui::TopologySectionsContainer &topology_sections_container,
				GPlatesQtWidgets::EditTopologyWidget &edit_topology_widget,
				GPlatesQtWidgets::EditTopologyWidget::GeometryType geom_type,
				GPlatesGui::FeatureFocus &feature_focus)
		{
			EditTopology::non_null_ptr_type ptr(
					new EditTopology(
							rendered_geom_collection,
							globe, 
							globe_canvas, 
							view_state, 
							clicked_table_model,
							topology_sections_container,
							edit_topology_widget, 
							geom_type, 
							feature_focus),
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

		virtual
		void
		handle_create_new_feature(GPlatesModel::FeatureHandle::weak_ref feature);

	public slots:


	signals:

		void
		sorted_hits_updated();

		void
		no_hits_found();

	protected:
		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		EditTopology(
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
				GPlatesGui::Globe &globe,
				GPlatesQtWidgets::GlobeCanvas &globe_canvas,
				const GPlatesQtWidgets::ViewportWindow &view_state,
				GPlatesGui::FeatureTableModel &clicked_table_model_,	
				//GPlatesGui::FeatureTableModel &segments_table_model_,	
				GPlatesGui::TopologySectionsContainer &topology_sections_container,
				GPlatesQtWidgets::EditTopologyWidget &edit_topology_widget,
				GPlatesQtWidgets::EditTopologyWidget::GeometryType geom_type_,
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
		 * This is the external table of selected features for the closed boundary
		 */
		GPlatesGui::TopologySectionsContainer *d_topology_sections_container_ptr;

		/**
		 * This is the EditTopologyWidget in the Task Panel.
		 * It accumulates points for us and handles the actual feature creation step.
		 */
		GPlatesQtWidgets::EditTopologyWidget *d_edit_topology_widget_ptr;
		
		/**
		 * This is the type of geometry this particular EditTopology tool
		 * should default to.
		 */
		GPlatesQtWidgets::EditTopologyWidget::GeometryType d_default_geom_type;
	
		/**
		 * This is our reference to the Feature Focus, which we use to let the rest of the
		 * application know what the user just clicked on.
		 */
		GPlatesGui::FeatureFocus *d_feature_focus_ptr;
		
		// This constructor should never be defined, because we don't want/need to allow
		// copy-construction.
		EditTopology(
				const EditTopology &);

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment.
		EditTopology &
		operator=(
				const EditTopology &);
		
	};
}

#endif  // GPLATES_CANVASTOOLS_EDIT_TOPLOGY_H
