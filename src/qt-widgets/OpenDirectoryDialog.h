/* $Id: SaveFileDialog.h 10510 2010-12-11 02:25:34Z elau $ */

/**
 * @file
 * Contains definition of OpenDirectoryDialog.
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
 
#ifndef GPLATES_QTWIDGETS_OPENDIRECTORYDIALOG_H
#define GPLATES_QTWIDGETS_OPENDIRECTORYDIALOG_H

#include <QWidget>
#include <QString>
#include <QStringList>


namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	class OpenDirectoryDialog
	{
	public:

		OpenDirectoryDialog(
				QWidget *parent,
				const QString &caption,
				GPlatesPresentation::ViewState &view_state);

		QString
		get_existing_directory();

		void
		select_directory(
				const QString &directory)
		{
			d_last_open_directory = directory;
		}

	private:

		QWidget *d_parent;
		QString d_caption;
		QString &d_last_open_directory;
	};
}

#endif  // GPLATES_QTWIDGETS_OPENDIRECTORYDIALOG_H

