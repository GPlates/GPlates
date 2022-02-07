/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2016 The University of Sydney, Australia
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

#include "OpenProjectRelativeOrAbsoluteDialog.h"

#include "model/Gpgim.h"


GPlatesQtWidgets::OpenProjectRelativeOrAbsoluteDialog::OpenProjectRelativeOrAbsoluteDialog(
		QWidget *parent_) :
	QDialog(parent_, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint)
{
	setupUi(this);

	// Add buttons to open absolute and relative files.
	QPushButton *button_open_absolute = buttonbox->addButton(tr("Load Original Files"), QDialogButtonBox::AcceptRole);
	QPushButton *button_open_relative = buttonbox->addButton(tr("Load Current Files"), QDialogButtonBox::AcceptRole);

	QPushButton *button_abort_open = buttonbox->button(QDialogButtonBox::Abort);
	button_abort_open->setIcon(QIcon(":/tango_process_stop_22.png"));
	button_abort_open->setDefault(true);

	set_file_paths(QStringList(), QStringList(), QStringList(), QStringList());

	connect(
			button_open_absolute, SIGNAL(clicked()),
			this, SLOT(open_absolute()));
	connect(
			button_open_relative, SIGNAL(clicked()),
			this, SLOT(open_relative()));
	connect(
			button_abort_open, SIGNAL(clicked()),
			this, SLOT(abort_open()));
}


void
GPlatesQtWidgets::OpenProjectRelativeOrAbsoluteDialog::set_file_paths(
		QStringList existing_absolute_file_paths,
		QStringList missing_absolute_file_paths,
		QStringList existing_relative_file_paths,
		QStringList missing_relative_file_paths)
{
	// List the existing absolute files (there should always be at least one).
	list_the_existing_absolute_files->clear();
	list_the_existing_absolute_files->addItems(existing_absolute_file_paths);

	// List the missing absolute files (or hide if none missing).
	if (!missing_absolute_file_paths.isEmpty())
	{
		list_the_missing_absolute_files->clear();
		list_the_missing_absolute_files->addItems(missing_absolute_file_paths);

		missing_absolute_files_widget->show();
	}
	else
	{
		missing_absolute_files_widget->hide();
	}

	// List the existing relative files (there should always be at least one).
	list_the_existing_relative_files->clear();
	list_the_existing_relative_files->addItems(existing_relative_file_paths);

	// List the missing relative files (or hide if none missing).
	if (!missing_relative_file_paths.isEmpty())
	{
		list_the_missing_relative_files->clear();
		list_the_missing_relative_files->addItems(missing_relative_file_paths);

		missing_relative_files_widget->show();
	}
	else
	{
		missing_relative_files_widget->hide();
	}

	buttonbox->button(QDialogButtonBox::Abort)->setFocus();
}


void
GPlatesQtWidgets::OpenProjectRelativeOrAbsoluteDialog::open_absolute()
{
	done(OPEN_ABSOLUTE);
}


void
GPlatesQtWidgets::OpenProjectRelativeOrAbsoluteDialog::open_relative()
{
	done(OPEN_RELATIVE);
}


void
GPlatesQtWidgets::OpenProjectRelativeOrAbsoluteDialog::abort_open()
{
	done(ABORT_OPEN);
}
