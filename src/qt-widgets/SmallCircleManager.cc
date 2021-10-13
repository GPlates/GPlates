/* $Id: PanMap.h 8194 2010-04-26 16:24:42Z rwatson $ */

/**
 * \file 
 * $Revision: 8194 $
 * $Date: 2010-04-26 18:24:42 +0200 (ma, 26 apr 2010) $ 
 * 
 * Copyright (C)  2010 Geological Survey of Norway
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
#include <QHeaderView>
#include <QMessageBox>
#include <QTableWidget>
#include <QTableWidgetItem>

#include "CreateSmallCircleDialog.h"
#include "SmallCircleManager.h"

#include "app-logic/ApplicationState.h"
#include "maths/LatLonPoint.h"
#include "maths/MathsUtils.h"
#include "view-operations/RenderedGeometryCollection.h"
#include "view-operations/RenderedGeometryFactory.h"

namespace
{
	void
	remove_row(
		int row,
		std::vector<GPlatesMaths::SmallCircle> &small_circle_collection,
		QTableWidget *table_widget)
	{
		if ((row < 0) || (row >= static_cast<int>(small_circle_collection.size())))
		{
			return;
		}
		small_circle_collection.erase(small_circle_collection.begin() + row);
		table_widget->removeRow(row);
	}

	/**
	 * This removes contiguous rows from a QTableWidget specified by the the table widget's 
	 * selectedRanges() function.
	 *
	 * It will only behave correctly if the widget's "selectionMode" is set to "ContiguousSelection". 
	 * In this case, the QList should have only one entry.
	 *
	 *
	 */
	void
	remove_rows(
		std::vector<GPlatesMaths::SmallCircle> &small_circle_collection,
		QTableWidget *table_widget)
	{
		QList<QTableWidgetSelectionRange> range = table_widget->selectedRanges();


		QList<QTableWidgetSelectionRange>::const_iterator it = range.begin();

		for (; it != range.end() ; ++it)
		{
			int row_to_remove = it->topRow();
			int number_of_rows_to_remove = it->rowCount();

			for (int i = 0 ; i < number_of_rows_to_remove ; ++i)
			{
				remove_row(row_to_remove,small_circle_collection,table_widget);
			}
		}
	}

	bool
	small_circles_are_approximately_equal(
		const GPlatesMaths::SmallCircle &c1,
		const GPlatesMaths::SmallCircle &c2)
	{
		return(collinear(c1.axis_vector(),c2.axis_vector()) &&
			GPlatesMaths::are_almost_exactly_equal(c1.colatitude().dval(),c2.colatitude().dval()));
	}

	bool
	collection_contains(
		const std::vector<GPlatesMaths::SmallCircle> &small_circle_collection,
		const GPlatesMaths::SmallCircle &small_circle)
	{

		std::vector<GPlatesMaths::SmallCircle>::const_iterator 
												it = small_circle_collection.begin(),
												end = small_circle_collection.end();

		for (; it != end ; ++it)
		{
			if (small_circles_are_approximately_equal(*it,small_circle))
			{
				return true;
			}
		}

		return false;
	}

	void
	update_layer(
		GPlatesViewOperations::RenderedGeometryLayer *layer,
		const std::vector<GPlatesMaths::SmallCircle> &small_circles)
	{
		using namespace GPlatesMaths;

		layer->clear_rendered_geometries();
		std::vector<SmallCircle>::const_iterator it = small_circles.begin(),
															end = small_circles.end();

		for ( ; it != end ; ++it)
		{
			GPlatesViewOperations::RenderedGeometry circle = GPlatesViewOperations::create_rendered_small_circle(
				PointOnSphere(it->axis_vector()),
				it->colatitude());
				
			layer->add_rendered_geometry(circle);
		}
	}


	void
	update_table(
		QTableWidget *table_widget,
		const std::vector<GPlatesMaths::SmallCircle> &small_circles)
	{
		using namespace GPlatesMaths;

		table_widget->clearContents();
		table_widget->setRowCount(0);

		std::vector<SmallCircle>::const_iterator it = small_circles.begin(),
											   end = small_circles.end();
		for (; it != end ; ++it)
		{
			LatLonPoint llp = make_lat_lon_point(PointOnSphere(it->axis_vector()));

			QLocale locale_;

			QString lat_string = locale_.toString(llp.latitude(),'f',2);
			QString lon_string = locale_.toString(llp.longitude(),'f',2);

			QString centre;
			centre.append("(");
			centre.append(lat_string);
			centre.append(" ; ");
			centre.append(lon_string);
			centre.append(")");

			QTableWidgetItem *centre_item = new QTableWidgetItem;
			centre_item->setData(Qt::DisplayRole,centre);
			centre_item->setFlags(centre_item->flags() & ~Qt::ItemIsEditable);

			QTableWidgetItem *radius_item = new QTableWidgetItem;
			radius_item->setData(Qt::DisplayRole,convert_rad_to_deg(it->colatitude()).dval());
			radius_item->setFlags(radius_item->flags() & ~Qt::ItemIsEditable);

			int row = table_widget->rowCount();
			table_widget->insertRow(row);
			table_widget->setItem(row, GPlatesQtWidgets::SmallCircleManager::CENTRE_COLUMN, centre_item);
			table_widget->setItem(row, GPlatesQtWidgets::SmallCircleManager::RADIUS_COLUMN, radius_item);
		}


	}
}

