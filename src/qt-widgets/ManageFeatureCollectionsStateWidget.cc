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
#include "feature-visitors/FeatureCollectionClassifier.h"

#include "ManageFeatureCollectionsStateWidget.h"


GPlatesQtWidgets::ManageFeatureCollectionsStateWidget::ManageFeatureCollectionsStateWidget(
		GPlatesQtWidgets::ManageFeatureCollectionsDialog &feature_collections_dialog,
		GPlatesAppState::ApplicationState::file_info_iterator file_it,
		bool reconstructable_active,
		bool reconstruction_active,
		QWidget *parent_):
	QWidget(parent_),
	d_feature_collections_dialog(feature_collections_dialog),
	d_file_info_iterator(file_it)
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

	update_state(reconstructable_active, reconstruction_active);
}

void
GPlatesQtWidgets::ManageFeatureCollectionsStateWidget::update_state(
		bool reconstructable_active,
		bool reconstruction_active)
{
	// Get the FeatureCollection from the FileInfo and determine what kinds
	// of features it contains using the FeatureCollectionClassifier visitor,
	// then entirely disable the Reconstructable or Reconstruction buttons
	// accordingly.
	bool reconstructable_enabled = true;
	bool reconstruction_enabled = true;
	if (d_file_info_iterator->get_feature_collection()) {
		GPlatesFeatureVisitors::FeatureCollectionClassifier classifier;
		classifier.scan_feature_collection(
				GPlatesModel::FeatureCollectionHandle::get_const_weak_ref(
						*d_file_info_iterator->get_feature_collection()) );
		
		// We have to be a little cautious in testing for reconstructable
		// features, as if we don't enable them for reconstruction,
		// GPlates just won't bother displaying them. So the present
		// behaviour only disables the file for the reconstructable list
		// IFF there is rotation data present. Having 0 on both counts
		// means that Something Is Amiss and we should not rule out
		// 'reconstructable' as an option.
		if (classifier.reconstructable_feature_count() == 0 &&
				classifier.reconstruction_feature_count() > 0) {
			// Disable reconstructable button, obviously a rotations-only file.
			reconstructable_enabled = false;
		}
		// In testing for reconstruction features, we can be a little
		// more relaxed, because the set of features which make up
		// a reconstruction tree are smaller and easier to identify.
		// In this case, if we can't identify them, it would be pointless
		// attempting to reconstruct data with them anyway.
		if (classifier.reconstruction_feature_count() == 0) {
			// Disable reconstruction button, obviously no rotations here.
			reconstruction_enabled = false;
		}
	}

	// FIXME: It occurs to me we may have to sync the state of the buttons to
	// the actual program state - or at least initialise them when they are
	// constructed, and just let ManageFeatureCollectionsDialog create new
	// buttons (and rows, etc) when program state changes (as it has been doing)
	set_reconstructable_button_properties(reconstructable_active, reconstructable_enabled);
	set_reconstruction_button_properties(reconstruction_active, reconstruction_enabled);
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
	// Change the icon and tooltip depending on whether we should be depressed or not.
	set_reconstructable_button_properties(checked, true);
	// Activate or deactivate the file.
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
	// Change the icon and tooltip depending on whether we should be depressed or not.
	set_reconstruction_button_properties(checked, true);
	// Activate or deactivate the file.
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

