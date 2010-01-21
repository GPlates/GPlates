/* $Id$ */

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


#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QLabel>
#include "TaskPanel.h"

#include "ActionButtonBox.h"
#include "DigitisationWidget.h"
#include "FeatureSummaryWidget.h"
#include "MeasureDistanceWidget.h"
#include "ModifyGeometryWidget.h"
#include "ReconstructionPoleWidget.h"
#include "TopologyToolsWidget.h"

#include "gui/FeatureFocus.h"
#include "presentation/ViewState.h"


namespace
{
	/**
	 * For use with set_up_xxxx_tab methods: Adds a standard vertical box
	 * layout to the taskpanel page, and returns it for convenience.
	 *
	 * You don't have to use this if you want to set up a TaskPanel page
	 * with some unusual layout - this function is just to help keep things
	 * consistent.
	 */
	QLayout *
	add_default_layout(
			QWidget* page)
	{
		// Set up the layout to be used by the tab.
		// The layout will be parented to the widget, and cleaned up by Qt.
		QVBoxLayout *lay = new QVBoxLayout(page);
		lay->setSpacing(2);
		lay->setContentsMargins(2, 2, 2, 2);
		return lay;
	}


	/**
	 * For use with set_up_xxxx_tab methods: Adds a new taskpanel page,
	 * and returns it for use.
	 *
	 * This version also adds a text label at the top of the layout; this
	 * is a temporary measure while transitioning away from the tab widget
	 * and to a new, cleaner stacked widget.
	 *
	 * Basically, I really really want to get the look of the task panel
	 * fixed up before the 0.9.7 release -JC
	 */
	QWidget *
	add_page_with_title(
			QStackedWidget* stacked_widget,
			const QString &title)
	{
		// Create the stack's page
		QWidget *stack_page = new QWidget(stacked_widget);
		stacked_widget->addWidget(stack_page);
		// Set up the layout to be used by the page itself, to keep the title at the top.
		QVBoxLayout *lay = new QVBoxLayout(stack_page);
		lay->setSpacing(2);
		lay->setContentsMargins(0, 0, 0, 0);
		
		// Add a label to make the title.
		QLabel *title_label = new QLabel(title, stack_page);
		title_label->setStyleSheet("border-bottom: 1px dotted palette(text); font: bold;");
		lay->addWidget(title_label);
		
		// Add an inner widget for the individual task panel stuff to use.
		// This widget will expand to fill as much space as it can politely take.
		QWidget *inner_widget = new QWidget(stack_page);
		QSizePolicy sizep(QSizePolicy::Preferred, QSizePolicy::Expanding);
		inner_widget->setSizePolicy(sizep);
		lay->addWidget(inner_widget);
		
		return inner_widget;
	}
}



GPlatesQtWidgets::TaskPanel::TaskPanel(
		GPlatesPresentation::ViewState &view_state,
		GPlatesViewOperations::GeometryBuilder &digitise_geometry_builder,
		GPlatesViewOperations::GeometryOperationTarget &geometry_operation_target,
		GPlatesViewOperations::ActiveGeometryOperation &active_geometry_operation,
		GPlatesCanvasTools::MeasureDistanceState &measure_distance_state,
		ViewportWindow &viewport_window,
		GPlatesGui::ChooseCanvasTool &choose_canvas_tool,
		QWidget *parent_):
	QWidget(parent_),
	d_stacked_widget_ptr(new QStackedWidget(this)),
	d_feature_action_button_box_ptr(new ActionButtonBox(5, 22, this)),
	d_digitisation_widget_ptr(new DigitisationWidget(
			digitise_geometry_builder, view_state, viewport_window, choose_canvas_tool)),
	d_modify_geometry_widget_ptr(new ModifyGeometryWidget(
			geometry_operation_target, active_geometry_operation)),
	d_reconstruction_pole_widget_ptr(new ReconstructionPoleWidget(
			view_state, viewport_window)),
	d_topology_tools_widget_ptr( new TopologyToolsWidget(
			view_state, viewport_window, choose_canvas_tool)),
	d_measure_distance_widget_ptr(new MeasureDistanceWidget(
			measure_distance_state))
{
	// Note that the ActionButtonBox uses 22x22 icons. This equates to a QToolButton
	// 32 pixels wide (and 31 high, for some reason) on Linux/Qt/Plastique. Including
	// the gap between icons, this means you need to increase the width of the Task
	// Panel by 34 pixels if you want to add another column of buttons.
	// Obviously on some platforms these pixel measurements might not be accurate;
	// Qt should still manage to arrange things tastefully though.
	set_up_ui();
	
	// Set up the EX-TREME Task Panel's tabs. Please ensure this order matches the
	// enumeration set up in the class declaration.
	set_up_feature_tab(view_state);
	set_up_digitisation_tab();
	set_up_modify_geometry_tab();
	set_up_modify_pole_tab();
	set_up_topology_tools_tab();
	set_up_measure_distance_tab();
	
	choose_feature_tab();
}


