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
#include "TaskPanel.h"

#include "FeatureSummaryWidget.h"
#include "DigitisationWidget.h"
#include "ModifyGeometryWidget.h"
#include "ReconstructionPoleWidget.h"
#include "ActionButtonBox.h"
#include "gui/FeatureFocus.h"


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
}



GPlatesQtWidgets::TaskPanel::TaskPanel(
		GPlatesGui::FeatureFocus &feature_focus,
		GPlatesModel::ModelInterface &model_interface,
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
		GPlatesViewOperations::GeometryBuilder &digitise_geometry_builder,
		GPlatesViewOperations::GeometryOperationTarget &geometry_operation_target,
		GPlatesViewOperations::ActiveGeometryOperation &active_geometry_operation,
		ViewportWindow &view_state,
		GPlatesGui::ChooseCanvasTool &choose_canvas_tool,
		QWidget *parent_):
	QWidget(parent_),
	d_feature_action_button_box_ptr(new ActionButtonBox(5, 22, this)),
	d_digitisation_widget_ptr(new DigitisationWidget(
			model_interface, digitise_geometry_builder, view_state, choose_canvas_tool)),
	d_modify_geometry_widget_ptr(new ModifyGeometryWidget(
			geometry_operation_target, active_geometry_operation)),
	d_reconstruction_pole_widget_ptr(new ReconstructionPoleWidget(
			rendered_geom_collection, view_state))
	d_build_topology_widget_ptr( new BuildTopologyWidget(
			rendered_geom_collection, rendered_geom_factory, feature_focus, model_interface, view_state)),
	d_edit_topology_widget_ptr( new EditTopologyWidget(
			rendered_geom_collection, rendered_geom_factory, feature_focus, model_interface, view_state)),
{
	// Note that the ActionButtonBox uses 22x22 icons. This equates to a QToolButton
	// 32 pixels wide (and 31 high, for some reason) on Linux/Qt/Plastique. Including
	// the gap between icons, this means you need to increase the width of the Task
	// Panel by 34 pixels if you want to add another column of buttons.
	// Obviously on some platforms these pixel measurements might not be accurate;
	// Qt should still manage to arrange things tastefully though.
	setupUi(this);
	
	// Prevent the user from clicking tabs directly; instead, gently encourage them
	// to select the appropriate CanvasTool for the job.
	tabwidget_task_panel->setTabEnabled(0, false);
	tabwidget_task_panel->setTabEnabled(1, false);
	tabwidget_task_panel->setTabEnabled(2, false);
	tabwidget_task_panel->setTabEnabled(3, false);
	
	// Set up the EX-TREME Task Panel's tabs.
	set_up_feature_tab(feature_focus);
	set_up_digitisation_tab();
	set_up_modify_geometry_tab();
	set_up_modify_pole_tab();
	set_up_build_topology_tab();
	set_up_edit_topology_tab();
	
	choose_feature_tab();
}


void
GPlatesQtWidgets::TaskPanel::set_up_feature_tab(
		GPlatesGui::FeatureFocus &feature_focus)
{
	// Set up the layout to be used by the Feature tab.
	QVBoxLayout *lay = new QVBoxLayout(tab_feature);
	lay->setSpacing(2);
	lay->setContentsMargins(2, 2, 2, 2);
	
	// Add a summary of the currently-focused Feature.
	// As usual, Qt will take ownership of memory so we don't have to worry.
	lay->addWidget(new FeatureSummaryWidget(feature_focus, tab_feature));
	
	// Action Buttons; these are added by ViewportWindow via
	// TaskPanel::feature_action_button_box().add_action().
	QHBoxLayout* ab_lay = new QHBoxLayout();
	// We need to include a spacer to keep the buttons left-aligned,
	// as we have less than five buttons right now.
	ab_lay->addWidget(d_feature_action_button_box_ptr);
	ab_lay->addItem(new QSpacerItem(10, 10, QSizePolicy::Expanding, QSizePolicy::Minimum));
	lay->addLayout(ab_lay);
	
	// After the action buttons, a spacer to eat up remaining space and push all
	// the widgets to the top of the Feature tab.
	lay->addItem(new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Expanding));
}


void
GPlatesQtWidgets::TaskPanel::set_up_digitisation_tab()
{
	// Set up the layout to be used by the Digitisation tab.
	QLayout *lay = add_default_layout(tab_digitisation);
	
	// Add a summary of the current geometry being digitised.
	// As usual, Qt will take ownership of memory so we don't have to worry.
	// We cannot set this parent widget in the TaskPanel initialiser list because
	// setupUi() has not been called yet.
	lay->addWidget(d_digitisation_widget_ptr);

	// After the main widget and anything else we might want to cram in there,
	// a spacer to eat up remaining space and push all the widgets to the top
	// of the Digitisation tab.
	lay->addItem(new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Expanding));
}


void
GPlatesQtWidgets::TaskPanel::set_up_modify_geometry_tab()
{
	// Set up the layout to be used by the Modify Geometry tab.
	QLayout *lay = add_default_layout(tab_modify_geometry);
	
	// Add a summary of the current geometry being modified by a modify geometry tool.
	// As usual, Qt will take ownership of memory so we don't have to worry.
	// We cannot set this parent widget in the TaskPanel initialiser list because
	// setupUi() has not been called yet.
	lay->addWidget(d_modify_geometry_widget_ptr);

	// After the main widget and anything else we might want to cram in there,
	// a spacer to eat up remaining space and push all the widgets to the top
	// of the Modify Geometry tab.
	lay->addItem(new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Expanding));
}


void
GPlatesQtWidgets::TaskPanel::set_up_modify_pole_tab()
{
	// Set up the layout to be used by the Modify Pole tab.
	QLayout *lay = add_default_layout(tab_modify_pole);
	
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
GPlatesQtWidgets::TaskPanel::set_up_build_topology_tab()
{
	// Set up the layout to be used by the Modify Pole tab.
	QVBoxLayout *lay = new QVBoxLayout(tab_build_topology);
	lay->setSpacing(2);
	lay->setContentsMargins(2, 2, 2, 2);
	
	// Add the main ReconstructionPoleWidget.
	// As usual, Qt will take ownership of memory so we don't have to worry.
	// We cannot set this parent widget in the TaskPanel initialiser list because
	// setupUi() has not been called yet.
	lay->addWidget(d_build_topology_widget_ptr);

	// After the main widget and anything else we might want to cram in there,
	// a spacer to eat up remaining space and push all the widgets to the top
	// of the tab.
	lay->addItem(new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Expanding));
}

void
GPlatesQtWidgets::TaskPanel::set_up_edit_topology_tab()
{
	// Set up the layout to be used by the Modify Pole tab.
	QVBoxLayout *lay = new QVBoxLayout(tab_edit_topology);
	lay->setSpacing(2);
	lay->setContentsMargins(2, 2, 2, 2);
	
	// Add the main ReconstructionPoleWidget.
	// As usual, Qt will take ownership of memory so we don't have to worry.
	// We cannot set this parent widget in the TaskPanel initialiser list because
	// setupUi() has not been called yet.
	lay->addWidget(d_edit_topology_widget_ptr);

	// After the main widget and anything else we might want to cram in there,
	// a spacer to eat up remaining space and push all the widgets to the top
	// of the tab.
	lay->addItem(new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Expanding));
}

