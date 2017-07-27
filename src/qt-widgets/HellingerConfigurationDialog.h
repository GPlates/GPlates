/**
 * \file
 * $Revision$
 * $Date$
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
#ifndef HELLINGERCONFIGURATIONDIALOG_H
#define HELLINGERCONFIGURATIONDIALOG_H

#include <QDialog>

#include "HellingerConfigurationDialogUi.h"
#include "HellingerDialog.h"

namespace GPlatesAppLogic
{
	class ApplicationState;
}
namespace GPlatesQtWidgets
{

	class HellingerConfigurationWidget;

	class HellingerConfigurationDialog :
			public QDialog,
			protected Ui_HellingerConfigurationDialog
	{
		Q_OBJECT



	public:
		explicit HellingerConfigurationDialog(
				HellingerDialog::Configuration &configuration,
				GPlatesAppLogic::ApplicationState &app_state,
				QWidget *parent = 0);

		~HellingerConfigurationDialog();

		void
		read_values_from_settings() const;

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


	Q_SIGNALS:

		void
		configuration_changed();

	private:

		void
		write_values_to_settings();

		void
		initialise_widget();

		HellingerConfigurationWidget *d_configuration_widget;

		HellingerDialog::Configuration &d_configuration_ref;

		GPlatesAppLogic::ApplicationState &d_app_state_ref;
	};

}
#endif // HELLINGERCONFIGURATIONDIALOG_H
