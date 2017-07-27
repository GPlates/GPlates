/* $Id: HellingerSegmentDialog.cc 241 2012-02-28 11:28:13Z robin.watson@ngu.no $ */

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
#include <QPainter>
#include <QRadioButton>
#include <QTableView>

#include "utils/ComponentManager.h"
#include "HellingerDialog.h"
#include "HellingerModel.h"
#include "HellingerSegmentDialog.h"
#include "HellingerNewSegmentWarning.h"
#include "QtWidgetUtils.h"

/**
 * @brief DEFAULT_UNCERTAINTY - initial uncertainty (km) to use in new picks.
 * Candidate for settings/preferences.
 */
const double DEFAULT_UNCERTAINTY = 5.;

namespace
{

	/**
	 * @brief translate_segment_type
	 *	Convert MOVING/DISABLED_MOVING types to a QString form of MOVING; similarly for FIXED/DISABLED_FIXED.
	 *
	 * This is copied from HellingerDialog anonymous namespace - could be moved into a common HellingerUtils file, but
	 * this is the only candidate for that at the moment.
	 * @param type
	 * @return
	 */
	QString
	translate_segment_type(
			GPlatesQtWidgets::HellingerPlateIndex type)
	{
		switch(type)
		{
		case GPlatesQtWidgets::PLATE_ONE_PICK_TYPE:
		case GPlatesQtWidgets::DISABLED_PLATE_ONE_PICK_TYPE:
			return QString::number(GPlatesQtWidgets::PLATE_ONE_PICK_TYPE);
			break;
		case GPlatesQtWidgets::PLATE_TWO_PICK_TYPE:
		case GPlatesQtWidgets::DISABLED_PLATE_TWO_PICK_TYPE:
			return QString::number(GPlatesQtWidgets::PLATE_TWO_PICK_TYPE);
			break;
		case GPlatesQtWidgets::PLATE_THREE_PICK_TYPE:
		case GPlatesQtWidgets::DISABLED_PLATE_THREE_PICK_TYPE:
			return QString::number(GPlatesQtWidgets::PLATE_THREE_PICK_TYPE);
			break;
		default:
			return QString();
		}
	}

	void
	update_entire_row(QTableView *table, const QModelIndex &index)
	{
		int row = index.row();
		for (int i = 0; i < GPlatesQtWidgets::HellingerSegmentDialog::NUM_COLUMNS; ++i)
		{
			QModelIndex cell_index =  index.model()->index(row,i);
			table->update(cell_index);
		}
	}

}

