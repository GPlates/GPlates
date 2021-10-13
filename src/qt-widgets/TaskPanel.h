/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008, 2011 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_TASKPANEL_H
#define GPLATES_QTWIDGETS_TASKPANEL_H

#include <QWidget>
#include <QStackedWidget>
#include <QAction>

#include "ActionButtonBox.h"
#include "TaskPanelWidget.h"

#include "model/ModelInterface.h"
#include "model/types.h"

#include "presentation/ViewState.h"


namespace GPlatesCanvasTools
{
	class MeasureDistanceState;
}

namespace GPlatesAppLogic
{
	class FeatureCollectionFileState;
}

namespace GPlatesGui
{
	class ChooseCanvasTool;
}

namespace GPlatesViewOperations
{
	class ActiveGeometryOperation;
	class GeometryBuilder;
	class GeometryOperationTarget;
	class RenderedGeometryCollection;
}

namespace GPlatesQtWidgets
{
	class DigitisationWidget;
	class FeatureSummaryWidget;
	class MeasureDistanceWidget;
	class ModifyGeometryWidget;
	class ModifyReconstructionPoleWidget;
	class SmallCircleWidget;
	class SnapNearbyVerticesWidget;
	class TopologyToolsWidget;
	class ViewportWindow;

	/**
	 * The Xtreme Task Panel - A contextual tabbed interface to expose powerful tasks
	 * to manipulate GPlates.... to the XTREME!
	 */
	class TaskPanel :
			public QWidget
	{
		Q_OBJECT

	public:

		/**
		 * Enumeration of all possible pages (or 'tabs') that the TaskPanel can display.
		 *
		 * Please ensure that this enumeration matches the order in which tabs are set
		 * up in the TaskPanel constructor.
		 */
		enum Page
		{
			CURRENT_FEATURE,
			DIGITISATION,
			MODIFY_GEOMETRY,
			MODIFY_POLE,
			TOPOLOGY_TOOLS,
			MEASURE_DISTANCE,
			SMALL_CIRCLE,

			NUM_PAGES // Must be the last entry.
		};


		explicit
		TaskPanel(
				GPlatesPresentation::ViewState &view_state,
				GPlatesViewOperations::GeometryBuilder &digitise_geometry_builder,
				GPlatesViewOperations::GeometryOperationTarget &geometry_operation_target,
				GPlatesViewOperations::ActiveGeometryOperation &active_geometry_operation,
				GPlatesCanvasTools::MeasureDistanceState &measure_distance_state,
				ViewportWindow &viewport_window_,
				QAction *undo_action_ptr,
				QAction *redo_action_ptr,
				GPlatesGui::ChooseCanvasTool &choose_canvas_tool,
				QWidget *parent_ = NULL);
		
		/**
		 * Accessor for the Action Button Box of the Feature Tab.
		 *
		 * This lets ViewportWindow set up the QActions for the Feature tab
		 * to use. ViewportWindow needs to be the owner of the QActions so that
		 * they can be used as menu entries.
		 */
		ActionButtonBox &
		feature_action_button_box() const
		{
			return *d_feature_action_button_box_ptr;
		}

		/**
		 * Accessor for the Digitisation Widget of the Digitisation Tab.
		 *
		 * This lets the DigitiseGeometry canvas tool interact with the
		 * DigitisationWidget.
		 */
		DigitisationWidget &
		digitisation_widget() const
		{
			return *d_digitisation_widget_ptr;
		}

		/**
		 * Accessor for the Reconstruction Pole Widget of the Modify Pole Tab.
		 *
		 * This lets the interactive manipulation of reconstructions canvas tool
		 * interact with the ModifyReconstructionPoleWidget.
		 */
		ModifyReconstructionPoleWidget &
		modify_reconstruction_pole_widget() const
		{
			return *d_modify_reconstruction_pole_widget_ptr;
		}

		/**
		 * Accessor for the TopologyToolsWidget 
		 *
		 * This lets the topology canvas tools interact with the widget in the task panel
		 */
		TopologyToolsWidget &
		topology_tools_widget() const
		{
			return *d_topology_tools_widget_ptr;
		}
		
		/**
		 * Access for the MeasureDistanceWidget
		 *
		 * This lets the Measure Distance canvas tool interact with the
		 * MeasureDistanceWidget
		 */
		MeasureDistanceWidget &
		measure_distance_widget() const
		{
			return *d_measure_distance_widget_ptr;
		}

		/**
		 *  Accessor for SmallCircleWidget                                                                    
		 */
		SmallCircleWidget &
		small_circle_widget() const
		{
			return *d_small_circle_widget_ptr;
		}

		/**
		 * Gets action shared between all task panel widgets that, when triggered,
		 * clears the state of that widget.
		 */
		QAction *
		get_clear_action()
		{
			return d_clear_action;
		}

		void
		emit_vertex_data_changed_signal(
				bool should_check_nearby_vertices,
				double threshold,
				bool should_use_plate_id,
				GPlatesModel::integer_plate_id_type plate_id);
			
	signals:
		
		void
		vertex_data_changed(bool,double,bool,GPlatesModel::integer_plate_id_type);
	
	public slots:
		
		/**
		 * Select a particular tab/page and make it visible.
		 */
		void
		choose_tab(
				GPlatesQtWidgets::TaskPanel::Page page);

#if 0
		/**
		 * Used to disable (and re-enable) widgets for a particular tab.
		 *
		 * Currently this is needed by ViewportWindow::update_tools_and_status_message(),
		 * to disable some task panels that do not have a Map view implementation.
		 */
		void
		set_tab_enabled(
				GPlatesQtWidgets::TaskPanel::Page page,
				bool enabled)
		{
			int page_idx = static_cast<int>(page);
			d_stacked_widget_ptr->widget(page_idx)->setEnabled(enabled);
		}
#endif
		
		void
		choose_feature_tab();
		
		void
		choose_digitisation_tab();
		
		void
		choose_modify_geometry_tab();

		void
		choose_modify_pole_tab();
		
		void
		choose_topology_tools_tab();

		void
		choose_measure_distance_tab();

		void
		choose_small_circle_tab();

		void
		enable_move_nearby_vertices_widget(
				bool enable);

	private slots:

		void
		handle_clear_action_enabled_changed(
				bool enabled);

		void
		handle_clear_action_triggered();

	private:
		
		/**
		 * Does the basic tasks that setupUi(this) would do if we were using a
		 * Designer-made widget.
		 */
		void
		set_up_ui();


		/**
		 * Sets up the "Current Feature" tab in the X-Treme Task Panel.
		 * This connects all the QToolButtons in the "Feature" tab to QActions,
		 * and adds the special FeatureSummaryWidget.
		 */
		void
		set_up_feature_tab(
				GPlatesPresentation::ViewState &view_state);


		/**
		 * Sets up the "Digitisation" tab in the eXtreme Task Panel.
		 * This connects all the QToolButtons in the "Digitisation" tab to QActions,
		 * and adds the special DigitisationWidget.
		 */
		void
		set_up_digitisation_tab();


		/**
		 * Sets up the "Modify Geometry" tab in the eXtreme Task Panel.
		 * This connects all the QToolButtons in the "Digitisation" tab to QActions,
		 * and adds the special ModifyGeometryWidget.
		 */
		void
		set_up_modify_geometry_tab();


		/**
		 * Sets up the "Modify Pole" tab in the Extr3me Task Panel.
		 * This adds the special ModifyReconstructionPoleWidget.
		 */
		void
		set_up_modify_pole_tab();

		/**
		 * Sets up the "Topology Tools" tab in the Extra Creamy Task Panel.
		 * This adds the special TopologyToolsWidget.
		 */
		void
		set_up_topology_tools_tab();

		/**
		 * Sets up the "Measure Distance" tab in the Task Panel.
		 * This adds the MeasureDistanceWidget.
		 */
		void
		set_up_measure_distance_tab();

		/**
		 *  Sets up the "Small Circle" tab.  In the egg-stream task panel.                                                                   
		 */
		void
		set_up_small_circle_tab();


		/**
		 * The QStackedWidget that emulates tab-like behaviour without actual tabs.
		 * Parented to the TaskPanel, memory managed by Qt.
		 */
		QStackedWidget *d_stacked_widget_ptr;

		/**
		 * Widget responsible for the buttons in the Feature Tab.
		 * Memory managed by Qt.
		 *
		 * ActionButtonBox:
		 * NOTE: The buttons you see in the Task Panel are QToolButtons.
		 * This is done so that they can be connected to a QAction and immediately inherit
		 * the icon, text, tooltips, etc of that action; furthermore, if the action is
		 * disabled/enabled, the button automagically reflects that. This is great, since
		 * most of the buttons in the Task Panel should also be available as menu items.
		 *
		 * The buttons are automatically set up by a GPlatesQtWidgets::ActionButtonBox.
		 * All you need to do is tell it how many columns to use and the pixel size of
		 * their icons (done in TaskPanel's initialiser list).
		 */
		ActionButtonBox *d_feature_action_button_box_ptr;

		/**
		 * Widget for controls relating to moving nearby vertices
		 */
		SnapNearbyVerticesWidget *d_snap_nearby_vertices_widget_ptr;

		/**
		 * Action shared between all task panel widgets that, when triggered,
		 * clears the state of that widget.
		 */
		QAction *d_clear_action;

		// This union allows access to widgets by name and by index.
		union
		{
			/**
			 * An array of pointers to all the widgets in the various tabs.
			 */
			TaskPanelWidget *d_task_panel_widgets[NUM_PAGES];

			struct
			{
				/**
				 * Widget responsible for the controls in the Current Feature tab.
				 * Memory managed by Qt.
				 */
				FeatureSummaryWidget *d_feature_summary_widget_ptr;

				/**
				 * Widget responsible for the controls in the Digitisation Tab.
				 * Memory managed by Qt.
				 */
				DigitisationWidget *d_digitisation_widget_ptr;

				/**
				 * Widget responsible for the controls in the Modify Geometry Tab.
				 * Memory managed by Qt.
				 */
				ModifyGeometryWidget *d_modify_geometry_widget_ptr;

				/**
				 * Widget responsible for the controls in the Modify Pole Tab.
				 * Memory managed by Qt.
				 */
				ModifyReconstructionPoleWidget *d_modify_reconstruction_pole_widget_ptr;

				/**
				 * Widget responsible for the controls in the Topology Tools Tab.
				 * Memory managed by Qt.
				 */
				TopologyToolsWidget *d_topology_tools_widget_ptr;
				 
				/**
				 * Widget responsible for the controls in the Measure Distance Tab.
				 * Memory managed by Qt.
				 */
				MeasureDistanceWidget *d_measure_distance_widget_ptr;

				/**
				 * Widget responsible for the Small Circle tab.                                                                    
				 */
				SmallCircleWidget *d_small_circle_widget_ptr;
			};
		};

		/**
		 * The task panel widget currently active.
		 */
		TaskPanelWidget *d_active_widget;
	};
}

#endif  // GPLATES_QTWIDGETS_TASKPANEL_H
