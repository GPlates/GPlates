/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#include <deque>
#include <utility>
#include <boost/foreach.hpp>
#include <Qt>
#include <QtGlobal>
#include <QIcon>
#include <QKeySequence>
#include <QList>
#include <QPixmap>
#include <QTransform>
#include <QVariant>
#include <QVector>

#include "CanvasToolBarDockWidget.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

namespace GPlatesQtWidgets
{
	namespace
	{
		//! Index into QAction data() list representing the canvas tool workflow.
		const unsigned int TOOL_ACTION_DATA_LIST_WORKFLOW_INDEX = 0;

		//! Index into QAction data() list representing the canvas tool.
		const unsigned int TOOL_ACTION_DATA_LIST_TOOL_INDEX = 1;


		/**
		 * Returns the workflow/tool associated with the specified tool action.
		 */
		std::pair<
				GPlatesGui::CanvasToolWorkflows::WorkflowType,
				GPlatesGui::CanvasToolWorkflows::ToolType>
		get_workflow_tool_from_action(
				QAction *tool_action)
		{
			// Determine which workflow/tool to activate.
			QList<QVariant> tool_action_data_list = tool_action->data().toList();
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					tool_action_data_list.size() == 2,
					GPLATES_ASSERTION_SOURCE);
			const GPlatesGui::CanvasToolWorkflows::WorkflowType workflow =
					static_cast<GPlatesGui::CanvasToolWorkflows::WorkflowType>(
							tool_action_data_list[TOOL_ACTION_DATA_LIST_WORKFLOW_INDEX].toUInt());
			const GPlatesGui::CanvasToolWorkflows::ToolType tool =
					static_cast<GPlatesGui::CanvasToolWorkflows::ToolType>(
							tool_action_data_list[TOOL_ACTION_DATA_LIST_TOOL_INDEX].toUInt());

			return std::make_pair(workflow, tool);
		}


		/**
		 * Returns true if @a tool_action corresponds to the specified workflow/tool.
		 */
		bool
		is_tool_action(
				QAction *tool_action,
				GPlatesGui::CanvasToolWorkflows::WorkflowType workflow,
				GPlatesGui::CanvasToolWorkflows::ToolType tool)
		{
			return get_workflow_tool_from_action(tool_action) == std::make_pair(workflow, tool);
		}


		/*
		 * For macOS only, we'll style our tab widget tool palette using a stylesheet.
		 *
		 * The native look of our vertical QTabWidget on macOS is very different than Linux and Windows.
		 * The native macOS look wasn't particularly good for a tool palette when using Qt4, and now it looks
		 * even worse with Qt5. So we use a stylesheet to rectify that (for macOS only).
		 */
#if defined(Q_OS_MACOS)
		const QString MACOS_STYLESHEET = QString(
			R"(
				/* The entire background. */
				QWidget#dock_canvas_tools_contents
				{
					background: %1; border: none;
				}

				/* Just the area where the QToolBars will go (not the tab bar). */
				QTabWidget::pane
				{
					background: %2;

					/* Prevent drawing of a native border - we want entire pane to be a single colour. */
					border: none;
				}

				QTabWidget::pane QToolBar
				{
					background: %2; border: none;
				}

				/* Tool buttons in the QToolBars. */
				QTabWidget::pane QToolButton
				{
					background: %2;

					/*
					 * Qt docs state that this is required for QToolButton when only specifying background colour:
					 * "This is because, by default, the QToolButton draws a native border which completely overlaps the background-color".
					 * ...and we also do it for other widgets with no border (just in case; eg, seems QToolBar also needs it).
					 */
					border: none;

					/* Sum of 'checked' margin/border/padding so that contents (icon) remains same size whether selected or not. */
					margin: 3px;
				}

				/* Use a different colour when selected and put a border around button. */
				QTabWidget::pane QToolButton:checked
				{
					background: %3;
					border: 1px solid %4;
					border-radius: 4px;
					padding: 1px;
					margin: 1px;
				}

				/* Position the main tab bar. */
				QTabWidget::tab-bar
				{ 
					/* Ensure tab bar is at the top (ie, not centered vertically). */
					top: 0px; 
				}

				/* Each tab in the tab bar. */
				QTabBar::tab
				{
					/* A little clearance around the content (icon). */
					padding: 1px;

					/* Seems the content (icon) is aligned with bottom of tab, so move it upwards to be more centered in the tab. */
					padding-top: -7px;
					padding-bottom: 7px;

					/* Give it a curved look on the left side. */
					border-top-left-radius: 4px;
					border-bottom-left-radius: 4px;
				}

				QTabBar::tab:selected
				{
					/* Give selected tab same colour as QToolBar's. */
					background: %2;
				}

				QTabBar::tab:!selected
				{
					/* Give unselected tabs same colour as selected tool in QToolBar. */
					background: %3;

					/* Make non-selected tabs look smaller. */
					margin-left: 2px;
					margin-top: 1px;
					margin-bottom: 1px;
				}
			)")

		// Colour of dock widget contents background...
		.arg("rgb(220, 220, 222)")

		// Colour of selected tab in tab bar (ie, currently selected group of tools) and
		// background of entire tool bar (ie, all tools in currently selected group) and
		// unselected buttons in that tool bar...
		.arg("rgb(240, 240, 242)")

		// Colour of unselected tabs in tab bar (ie, unselected groups of tools) and
		// selected button in tool bar (ie, currently selected tool)...
		.arg("rgb(190, 190, 190)")

