/* $Id: HellingerNewSegment.cc 241 2012-02-28 11:28:13Z robin.watson@ngu.no $ */

/**
 * \file
 * $Revision: 241 $
 * $Date: 2012-02-28 12:28:13 +0100 (Tue, 28 Feb 2012) $
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

#include <QDebug>
#include <QRadioButton>
#include <QTableView>
#include <QTextStream>

#include "HellingerDialog.h"
#include "HellingerDialogUi.h"
#include "HellingerModel.h"
#include "HellingerNewSegment.h"
#include "HellingerNewSegmentWarning.h"
#include "QtWidgetUtils.h"

GPlatesQtWidgets::HellingerNewSegment::HellingerNewSegment(
		HellingerDialog *hellinger_dialog,
		HellingerModel *hellinger_model,
		QWidget *parent_):
	QDialog(parent_,Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
	d_hellinger_dialog_ptr(hellinger_dialog),
	d_hellinger_model_ptr(hellinger_model),
	d_hellinger_new_segment_warning(0),
	d_row_count(1)
{
	setupUi(this);
	QObject::connect(button_add_segment, SIGNAL(clicked()), this, SLOT(handle_add_segment()));
	QObject::connect(button_add_line, SIGNAL(clicked()), this, SLOT(handle_add_line()));
	QObject::connect(button_remove_line, SIGNAL(clicked()), this, SLOT(handle_remove_line()));
	QObject::connect(radio_moving, SIGNAL(clicked()), this, SLOT(change_table_stats_pick()));
	QObject::connect(radio_fixed, SIGNAL(clicked()), this, SLOT(change_table_stats_pick()));
	QObject::connect(radio_custom, SIGNAL(clicked()), this, SLOT(change_table_stats_pick()));

	update_buttons();
	model = new QStandardItemModel(NUM_COLUMNS,d_row_count, this);
	model->setHorizontalHeaderItem(COLUMN_MOVING_FIXED,new QStandardItem("Moving(1)/Fixed(2)"));
	model->setHorizontalHeaderItem(COLUMN_LAT, new QStandardItem("Lat"));
	model->setHorizontalHeaderItem(COLUMN_LON, new QStandardItem("Long"));
	model->setHorizontalHeaderItem(COLUMN_UNCERTAINTY, new QStandardItem("Uncertainty (km)"));



	for (int row = 0; row < d_row_count; ++row)
	{
		for (int col = 0; col < NUM_COLUMNS; ++col)
		{
			QModelIndex index = model->index(row, col, QModelIndex());
			model ->setData(index, 0.00);
		}
		QModelIndex index_move_fix = model->index(row, COLUMN_MOVING_FIXED, QModelIndex());
		model ->setData(index_move_fix, 1);
	}
	table_new_segment->setModel(model);

	table_new_segment->horizontalHeader()->resizeSection(COLUMN_MOVING_FIXED,140);
	table_new_segment->horizontalHeader()->resizeSection(COLUMN_LAT,100);
	table_new_segment->horizontalHeader()->resizeSection(COLUMN_LON,100);
	table_new_segment->horizontalHeader()->resizeSection(COLUMN_UNCERTAINTY,100);

	table_new_segment->horizontalHeader()->setStretchLastSection(true);


	connect(model,SIGNAL(itemChanged(QStandardItem*)),this,SLOT(item_changed(QStandardItem*)));

}

void
GPlatesQtWidgets::HellingerNewSegment::handle_add_segment()
{
	// FIXME: We don't check for contiguous segment numbers here. It could be an idea to
	// check for this here and suggest the next "available" segment number if the user has
	// entered a value greater than (highest-so-far)+1. The contiguity is checked and corrected
	// before performing the fit anyway, so it doesn't have to be here by any means.
	int segment_number = spinbox_segment->value();

	if (d_hellinger_model_ptr->segment_number_exists(segment_number))
	{
		// We have a clash: the desired segment number is already in the model. Warn the user
		// and get their desired action.
		if (!d_hellinger_new_segment_warning)
		{
			d_hellinger_new_segment_warning = new GPlatesQtWidgets::HellingerNewSegmentWarning(
						d_hellinger_dialog_ptr,
						segment_number);

		}

		d_hellinger_new_segment_warning->exec(); // necessary for applied changes!
		int value_error = d_hellinger_new_segment_warning->error_type_new_segment();
		if (value_error == ACTION_ADD_NEW_SEGMENT)
		{
			add_segment_to_model();
		}
		else if (value_error == ACTION_REPLACE_NEW_SEGMENT)
		{
			d_hellinger_model_ptr->remove_segment(segment_number);
			add_segment_to_model();

		}
		else if (value_error == ACTION_INSERT_NEW_SEGMENT)
		{
			d_hellinger_model_ptr->reorder_segment(segment_number);
			add_segment_to_model();
		}
		else
		{
			return;
		}
	}
	else
	{
		// Everything was cool.
		add_segment_to_model();
	}

}

void
GPlatesQtWidgets::HellingerNewSegment::add_segment_to_model()
{
	QStringList value_from_table;
	QStringList value_on_row;
	int segment = spinbox_segment->value();
	QString segment_str = QString("%1").arg(segment);
	QString is_enabled = "1";

	for (int row = 0; row < d_row_count; ++row)
	{
		for (int col = 0; col < NUM_COLUMNS; ++col)
		{
			QModelIndex index = model->index(row, col, QModelIndex());
			QString value = table_new_segment->model()->data(
						table_new_segment->model()->index( index.row() , index.column() ) )
					.toString();

			value_on_row<< value;
		}
		value_from_table<<value_on_row.at(0)<<segment_str<<value_on_row.at(1)<<value_on_row.at(2)<<value_on_row.at(3)<<is_enabled;
		d_hellinger_model_ptr->add_pick(value_from_table);
		d_hellinger_dialog_ptr->update_from_model();
		value_on_row.clear();
		value_from_table.clear();
	}
}

void
GPlatesQtWidgets::HellingerNewSegment::handle_add_line()
{
	//const QModelIndex index = treeWidget->selectionModel()->currentIndex();
	const QModelIndex index = table_new_segment->currentIndex();
	int row = index.row();
	int col = index.column();
	model->insertRow(row);
	for (col = 0; col < 4; ++col)
	{
		QModelIndex index_m = model ->index(row, col, QModelIndex());
		model ->setData(index_m, 0.00);
	}
	QModelIndex index_move_fix = model ->index(row, 0, QModelIndex());
	model->setData(index_move_fix, 1);
	++d_row_count;
}

void
GPlatesQtWidgets::HellingerNewSegment::handle_remove_line()
{
	const QModelIndex index = table_new_segment->currentIndex();
	int row = index.row();
	model->removeRow(row);
	d_row_count= d_row_count-1;
}

void
GPlatesQtWidgets::HellingerNewSegment::change_table_stats_pick()
{
	if (radio_moving->isChecked())
	{
		for (double row = 0; row < d_row_count; ++row)
		{
			QModelIndex index_move_fix = model ->index(row, 0, QModelIndex());
			model->setData(index_move_fix, 1);
			disconnect(model,SIGNAL(itemChanged(QStandardItem*)),this,SLOT(item_changed(QStandardItem*)));
		}
		connect(model,SIGNAL(itemChanged(QStandardItem*)),this,SLOT(item_changed(QStandardItem*)));
		radio_moving->setChecked(true);
	}
	else if (radio_fixed->isChecked())
	{
		for (double row = 0; row < d_row_count; ++row)
		{
			QModelIndex index_move_fix = model ->index(row, 0, QModelIndex());
			model->setData(index_move_fix, 2);
			disconnect(model,SIGNAL(itemChanged(QStandardItem*)),this,SLOT(item_changed(QStandardItem*)));
		}
		connect(model,SIGNAL(itemChanged(QStandardItem*)),this,SLOT(item_changed(QStandardItem*)));
		radio_fixed->setChecked(true);
	}
}

void
GPlatesQtWidgets::HellingerNewSegment::item_changed(QStandardItem *item)
{
	int column = item->column();
	int row = item->row();
	QModelIndex index = model ->index(row, column, QModelIndex());
	QString value = table_new_segment->model()->data(
				table_new_segment->model()->index( index.row() , index.column() ) )
			.toString();
	double value_double = value.toDouble();

	if (column == COLUMN_MOVING_FIXED)
	{
		if (value_double < MOVING_SEGMENT_TYPE)
		{
			model->setData(index,1);
		}
		else if (value_double > FIXED_SEGMENT_TYPE)
		{
			model->setData(index,2);
		}
		change_quick_set_state();
	}
	else if (column == COLUMN_LAT)
	{
		if (value_double < -90.00)
		{
			model->setData(index,-90.00);
		}
		else if (value_double > 90.00)
		{
			model->setData(index,90.00);
		}
	}
	else if (column == COLUMN_LON)
	{
		if (value_double < -360.00)
		{
			model->setData(index,-360.00);
		}
		else if (value_double > 360.00)
		{
			model->setData(index,360.00);
		}
	}
	else if (column == COLUMN_UNCERTAINTY)
	{
		if (value_double < 0.00)
		{
			model->setData(index,0.00);
		}
		else if (value_double > 999.00)
		{
			model->setData(index,999.00);
		}
	}

}

void
GPlatesQtWidgets::HellingerNewSegment::change_quick_set_state()
{

	int value = table_new_segment->model()->data(
				table_new_segment->model()->index( 0, 0 ) ).toInt();

	for (double row = 0; row < d_row_count; ++row)
	{
		QModelIndex index_compare = model ->index(row, 0, QModelIndex());
		int value_compare = table_new_segment->model()->data(
					table_new_segment->model()->index( index_compare.row(), 0 ) ).toInt();
		if (value != value_compare)
		{
			radio_custom->setChecked(true);
		}

	}


}

void
GPlatesQtWidgets::HellingerNewSegment::update_buttons()
{

}





