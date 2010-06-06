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

#include <QIcon>
#include <QPushButton>

#include "ManageFeatureCollectionsDialog.h"

#include "app-logic/ClassifyFeatureCollection.h"
#include "feature-visitors/FeatureClassifier.h"

#include "ManageFeatureCollectionsStateWidget.h"


GPlatesQtWidgets::ManageFeatureCollectionsStateWidget::ManageFeatureCollectionsStateWidget(
		GPlatesQtWidgets::ManageFeatureCollectionsDialog &feature_collections_dialog,
		GPlatesAppLogic::FeatureCollectionFileState::file_reference file_ref,
		bool active,
		bool enable,
		QWidget *parent_):
	QWidget(parent_),
	d_feature_collections_dialog(feature_collections_dialog),
	d_file_reference(file_ref)
{
	setupUi(this);

	// Set up slots for the state checkboxen. Note we listen for the 'clicked' signal,
	// NOT the 'toggled' signal, as the latter is emitted even when changing the
	// checkbox state programattically.
	QObject::connect(checkbox_active, SIGNAL(clicked(bool)),
			this, SLOT(handle_active_checked(bool)));
	// We also set up slots for the single-push buttons to the side of the checkboxes.
	QObject::connect(button_active, SIGNAL(clicked()),
			this, SLOT(handle_active_toggled()));

	update_state(active, enable);
}


void
GPlatesQtWidgets::ManageFeatureCollectionsStateWidget::update_state(
		bool active,
		bool enable)
{
	set_button_properties(active, enable);
}


void
GPlatesQtWidgets::ManageFeatureCollectionsStateWidget::handle_active_toggled()
{
	handle_active_checked( ! checkbox_active->isChecked());
}

void
GPlatesQtWidgets::ManageFeatureCollectionsStateWidget::handle_active_checked(
		bool checked)
{
	// Activate or deactivate the file.
	// This will cause ManageFeatureCollectionsDialog to attempt to activate/deactivate
	// the file through FeatureCollectionFileState and if successful it will get signaled
	// and call our 'update_state()' which will set the button properties
	// appropriately.
	d_feature_collections_dialog.set_state_for_file(this, checked);
}


void
GPlatesQtWidgets::ManageFeatureCollectionsStateWidget::set_button_properties(
		bool is_active,
		bool is_enabled)
{
	static const QIcon icon_active(":/globe_reconstructable_22.png");
	static const QIcon icon_inactive(":/globe_reconstructable_22.png");
	static const QString tooltip_active(tr("The file contains feature data in use by GPlates."));
	static const QString tooltip_inactive(tr("The file contains feature data not currently in use."));
	static const QString tooltip_disabled(tr("The file does not contain reconstructable feature data."));
	
	// Ensure the checkbox state is what it's supposed to be, as we have several ways
	// of setting it.
	checkbox_active->setChecked(is_active);
	
	// Update icons and tooltips.
	if (is_active) {
		button_active->setIcon(icon_active);
		button_active->setToolTip(tooltip_active);
		checkbox_active->setToolTip(tooltip_active);
	} else {
		button_active->setIcon(icon_inactive);
		button_active->setToolTip(tooltip_inactive);
		checkbox_active->setToolTip(tooltip_inactive);
	}
	if ( ! is_enabled) {
		button_active->setToolTip(tooltip_disabled);
		checkbox_active->setToolTip(tooltip_disabled);
	}
	
	// Disable/Enable the user's ability to change the 'checked' state, if we know
	// that there are no features of this type available anyway - which also acts
	// as a handy indicator of what is in the file.
	button_active->setEnabled(is_enabled);
	checkbox_active->setEnabled(is_enabled);
}