		// Border colour of selected button in tool bar (ie, currently selected tool)...
		.arg("rgb(150, 150, 150)");
#endif
	}
}


GPlatesQtWidgets::CanvasToolBarDockWidget::CanvasToolBarDockWidget(
		GPlatesGui::DockState &dock_state,
		GPlatesGui::CanvasToolWorkflows &canvas_tool_workflows,
		ViewportWindow &main_window,
		const QSize &tool_icon_size) :
	// Use empty string for dock title so it doesn't display in the title bar...
	DockWidget("", dock_state, main_window, QString("canvas_toolbar")),
	d_canvas_tool_workflows(canvas_tool_workflows),
	d_tool_icon_regular_size(tool_icon_size)
{
	setupUi(this);

	// The native look of our vertical QTabWidget on macOS is very different than Linux and Windows.
	// The native macOS look wasn't particularly good for a tool palette when using Qt4, and now it looks
	// even worse with Qt5. So, for macOS only, we'll style it ourselves using a stylesheet. However we'll
	// retain the native look for Linux and Windows (as before) since they look similar to each other and also
	// look good, and this means all widgets across GPlates have a the unified (native) look (for Linux and Windows).
#if defined(Q_OS_MACOS)
	setStyleSheet(MACOS_STYLESHEET);
#endif

	// Create a tool bar for each canvas tools workflow and populate the tool actions.
	set_up_workflows();

	// Orient the workflow tab icons - they need to be rotated 90 degrees clockwise.
	set_up_workflow_tab_icons();

	// Setup canvas tool shortcuts separately from their equivalent QActions.
	// This is because we can't have the same shortcut for two or more QActions -
	// which can occur when the same tool type is used by multiple workflows.
	set_up_canvas_tool_shortcuts();

	// Setup canvas workflow shortcuts (for the workflow tabs).
	set_up_canvas_workflow_shortcuts();

	// Handle enable/disable of canvas tools.
	QObject::connect(
			&canvas_tool_workflows,
			SIGNAL(canvas_tool_enabled(
					GPlatesGui::CanvasToolWorkflows::WorkflowType,
					GPlatesGui::CanvasToolWorkflows::ToolType,
					bool)),
			this,
			SLOT(handle_canvas_tool_enabled(
					GPlatesGui::CanvasToolWorkflows::WorkflowType,
					GPlatesGui::CanvasToolWorkflows::ToolType,
					bool)));

	// Handle activation of a canvas tool.
	// It's either us activating a canvas tool or the menu in the main window (or something else
	// like an undo command).
	QObject::connect(
			&canvas_tool_workflows,
			SIGNAL(canvas_tool_activated(
					GPlatesGui::CanvasToolWorkflows::WorkflowType,
					GPlatesGui::CanvasToolWorkflows::ToolType)),
			this,
			SLOT(handle_canvas_tool_activated(
					GPlatesGui::CanvasToolWorkflows::WorkflowType,
					GPlatesGui::CanvasToolWorkflows::ToolType)));

	// When the workflow tab is directly changed by the user we need to select that workflow's current tool.
	connect_to_workflow_tab_changed();
}


void
GPlatesQtWidgets::CanvasToolBarDockWidget::set_up_workflows()
{
	set_up_view_workflow();
	set_up_feature_inspection_workflow();
	set_up_digitisation_workflow();
	set_up_topology_workflow();
	set_up_pole_manipulation_workflow();
	set_up_small_circle_workflow();
	set_up_hellinger_workflow();
}


void
GPlatesQtWidgets::CanvasToolBarDockWidget::set_up_view_workflow()
{
	//
	// Set up the 'view' canvas tools workflow.
	//

	Workflow view_workflow = create_workflow(
			GPlatesGui::CanvasToolWorkflows::WORKFLOW_VIEW,
			tr("View"),
			tab_view,
			view_toolbar_placeholder);

	add_tool_action_to_workflow(
			view_workflow,
			GPlatesGui::CanvasToolWorkflows::TOOL_DRAG_GLOBE,
			action_Drag_Globe);
	add_tool_action_to_workflow(
			view_workflow,
			GPlatesGui::CanvasToolWorkflows::TOOL_ZOOM_GLOBE,
			action_Zoom_Globe);
#if 0 // Disable lighting tool until volume visualisation is officially released (in GPlates 1.5)...
	add_tool_action_to_workflow(
			view_workflow,
			GPlatesGui::CanvasToolWorkflows::TOOL_CHANGE_LIGHTING,
			action_Change_Lighting);
#endif
}


void
GPlatesQtWidgets::CanvasToolBarDockWidget::set_up_feature_inspection_workflow()
{
	//
	// Set up the 'feature inspection' canvas tools workflow.
	//

	Workflow feature_inspection_workflow = create_workflow(
			GPlatesGui::CanvasToolWorkflows::WORKFLOW_FEATURE_INSPECTION,
			tr("Feature Inspection"),
			tab_feature_inspection,
			feature_inspection_toolbar_placeholder);

	add_tool_action_to_workflow(
			feature_inspection_workflow,
			GPlatesGui::CanvasToolWorkflows::TOOL_MEASURE_DISTANCE,
			action_Measure_Distance);
	add_tool_action_to_workflow(
			feature_inspection_workflow,
			GPlatesGui::CanvasToolWorkflows::TOOL_CLICK_GEOMETRY,
			action_Click_Geometry);
	add_tool_action_to_workflow(
			feature_inspection_workflow,
			GPlatesGui::CanvasToolWorkflows::TOOL_MOVE_VERTEX,
			action_Move_Vertex);
	add_tool_action_to_workflow(
			feature_inspection_workflow,
			GPlatesGui::CanvasToolWorkflows::TOOL_INSERT_VERTEX,
			action_Insert_Vertex);
	add_tool_action_to_workflow(
			feature_inspection_workflow,
			GPlatesGui::CanvasToolWorkflows::TOOL_DELETE_VERTEX,
			action_Delete_Vertex);
	add_tool_action_to_workflow(
			feature_inspection_workflow,
			GPlatesGui::CanvasToolWorkflows::TOOL_SPLIT_FEATURE,
			action_Split_Feature);
}