GPlatesQtWidgets::HellingerSegmentDialog::HellingerSegmentDialog(
		HellingerDialog *hellinger_dialog,
		HellingerModel *hellinger_model,
		bool create_new_segment):
	QDialog(hellinger_dialog,
			Qt::CustomizeWindowHint |
			Qt::WindowTitleHint |
			Qt::WindowSystemMenuHint |
			Qt::WindowStaysOnTopHint),
	d_hellinger_dialog_ptr(hellinger_dialog),
	d_hellinger_model_ptr(hellinger_model),
	d_hellinger_new_segment_warning(0),
	d_spin_box_delegate(new GPlatesQtWidgets::SpinBoxDelegate(
							GPlatesUtils::ComponentManager::instance().is_enabled(
								GPlatesUtils::ComponentManager::Component::hellinger_three_plate()),
							this)),
	d_creating_new_segment(create_new_segment),
	d_current_row(0),
	d_three_way_fitting_is_enabled(
		GPlatesUtils::ComponentManager::instance().is_enabled(
			GPlatesUtils::ComponentManager::Component::hellinger_three_plate()))
{
	setupUi(this);
	QObject::connect(button_add_segment, SIGNAL(clicked()), this, SLOT(handle_add_segment()));
	QObject::connect(button_add_line, SIGNAL(clicked()), this, SLOT(handle_add_line()));
	QObject::connect(button_remove_line, SIGNAL(clicked()), this, SLOT(handle_remove_line()));
	QObject::connect(radio_plate_index_1, SIGNAL(clicked()), this, SLOT(change_pick_type_of_whole_table()));
	QObject::connect(radio_plate_index_2, SIGNAL(clicked()), this, SLOT(change_pick_type_of_whole_table()));
	QObject::connect(radio_plate_index_3, SIGNAL(clicked()), this, SLOT(change_pick_type_of_whole_table()));
	QObject::connect(radio_custom, SIGNAL(clicked()), this, SLOT(change_pick_type_of_whole_table()));
	QObject::connect(button_reset,SIGNAL(clicked()), this, SLOT(handle_reset()));
	QObject::connect(button_enable,SIGNAL(clicked()), this, SLOT(handle_enable()));
	QObject::connect(button_disable,SIGNAL(clicked()), this, SLOT(handle_disable()));
	QObject::connect(button_cancel,SIGNAL(clicked()),this,SLOT(close()));

	// NOTE: I've added these two so that the "remove" button is enabled whenever a row/cell is highlighted.
	// Initially nothing is selected so it would be unclear which row is the target of the removal operation.
	QObject::connect(table_new_segment->verticalHeader(),SIGNAL(sectionClicked(int)),this,SLOT(update_buttons()));
	QObject::connect(table_new_segment,SIGNAL(clicked(QModelIndex)),this,SLOT(update_buttons()));

	d_table_model = new QStandardItemModel(NUM_COLUMNS,1, this);
	d_table_model->setHorizontalHeaderItem(COLUMN_PLATE_INDEX,new QStandardItem("Plate index"));
	d_table_model->setHorizontalHeaderItem(COLUMN_LAT, new QStandardItem("Lat"));
	d_table_model->setHorizontalHeaderItem(COLUMN_LON, new QStandardItem("Long"));
	d_table_model->setHorizontalHeaderItem(COLUMN_UNCERTAINTY, new QStandardItem("Uncertainty (km)"));

	// We need to specify this header even though we're not going to display it. If we don't
	// provide it, the model thinks it only has 4 columns (it returns (-1,-1) as index column/row
	// for anything in the COLUMN_ENABLED column.
	d_table_model->setHorizontalHeaderItem(COLUMN_ENABLED, new QStandardItem("Enabled"));

	d_table_model->setRowCount(1);

	set_initial_row_values(0);

	table_new_segment->setModel(d_table_model);
	table_new_segment->setColumnHidden(COLUMN_ENABLED,true);
	table_new_segment->horizontalHeader()->resizeSection(COLUMN_PLATE_INDEX,140);
	table_new_segment->horizontalHeader()->resizeSection(COLUMN_LAT,100);
	table_new_segment->horizontalHeader()->resizeSection(COLUMN_LON,100);
	table_new_segment->horizontalHeader()->resizeSection(COLUMN_UNCERTAINTY,100);

	table_new_segment->horizontalHeader()->setStretchLastSection(true);

	update_buttons();

	// The spinbox delegate lets us customise spinbox behaviour for the different cells.
	table_new_segment->setItemDelegate(d_spin_box_delegate);

	// Mark row 0 (or at least an item in row 0) as the current index.
	QModelIndex index = d_table_model->index(0,COLUMN_PLATE_INDEX);
	table_new_segment->selectionModel()->setCurrentIndex(index,QItemSelectionModel::NoUpdate);

	if (create_new_segment)
	{
		setWindowTitle(QObject::tr("Create New Segment"));
	}
	if (!create_new_segment)
	{
		button_add_segment->setText(QObject::tr("Apply"));
		setWindowTitle(QObject::tr("Edit Segment"));
	}

	// Wait until table is initialised before we set up this connection.
	QObject::connect(table_new_segment->selectionModel(),
					 SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
					 this,
					 SLOT(handle_selection_changed(QItemSelection,QItemSelection)));

	radio_plate_index_3->setVisible(d_three_way_fitting_is_enabled);

}

void
GPlatesQtWidgets::HellingerSegmentDialog::initialise_with_segment(
		const int &segment_number)
{
	d_original_segment_number.reset(segment_number);

	fill_widgets();
}

void
GPlatesQtWidgets::HellingerSegmentDialog::initialise()
{
	d_table_model->setRowCount(1);
	set_initial_row_values(0);
	d_original_segment_number.reset(1);
}

boost::optional<GPlatesQtWidgets::HellingerPick>
GPlatesQtWidgets::HellingerSegmentDialog::current_pick() const
{
	return d_current_pick;
}

void
GPlatesQtWidgets::HellingerSegmentDialog::update_pick_coords(
		const GPlatesMaths::LatLonPoint &llp)
{
	HellingerPick pick;

	// Some default values while testing.
	pick.d_is_enabled = true;
	pick.d_uncertainty = 5.;
	pick.d_segment_type = GPlatesQtWidgets::PLATE_ONE_PICK_TYPE;


	pick.d_lat = llp.latitude();
	pick.d_lon = llp.longitude();

	set_row_values(d_current_row,pick);

}

