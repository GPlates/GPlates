/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
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
 
#ifndef GPLATES_QTWIDGETS_ADDNEWLAYERDIALOG_H
#define GPLATES_QTWIDGETS_ADDNEWLAYERDIALOG_H

#include "AddNewLayerDialogUi.h"


namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesQtWidgets
{
	class AddNewLayerDialog: 
			public QDialog,
			protected Ui_AddNewLayerDialog 
	{
		Q_OBJECT
		
	public:

		explicit
		AddNewLayerDialog(
				GPlatesAppLogic::ApplicationState &application_state,
				QWidget *parent_ = NULL);

	private slots:

		void
		handle_accept();

		void
		handle_combobox_index_changed(
				int index);

	private:

		void
		populate_combobox();

		GPlatesAppLogic::ApplicationState &d_application_state;
	};
}

#endif  // GPLATES_QTWIDGETS_ADDNEWLAYERDIALOG_H