void
GPlatesQtWidgets::CanvasToolBarDockWidget::set_up_digitisation_workflow()
{
	//
	// Set up the 'digitisation' canvas tools workflow.
	//

	Workflow digitisation_workflow = create_workflow(
			GPlatesGui::CanvasToolWorkflows::WORKFLOW_DIGITISATION,
			tr("Digitisation"),
			tab_digitisation,
			digitisation_toolbar_placeholder);

	add_tool_action_to_workflow(
			digitisation_workflow,
			GPlatesGui::CanvasToolWorkflows::TOOL_MEASURE_DISTANCE,
			action_Measure_Distance);
	add_tool_action_to_workflow(
			digitisation_workflow,
			GPlatesGui::CanvasToolWorkflows::TOOL_DIGITISE_NEW_POLYLINE,
			action_Digitise_New_Polyline);
	add_tool_action_to_workflow(
			digitisation_workflow,
			GPlatesGui::CanvasToolWorkflows::TOOL_DIGITISE_NEW_MULTIPOINT,
			action_Digitise_New_MultiPoint);
	add_tool_action_to_workflow(
			digitisation_workflow,
			GPlatesGui::CanvasToolWorkflows::TOOL_DIGITISE_NEW_POLYGON,
			action_Digitise_New_Polygon);
	add_tool_action_to_workflow(
			digitisation_workflow,
			GPlatesGui::CanvasToolWorkflows::TOOL_MOVE_VERTEX,
			action_Move_Vertex);
	add_tool_action_to_workflow(
			digitisation_workflow,
			GPlatesGui::CanvasToolWorkflows::TOOL_INSERT_VERTEX,
			action_Insert_Vertex);
	add_tool_action_to_workflow(
			digitisation_workflow,
			GPlatesGui::CanvasToolWorkflows::TOOL_DELETE_VERTEX,
			action_Delete_Vertex);
}


void
GPlatesQtWidgets::CanvasToolBarDockWidget::set_up_topology_workflow()
{
	//
	// Set up the 'topology' canvas tools workflow.
	//

	Workflow topology_workflow = create_workflow(
			GPlatesGui::CanvasToolWorkflows::WORKFLOW_TOPOLOGY,
			tr("Topology"),
			tab_topology,
			topology_toolbar_placeholder);

	add_tool_action_to_workflow(
			topology_workflow,
			GPlatesGui::CanvasToolWorkflows::TOOL_CLICK_GEOMETRY,
			action_Click_Geometry);
	add_tool_action_to_workflow(
			topology_workflow,
			GPlatesGui::CanvasToolWorkflows::TOOL_BUILD_LINE_TOPOLOGY,
			action_Build_Line_Topology);
	add_tool_action_to_workflow(
			topology_workflow,
			GPlatesGui::CanvasToolWorkflows::TOOL_BUILD_BOUNDARY_TOPOLOGY,
			action_Build_Boundary_Topology);
	add_tool_action_to_workflow(
			topology_workflow,
			GPlatesGui::CanvasToolWorkflows::TOOL_BUILD_NETWORK_TOPOLOGY,
			action_Build_Network_Topology);
	add_tool_action_to_workflow(
			topology_workflow,
			GPlatesGui::CanvasToolWorkflows::TOOL_EDIT_TOPOLOGY,
			action_Edit_Topology);
}


void
GPlatesQtWidgets::CanvasToolBarDockWidget::set_up_pole_manipulation_workflow()
{
	//
	// Set up the 'pole manipulation' canvas tools workflow.
	//

	Workflow pole_manipulation_workflow = create_workflow(
			GPlatesGui::CanvasToolWorkflows::WORKFLOW_POLE_MANIPULATION,
			tr("Pole Manipulation"),
			tab_pole_manipulation,
			pole_manipulation_toolbar_placeholder);

	add_tool_action_to_workflow(
			pole_manipulation_workflow,
			GPlatesGui::CanvasToolWorkflows::TOOL_CLICK_GEOMETRY,
			action_Click_Geometry);
	add_tool_action_to_workflow(
			pole_manipulation_workflow,
			GPlatesGui::CanvasToolWorkflows::TOOL_MOVE_POLE,
			action_Move_Pole);
	add_tool_action_to_workflow(
			pole_manipulation_workflow,
			GPlatesGui::CanvasToolWorkflows::TOOL_MANIPULATE_POLE,
			action_Manipulate_Pole);
}


void
GPlatesQtWidgets::CanvasToolBarDockWidget::set_up_small_circle_workflow()
{
	//
	// Set up the 'small circle' canvas tools workflow.
	//

	Workflow small_circle_workflow = create_workflow(
			GPlatesGui::CanvasToolWorkflows::WORKFLOW_SMALL_CIRCLE,
			tr("Small Circle"),
			tab_small_circle,
			small_circle_toolbar_placeholder);

	add_tool_action_to_workflow(
			small_circle_workflow,
			GPlatesGui::CanvasToolWorkflows::TOOL_CREATE_SMALL_CIRCLE,
			action_Create_Small_Circle);
}