void
GPlatesQtWidgets::HellingerSegmentDialog::begin_segment_operation()
{
	initialise();
	show();
	raise();
	setEnabled(true);
}

void
GPlatesQtWidgets::HellingerSegmentDialog::handle_selection_changed(
		const QItemSelection &,
		const QItemSelection &)
{
	update_buttons();
}

void
GPlatesQtWidgets::HellingerSegmentDialog::fill_widgets()
{
	if (d_original_segment_number)
	{
		spinbox_segment->setValue(*d_original_segment_number);

		d_table_model->removeRows(0,d_table_model->rowCount());


		hellinger_model_const_range_type range =
				d_hellinger_model_ptr->get_segment_as_range(*d_original_segment_number);

		hellinger_model_type::const_iterator iter = range.first,
				iter_end = range.second;
		for (; iter != iter_end ; ++iter)
		{
			d_table_model->insertRow(d_table_model->rowCount());
			set_row_values(d_table_model->rowCount()-1,iter->second);
		}
	}
}


void
GPlatesQtWidgets::HellingerSegmentDialog::handle_add_segment()
{

	// NOTE: We don't check for contiguous segment numbers here. It could be an idea to
	// check for this here and suggest the next "available" segment number if the user has
	// entered a value greater than (highest-so-far)+1. The contiguity is checked and corrected
	// before performing the fit anyway, so it doesn't have to be here by any means.


	if (!d_creating_new_segment && d_original_segment_number)
	{
		handle_edited_segment();
	}
	else
	{
		handle_new_segment();
	}

}

void
GPlatesQtWidgets::HellingerSegmentDialog::add_segment_to_model()
{
	int segment = spinbox_segment->value();

	for (int row = 0; row < d_table_model->rowCount(); ++row)
	{
		QModelIndex index;
		QVariant variant;

		// Moving or fixed
		index = d_table_model->index(row,COLUMN_PLATE_INDEX);
		variant = table_new_segment->model()->data(index);
		// The spinboxes should already ensure valid data types/values for each column.
		HellingerPlateIndex type = static_cast<HellingerPlateIndex>(variant.toInt());

		// Latitude
		index = d_table_model->index(row,COLUMN_LAT);
		variant = table_new_segment->model()->data(index);
		double lat = variant.toDouble();

		// Longitude
		index = d_table_model->index(row,COLUMN_LON);
		variant = table_new_segment->model()->data(index);
		double lon = variant.toDouble();

		// Uncertainty
		index = d_table_model->index(row,COLUMN_UNCERTAINTY);
		variant = table_new_segment->model()->data(index);
		double uncertainty = variant.toDouble();

		// Enabled
		index = d_table_model->index(row,COLUMN_ENABLED);
		bool enabled = table_new_segment->model()->data(index).toBool();

		GPlatesQtWidgets::HellingerPick pick(type,lat,lon,uncertainty,enabled);
		d_hellinger_model_ptr->add_pick(pick,segment);
	}
	d_hellinger_dialog_ptr->update_after_new_or_edited_segment(segment);
}

void
GPlatesQtWidgets::HellingerSegmentDialog::handle_add_line()
{
	int insertion_row;
	if ((d_table_model->rowCount() == 0) ||
			table_new_segment->selectionModel()->selection().indexes().isEmpty())
	{
		insertion_row = 0;
	}
	else
	{

		const QModelIndex index = table_new_segment->currentIndex();
		insertion_row = index.row();
	}

	d_table_model->insertRow(insertion_row);
	set_initial_row_values(insertion_row);
	update_buttons();
}

void
GPlatesQtWidgets::HellingerSegmentDialog::handle_remove_line()
{
	if (table_new_segment->selectionModel()->selection().indexes().isEmpty())
	{
		return;
	}
	const QModelIndex index = table_new_segment->currentIndex();
	int row = index.row();
	d_table_model->removeRow(row);
	update_buttons();
}

