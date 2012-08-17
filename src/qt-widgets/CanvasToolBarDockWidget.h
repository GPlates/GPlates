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

#ifndef GPLATES_QT_WIDGETS_CANVASTOOLBARDOCKWIDGET_H
#define GPLATES_QT_WIDGETS_CANVASTOOLBARDOCKWIDGET_H

#include <vector>
#include <boost/optional.hpp>
#include <QAction>
#include <QActionGroup>
#include <QPointer>
#include <QSize>
#include <QToolBar>
#include <QWidget>

#include "CanvasToolBarDockWidgetUi.h"

#include "DockWidget.h"

#include "gui/CanvasToolWorkflows.h"


namespace GPlatesQtWidgets
{
	/**
	 * A tabbed canvas toolbar that groups canvas tools into 'workflows' such as digitisation, topology, etc.
	 */
	class CanvasToolBarDockWidget :
			public DockWidget, 
			protected Ui_CanvasToolBarDockWidget
	{
		Q_OBJECT

	public:

		explicit
		CanvasToolBarDockWidget(
				GPlatesGui::DockState &dock_state,
				GPlatesGui::CanvasToolWorkflows &canvas_tool_workflows,
				ViewportWindow &main_window,
				const QSize &tool_icon_size = QSize(35,35));


		/**
		 * Returns the name to use for the canvas tool sub-menu for the specified workflow.
		 *
		 * The name may also include a menu accelerator.
		 */
		QString
		get_workflow_tool_menu_name(
				GPlatesGui::CanvasToolWorkflows::WorkflowType workflow) const;

		/**
		 * Returns the list of tool actions, for the specified workflow, for use in a menu.
		 *
		 * NOTE: The returned actions are already connected internally such that when they are
		 * triggered they will invoke the appropriate canvas tool.
		 *
		 * This is used to add the canvas tools to the main menu as an alternative way for the user
		 * to invoke the tools (besides invoking directly from the tabbed tool bar).
		 */
		QList<QAction *>
		get_workflow_tool_menu_actions(
				GPlatesGui::CanvasToolWorkflows::WorkflowType workflow) const;

	signals:

		/**
		 * Emitted when a canvas tool action is triggered by the user (as opposed to automatically by GPlates).
		 *
		 * This is useful if you need to know that a canvas tool was explicitly selected when the user
		 * clicked on the tool action as opposed to an automatic canvas tool selection.
		 * An example is:
		 *     In particular, this is used to switch back to the Click Geometry tool
		 *     after the user clicks "Clone Geometry" and completes the "Create
		 *     Feature" dialog; this is because "Clone Geometry" automatically takes
		 *     the user to a digitisation tool.
		 * Otherwise you are probably better off listening to the *CanvasToolWorkflows* signal:
		 *    void
		 *    canvas_tool_activated(
		 *        GPlatesGui::CanvasToolWorkflows::WorkflowType workflow,
		 *        GPlatesGui::CanvasToolWorkflows::ToolType tool);
		 */
		void
		canvas_tool_triggered_by_user(
				GPlatesGui::CanvasToolWorkflows::WorkflowType workflow,
				GPlatesGui::CanvasToolWorkflows::ToolType tool);

	public slots:

		/**
		 * Use dimension 16 or 35 icons depending on @a use_small_icons.
		 */
		void
		use_small_canvas_tool_icons(
				bool use_small_icons);

		/**
		 * Sets the icon size for each tool bar tab.
		 */
		void
		set_icon_size(
				const QSize &icon_size);

	private slots:

		void
		handle_tool_action_triggered();

		void
		handle_tool_shortcut_triggered();

		void
		handle_workflow_tab_changed(
				int workflow_tab_index);

		void
		handle_canvas_tool_enabled(
				GPlatesGui::CanvasToolWorkflows::WorkflowType workflow,
				GPlatesGui::CanvasToolWorkflows::ToolType tool,
				bool enable);

		void
		handle_canvas_tool_activated(
				GPlatesGui::CanvasToolWorkflows::WorkflowType workflow,
				GPlatesGui::CanvasToolWorkflows::ToolType tool);

	private:

		/**
		 * Manages information for a specific canvas tool workflow.
		 */
		struct Workflow
		{
			explicit
			Workflow(
					GPlatesGui::CanvasToolWorkflows::WorkflowType workflow_type_,
					const QString &menu_name_,
					QWidget *tool_bar_placeholder_widget_,
					int tab_index_);

			GPlatesGui::CanvasToolWorkflows::WorkflowType workflow_type;

			//! The name for the workflow - can be used for tool sub-menu.
			QString menu_name;

			//! Each workflow has its own tool bar.
			QPointer<QToolBar> tool_bar;

			/**
			 * We only want one canvas tool action, within a workflow, to be checked at any time.
			 *
			 * This also serves as the list of actions on the tool bar.
			 */
			QPointer<QActionGroup> action_group;

			//! The page/tab index on the QTabWidget corresponding to this workflow.
			int tab_index;
		};


		/**
		 * Manages the canvas tool workflows and tool activation (and which tools are enabled).
		 */
		GPlatesGui::CanvasToolWorkflows &d_canvas_tool_workflows;

		//! A list of all workflows.
		std::vector<Workflow> d_workflows;

		//! The tool icon size to use when not using the small size;
		const QSize d_tool_icon_regular_size;


		void
		set_up_workflows();

		void
		set_up_feature_inspection_workflow();

		void
		set_up_digitisation_workflow();

		void
		set_up_topology_workflow();

		void
		set_up_pole_manipulation_workflow();

		void
		set_up_small_circle_workflow();

		Workflow
		create_workflow(
				GPlatesGui::CanvasToolWorkflows::WorkflowType workflow_type,
				const QString &workflow_menu_name,
				QWidget *tab_widget,
				QWidget *tool_bar_placeholder_widget);

		void
		add_tool_action_to_workflow(
				Workflow &workflow,
				GPlatesGui::CanvasToolWorkflows::ToolType tool,
				const QAction *tool_action);

		void
		set_up_canvas_tool_shortcuts();

		void
		add_canvas_tool_shortcut(
				GPlatesGui::CanvasToolWorkflows::ToolType tool,
				QAction *shortcut_tool_action);

		QAction *
		get_tool_action(
				GPlatesGui::CanvasToolWorkflows::WorkflowType workflow,
				GPlatesGui::CanvasToolWorkflows::ToolType tool);

		void
		choose_canvas_tool_selected_by_user(
				GPlatesGui::CanvasToolWorkflows::WorkflowType workflow,
				boost::optional<GPlatesGui::CanvasToolWorkflows::ToolType> tool = boost::none);

		void
		connect_to_workflow_tab_changed(
				bool connect_to_workflow = true);
	};
}

#endif // GPLATES_QT_WIDGETS_CANVASTOOLBARDOCKWIDGET_H