void GPlatesQtWidgets::CanvasToolBarDockWidget::set_up_hellinger_workflow()
{
	//
	// Set up the 'hellinger' canvas tools workflow.
	//

	Workflow hellinger_workflow = create_workflow(
			GPlatesGui::CanvasToolWorkflows::WORKFLOW_HELLINGER,
			tr("Hellinger"),
			tab_hellinger,
			hellinger_toolbar_placeholder);

	add_tool_action_to_workflow(
			hellinger_workflow,
			GPlatesGui::CanvasToolWorkflows::TOOL_SELECT_HELLINGER_GEOMETRIES,
				action_Select_Hellinger_Geometries);

	add_tool_action_to_workflow(
				hellinger_workflow,
				GPlatesGui::CanvasToolWorkflows::TOOL_ADJUST_FITTED_POLE_ESTIMATE,
				action_Adjust_Pole_Estimate);
}


GPlatesQtWidgets::CanvasToolBarDockWidget::Workflow
GPlatesQtWidgets::CanvasToolBarDockWidget::create_workflow(
		GPlatesGui::CanvasToolWorkflows::WorkflowType workflow_type,
		const QString &workflow_menu_name,
		QWidget *tab_widget,
		QWidget *tool_bar_placeholder_widget)
{
	// Remove existing layout (if exists due to UI designer) - delete(NULL) is a no-op.
	delete tool_bar_placeholder_widget->layout();

	// Create a new layout.
	QVBoxLayout *canvas_tools_layout = new QVBoxLayout(tool_bar_placeholder_widget);
	canvas_tools_layout->setSpacing(0);
	canvas_tools_layout->setContentsMargins(0, 0, 0, 0);

	// Get the tab/page index of the current workflow in the QTabWidget.
	const int tab_index = tab_widget_canvas_tools->indexOf(tab_widget);
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			tab_index >= 0,
			GPLATES_ASSERTION_SOURCE);

	// Create the current tools workflow.
	Workflow workflow(workflow_type, workflow_menu_name, tool_bar_placeholder_widget, tab_index);
	workflow.tool_bar->setOrientation(Qt::Vertical);
	workflow.tool_bar->setIconSize(d_tool_icon_regular_size);
	canvas_tools_layout->addWidget(workflow.tool_bar);

	// Add to the list of all workflows.
	d_workflows.push_back(workflow);

	return workflow;
}


void
GPlatesQtWidgets::CanvasToolBarDockWidget::add_tool_action_to_workflow(
		Workflow &workflow,
		GPlatesGui::CanvasToolWorkflows::ToolType tool,
		const QAction *original_tool_action)
{
	// Create a copy of the tool action.
	// We do this because the same tool can be used in multiple workflows and within each
	// workflow only one action can be checked at any time and this requires a separate QAction
	// instance for each workflow (for the same tool).
	QAction *tool_action = new QAction(
			original_tool_action->icon(),
			original_tool_action->text(),
			original_tool_action->parent());
	tool_action->setCheckable(original_tool_action->isCheckable());
	tool_action->setFont(original_tool_action->font());
	tool_action->setToolTip(original_tool_action->toolTip());

	// Set the shortcut to be active when any applications windows are active. This is necessary
	// because canvas tools are in a dock widget which can be separated from the main window.
	tool_action->setShortcutContext(Qt::ApplicationShortcut);

	// NOTE: We can't have the same shortcut for two or more QActions which can occur when
	// the same tool type is used by multiple workflows.
	// To get around this we will use the shortcuts on the original (unique) QActions but those QAction
	// instances won't be visible - instead, when they are triggered by a keyboard shortcut, we will
	// determine which workflow is currently active to determine which of the  multiple canvas tools
	// (using that same shortcut) we should target.
	// This will be done in "set_up_canvas_tool_shortcuts()".
#if 0
	tool_action->setShortcut(original_tool_action->shortcut());
#endif

	// Add to the workflow tool bar.
	workflow.tool_bar->addAction(tool_action);

	// We only want one canvas tool action, within a workflow, to be checked at any time.
	workflow.action_group->addAction(tool_action);

	// Set some data on the QAction so we know which workflow/tool it corresponds to when triggered.
	QVector<QVariant> tool_action_data_list(2); // A vector of size 2.
	tool_action_data_list[TOOL_ACTION_DATA_LIST_WORKFLOW_INDEX].setValue<uint>(workflow.workflow_type);
	tool_action_data_list[TOOL_ACTION_DATA_LIST_TOOL_INDEX].setValue<uint>(tool);
	tool_action->setData(QVariant(tool_action_data_list.toList()));

	// Call handler when action is triggered.
	QObject::connect(
			tool_action, SIGNAL(triggered()),
			this, SLOT(handle_tool_action_triggered()));
}


