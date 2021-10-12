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
		GPlatesAppLogic::FeatureCollectionFileState::file_iterator file_it,
		bool reconstructable_active,
		bool reconstruction_active,
		bool enable_reconstructable,
		bool enable_reconstruction,
		QWidget *parent_):
	QWidget(parent_),
	d_feature_collections_dialog(feature_collections_dialog),
	d_file_iterator(file_it)
{
	setupUi(this);

	// Set up slots for the state checkboxen. Note we listen for the 'clicked' signal,
	// NOT the 'toggled' signal, as the latter is emitted even when changing the
	// checkbox state programattically.
	QObject::connect(checkbox_active_reconstructable, SIGNAL(clicked(bool)),
			this, SLOT(handle_active_reconstructable_checked(bool)));
	QObject::connect(checkbox_active_reconstruction, SIGNAL(clicked(bool)),
			this, SLOT(handle_active_reconstruction_checked(bool)));
	// We also set up slots for the single-push buttons to the side of the checkboxes.
	QObject::connect(button_active_reconstructable, SIGNAL(clicked()),
			this, SLOT(handle_active_reconstructable_toggled()));
	QObject::connect(button_active_reconstruction, SIGNAL(clicked()),
			this, SLOT(handle_active_reconstruction_toggled()));

	update_reconstructable_state(reconstructable_active, enable_reconstructable);
	update_reconstruction_state(reconstruction_active, enable_reconstruction);
}


void
GPlatesQtWidgets::ManageFeatureCollectionsStateWidget::update_reconstructable_state(
		bool active,
		bool enable_reconstructable)
{
	set_reconstructable_button_properties(active, enable_reconstructable);
}


void
GPlatesQtWidgets::ManageFeatureCollectionsStateWidget::update_reconstruction_state(
		bool active,
		bool enable_reconstruction)
{
	set_reconstruction_button_properties(active, enable_reconstruction);
}


void
GPlatesQtWidgets::ManageFeatureCollectionsStateWidget::handle_active_reconstructable_toggled()
{
	handle_active_reconstructable_checked( ! checkbox_active_reconstructable->isChecked());
}

void
GPlatesQtWidgets::ManageFeatureCollectionsStateWidget::handle_active_reconstructable_checked(
		bool checked)
{
	// Activate or deactivate the file.
	// This will cause ManageFeatureCollectionsDialog to attempt to activate/deactivate
	// the file through FeatureCollectionFileState and if successful it will get signaled
	// and call our 'update_reconstructable_state()' which will set the button properties
	// appropriately.
	d_feature_collections_dialog.set_reconstructable_state_for_file(this, checked);
}


void
GPlatesQtWidgets::ManageFeatureCollectionsStateWidget::handle_active_reconstruction_toggled()
{
	handle_active_reconstruction_checked( ! checkbox_active_reconstruction->isChecked());
}

void
GPlatesQtWidgets::ManageFeatureCollectionsStateWidget::handle_active_reconstruction_checked(
		bool checked)
{
	// Activate or deactivate the file.
	// This will cause ManageFeatureCollectionsDialog to attempt to activate/deactivate
	// the file through FeatureCollectionFileState and if successful it will get signaled
	// and call our 'update_reconstruction_state()' which will set the button properties
	// appropriately.
	d_feature_collections_dialog.set_reconstruction_state_for_file(this, checked);
}


void
GPlatesQtWidgets::ManageFeatureCollectionsStateWidget::set_reconstructable_button_properties(
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
	checkbox_active_reconstructable->setChecked(is_active);
	
	// Update icons and tooltips.
	if (is_active) {
		button_active_reconstructable->setIcon(icon_active);
		button_active_reconstructable->setToolTip(tooltip_active);
		checkbox_active_reconstructable->setToolTip(tooltip_active);
	} else {
		button_active_reconstructable->setIcon(icon_inactive);
		button_active_reconstructable->setToolTip(tooltip_inactive);
		checkbox_active_reconstructable->setToolTip(tooltip_inactive);
	}
	if ( ! is_enabled) {
		button_active_reconstructable->setToolTip(tooltip_disabled);
		checkbox_active_reconstructable->setToolTip(tooltip_disabled);
	}
	
	// Disable/Enable the user's ability to change the 'checked' state, if we know
	// that there are no features of this type available anyway - which also acts
	// as a handy indicator of what is in the file.
	button_active_reconstructable->setEnabled(is_enabled);
	checkbox_active_reconstructable->setEnabled(is_enabled);
}


void
GPlatesQtWidgets::ManageFeatureCollectionsStateWidget::set_reconstruction_button_properties(
		bool is_active,
		bool is_enabled)
{
	static const QIcon icon_active(":/globe_reconstruction_22.png");
	static const QIcon icon_inactive(":/globe_reconstruction_22.png");
	static const QString tooltip_active(tr("The file contains reconstruction poles in use by GPlates."));
	static const QString tooltip_inactive(tr("The file contains reconstruction poles which are not in use."));
	static const QString tooltip_disabled(tr("The file does not contain reconstruction pole data."));

	// Ensure the checkbox state is what it's supposed to be, as we have several ways
	// of setting it.
	checkbox_active_reconstruction->setChecked(is_active);

	// Update icons and tooltips.
	if (is_active) {
		button_active_reconstruction->setIcon(icon_active);
		button_active_reconstruction->setToolTip(tooltip_active);
		checkbox_active_reconstruction->setToolTip(tooltip_active);
	} else {
		button_active_reconstruction->setIcon(icon_inactive);
		button_active_reconstruction->setToolTip(tooltip_inactive);
		checkbox_active_reconstruction->setToolTip(tooltip_inactive);
	}
	if ( ! is_enabled) {
		button_active_reconstruction->setToolTip(tooltip_disabled);
		checkbox_active_reconstruction->setToolTip(tooltip_disabled);
	}

	// Disable/Enable the user's ability to change the 'checked' state, if we know
	// that there are no features of this type available anyway - which also acts
	// as a handy indicator of what is in the file.
	button_active_reconstruction->setEnabled(is_enabled);
	checkbox_active_reconstruction->setEnabled(is_enabled);
}

