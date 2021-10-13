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

#include "ManageFeatureCollectionsActionWidget.h"

#include "ManageFeatureCollectionsDialog.h"

#include "file-io/FeatureCollectionFileFormat.h"
#include "file-io/FeatureCollectionFileFormatRegistry.h"


GPlatesQtWidgets::ManageFeatureCollectionsActionWidget::ManageFeatureCollectionsActionWidget(
		GPlatesQtWidgets::ManageFeatureCollectionsDialog &feature_collections_dialog,
		GPlatesAppLogic::FeatureCollectionFileState::file_reference file_ref,
		QWidget *parent_):
	QWidget(parent_),
	d_feature_collections_dialog(feature_collections_dialog),
	d_file_reference(file_ref)
{
	setupUi(this);

	// Set up slots for each button
	QObject::connect(button_edit_configuration, SIGNAL(clicked()), this, SLOT(handle_edit_configuration()));
	QObject::connect(button_save, SIGNAL(clicked()), this, SLOT(handle_save()));
	QObject::connect(button_save_as, SIGNAL(clicked()), this, SLOT(handle_save_as()));
	QObject::connect(button_save_copy, SIGNAL(clicked()), this, SLOT(handle_save_copy()));
	QObject::connect(button_reload, SIGNAL(clicked()), this, SLOT(handle_reload()));
	QObject::connect(button_unload, SIGNAL(clicked()), this, SLOT(handle_unload()));

	// Disable all buttons initially.
	// The caller needs to call 'update()' to enable the appropriate buttons.
	button_edit_configuration->setDisabled(true);
	button_save->setDisabled(true);
	button_save_as->setDisabled(true);
	button_save_copy->setDisabled(true);
	button_reload->setDisabled(true);
	button_unload->setDisabled(true);
}


void
GPlatesQtWidgets::ManageFeatureCollectionsActionWidget::update(
		const GPlatesFileIO::FeatureCollectionFileFormat::Registry &file_format_registry,
		const GPlatesFileIO::FileInfo &fileinfo,
		boost::optional<GPlatesFileIO::FeatureCollectionFileFormat::Format> file_format,
		bool enable_edit_configuration)
{
	// Start out with the all buttons enabled and then disable as needed.
	button_edit_configuration->setEnabled(true);
	button_save->setEnabled(true);
	button_save_as->setEnabled(true);
	button_save_copy->setEnabled(true);
	button_reload->setEnabled(true);
	button_unload->setEnabled(true);

	// Disable reload button for file formats that we cannot read.
	if (!file_format ||
		!file_format_registry.does_file_format_support_reading(file_format.get()))
	{
		button_reload->setEnabled(false);
	}

	// Disable save button for file formats that we cannot write.
	if (!file_format ||
		!file_format_registry.does_file_format_support_writing(file_format.get()))
	{
		button_save->setEnabled(false);
	}

	// Disable the edit configuration button if a edit configuration is not available.
	if (!enable_edit_configuration)
	{
		button_edit_configuration->setEnabled(false);
	}

	// Edge case - if the FileInfo has been created for a FeatureCollection
	// which only exists in GPlates' memory so far (i.e., created as a new
	// feature collection at the end of d10n), then any "Save" function
	// should be disabled in favour of "Save As". Similarly, the FeatureCollection
	// cannot be "Reloaded".
	if ( ! GPlatesFileIO::file_exists(fileinfo))
	{
		button_save->setEnabled(false);
		button_reload->setEnabled(false);
	}
}


void
GPlatesQtWidgets::ManageFeatureCollectionsActionWidget::handle_edit_configuration()
{
	d_feature_collections_dialog.edit_configuration(this);
}

void
GPlatesQtWidgets::ManageFeatureCollectionsActionWidget::handle_save()
{
	d_feature_collections_dialog.save_file(this);
}

void
GPlatesQtWidgets::ManageFeatureCollectionsActionWidget::handle_save_as()
{
	d_feature_collections_dialog.save_file_as(this);
}

void
GPlatesQtWidgets::ManageFeatureCollectionsActionWidget::handle_save_copy()
{
	d_feature_collections_dialog.save_file_copy(this);
}

void
GPlatesQtWidgets::ManageFeatureCollectionsActionWidget::handle_reload()
{
	d_feature_collections_dialog.reload_file(this);
}

void
GPlatesQtWidgets::ManageFeatureCollectionsActionWidget::handle_unload()
{
	d_feature_collections_dialog.unload_file(this);
}