void
GPlatesQtWidgets::TaskPanel::set_up_ui()
{
	// Object name: important for F11 functionality.
	setObjectName("TaskPanel");
	// Set up UI in the same way the old TaskPanelUi.h used to, just in case there is
	// some subtle interaction between size policies etc.
	QSizePolicy sizep(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
	sizep.setHorizontalStretch(1);
	sizep.setVerticalStretch(0);
	setSizePolicy(sizep);
	setMinimumSize(QSize(183, 0));
	setMaximumSize(QSize(1024, 16777215));
	
	// Add the QStackedWidget as the centrepiece.
	QVBoxLayout *lay = new QVBoxLayout(this);
	lay->setSpacing(0);
	lay->setContentsMargins(0, 0, 0, 0);

	lay->addWidget(d_stacked_widget_ptr);
}


void
GPlatesQtWidgets::TaskPanel::set_up_feature_tab(
		GPlatesPresentation::ViewState &view_state)
{
	// Set up the layout to be used by the Feature tab.
	QWidget *page = add_page_with_title(d_stacked_widget_ptr, tr("Current Feature"));
	QLayout *lay = add_default_layout(page);
	
	// Add a summary of the currently-focused Feature.
	// As usual, Qt will take ownership of memory so we don't have to worry.
	lay->addWidget(new FeatureSummaryWidget(view_state, this));
	
	// Action Buttons; these are added by ViewportWindow via
	// TaskPanel::feature_action_button_box().add_action().
	QHBoxLayout* ab_lay = new QHBoxLayout();
	lay->addItem(ab_lay);
	// Ensure the button box understands the new parent. This one is finicky.
	d_feature_action_button_box_ptr->setParent(page);
	ab_lay->addWidget(d_feature_action_button_box_ptr);
	// We also need to include a spacer to keep the buttons left-aligned,
	// as we have less than five buttons right now.
	ab_lay->addItem(new QSpacerItem(10, 10, QSizePolicy::Expanding, QSizePolicy::Minimum));
	
	// After the action buttons, a spacer to eat up remaining space and push all
	// the widgets to the top of the Feature tab.
	lay->addItem(new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Expanding));
}


void
GPlatesQtWidgets::TaskPanel::set_up_digitisation_tab()
{
	// Set up the layout to be used by the Digitisation tab.
	QLayout *lay = add_default_layout(
			add_page_with_title(d_stacked_widget_ptr, tr("New Geometry")));
	
	// Add a summary of the current geometry being digitised.
	// As usual, Qt will take ownership of memory so we don't have to worry.
	// We cannot set this parent widget in the TaskPanel initialiser list because
	// setupUi() has not been called yet.
	lay->addWidget(d_digitisation_widget_ptr);

	// The Digitisation tab needs no spacer - give the table all the room it can use.
}


void
GPlatesQtWidgets::TaskPanel::set_up_modify_geometry_tab()
{
	// Set up the layout to be used by the Modify Geometry tab.
	QLayout *lay = add_default_layout(
			add_page_with_title(d_stacked_widget_ptr, tr("Modify Geometry")));
	
	// Add a summary of the current geometry being modified by a modify geometry tool.
	// As usual, Qt will take ownership of memory so we don't have to worry.
	// We cannot set this parent widget in the TaskPanel initialiser list because
	// setupUi() has not been called yet.
	lay->addWidget(d_modify_geometry_widget_ptr);

	// The Modify Geometry tab needs no spacer - give the table all the room it can use.
}


void
GPlatesQtWidgets::TaskPanel::set_up_modify_pole_tab()
{
	// Set up the layout to be used by the Modify Pole tab.
	QLayout *lay = add_default_layout(
			add_page_with_title(d_stacked_widget_ptr, tr("Reconstruction Pole")));
	
	// Add the main ReconstructionPoleWidget.
	// As usual, Qt will take ownership of memory so we don't have to worry.
	// We cannot set this parent widget in the TaskPanel initialiser list because
	// setupUi() has not been called yet.
	lay->addWidget(d_reconstruction_pole_widget_ptr);

	// After the main widget and anything else we might want to cram in there,
	// a spacer to eat up remaining space and push all the widgets to the top
	// of the Modify Pole tab.
	lay->addItem(new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Expanding));
}


void
GPlatesQtWidgets::TaskPanel::set_up_topology_tools_tab()
{
	// Set up the layout to be used by the Topology Tools tab.
	QLayout *lay = add_default_layout(
			add_page_with_title(d_stacked_widget_ptr, tr("Topology Tools")));
	
	// Add the main ReconstructionPoleWidget.
	// As usual, Qt will take ownership of memory so we don't have to worry.
	// We cannot set this parent widget in the TaskPanel initialiser list because
	// setupUi() has not been called yet.
	lay->addWidget(d_topology_tools_widget_ptr);

	// After the main widget and anything else we might want to cram in there,
	// a spacer to eat up remaining space and push all the widgets to the top
	// of the tab.
	lay->addItem(new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Expanding));
}

void
GPlatesQtWidgets::TaskPanel::set_up_measure_distance_tab()
{
	// Set up the layout to be used by the Measure Distance tab.
	QLayout *lay = add_default_layout(
			add_page_with_title(d_stacked_widget_ptr, tr("Measure Distance")));
	
	// Add the main ReconstructionPoleWidget.
	// As usual, Qt will take ownership of memory so we don't have to worry.
	// We cannot set this parent widget in the TaskPanel initialiser list because
	// setupUi() has not been called yet.
	lay->addWidget(d_measure_distance_widget_ptr);

	// After the main widget and anything else we might want to cram in there,
	// a spacer to eat up remaining space and push all the widgets to the top
	// of the Modify Pole tab.
	lay->addItem(new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Expanding));
}