void
GPlatesQtWidgets::HellingerSegmentDialog::change_pick_type_of_whole_table()
{
	HellingerPlateIndex index;
	if (radio_plate_index_1->isChecked())
	{
		index = GPlatesQtWidgets::PLATE_ONE_PICK_TYPE;
	}
	else if (radio_plate_index_2->isChecked())
	{
		index = GPlatesQtWidgets::PLATE_TWO_PICK_TYPE;
	}
	else if (radio_plate_index_3->isChecked())
	{
		index = GPlatesQtWidgets::PLATE_THREE_PICK_TYPE;
	}
	else
	{
		// The custom button was checked.
		// What value should 'index' be ?
		return;
	}

	for (int row = 0; row < d_table_model->rowCount(); ++row)
	{
		QModelIndex index_move_fix = d_table_model ->index(row, COLUMN_PLATE_INDEX, QModelIndex());
		d_table_model->setData(index_move_fix, index);
	}
}

void
GPlatesQtWidgets::HellingerSegmentDialog::update_buttons()
{
	button_enable->setEnabled(false);
	button_disable->setEnabled(false);
	QModelIndexList indices = table_new_segment->selectionModel()->selection().indexes();
	button_remove_line->setEnabled(!indices.isEmpty());
	button_add_segment->setEnabled(d_table_model->rowCount()!=0);

	if (indices.isEmpty())
	{
		return;
	}

	QModelIndex index = indices.front();
	int selected_row = index.row();
	QModelIndex enabled_index = d_table_model->index(selected_row,COLUMN_ENABLED);
	bool enabled = d_table_model->data(enabled_index).toBool();
	button_enable->setEnabled(!enabled);
	button_disable->setEnabled(enabled);
}

void
GPlatesQtWidgets::HellingerSegmentDialog::handle_reset()
{
	if (d_creating_new_segment)
	{
		initialise();
	}
	else
	{
		fill_widgets();
	}
}

void
GPlatesQtWidgets::HellingerSegmentDialog::handle_enable()
{
	QModelIndexList indices = table_new_segment->selectionModel()->selection().indexes();
	if (indices.isEmpty())
	{
		return;
	}
	QModelIndex index = indices.front();

	QModelIndex index_of_enabled_column_in_row_of_selected_cell =
			index.model()->index(index.row(),GPlatesQtWidgets::HellingerSegmentDialog::COLUMN_ENABLED);
	d_table_model->setData(index_of_enabled_column_in_row_of_selected_cell,true);

	update_entire_row(table_new_segment,index);
	table_new_segment->selectionModel()->setCurrentIndex(index,QItemSelectionModel::Select);
	update_buttons();
}

void
GPlatesQtWidgets::HellingerSegmentDialog::handle_disable()
{
	QModelIndexList indices = table_new_segment->selectionModel()->selection().indexes();
	if (indices.isEmpty())
	{
		return;
	}
	QModelIndex index = indices.front();

	QModelIndex index_of_enabled_column_in_row_of_selected_cell =
			index.model()->index(index.row(),GPlatesQtWidgets::HellingerSegmentDialog::COLUMN_ENABLED);
	d_table_model->setData(index_of_enabled_column_in_row_of_selected_cell,false);
	update_entire_row(table_new_segment,index);
	table_new_segment->selectionModel()->setCurrentIndex(index,QItemSelectionModel::Select);
	update_buttons();
}

void
GPlatesQtWidgets::HellingerSegmentDialog::close()
{
	reject();
}

void
GPlatesQtWidgets::HellingerSegmentDialog::reject()
{
	Q_EMIT finished_editing();
	QDialog::reject();
}

void
GPlatesQtWidgets::HellingerSegmentDialog::handle_edited_segment()
{
	int segment_number = spinbox_segment->value();

	if (d_original_segment_number.get() == segment_number)
	{
		d_hellinger_model_ptr->remove_segment(d_original_segment_number.get());
		add_segment_to_model();
	}
	else if(d_hellinger_model_ptr->segment_number_exists(segment_number))
	{

		if (!d_hellinger_new_segment_warning)
		{
			d_hellinger_new_segment_warning = new GPlatesQtWidgets::HellingerNewSegmentWarning(this);
		}

		d_hellinger_new_segment_warning->initialise(segment_number);
		d_hellinger_new_segment_warning->exec();
		int value_error = d_hellinger_new_segment_warning->error_type_new_segment();
		if (value_error == ACTION_ADD_TO_EXISTING_SEGMENT)
		{
			d_hellinger_model_ptr->remove_segment(d_original_segment_number.get());
			add_segment_to_model();
		}
		else if (value_error == ACTION_REPLACE_SEGMENT)
		{
			d_hellinger_model_ptr->remove_segment(segment_number);
			d_hellinger_model_ptr->remove_segment(d_original_segment_number.get());
			add_segment_to_model();
		}
		else if (value_error == ACTION_INSERT_NEW_SEGMENT)
		{
			d_hellinger_model_ptr->remove_segment(d_original_segment_number.get());
			d_hellinger_model_ptr->make_space_for_new_segment(segment_number);
			add_segment_to_model();
		}
		else
		{
			// We should only get here if the user pressed cancel. In this case we return,
			// which will keep this dialog open so that the user can adjust the fields in their
			// prospective new segment and try again if they want to.
			return;
		}
	}
	else
	{
		// Everything was cool.
		d_hellinger_model_ptr->remove_segment(d_original_segment_number.get());
		add_segment_to_model();
	}
	close();
}

