/* $Id: HellingerNewSegmentDialog.h 227 2012-02-24 14:46:55Z juraj.cirbus $ */

/**
 * \file
 * $Revision: 227 $
 * $Date: 2012-02-24 15:46:55 +0100 (Fri, 24 Feb 2012) $
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

#ifndef GPLATES_QTWIDGETS_HELLINGERNEWSEGMENTDIALOG_H
#define GPLATES_QTWIDGETS_HELLINGERNEWSEGMENTDIALOG_H

#include <QAbstractTableModel>
#include <QItemDelegate>
#include <QtCore>
#include <QtGui>
#include <QWidget>

#include "HellingerNewSegmentDialogUi.h"
#include "HellingerDialog.h"




namespace GPlatesQtWidgets
{
	enum ColumnType{
		COLUMN_MOVING_FIXED = 0,
		COLUMN_LAT,
		COLUMN_LON,
		COLUMN_UNCERTAINTY,

		NUM_COLUMNS
	};

	class HellingerDialog;
	class HellingerModel;
	class HellingerNewSegmentDialogWarning;

	/**
	 * @brief The SpinBoxDelegate class
	 *
	 * This lets us customise the spinbox behaviour in the TableView. Borrowed largely from the
	 * Qt example here:
	 * http://qt-project.org/doc/qt-4.8/itemviews-spinboxdelegate.html
	 *
	 */
	class SpinBoxDelegate : public QItemDelegate
	{
		Q_OBJECT

	public:
		SpinBoxDelegate(QObject *parent = 0);

		QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
							  const QModelIndex &index) const;

		void setEditorData(QWidget *editor, const QModelIndex &index) const;
		void setModelData(QWidget *editor, QAbstractItemModel *model,
						  const QModelIndex &index) const;

		void updateEditorGeometry(QWidget *editor,
						const QStyleOptionViewItem &option, const QModelIndex &index) const;
	};





	class HellingerNewSegmentDialog:
			public QDialog,
			protected Ui_HellingerNewSegmentDialog
	{
		Q_OBJECT
	public:
		HellingerNewSegmentDialog(
				HellingerDialog *hellinger_dialog,
				HellingerModel *hellinger_model,
				QWidget *parent_ = NULL);

		void
		reset();

		int d_type_new_segment_error;

	private Q_SLOTS:

		void
		handle_add_segment();

		void
		handle_add_line();

		void
		handle_remove_line();

		void
		add_segment_to_model();

		void
		change_pick_type_of_whole_table();

		void
		update_buttons();

	private:

		void
		set_initial_row_values(const int &row);

		void
		selectionChanged(const QItemSelection &selected,
						 const QItemSelection &deselected);

		HellingerDialog *d_hellinger_dialog_ptr;
		QStandardItemModel *d_model;
		HellingerModel *d_hellinger_model_ptr;
		HellingerNewSegmentDialogWarning *d_hellinger_new_segment_warning;

		// TODO: we can probably remove this and use model->rowCount() where necessary. Check this.
		int d_row_count;

		SpinBoxDelegate d_spin_box_delegate;
	};
}

#endif //GPLATES_QTWIDGETS_HELLINGERNEWSEGMENTDIALOG_H