void
GPlatesQtWidgets::CanvasToolBarDockWidget::set_up_workflow_tab_icons()
{
	// Set the icon size in each toolbar (in each group tab).
	for (unsigned int workflow_index = 0; workflow_index < d_workflows.size(); ++workflow_index)
	{
		// Get the current workflow's tab.
		const int workflow_tab_index = d_workflows[workflow_index].tab_index;

		// Get the current icon size.
		const QSize tool_icon_size = tab_widget_canvas_tools->iconSize();

		// Get the enabled/disabled icon pixmap from the existing workflow tab icon.
		const QPixmap un_rotated_icon_pixmap =
				tab_widget_canvas_tools->tabIcon(workflow_tab_index).pixmap(tool_icon_size, QIcon::Normal);

		// Rotate the pixmap 90 degree clockwise.
		// We need to do this because the tab widget is vertical instead of horizontal and so placing
		// icons on the tabs has the effect of rotating them 90 degrees counter-clockwise.
		// So we need to undo that effect.
		QTransform rotate_90_degree_clockwise;
		rotate_90_degree_clockwise.rotate(90);
		const QPixmap rotated_icon_pixmap = un_rotated_icon_pixmap.transformed(rotate_90_degree_clockwise);

		// The rotated icon.
		const QIcon tab_icon(rotated_icon_pixmap);
		
		// Set the icon back onto the tab widget.
		tab_widget_canvas_tools->setTabIcon(workflow_tab_index, tab_icon);
	}
}


void
GPlatesQtWidgets::CanvasToolBarDockWidget::set_up_canvas_tool_shortcuts()
{
	// Handle canvas tool shortcuts separately from their equivalent QActions.
	// This is because we can't have the same shortcut for two or more QActions -
	// which can occur when the same tool type is used by multiple workflows.
	// To get around this we will use the shortcuts on the original (unique) QActions but they
	// won't be visible - instead, when they are triggered by a keyboard shortcut, we will determine
	// which workflow is currently active to determine which of the  multiple canvas tools
	// (using that same shortcut) we should target.
	add_canvas_tool_shortcut(GPlatesGui::CanvasToolWorkflows::TOOL_DRAG_GLOBE, action_Drag_Globe);
	add_canvas_tool_shortcut(GPlatesGui::CanvasToolWorkflows::TOOL_ZOOM_GLOBE, action_Zoom_Globe);
#if 0 // Disable lighting tool until volume visualisation is officially released (in GPlates 1.5)...
	add_canvas_tool_shortcut(GPlatesGui::CanvasToolWorkflows::TOOL_CHANGE_LIGHTING, action_Change_Lighting);
#endif
	add_canvas_tool_shortcut(GPlatesGui::CanvasToolWorkflows::TOOL_MEASURE_DISTANCE, action_Measure_Distance);
	add_canvas_tool_shortcut(GPlatesGui::CanvasToolWorkflows::TOOL_CLICK_GEOMETRY, action_Click_Geometry);
	add_canvas_tool_shortcut(GPlatesGui::CanvasToolWorkflows::TOOL_DIGITISE_NEW_POLYLINE, action_Digitise_New_Polyline);
	add_canvas_tool_shortcut(GPlatesGui::CanvasToolWorkflows::TOOL_DIGITISE_NEW_MULTIPOINT, action_Digitise_New_MultiPoint);
	add_canvas_tool_shortcut(GPlatesGui::CanvasToolWorkflows::TOOL_DIGITISE_NEW_POLYGON, action_Digitise_New_Polygon);
	add_canvas_tool_shortcut(GPlatesGui::CanvasToolWorkflows::TOOL_MOVE_VERTEX, action_Move_Vertex);
	add_canvas_tool_shortcut(GPlatesGui::CanvasToolWorkflows::TOOL_DELETE_VERTEX, action_Delete_Vertex);
	add_canvas_tool_shortcut(GPlatesGui::CanvasToolWorkflows::TOOL_INSERT_VERTEX, action_Insert_Vertex);
	add_canvas_tool_shortcut(GPlatesGui::CanvasToolWorkflows::TOOL_SPLIT_FEATURE, action_Split_Feature);
	add_canvas_tool_shortcut(GPlatesGui::CanvasToolWorkflows::TOOL_MANIPULATE_POLE, action_Manipulate_Pole);
	add_canvas_tool_shortcut(GPlatesGui::CanvasToolWorkflows::TOOL_MOVE_POLE, action_Move_Pole);
	add_canvas_tool_shortcut(GPlatesGui::CanvasToolWorkflows::TOOL_SELECT_HELLINGER_GEOMETRIES, action_Select_Hellinger_Geometries);
	add_canvas_tool_shortcut(GPlatesGui::CanvasToolWorkflows::TOOL_ADJUST_FITTED_POLE_ESTIMATE, action_Adjust_Pole_Estimate);
	add_canvas_tool_shortcut(GPlatesGui::CanvasToolWorkflows::TOOL_BUILD_LINE_TOPOLOGY, action_Build_Line_Topology);
	add_canvas_tool_shortcut(GPlatesGui::CanvasToolWorkflows::TOOL_BUILD_BOUNDARY_TOPOLOGY, action_Build_Boundary_Topology);
	add_canvas_tool_shortcut(GPlatesGui::CanvasToolWorkflows::TOOL_BUILD_NETWORK_TOPOLOGY, action_Build_Network_Topology);
	add_canvas_tool_shortcut(GPlatesGui::CanvasToolWorkflows::TOOL_EDIT_TOPOLOGY, action_Edit_Topology);
	add_canvas_tool_shortcut(GPlatesGui::CanvasToolWorkflows::TOOL_CREATE_SMALL_CIRCLE, action_Create_Small_Circle);
}


void
GPlatesQtWidgets::CanvasToolBarDockWidget::add_canvas_tool_shortcut(
		GPlatesGui::CanvasToolWorkflows::ToolType tool,
		QAction *shortcut_tool_action)
{
	// Add the original QAction to the tab widget just so it becomes active
	// (since the tab widget is always visible).
	// NOTE: There's no way for the user to select these actions other than through shortcuts.
	// Each workflow has its own *copy* of these actions that the user can click on in the
	// tabbed toolbar or select via the main menu.
	tab_widget_canvas_tools->addAction(shortcut_tool_action);

	// Set some data on the QAction so we know which tool it corresponds to when triggered.
	shortcut_tool_action->setData(static_cast<uint>(tool));

	// Call handler when action is triggered.
	QObject::connect(
			shortcut_tool_action, SIGNAL(triggered()),
			this, SLOT(handle_tool_shortcut_triggered()));
}