void
GPlatesQtWidgets::HellingerSegmentDialog::handle_new_segment()
{
	int segment_number = spinbox_segment->value();

	if(d_hellinger_model_ptr->segment_number_exists(segment_number))
	{
		if (!d_hellinger_new_segment_warning)
		{
			d_hellinger_new_segment_warning = new GPlatesQtWidgets::HellingerNewSegmentWarning(this);

		}

		d_hellinger_new_segment_warning->initialise(segment_number);
		d_hellinger_new_segment_warning->exec();
		int value_error = d_hellinger_new_segment_warning->error_type_new_segment();
		switch(value_error)
		{
		case ACTION_ADD_TO_EXISTING_SEGMENT:
			add_segment_to_model();
			break;
		case ACTION_REPLACE_SEGMENT:
			d_hellinger_model_ptr->remove_segment(segment_number);
			add_segment_to_model();
			break;
		case ACTION_INSERT_NEW_SEGMENT:
			d_hellinger_model_ptr->make_space_for_new_segment(segment_number);
			add_segment_to_model();
			break;
		default:
			// We should only get here if the user pressed cancel. In this case we return,
			// which will keep this dialog open so that the user can adjust the fields in their
			// prospective new segment and try again if they want to.
			return;
		}
	}
	else
	{
		// Everything was cool.
		add_segment_to_model();
	}
	close();
}

void
GPlatesQtWidgets::HellingerSegmentDialog::set_initial_row_values(
		const int &row)
{
	QModelIndex index_plate_index = d_table_model->index(row, COLUMN_PLATE_INDEX);
	d_table_model->setData(index_plate_index, 1);

	QModelIndex index_enabled = d_table_model->index(row, COLUMN_ENABLED);
	d_table_model->setData(index_enabled, true);

	QModelIndex index_lat = d_table_model->index(row, COLUMN_LAT);
	d_table_model->setData(index_lat, 0.);

	QModelIndex index_lon = d_table_model->index(row, COLUMN_LON);
	d_table_model->setData(index_lon, 0.);

	QModelIndex index_uncertainty = d_table_model->index(row, COLUMN_UNCERTAINTY);
	d_table_model->setData(index_uncertainty, DEFAULT_UNCERTAINTY);
}

void
GPlatesQtWidgets::HellingerSegmentDialog::set_row_values(
		const int &row,
		const GPlatesQtWidgets::HellingerPick &pick)
{
	QModelIndex index = d_table_model->index(row,COLUMN_PLATE_INDEX);
	d_table_model->setData(index,translate_segment_type(pick.d_segment_type));

	index = d_table_model->index(row,COLUMN_LAT);
	//QVariant v = QVariant(QString::number(pick.d_lat,'g',6));
	d_table_model->setData(index,QVariant(QString::number(pick.d_lat,'g',6)));

	index = d_table_model->index(row,COLUMN_LON);
	d_table_model->setData(index,QVariant(QString::number(pick.d_lon,'g',6)));

	index = d_table_model->index(row,COLUMN_UNCERTAINTY);
	d_table_model->setData(index,pick.d_uncertainty);

	index = d_table_model->index(row,COLUMN_ENABLED);
	d_table_model->setData(index,pick.d_is_enabled);

}


GPlatesQtWidgets::SpinBoxDelegate::SpinBoxDelegate(
		bool three_way_fitting_is_enabled,
		QObject *parent_):
	QItemDelegate(parent_),
	d_three_way_fitting_is_enabled(three_way_fitting_is_enabled)
{}

