/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2006, 2007, 2008 The University of Sydney, Australia
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

#include "ManageFeatureCollectionsDialog.h"
#include "ManageFeatureCollectionsActionWidget.h"


GPlatesQtWidgets::ManageFeatureCollectionsActionWidget::ManageFeatureCollectionsActionWidget(
		GPlatesQtWidgets::ManageFeatureCollectionsDialog &feature_collections_dialog,
		GPlatesAppState::ApplicationState::file_info_iterator file_it,
		QWidget *parent_):
	QWidget(parent_),
	d_feature_collections_dialog(feature_collections_dialog),
	d_file_info_iterator(file_it)
{
	setupUi(this);

	// Set up slots for each button
	QObject::connect(button_edit_configuration, SIGNAL(clicked()), this, SLOT(edit_configuration()));
	QObject::connect(button_save, SIGNAL(clicked()), this, SLOT(save()));
	QObject::connect(button_save_as, SIGNAL(clicked()), this, SLOT(save_as()));
	QObject::connect(button_save_copy, SIGNAL(clicked()), this, SLOT(save_copy()));
	QObject::connect(button_reload, SIGNAL(clicked()), this, SLOT(reload()));
	QObject::connect(button_unload, SIGNAL(clicked()), this, SLOT(unload()));

	// Edge case - if the FileInfo has been created for a FeatureCollection
	// which only exists in GPlates' memory so far (i.e., created as a new
	// feature collection at the end of d10n), then any "Save" function
	// should be disabled in favour of "Save As". Similarly, the FeatureCollection
	// cannot be "Reloaded".
	if ( ! file_it->get_qfileinfo().exists()) {
		button_save->setDisabled(true);
		button_reload->setDisabled(true);
	}
}

void
GPlatesQtWidgets::ManageFeatureCollectionsActionWidget::edit_configuration()
{
	d_feature_collections_dialog.edit_configuration(this);
}

void
GPlatesQtWidgets::ManageFeatureCollectionsActionWidget::save()
{
	d_feature_collections_dialog.save_file(this);
}

void
GPlatesQtWidgets::ManageFeatureCollectionsActionWidget::save_as()
{
	d_feature_collections_dialog.save_file_as(this);
}

void
GPlatesQtWidgets::ManageFeatureCollectionsActionWidget::save_copy()
{
	d_feature_collections_dialog.save_file_copy(this);
}

void
GPlatesQtWidgets::ManageFeatureCollectionsActionWidget::reload()
{
	d_feature_collections_dialog.reload_file(this);
}

void
GPlatesQtWidgets::ManageFeatureCollectionsActionWidget::unload()
{
	d_feature_collections_dialog.unload_file(this);
}

void
GPlatesQtWidgets::ManageFeatureCollectionsActionWidget::enable_edit_configuration_button()
{
	button_edit_configuration->setEnabled(true);
}
