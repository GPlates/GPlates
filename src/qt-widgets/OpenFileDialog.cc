/* $Id: SaveFileDialog.cc 10510 2010-12-11 02:25:34Z elau $ */

/**
 * @file
 * Contains implementation of OpenFileDialog.
 *
 * $Revision: 10510 $
 * $Date: 2010-12-11 13:25:34 +1100 (Sat, 11 Dec 2010) $
 *
 * Copyright (C) 2010 The University of Sydney, Australia
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

#include <QFileDialog>
#include <QFileInfo>

#include "OpenFileDialog.h"


GPlatesQtWidgets::OpenFileDialog::OpenFileDialog(QWidget *parent,
												 const QString &caption,
												 const QString &filter,
												 GPlatesPresentation::ViewState &view_state) :
	d_parent(parent),
	d_caption(caption),
	d_filter(filter),
	d_directory_configuration(
		view_state.get_file_io_directory_configurations().feature_collection_configuration())
{  }


QString
GPlatesQtWidgets::OpenFileDialog::get_open_file_name()
{
	QString file_name = QFileDialog::getOpenFileName(
				d_parent,
				d_caption,
				d_directory_configuration.directory(),
				d_filter,
				&d_selected_filter);
	if (!file_name.isEmpty())
	{
		d_directory_configuration.update_last_used_directory(
					QFileInfo(file_name).path());
	}

	return file_name;
}


QStringList
GPlatesQtWidgets::OpenFileDialog::get_open_file_names()
{
	QStringList file_names = QFileDialog::getOpenFileNames(
				d_parent,
				d_caption,
				d_directory_configuration.directory(),
				d_filter,
				&d_selected_filter);
	if (!file_names.isEmpty())
	{
		d_directory_configuration.update_last_used_directory(
					QFileInfo(file_names.first()).path());
	}

	return file_names;
}