QWidget*
GPlatesQtWidgets::SpinBoxDelegate::createEditor(
		QWidget *parent_,
		const QStyleOptionViewItem &/* option */,
		const QModelIndex &index) const
{
	int column = index.column();

	switch(column){
	case GPlatesQtWidgets::HellingerSegmentDialog::COLUMN_PLATE_INDEX:
	{
		QSpinBox *editor = new QSpinBox(parent_);
		editor->setMinimum(1);
		editor->setMaximum(d_three_way_fitting_is_enabled ? 3 : 2);
		return editor;
		break;
	}
	case GPlatesQtWidgets::HellingerSegmentDialog::COLUMN_LAT:
	{
		QDoubleSpinBox *editor = new QDoubleSpinBox(parent_);
		editor->setDecimals(4);
		editor->setMinimum(-90.);
		editor->setMaximum(90.);
		return editor;
		break;
	}
	case GPlatesQtWidgets::HellingerSegmentDialog::COLUMN_LON:
	{
		QDoubleSpinBox *editor = new QDoubleSpinBox(parent_);
		editor->setDecimals(4);
		editor->setMinimum(-360.);
		editor->setMaximum(360.);
		return editor;
		break;
	}
	case GPlatesQtWidgets::HellingerSegmentDialog::COLUMN_UNCERTAINTY:
	default:
	{
		QDoubleSpinBox *editor = new QDoubleSpinBox(parent_);
		editor->setDecimals(4);
		editor->setMinimum(0.);
		editor->setMaximum(1000.);
		return editor;
		break;
	}
	}
}

void
GPlatesQtWidgets::SpinBoxDelegate::setEditorData(
		QWidget *editor,
		const QModelIndex &index) const
{
	int column = index.column();

	switch(column){
	case GPlatesQtWidgets::HellingerSegmentDialog::COLUMN_PLATE_INDEX:
	{
		int value = index.model()->data(index, Qt::EditRole).toInt();
		QSpinBox *spinbox = static_cast<QSpinBox*>(editor);
		spinbox->setValue(value);
		break;
	}
	case GPlatesQtWidgets::HellingerSegmentDialog::COLUMN_LAT:
	case GPlatesQtWidgets::HellingerSegmentDialog::COLUMN_LON:
	case GPlatesQtWidgets::HellingerSegmentDialog::COLUMN_UNCERTAINTY:
	{
		int value = index.model()->data(index, Qt::EditRole).toDouble();
		QDoubleSpinBox *spinbox = static_cast<QDoubleSpinBox*>(editor);
		spinbox->setValue(value);
		break;
	}

	}

}

void
GPlatesQtWidgets::SpinBoxDelegate::setModelData(
		QWidget *editor,
		QAbstractItemModel *model,
		const QModelIndex &index) const
{
	int column = index.column();

	QVariant value;
	switch(column){
	case GPlatesQtWidgets::HellingerSegmentDialog::COLUMN_PLATE_INDEX:
	{
		QSpinBox *spinbox = static_cast<QSpinBox*>(editor);
		value = spinbox->value();
		break;
	}
	case GPlatesQtWidgets::HellingerSegmentDialog::COLUMN_LAT:
	case GPlatesQtWidgets::HellingerSegmentDialog::COLUMN_LON:
	case GPlatesQtWidgets::HellingerSegmentDialog::COLUMN_UNCERTAINTY:
	{
		QDoubleSpinBox *spinbox = static_cast<QDoubleSpinBox*>(editor);
		value = spinbox->value();
		break;
	}
	}
	model->setData(index,value,Qt::EditRole);

}

void
GPlatesQtWidgets::SpinBoxDelegate::updateEditorGeometry(
		QWidget *editor,
		const QStyleOptionViewItem &option,
		const QModelIndex &/* index */) const
{
	editor->setGeometry(option.rect);
}

void
GPlatesQtWidgets::SpinBoxDelegate::paint(
		QPainter *painter,
		const QStyleOptionViewItem &option,
		const QModelIndex &index) const
{

	// Get the row of the current index, and then get the data in the "enabled" column for that row.

	QModelIndex index_of_enabled_column_in_row_of_selected_cell =
			index.model()->index(index.row(),GPlatesQtWidgets::HellingerSegmentDialog::COLUMN_ENABLED);
	bool enabled = index.model()->data(index_of_enabled_column_in_row_of_selected_cell).toBool();

	painter->setPen(enabled? Qt::black : Qt::gray);
	painter->drawText(option.rect, Qt::AlignCenter,index.data().toString());
}
