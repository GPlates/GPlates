/* $Id: HellingerEditSegment.cc 241 2012-02-28 11:28:13Z robin.watson@ngu.no $ */

/**
 * \file 
 * $Revision: 241 $
 * $Date: 2012-02-28 12:28:13 +0100 (Tue, 28 Feb 2012) $ 
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


#include <string>

#include <QDebug>
#include <QFileDialog>
#include <QLocale>
#include <QRadioButton>
#include <QTextStream>

#include "global/CompilerWarnings.h"
#include "HellingerDialog.h"
#include "HellingerEditSegment.h"
#include "HellingerNewSegmentError.h"
#include "HellingerDialogUi.h"
#include "QtWidgetUtils.h"

GPlatesQtWidgets::HellingerEditSegment::HellingerEditSegment(
                HellingerDialog *hellinger_dialog,
                HellingerModel *hellinger_model,
                QWidget *parent_):
        QDialog(parent_,Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
        d_hellinger_dialog_ptr(hellinger_dialog),
        d_hellinger_model_ptr(hellinger_model),
        d_number_rows(0),
        d_segment(0),
        d_disabled_picks(0),
        d_active_picks(0),
        d_hellinger_new_segment_error(0)

{
	setupUi(this);

        QObject::connect(button_edit_segment, SIGNAL(clicked()), this, SLOT(edit()));
        QObject::connect(button_add_line, SIGNAL(clicked()), this, SLOT(add_line()));
        QObject::connect(button_remove_line, SIGNAL(clicked()), this, SLOT(remove_line()));
        QObject::connect(radiobtn_move, SIGNAL(clicked()), this, SLOT(change_table_stats_pick()));
        QObject::connect(radiobtn_fixed, SIGNAL(clicked()), this, SLOT(change_table_stats_pick()));
        QObject::connect(radiobtn_custom, SIGNAL(clicked()), this, SLOT(change_table_stats_pick()));
        update_buttons();
}

void
GPlatesQtWidgets::HellingerEditSegment::initialization_table(QStringList &input_value)
{

    model = new QStandardItemModel(d_number_rows,4, this);

    model->setHorizontalHeaderItem(0, new QStandardItem("Move/Fix"));
    model->setHorizontalHeaderItem(1, new QStandardItem("Lat"));
    model->setHorizontalHeaderItem(2, new QStandardItem("Long"));
    model->setHorizontalHeaderItem(3, new QStandardItem("Error"));

    if (!input_value.isEmpty())
    {
        int a=0;
        for (double row = 0; row < d_number_rows; ++row)
        {
                for (double col = 0; col < 4; ++col)
                {

                    QModelIndex index = model ->index(row, col, QModelIndex());

                    model ->setData(index, input_value.at(a).toDouble());
                    a++;
                }

            a++;
        }

    }

    tableView->setModel(model);
    connect(model,SIGNAL(itemChanged(QStandardItem*)),this,SLOT(handle_item_changed(QStandardItem*)));
}

void
GPlatesQtWidgets::HellingerEditSegment::initialization(int &segment)
{
    QStringList get_data_segment = d_hellinger_model_ptr->get_segment(segment);
    check_picks(get_data_segment);
    d_segment = segment;
    spinbox_segment->setValue(d_segment);

}

void
GPlatesQtWidgets::HellingerEditSegment::check_picks(QStringList &picks)
{
    int position = 4;
    d_disabled_picks.clear();
    d_active_picks.clear();
    for (int iter = 0; iter<picks.count();++iter)
    {

        if (position == iter+4)
        {            
            if (picks.at(iter+4).toInt()==0)
            {
                d_disabled_picks<<picks.at(iter)<<picks.at(iter+1)<<picks.at(iter+2)<<picks.at(iter+3)<<picks.at(iter+4);
            }
            else
            {
                d_active_picks<<picks.at(iter)<<picks.at(iter+1)<<picks.at(iter+2)<<picks.at(iter+3)<<picks.at(iter+4);
            }
            position = position+5;
        }                
    }
    int model_column_count = 5;
    int count_picks = d_active_picks.count()/model_column_count;
    d_number_rows = count_picks;
    initialization_table(d_active_picks);

}

void
GPlatesQtWidgets::HellingerEditSegment::edit()
{

	if (d_hellinger_model_ptr->segment_number_exists(d_segment))
    {
        if (!d_hellinger_new_segment_error)
        {
            d_hellinger_new_segment_error = new GPlatesQtWidgets::HellingerNewSegmentError(d_hellinger_dialog_ptr);
        }

        d_hellinger_new_segment_error->exec(); // necessary for applied changes!
        int value_error = d_hellinger_new_segment_error->error_type_new_segment();
        if (value_error == ERROR_ADD_NEW_SEGMENT)
        {
            edit_segment();
        }
        else if (value_error == ERROR_REPLACE_NEW_SEGMENT)
        {
            d_hellinger_model_ptr->remove_segment(d_segment);
            edit_segment();

        }
        else if (value_error == ERROR_INSERT_NEW_SEGMENT)
        {
			d_hellinger_model_ptr->reorder_segment(d_segment);
            edit_segment();
        }
        else
        {
            return;
        }
    }
    else
    {
        edit_segment();
    }


}

void
GPlatesQtWidgets::HellingerEditSegment::edit_segment()
{
    QStringList value_from_table;
    double segment = spinbox_segment -> value();
    QString segment_str = QString("%1").arg(segment);
//    d_hellinger_model_ptr->remove_segment(d_segment);
    QStringList data_to_model;
    for (double row = 0; row < d_number_rows; ++row)
    {        
        for (double col = 0; col < 4; ++col)
        {
            QModelIndex index = model ->index(row, col, QModelIndex());
            QString value = tableView->model()->data(
                            tableView->model()->index( index.row() , index.column() ) )
                            .toString();
            value_from_table << value;
          }
        QString is_enabled = "1";
        data_to_model<<value_from_table.at(0)<<segment_str<<value_from_table.at(1)<<value_from_table.at(2)<<value_from_table.at(3)<<is_enabled;
        d_hellinger_model_ptr->add_pick(data_to_model);
        value_from_table.clear();
        data_to_model.clear();
    }
    if (!d_disabled_picks.empty())
    {
        int position = 0;
        int count_disabled_picks = d_disabled_picks.count()/5;
        for (int iter = 0; iter<count_disabled_picks;++iter)
        {
            QString is_disabled = "0";
			QString comment_moving_segment_str = QString("%1").arg(DISABLED_MOVING_SEGMENT_TYPE);
			QString comment_fixed_segment_str = QString("%1").arg(DISABLED_FIXED_SEGMENT_TYPE);
            if (d_disabled_picks.at(position).toInt()==MOVING_SEGMENT_TYPE)
            {
                data_to_model<<comment_moving_segment_str<<segment_str<<d_disabled_picks.at(position+1)<<d_disabled_picks.at(position+2)<<d_disabled_picks.at(position+3)<<is_disabled;
            }
            else if(d_disabled_picks.at(position).toInt()==FIXED_SEGMENT_TYPE)
            {
                data_to_model<<comment_fixed_segment_str<<segment_str<<d_disabled_picks.at(position+1)<<d_disabled_picks.at(position+2)<<d_disabled_picks.at(position+3)<<is_disabled;
            }

            position = position+5;
            d_hellinger_model_ptr->add_pick(data_to_model);
            data_to_model.clear();
        }
    }
    d_hellinger_dialog_ptr ->update();
}
void
GPlatesQtWidgets::HellingerEditSegment::add_line()
{
    const QModelIndex index = tableView->currentIndex();
    int row = index.row();
    int col = index.column();
    model->insertRow(row);
        for (col = 0; col < 4; ++col)
        {
            QModelIndex index_m = model ->index(row, col, QModelIndex());
            model ->setData(index_m, 0.00);
        }
    ++d_number_rows;
    QModelIndex index_move_fix = model ->index(row, 0, QModelIndex());
    model ->setData(index_move_fix, 1);
}
void
GPlatesQtWidgets::HellingerEditSegment::remove_line()
{
    const QModelIndex index = tableView->currentIndex();
    int row = index.row();
    model->removeRow(row);
    d_number_rows = d_number_rows-1;
}

void
GPlatesQtWidgets::HellingerEditSegment::change_table_stats_pick()
{
    if (radiobtn_move->isChecked())
    {
        for (double row = 0; row < d_number_rows; ++row)
        {
            QModelIndex index_move_fix = model ->index(row, 0, QModelIndex());
            model->setData(index_move_fix, 1);
            disconnect(model,SIGNAL(itemChanged(QStandardItem*)),this,SLOT(handle_item_changed(QStandardItem*)));
        }
         connect(model,SIGNAL(itemChanged(QStandardItem*)),this,SLOT(handle_item_changed(QStandardItem*)));
         radiobtn_move->setChecked(true);
    }
    else if (radiobtn_fixed->isChecked())
    {
        for (double row = 0; row < d_number_rows; ++row)
        {
            QModelIndex index_move_fix = model ->index(row, 0, QModelIndex());
            model->setData(index_move_fix, 2);
            disconnect(model,SIGNAL(itemChanged(QStandardItem*)),this,SLOT(handle_item_changed(QStandardItem*)));
        }
         connect(model,SIGNAL(itemChanged(QStandardItem*)),this,SLOT(handle_item_changed(QStandardItem*)));
         radiobtn_fixed->setChecked(true);
    }

}

void
GPlatesQtWidgets::HellingerEditSegment::handle_item_changed(QStandardItem *item)
{
    int column = item->column();
    int row = item->row();
    QModelIndex index = model ->index(row, column, QModelIndex());
    QString value = tableView->model()->data(
                tableView->model()->index( index.row() , index.column() ) )
                .toString();
    double value_double = value.toDouble();

    if (column == COLUMN_MOVE_FIX)
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
    else if (column == COLUMN_ERROR)
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
GPlatesQtWidgets::HellingerEditSegment::change_quick_set_state()
{

    int value = tableView->model()->data(
                    tableView->model()->index( 0, 0 ) ).toInt();
    for (double row = 0; row < d_number_rows; ++row)
    {
        QModelIndex index_compare = model ->index(row, 0, QModelIndex());
        int value_compare = tableView->model()->data(
                        tableView->model()->index( index_compare.row(), 0 ) ).toInt();
        if (value != value_compare)
        {
            radiobtn_custom->setChecked(true);
        }
    }    



}

void
GPlatesQtWidgets::HellingerEditSegment::update_buttons()
{
    
}





