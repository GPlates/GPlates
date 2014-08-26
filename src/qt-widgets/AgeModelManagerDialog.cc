/**
 * \file
 * $Revision: 254 $
 * $Date: 2012-03-01 13:00:21 +0100 (Thu, 01 Mar 2012) $
 *
 * Copyright (C) 2014 Geological Survey of Norway
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


#include "AgeModelManagerDialog.h"
#include "app-logic/AgeModelCollection.h"
#include "app-logic/ApplicationState.h"


GPlatesQtWidgets::AgeModelManagerDialog::AgeModelManagerDialog(
		GPlatesAppLogic::ApplicationState &app_state,
		QWidget *parent_):
	GPlatesDialog(
		parent_,
		Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
	d_age_model_collection(app_state.get_age_model_collection())
{
	setupUi(this);

	setup_widgets();

	setup_connections();
}

void
GPlatesQtWidgets::AgeModelManagerDialog::handle_import()
{

}

void
GPlatesQtWidgets::AgeModelManagerDialog::handle_combo_box_selection_changed()
{

}

void
GPlatesQtWidgets::AgeModelManagerDialog::setup_widgets()
{
	line_edit_collection->setText(d_age_model_collection.get_filename());
}

void
GPlatesQtWidgets::AgeModelManagerDialog::setup_connections()
{

}


