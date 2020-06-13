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

#ifndef GPLATES_QTWIDGETS_MISSINGSESSIONFILESDIALOG_H
#define GPLATES_QTWIDGETS_MISSINGSESSIONFILESDIALOG_H

#include <boost/optional.hpp>
#include <QDialog>
#include <QDialogButtonBox>
#include <QMap>
#include <QPushButton>
#include <QSignalMapper>
#include <QString>
#include <QStringList>

#include "ui_MissingSessionFilesDialogUi.h"


namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	/**
	 * This dialog pops up if the user loads a project/session that has missing files and asks them
	 * to optionally locate the files before loading the project/session.
	 */
	class MissingSessionFilesDialog :
			public QDialog,
			protected Ui_MissingSessionFilesDialog
	{
		Q_OBJECT
		
	public:

		enum ActionRequested { LOAD_PROJECT, LOAD_SESSION};


		explicit
		MissingSessionFilesDialog(
				GPlatesPresentation::ViewState &view_state_,
				QWidget *parent_ = NULL);


		/**
		 * Set the missing file paths to be displayed in the dialog.
		 */
		void
		populate(
				ActionRequested action_requested,
				QStringList missing_file_paths);

		/**
		 * Returns those missing files that were remapped to existing files (if any were remapped).
		 *
		 * Only those files that were remapped by the user are returned (if any).
		 */
		boost::optional< QMap<QString/*missing*/, QString/*existing*/> >
		get_file_path_remapping() const;


	private Q_SLOTS:

		void
		load()
		{
			done(QDialogButtonBox::Ok);
		}

		void
		abort_load()
		{
			done(QDialogButtonBox::Abort);
		}

		void
		update(
				int row);

	private:

		struct ColumnNames
		{
			enum ColumnName { FILENAME, UPDATE };
		};

		GPlatesPresentation::ViewState &d_view_state;

		QSignalMapper *d_signal_mapper;

		/**
		 * The original missing file paths.
		 */
		QStringList d_missing_file_paths;

		/**
		 * Map of missing file paths to updated file paths of any remapped file paths.
		 */
		QMap<QString, QString> d_file_path_remapping;
	};
}

#endif  // GPLATES_QTWIDGETS_MISSINGSESSIONFILESDIALOG_H
