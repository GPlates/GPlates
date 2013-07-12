/* $Id: HellingerEditSegment.h 233 2012-02-27 10:16:37Z juraj.cirbus $ */

/**
 * \file
 * $Revision: 233 $
 * $Date: 2012-02-27 11:16:37 +0100 (Mon, 27 Feb 2012) $
 *
 * Copyright (C) 2011, 2012 Geological Survey of Norway
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

#ifndef GPLATES_QTWIDGETS_HELLINGEREDITSEGMENT_H
#define GPLATES_QTWIDGETS_HELLINGEREDITSEGMENT_H

#include <QAbstractTableModel>
#include <QtCore>
#include <QtGui>
#include <QWidget>

#include "HellingerEditSegmentUi.h"
#include "HellingerDialog.h"

namespace GPlatesQtWidgets
{


	class HellingerDialog;
	class HellingerModel;
	class HellingerNewSegmentWarning;

	class HellingerEditSegment:
			public QDialog,
			protected Ui_HellingerEditSegment
	{
		Q_OBJECT
	public:

		enum ColumnType{
			COLUMN_MOVING_FIXED = 0,
			COLUMN_LAT,
			COLUMN_LON,
			COLUMN_UNCERTAINTY,

			NUM_COLUMNS
		};


		HellingerEditSegment(
				HellingerDialog *hellinger_dialog,
				HellingerModel *hellinger_model,
				QWidget *parent_ = NULL);

		void
		reset();

		void
		initialise_table(QStringList &input_value);

		void
		initialise(int &segment);

	private Q_SLOTS:

		void
		edit_segment();

		void
		add_line();

		void
		remove_line();

		void
		edit();

		void
		change_table_stats_pick();

		void
		handle_item_changed(QStandardItem *item);

	private:

		void
		update_buttons();

		void
		change_quick_set_state();

		void
		check_picks(QStringList &picks);

		HellingerDialog *d_hellinger_dialog_ptr;
		HellingerModel *d_hellinger_model_ptr;
		QStandardItemModel *model;
		int d_number_rows;
		int d_segment_number;
		QStringList d_disabled_picks;
		QStringList d_active_picks;
		HellingerNewSegmentWarning *d_hellinger_new_segment_warning;

	};
}

#endif //GPLATES_QTWIDGETS_HELLINGEREDITSEGMENT_H
