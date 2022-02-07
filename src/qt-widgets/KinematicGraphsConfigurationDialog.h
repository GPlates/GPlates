/**
 * \file
 * $Revision: 12148 $
 * $Date: 2011-08-18 14:01:47 +0200 (Thu, 18 Aug 2011) $
 *
 * Copyright (C) 2015 Geological Survey of Norway
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

#ifndef GPLATES_QTWIDGETS_KINEMATICGRAPHSCONFIGURATIONDIALOG_H
#define GPLATES_QTWIDGETS_KINEMATICGRAPHSCONFIGURATIONDIALOG_H

#include <QDialog>

#include "KinematicGraphsDialog.h"
#include "ui_KinematicGraphsConfigurationDialogUi.h"

//TODO: Add some text here (i.e. to the UI) explaining something about saving to preferences through Edit->Preferences
//TODO: Possibly add direct "Save to preferences" option.

namespace GPlatesQtWidgets
{
	class KinematicGraphsConfigurationWidget;

	class KinematicGraphsConfigurationDialog :
			public QDialog,
			protected Ui_KinematicGraphsConfigurationDialog
	{
		Q_OBJECT

	public:
		explicit
		KinematicGraphsConfigurationDialog(
				KinematicGraphsDialog::Configuration &configuration,
				QWidget *parent = 0);

		~KinematicGraphsConfigurationDialog();

	private Q_SLOTS:

		void
		handle_apply();

		/**
		 * @brief handle_configuration_changed
		 * Responds to signal emitted by the child widget.
		 */
		void
		handle_configuration_changed(
				bool valid);

	private:

		void
		initialise_widget();

		KinematicGraphsConfigurationWidget *d_configuration_widget;

		KinematicGraphsDialog::Configuration &d_configuration_ref;
	};

}

#endif // GPLATES_QTWIDGETS_KINEMATICGRAPHSCONFIGURATIONDIALOG_H
