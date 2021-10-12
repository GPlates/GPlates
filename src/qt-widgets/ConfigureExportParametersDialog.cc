/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#include <QCheckBox>
#include <QLabel>
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>
#include "ConfigureExportParametersDialog.h"



GPlatesQtWidgets::ConfigureExportParametersDialog::ConfigureExportParametersDialog(
		GPlatesGui::ExportAnimationContext::non_null_ptr_type export_animation_context_ptr,
		QWidget *parent_):
	QDialog(parent_),
	d_export_animation_context_ptr(export_animation_context_ptr)
{
	setupUi(this);

	// Init widgets to match Context state.
	lineedit_target_directory->setText(d_export_animation_context_ptr->target_dir().absolutePath());
	lineedit_target_directory->end(false);
	label_target_directory_error->setVisible(false);
	// Kludgy way to do it, yes - but we need something useable for 0.9.5 NOW.
	checkbox_export_gmt->setChecked(d_export_animation_context_ptr->gmt_exporter_enabled());
	checkbox_export_shp->setChecked(d_export_animation_context_ptr->shp_exporter_enabled());
	checkbox_export_svg->setChecked(d_export_animation_context_ptr->svg_exporter_enabled());
	checkbox_export_velocity->setChecked(d_export_animation_context_ptr->velocity_exporter_enabled());
	checkbox_export_resolved_topology->setChecked(d_export_animation_context_ptr->resolved_topology_exporter_enabled());

	// Make our private signal/slot connections.
	// Note that we do not need to listen for enable/disable change events @em from the Context,
	// because this dialog is the sole source of those changes. Plus we are in a rush.
	QObject::connect(button_choose_target_directory, SIGNAL(clicked()),
			this, SLOT(react_choose_target_directory_clicked()));
	QObject::connect(lineedit_target_directory, SIGNAL(textEdited(const QString &)),
			this, SLOT(react_lineedit_target_directory_edited(const QString &)));

	QObject::connect(checkbox_export_gmt, SIGNAL(clicked(bool)),
			this, SLOT(react_export_gmt_checked(bool)));
	QObject::connect(checkbox_export_shp, SIGNAL(clicked(bool)),
			this, SLOT(react_export_shp_checked(bool)));
	QObject::connect(checkbox_export_svg, SIGNAL(clicked(bool)),
			this, SLOT(react_export_svg_checked(bool)));
	QObject::connect(checkbox_export_velocity, SIGNAL(clicked(bool)),
			this, SLOT(react_export_velocity_checked(bool)));
	QObject::connect(checkbox_export_resolved_topology, SIGNAL(clicked(bool)),
			this, SLOT(react_export_resolved_topology_checked(bool)));
}


void
GPlatesQtWidgets::ConfigureExportParametersDialog::react_choose_target_directory_clicked()
{
	// Open dirname chooser, set target directory from that.
	QString new_target = QFileDialog::getExistingDirectory(this, tr("Choose Target Directory"),
			d_export_animation_context_ptr->target_dir().absolutePath(),
			QFileDialog::ShowDirsOnly);
	if (new_target.isEmpty()) {
		// On "Cancel", the dialog returns the empty string. No change.
		return;
	}

	// See if this directory passes the validity tests and update Context if so.
	update_target_directory(new_target);

	// Update our line edit. Happily, this does not trigger
	// react_lineedit_target_directory_edited(), as that only listens for user edits.
	// We only muck with the line edit here, so as not to interfere with the
	// user's typing if they are editing things directly.
	QDir new_target_dir(new_target);
	lineedit_target_directory->setText(new_target);
	lineedit_target_directory->end(false);
}


void
GPlatesQtWidgets::ConfigureExportParametersDialog::react_lineedit_target_directory_edited(
		const QString &text)
{
	QString new_target = lineedit_target_directory->text();
	update_target_directory(new_target);
}


void
GPlatesQtWidgets::ConfigureExportParametersDialog::react_export_gmt_checked(
		bool enable)
{
	d_export_animation_context_ptr->set_gmt_exporter_enabled(enable);
}

void
GPlatesQtWidgets::ConfigureExportParametersDialog::react_export_shp_checked(
		bool enable)
{
	d_export_animation_context_ptr->set_shp_exporter_enabled(enable);
}

void
GPlatesQtWidgets::ConfigureExportParametersDialog::react_export_svg_checked(
		bool enable)
{
	d_export_animation_context_ptr->set_svg_exporter_enabled(enable);
}

void
GPlatesQtWidgets::ConfigureExportParametersDialog::react_export_velocity_checked(
		bool enable)
{
	d_export_animation_context_ptr->set_velocity_exporter_enabled(enable);
}

void
GPlatesQtWidgets::ConfigureExportParametersDialog::react_export_resolved_topology_checked(
		bool enable)
{
	d_export_animation_context_ptr->set_resolved_topology_exporter_enabled(enable);
}


void
GPlatesQtWidgets::ConfigureExportParametersDialog::update_target_directory(
		const QString &new_target)
{
	// Check properties of supplied pathname.
	QDir new_target_dir(new_target);
	QFileInfo new_target_fileinfo(new_target);
	
	// Update the Context if all ok. Show an error label if not.
	if ( ! new_target_fileinfo.exists()) {
		label_target_directory_error->setVisible(true);
		label_target_directory_error->setText(tr("Target directory does not exist."));
	} else if ( ! new_target_fileinfo.isDir()) {
		label_target_directory_error->setVisible(true);
		label_target_directory_error->setText(tr("Target is not a directory."));
	} else if ( ! new_target_fileinfo.isWritable()) {
		label_target_directory_error->setVisible(true);
		label_target_directory_error->setText(tr("Target directory is not writable."));
	} else {
		// Great! No major screwups.
		label_target_directory_error->setVisible(false);
		// We can update the Context with the new target.
		d_export_animation_context_ptr->set_target_dir(new_target_dir);
	}
}


