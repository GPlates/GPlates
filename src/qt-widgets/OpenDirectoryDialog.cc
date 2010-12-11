/* $Id: SaveFileDialog.cc 10510 2010-12-11 02:25:34Z elau $ */

/**
 * @file 
 * Contains implementation of OpenDirectoryDialog.
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

#include "OpenDirectoryDialog.h"

#include "presentation/ViewState.h"


GPlatesQtWidgets::OpenDirectoryDialog::OpenDirectoryDialog(
		QWidget *parent,
		const QString &caption,
		GPlatesPresentation::ViewState &view_state) :
	d_parent(parent),
	d_caption(caption),
	d_last_open_directory(view_state.get_last_open_directory())
{  }


QString
GPlatesQtWidgets::OpenDirectoryDialog::get_existing_directory()
{
	QString directory = QFileDialog::getExistingDirectory(
			d_parent,
			d_caption,
			d_last_open_directory);
	if (!directory.isEmpty())
	{
		d_last_open_directory = directory;
	}

	return directory;
}