void
GPlatesQtWidgets::CanvasToolBarDockWidget::set_up_canvas_workflow_shortcuts()
{
	// Use keys 0-9 for the workflows.
	unsigned int tool_workflow = 0;

	for ( ; tool_workflow < GPlatesGui::CanvasToolWorkflows::NUM_WORKFLOWS; ++tool_workflow)
	{
		const GPlatesGui::CanvasToolWorkflows::WorkflowType canvas_tool_workflow =
				static_cast<GPlatesGui::CanvasToolWorkflows::WorkflowType>(tool_workflow);

		add_canvas_workflow_shortcut(
				canvas_tool_workflow,
				QKeySequence(static_cast<Qt::Key>(Qt::Key_1 + tool_workflow)));
	}

	// We're expecting keys 0-9 to be enough for all workflows.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			tool_workflow < 10,
			GPLATES_ASSERTION_SOURCE);
}


void
GPlatesQtWidgets::CanvasToolBarDockWidget::add_canvas_workflow_shortcut(
		GPlatesGui::CanvasToolWorkflows::WorkflowType workflow,
		const QKeySequence &shortcut_key_sequence)
{
	// Create a QAction around the specified shortcut key sequence.
	QAction *shortcut_workflow_action = new QAction(this);
	shortcut_workflow_action->setShortcut(shortcut_key_sequence);
	// Set the shortcut to be active when any applications windows are active. This is necessary
	// because canvas tools are in a dock widget which can be separated from the main window.
	shortcut_workflow_action->setShortcutContext(Qt::ApplicationShortcut);

	// Add the shortcut QAction to the tab widget just so it becomes active
	// (since the tab widget is always visible).
	// NOTE: There's no way for the user to select these actions other than through shortcuts.
	// The user can, however, also still click on the tabs in the tab widget to select different workflows.
	tab_widget_canvas_tools->addAction(shortcut_workflow_action);

	// Set some data on the QAction so we know which workflow it corresponds to when triggered.
	shortcut_workflow_action->setData(static_cast<uint>(workflow));

	// Call handler when action is triggered.
	QObject::connect(
			shortcut_workflow_action, SIGNAL(triggered()),
			this, SLOT(handle_workflow_shortcut_triggered()));
}


QString
GPlatesQtWidgets::CanvasToolBarDockWidget::get_workflow_tool_menu_name(
		GPlatesGui::CanvasToolWorkflows::WorkflowType workflow) const
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			static_cast<unsigned int>(workflow) < d_workflows.size(),
			GPLATES_ASSERTION_SOURCE);

	return d_workflows[workflow].menu_name;
}


QList<QAction *>
GPlatesQtWidgets::CanvasToolBarDockWidget::get_workflow_tool_menu_actions(
		GPlatesGui::CanvasToolWorkflows::WorkflowType workflow) const
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			static_cast<unsigned int>(workflow) < d_workflows.size(),
			GPLATES_ASSERTION_SOURCE);

	return d_workflows[workflow].action_group->actions();
}


void
GPlatesQtWidgets::CanvasToolBarDockWidget::use_small_canvas_tool_icons(
		bool use_small_icons)
{
	if (use_small_icons)
	{
		set_icon_size(QSize(16, 16));
	}
	else
	{
		set_icon_size(d_tool_icon_regular_size);
	}
}


void
GPlatesQtWidgets::CanvasToolBarDockWidget::set_icon_size(
		const QSize &tool_icon_size)
{
	// Set the size of icons in the tabs.
	tab_widget_canvas_tools->setIconSize(tool_icon_size);

	// Set the icon size in each toolbar (in each group tab).
	for (unsigned int workflow_index = 0; workflow_index < d_workflows.size(); ++workflow_index)
	{
		d_workflows[workflow_index].tool_bar->setIconSize(tool_icon_size);
	}
}


void
GPlatesQtWidgets::CanvasToolBarDockWidget::handle_tool_action_triggered()
{
	// Get the QObject that triggered this slot.
	QObject *signal_sender = sender();
	// Return early in case this slot not activated by a signal - shouldn't happen.
	if (!signal_sender)
	{
		return;
	}

	// Cast to a QAction since only QAction objects trigger this slot.
	QAction *tool_action = qobject_cast<QAction *>(signal_sender);
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			tool_action,
			GPLATES_ASSERTION_SOURCE);

	// Determine which workflow/tool to activate.
	const std::pair<
			GPlatesGui::CanvasToolWorkflows::WorkflowType,
			GPlatesGui::CanvasToolWorkflows::ToolType>
					canvas_workflow_and_tool =
							get_workflow_tool_from_action(tool_action);

	// Select the new canvas tool.
	// This was caused by clicking a tool action by the user (versus an automatic tool conversion
	// by some code in GPlates that wishes to change the canvas tool).
	choose_canvas_tool_selected_by_user(canvas_workflow_and_tool.first, canvas_workflow_and_tool.second);
}


