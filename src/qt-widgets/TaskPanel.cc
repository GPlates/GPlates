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
#include "ActionButtonBox.h"
#include "gui/FeatureFocus.h"


GPlatesQtWidgets::TaskPanel::TaskPanel(
		GPlatesGui::FeatureFocus &feature_focus_,
		QWidget *parent_):
	QWidget(parent_),
	d_feature_action_button_box_ptr(new ActionButtonBox(5, 22, this)),
	d_digitisation_widget_ptr(new DigitisationWidget())
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
	
	// Set up the EX-TREME Task Panel's tabs.
	set_up_feature_tab(feature_focus_);
	set_up_digitisation_tab();
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
	lay->addWidget(d_feature_action_button_box_ptr);
	
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
	lay->addWidget(d_digitisation_widget_ptr);

	// After the action buttons, a spacer to eat up remaining space and push all
	// the widgets to the top of the Digitisation tab.
	lay->addItem(new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Expanding));
}


