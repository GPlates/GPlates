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
#include "MoveVertexWidget.h"
#include "ReconstructionPoleWidget.h"
#include "CreateTopologyWidget.h"
#include "ActionButtonBox.h"
#include "gui/FeatureFocus.h"


GPlatesQtWidgets::TaskPanel::TaskPanel(
		GPlatesGui::FeatureFocus &feature_focus,
		GPlatesModel::ModelInterface &model_interface,
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
		GPlatesViewOperations::RenderedGeometryFactory &rendered_geom_factory,
		GPlatesViewOperations::GeometryBuilder &digitise_geometry_builder,
		GPlatesViewOperations::GeometryBuilderToolTarget &geom_builder_tool_target,
		ViewportWindow &view_state,
		GPlatesGui::ChooseCanvasTool &choose_canvas_tool,
		QWidget *parent_):
	QWidget(parent_),
	d_feature_action_button_box_ptr(new ActionButtonBox(5, 22, this)),
	d_digitisation_widget_ptr(
		new DigitisationWidget(
			model_interface, digitise_geometry_builder, view_state, choose_canvas_tool)),
	d_move_vertex_widget_ptr(
		new MoveVertexWidget(
			geom_builder_tool_target)),
	d_plate_closure_widget_ptr(
		new PlateClosureWidget(
			rendered_geom_collection, rendered_geom_factory, 
			feature_focus, model_interface, view_state)),
	d_reconstruction_pole_widget_ptr(
		new ReconstructionPoleWidget(
			rendered_geom_collection, rendered_geom_factory, view_state))
{
	// Note that the ActionButtonBox uses 22x22 icons. This equates to a QToolButton
	// 32 pixels wide (and 31 high, for some reason) on Linux/Qt/Plastique. Including
	// the gap between icons, this means you need to increase the width of the Task
	// Panel by 34 pixels if you want to add another column of buttons.
	// Obviously on some platforms these pixel measurements might not be accurate;
	// Qt should still manage to arrange things tastefully though.
	setupUi(this);
	
// FIXME: remove this #if 0 endif 
#if 0
	// Prevent the user from clicking tabs directly; instead, gently encourage them
	// to select the appropriate CanvasTool for the job.
	tabwidget_task_panel->setTabEnabled(0, false);
	tabwidget_task_panel->setTabEnabled(1, false);
	tabwidget_task_panel->setTabEnabled(2, false);
	tabwidget_task_panel->setTabEnabled(3, false);
#endif
	
	// Set up the EX-TREME Task Panel's tabs.
	set_up_feature_tab(feature_focus);
	set_up_digitisation_tab();
	set_up_move_vertex_tab();
	set_up_modify_pole_tab();
	set_up_plate_closure_tab();
	
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
	QVBoxLayout *lay = new QVBoxLayout(tab_digitisation);
	lay->setSpacing(2);
	lay->setContentsMargins(2, 2, 2, 2);
	
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
GPlatesQtWidgets::TaskPanel::set_up_move_vertex_tab()
{
	// Set up the layout to be used by the Move Vertex tab.
	QVBoxLayout *lay = new QVBoxLayout(tab_move_vertex);
	lay->setSpacing(2);
	lay->setContentsMargins(2, 2, 2, 2);
	
	// Add a summary of the current geometry being modified by move vertex tool.
	// As usual, Qt will take ownership of memory so we don't have to worry.
	// We cannot set this parent widget in the TaskPanel initialiser list because
	// setupUi() has not been called yet.
	lay->addWidget(d_move_vertex_widget_ptr);

	// After the main widget and anything else we might want to cram in there,
	// a spacer to eat up remaining space and push all the widgets to the top
	// of the Move Vertex tab.
	lay->addItem(new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Expanding));
}


void
GPlatesQtWidgets::TaskPanel::set_up_modify_pole_tab()
{
	// Set up the layout to be used by the Modify Pole tab.
	QVBoxLayout *lay = new QVBoxLayout(tab_modify_pole);
	lay->setSpacing(2);
	lay->setContentsMargins(2, 2, 2, 2);
	
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
GPlatesQtWidgets::TaskPanel::set_up_plate_closure_tab()
{
	// Set up the layout to be used by the Modify Pole tab.
	QVBoxLayout *lay = new QVBoxLayout(tab_topology_tool);
	lay->setSpacing(2);
	lay->setContentsMargins(2, 2, 2, 2);
	
	// Add the main ReconstructionPoleWidget.
	// As usual, Qt will take ownership of memory so we don't have to worry.
	// We cannot set this parent widget in the TaskPanel initialiser list because
	// setupUi() has not been called yet.
	lay->addWidget(d_plate_closure_widget_ptr);

	// After the main widget and anything else we might want to cram in there,
	// a spacer to eat up remaining space and push all the widgets to the top
	// of the tab.
	// lay->addItem(new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Expanding));
}