void
GPlatesQtWidgets::CanvasToolBarDockWidget::handle_tool_shortcut_triggered()
{
	// Get the QObject that triggered this slot.
	QObject *signal_sender = sender();
	// Return early in case this slot not activated by a signal - shouldn't happen.
	if (!signal_sender)
	{
		return;
	}

	// Cast to a QAction since only QAction objects trigger this slot.
	QAction *shortcut_tool_action = qobject_cast<QAction *>(signal_sender);
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			shortcut_tool_action,
			GPLATES_ASSERTION_SOURCE);

	// Determine the tool type to activate.
	// Note that the *shortcut* tool action stores only the tool type in the QAction and not the
	// workflow (because the shortcut could apply to any workflow containing that tool type).
	const GPlatesGui::CanvasToolWorkflows::ToolType tool =
			static_cast<GPlatesGui::CanvasToolWorkflows::ToolType>(
					shortcut_tool_action->data().toUInt());

	// The shortcut applies, by default, to the tool in the currently active workflow
	// (if it exists in the workflow).
	const GPlatesGui::CanvasToolWorkflows::WorkflowType active_workflow =
			d_canvas_tool_workflows.get_active_canvas_tool().first;

	// We will search all workflows for the tool (starting with the active workflow).
	std::deque<GPlatesGui::CanvasToolWorkflows::WorkflowType> workflows;
	for (unsigned int tool_workflow = 0;
		tool_workflow < GPlatesGui::CanvasToolWorkflows::NUM_WORKFLOWS;
		++tool_workflow)
	{
		const GPlatesGui::CanvasToolWorkflows::WorkflowType workflow =
				static_cast<GPlatesGui::CanvasToolWorkflows::WorkflowType>(tool_workflow);

		if (workflow == active_workflow)
		{
			// Make sure the active workflow is at the start of the list.
			workflows.push_front(workflow);
		}
		else
		{
			// Inactive workflows get second priority and are searched in the order they are listed.
			workflows.push_back(workflow);
		}
	}

	// Iterate over all the workflows (starting with the active workflow).
	BOOST_FOREACH(GPlatesGui::CanvasToolWorkflows::WorkflowType workflow, workflows)
	{
		// See if the current workflow even has the tool (corresponding to the shortcut).
		// For example there is not a "Choose Feature F" tool in the digitisation workflow.
		if (!d_canvas_tool_workflows.does_workflow_contain_tool(workflow, tool))
		{
			// Try the next workflow in the list.
			continue;
		}

		// See if the canvas tool is enabled.
		// Only select the tool if it is currently enabled.
		if (!get_tool_action(workflow, tool)->isEnabled())
		{
			// Try the next workflow in the list.
			continue;
		}

		// Select the new canvas tool.
		// This was caused by the user selecting a tool shortcut (versus an automatic tool
		// conversion by some code in GPlates that wishes to change the canvas tool).
		choose_canvas_tool_selected_by_user(workflow, tool);

		break;
	}
}


void
GPlatesQtWidgets::CanvasToolBarDockWidget::handle_workflow_shortcut_triggered()
{
	// Get the QObject that triggered this slot.
	QObject *signal_sender = sender();
	// Return early in case this slot not activated by a signal - shouldn't happen.
	if (!signal_sender)
	{
		return;
	}

	// Cast to a QAction since only QAction objects trigger this slot.
	QAction *shortcut_workflow_action = qobject_cast<QAction *>(signal_sender);
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			shortcut_workflow_action,
			GPLATES_ASSERTION_SOURCE);

	// Determine the workflow to activate.
	// Note that the *shortcut* workflow action stores only the workflow type (and not the tool) in the QAction.
	const GPlatesGui::CanvasToolWorkflows::WorkflowType workflow =
			static_cast<GPlatesGui::CanvasToolWorkflows::WorkflowType>(
					shortcut_workflow_action->data().toUInt());

	// Select the tab of the QTabWidget corresponding to the workflow.
	// If this is the same as the currently active workflow then nothing will happen, otherwise
	// the 'handle_workflow_tab_changed()' slot will get called.
	tab_widget_canvas_tools->setCurrentIndex(d_workflows[workflow].tab_index);
}


void
GPlatesQtWidgets::CanvasToolBarDockWidget::handle_workflow_tab_changed(
		int workflow_tab_index)
{
	// Find the workflow given the workflow tab index in the QTabWidget.
	Workflow *workflow = NULL;
	// Search all workflows.
	BOOST_FOREACH(Workflow &workflow_info, d_workflows)
	{
		if (workflow_tab_index == workflow_info.tab_index)
		{
			workflow = &workflow_info;
			break;
		}
	}

	// Assert that the workflow was found.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			workflow,
			GPLATES_ASSERTION_SOURCE);

	// Select the new canvas tool (the current tool in the new workflow).
	// This was caused by a tab change by the user (versus an automatic tool conversion
	// by some code in GPlates that wishes to change the canvas tool).
	choose_canvas_tool_selected_by_user(workflow->workflow_type);
}


void
GPlatesQtWidgets::CanvasToolBarDockWidget::handle_canvas_tool_enabled(
		GPlatesGui::CanvasToolWorkflows::WorkflowType workflow,
		GPlatesGui::CanvasToolWorkflows::ToolType tool,
		bool enable)
{
	// Find the tool action corresponding to the specified workflow/tool.
	QAction *tool_action = get_tool_action(workflow, tool);

	// Enable or disable the tool action.
	tool_action->setEnabled(enable);

	// We no longer copy the icon of the selected canvas tool to the workflow tab.
#if 0
	// If the tool is the selected tool in its workflow then copy its icon to its workflow tab.
	if (d_canvas_tool_workflows.get_selected_canvas_tool_in_workflow(workflow) == tool)
	{
		copy_canvas_tool_icon_to_workflow_tab(workflow, tool);
	}
#endif
}


