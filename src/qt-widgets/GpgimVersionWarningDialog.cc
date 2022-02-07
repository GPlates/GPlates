/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#include "GpgimVersionWarningDialog.h"

#include "model/Gpgim.h"


GPlatesQtWidgets::GpgimVersionWarningDialog::GpgimVersionWarningDialog(
		bool show_dialog_on_loading_files,
		QWidget *parent_) :
	QDialog(parent_, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint)
{
	setupUi(this);
	set_action_requested(LOAD_FILES, QStringList(), QStringList());

	do_not_show_dialog_on_loading_check_box->setChecked(!show_dialog_on_loading_files);
}


void
GPlatesQtWidgets::GpgimVersionWarningDialog::set_action_requested(
		ActionRequested act,
		QStringList older_version_filenames,
		QStringList newer_version_filenames)
{
	const bool has_older_version_files = !older_version_filenames.isEmpty();
	const bool has_newer_version_files = !newer_version_filenames.isEmpty();

	list_older_version_files->clear();
	list_older_version_files->addItems(older_version_filenames);
	// Only show if there are older version files.
	older_version_group_box->setVisible(has_older_version_files);

	list_newer_version_files->clear();
	list_newer_version_files->addItems(newer_version_filenames);
	// Only show if there are newer version files.
	newer_version_group_box->setVisible(has_newer_version_files);

	tweak_buttons(act);
	tweak_label(act);
	adjustSize();
	ensurePolished();
}


bool
GPlatesQtWidgets::GpgimVersionWarningDialog::do_not_show_dialog_on_loading_files() const
{
	return do_not_show_dialog_on_loading_check_box->isChecked();
}


void
GPlatesQtWidgets::GpgimVersionWarningDialog::save_changes()
{
	done(QDialogButtonBox::Save);
}


void
GPlatesQtWidgets::GpgimVersionWarningDialog::abort_save()
{
	done(QDialogButtonBox::Abort);
}


void
GPlatesQtWidgets::GpgimVersionWarningDialog::close()
{
	done (QDialogButtonBox::Close);
}


void
GPlatesQtWidgets::GpgimVersionWarningDialog::tweak_buttons(
		ActionRequested act)
{
	switch (act)
	{
	default:
	case SAVE_FILES:
		{
			buttonbox->setStandardButtons(QDialogButtonBox::Save | QDialogButtonBox::Abort);
			QPushButton *save_button = buttonbox->button(QDialogButtonBox::Save);
			QPushButton *abort_button = buttonbox->button(QDialogButtonBox::Abort);

			save_button->setText(tr("&Save"));
			save_button->setIcon(QIcon(":/gnome_save_22.png"));
			save_button->setIconSize(QSize(22, 22));
			save_button->show();

			abort_button->setText(tr("D&on't save"));
			abort_button->setIcon(QIcon(":/tango_process_stop_22.png"));
			abort_button->setIconSize(QSize(22, 22));
			abort_button->show();

			connect(buttonbox->button(QDialogButtonBox::Save), SIGNAL(clicked()),
					this, SLOT(save_changes()));
			connect(buttonbox->button(QDialogButtonBox::Abort), SIGNAL(clicked()),
					this, SLOT(abort_save()));

			// Hide the do-not-show-dialog-on-loading-files check box when saving files.
			do_not_show_dialog_on_loading_check_box->hide();

			abort_button->setFocus();
		}
		break;

	case LOAD_FILES:
		{
			// It's just an information dialog so that the user is aware that any changes they make
			// might cause problems due to GPGIM versioning. They'll also get warning if/when they save.
			buttonbox->setStandardButtons(QDialogButtonBox::Close);
			QPushButton *close_button = buttonbox->button(QDialogButtonBox::Close);

			connect(buttonbox->button(QDialogButtonBox::Close), SIGNAL(clicked()),
					this, SLOT(close()));

			// Show the do-not-show-dialog-on-loading-files check box when loading files.
			do_not_show_dialog_on_loading_check_box->show();

			close_button->setFocus();
		}
		break;
	}

	buttonbox->adjustSize();
}


void
GPlatesQtWidgets::GpgimVersionWarningDialog::tweak_label(
		ActionRequested act)
{
	const GPlatesModel::Gpgim &gpgim = GPlatesModel::Gpgim::instance();

	switch (act)
	{
	default:
	case SAVE_FILES:
		label_context->setText(tr(
				"The current GPlates Geological Information Model (GPGIM) version is %1.\n"
				"GPlates will save files using the current GPGIM version.")
						.arg(gpgim.get_version().version_string()));
		break;

	case LOAD_FILES:
		label_context->setText(tr(
				"The current GPlates Geological Information Model (GPGIM) version is %1.")
						.arg(gpgim.get_version().version_string()));
		break;
	}
}
