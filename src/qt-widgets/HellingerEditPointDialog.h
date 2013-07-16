/* $Id: HellingerEditPoint.h 132 2012-01-24 15:39:28Z juraj.cirbus $ */

/**
 * \file
 * $Revision: 132 $
 * $Date: 2012-01-24 16:39:28 +0100 (Tue, 24 Jan 2012) $
 *
 * Copyright (C) 2011, 2012, 2013 Geological Survey of Norway
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

#ifndef GPLATES_QTWIDGETS_HELLINGEREDITPOINTDIALOG_H
#define GPLATES_QTWIDGETS_HELLINGEREDITPOINTDIALOG_H

#include <QWidget>

#include "HellingerEditPointDialogUi.h"
#include "HellingerDialog.h"

namespace GPlatesQtWidgets
{
	class HellingerDialog;
	class HellingerModel;

	class HellingerEditPointDialog:
			public QDialog,
			protected Ui_HellingerEditPointDialog
	{
		Q_OBJECT
	public:

		HellingerEditPointDialog(
				HellingerDialog *hellinger_dialog,
				HellingerModel *hellinger_model,
				bool create_new_point = false,
				QWidget *parent_ = NULL);

		void
		initialise_with_pick(
				const int &segment,
				const int &row);

		void
		initialise_with_segment_number(
				const int &segment_number);


	private Q_SLOTS:

		void
		handle_apply();

	private:

		void
		set_initial_values();

		HellingerDialog *d_hellinger_dialog_ptr;
		HellingerModel *d_hellinger_model_ptr;

		int d_segment;
		int d_row;

		bool d_create_new_point;
	};
}

#endif //GPLATES_QTWIDGETS_HELLINGEREDITPOINTDIALOG_H