void
GPlatesQtWidgets::CanvasToolBarDockWidget::handle_canvas_tool_activated(
		GPlatesGui::CanvasToolWorkflows::WorkflowType workflow,
		GPlatesGui::CanvasToolWorkflows::ToolType tool)
{
	// Find the tool action corresponding to the specified workflow/tool.
	QAction *tool_action = get_tool_action(workflow, tool);

	// Make sure the action is checked so that the toolbar icon shows as checked.
	// This is in case the user didn't select the canvas tool using the action - for example
	// if the tool was activated in an undo command.
	//
	// NOTE: This does not cause the triggered signal to be emitted.
	tool_action->setChecked(true);

	// Change the workflow tab to reflect the chosen workflow.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			static_cast<unsigned int>(workflow) < d_workflows.size(),
			GPLATES_ASSERTION_SOURCE);

	// NOTE: We avoid recursion by temporarily disconnecting from our 'handle_workflow_tab_changed' slot.
	// Also we don't want to emit the 'canvas_tool_triggered_by_user' signal by calling this because
	// the canvas tool may have been activated automatically by GPlates (and not explicitly by the user).
	connect_to_workflow_tab_changed(false);
	tab_widget_canvas_tools->setCurrentIndex(d_workflows[workflow].tab_index);
	connect_to_workflow_tab_changed(true);

	// We no longer copy the icon of the selected canvas tool to the workflow tab.
#if 0
	// Copy the icon of the newly activated canvas tool to its workflow tab.
	copy_canvas_tool_icon_to_workflow_tab(workflow, tool);
#endif
}


QAction *
GPlatesQtWidgets::CanvasToolBarDockWidget::get_tool_action(
		GPlatesGui::CanvasToolWorkflows::WorkflowType workflow,
		GPlatesGui::CanvasToolWorkflows::ToolType tool)
{
	// Find the tool action corresponding to the specified workflow/tool.
	QAction *tool_action = NULL;
	// Search all workflows.
	BOOST_FOREACH(Workflow &workflow_info, d_workflows)
	{
		QList<QAction *> workflow_tool_actions = workflow_info.action_group->actions();

		// Search all tool actions in the current workflow.
		Q_FOREACH(QAction *workflow_tool_action, workflow_tool_actions)
		{
			if (is_tool_action(workflow_tool_action, workflow, tool))
			{
				tool_action = workflow_tool_action;
				break;
			}
		}

		if (tool_action)
		{
			break;
		}
	}

	// Assert that a tool action was found.
	// If one was not found then an attempt was made to activate a tool that doesn't belong
	// in a workflow (not all workflows support all tools).
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			tool_action,
			GPLATES_ASSERTION_SOURCE);

	return tool_action;
}


void
GPlatesQtWidgets::CanvasToolBarDockWidget::choose_canvas_tool_selected_by_user(
		GPlatesGui::CanvasToolWorkflows::WorkflowType workflow,
		boost::optional<GPlatesGui::CanvasToolWorkflows::ToolType> tool)
{
	// The canvas tool before activating new tool.
	const std::pair<
			GPlatesGui::CanvasToolWorkflows::WorkflowType,
			GPlatesGui::CanvasToolWorkflows::ToolType>
					active_canvas_tool_before_selection =
							d_canvas_tool_workflows.get_active_canvas_tool();

	// Activate the user-triggered canvas tool.
	d_canvas_tool_workflows.choose_canvas_tool(workflow, tool);

	// The canvas tool after activating new tool.
	const std::pair<
			GPlatesGui::CanvasToolWorkflows::WorkflowType,
			GPlatesGui::CanvasToolWorkflows::ToolType>
					active_canvas_tool_after_selection =
							d_canvas_tool_workflows.get_active_canvas_tool();

	// Don't emit a signal if the canvas tool hasn't changed (it should change but just in case).
	// It should change since the user shouldn't be able to click the currently selected tool.
	if (active_canvas_tool_after_selection != active_canvas_tool_before_selection)
	{
		// Let any interested clients know that the canvas tool was triggered explicitly through
		// a tool action rather than an automatic canvas tool selection.
		Q_EMIT canvas_tool_triggered_by_user(
				active_canvas_tool_after_selection.first,
				active_canvas_tool_after_selection.second);
	}
}


void
GPlatesQtWidgets::CanvasToolBarDockWidget::connect_to_workflow_tab_changed(
		bool connect_to_workflow)
{
	if (connect_to_workflow)
	{
		QObject::connect(
				tab_widget_canvas_tools,
				SIGNAL(currentChanged(int)),
				this,
				SLOT(handle_workflow_tab_changed(int)));
	}
	else
	{
		QObject::disconnect(
				tab_widget_canvas_tools,
				SIGNAL(currentChanged(int)),
				this,
				SLOT(handle_workflow_tab_changed(int)));
	}
}


GPlatesQtWidgets::CanvasToolBarDockWidget::Workflow::Workflow(
		GPlatesGui::CanvasToolWorkflows::WorkflowType workflow_type_,
		const QString &menu_name_,
		QWidget *tool_bar_placeholder_widget_,
		int tab_index_) :
	workflow_type(workflow_type_),
	menu_name(menu_name_),
	tool_bar(new QToolBar(tool_bar_placeholder_widget_)),
	action_group(new QActionGroup(tool_bar_placeholder_widget_)),
	tab_index(tab_index_)
{
}