GPlatesQtWidgets::SmallCircleManager::SmallCircleManager(
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
		GPlatesAppLogic::ApplicationState &application_state,
		QWidget *parent_ /* = NULL */):
	QDialog(parent_),
	d_small_circle_layer(rendered_geometry_collection.get_main_rendered_layer(
			GPlatesViewOperations::RenderedGeometryCollection::SMALL_CIRCLE_LAYER)),
	d_create_small_circle_dialog_ptr(new GPlatesQtWidgets::CreateSmallCircleDialog(this,application_state,parent_))
{

	setupUi(this);

	table_circles->setColumnCount(NUM_COLUMNS);
	table_circles->horizontalHeader()->setResizeMode(CENTRE_COLUMN,QHeaderView::Stretch);
	table_circles->horizontalHeader()->setResizeMode(RADIUS_COLUMN,QHeaderView::Stretch);
	table_circles->verticalHeader()->setVisible(false);

	QObject::connect(button_add,SIGNAL(clicked()),this,SLOT(handle_add()));
	QObject::connect(button_remove,SIGNAL(clicked()),this,SLOT(handle_remove()));
	QObject::connect(d_create_small_circle_dialog_ptr,SIGNAL(circle_added()),this,SLOT(handle_circle_added()));
	QObject::connect(button_remove_all,SIGNAL(clicked()),this,SLOT(handle_remove_all()));
	QObject::connect(table_circles,SIGNAL(itemSelectionChanged()),this,SLOT(handle_item_selection_changed()));

	d_small_circle_layer->set_active();

        update_buttons();

}

void
GPlatesQtWidgets::SmallCircleManager::handle_add()
{
	d_create_small_circle_dialog_ptr->init();
	d_create_small_circle_dialog_ptr->exec();
}

void
GPlatesQtWidgets::SmallCircleManager::handle_remove()
{
// Remove the focused table item(s) from the table. 	
	remove_rows(d_small_circle_collection,table_circles);
	update_layer(d_small_circle_layer,d_small_circle_collection);
	update_buttons();
}

void
GPlatesQtWidgets::SmallCircleManager::add_circle(
	const GPlatesMaths::SmallCircle &small_circle)
{
	// Check we don't have this small circle (or one very similar to it) in the collection. 
	if (!collection_contains(d_small_circle_collection,small_circle))
	{
		d_small_circle_collection.push_back(small_circle);
	}

	update_buttons();
}

void
GPlatesQtWidgets::SmallCircleManager::handle_circle_added()
{
	update_layer(d_small_circle_layer,d_small_circle_collection);
	update_table(table_circles,d_small_circle_collection);
	update_buttons();
}

void
GPlatesQtWidgets::SmallCircleManager::handle_remove_all()
{
	QMessageBox message_box(this);
	message_box.setWindowTitle("Small Circles");
	message_box.setText("Remove all small circles?");
	QPushButton *remove_button = message_box.addButton(QObject::tr("Remove"),QMessageBox::AcceptRole);
	message_box.setStandardButtons(QMessageBox::Cancel);
	message_box.setDefaultButton(QMessageBox::Cancel);
	
	message_box.exec();

	if (message_box.clickedButton() != remove_button)
	{
		return;
	}

	d_small_circle_collection.clear();
	update_layer(d_small_circle_layer,d_small_circle_collection);
	update_table(table_circles,d_small_circle_collection);
	update_buttons();
}

void
GPlatesQtWidgets::SmallCircleManager::update_buttons()
{
	QList<QTableWidgetSelectionRange> range = table_circles->selectedRanges();
	button_remove->setEnabled(!range.isEmpty());

	button_remove_all->setEnabled(table_circles->rowCount()>0);
}

void
GPlatesQtWidgets::SmallCircleManager::handle_item_selection_changed()
{
	update_buttons();
}
