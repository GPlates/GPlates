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
#ifndef GPLATES_QTWIDGETS_AGEMODELMANAGERDIALOG_H
#define GPLATES_QTWIDGETS_AGEMODELMANAGERDIALOG_H

#include "GPlatesDialog.h"
#include "AgeModelManagerDialogUi.h"


namespace GPlatesAppLogic
{
	class AgeModelCollection;
	class ApplicationState;
}

namespace GPlatesQtWidgets
{
class AgeModelManagerDialog:
		public GPlatesDialog,
		protected Ui_AgeModelManagerDialog
{
	Q_OBJECT
public:
	AgeModelManagerDialog(
		GPlatesAppLogic::ApplicationState &app_state,
		QWidget *parent_ = NULL);


private Q_SLOTS:

	void
	handle_import();

	void
	handle_combo_box_selection_changed();

private:

	void
	setup_widgets();

	void
	setup_connections();

	GPlatesAppLogic::AgeModelCollection &d_age_model_collection;
};


}
#endif // GPLATES_QTWIDGETS_AGEMODELMANAGERDIALOG_H
