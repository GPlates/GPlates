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


#include "boost/foreach.hpp"

#include <QStandardItemModel>

#include "AgeModelManagerDialog.h"
#include "app-logic/AgeModelCollection.h"
#include "app-logic/ApplicationState.h"

namespace
{
	void
	add_model_identifiers_to_combo_box(
			const GPlatesAppLogic::AgeModelCollection &age_model_collection,
			QComboBox *combo_box)
	{
		BOOST_FOREACH(const GPlatesAppLogic::AgeModel model,age_model_collection.get_age_models())
		{
			combo_box->addItem(model.d_identifier);
		}
	}

	void
	fill_table_model(
			const GPlatesAppLogic::AgeModelCollection &age_model_collection,
			QStandardItemModel *standard_model)
	{
		standard_model->setHorizontalHeaderItem(GPlatesQtWidgets::AgeModelManagerDialog::CHRON_COLUMN,new QStandardItem(QObject::tr("Chron")));

		int column = GPlatesQtWidgets::AgeModelManagerDialog::NUM_FIXED_COLUMNS;
		BOOST_FOREACH(const GPlatesAppLogic::AgeModel model, age_model_collection.get_age_models())
		{
			standard_model->setHorizontalHeaderItem(column,new QStandardItem(model.d_identifier));
			++column;
		}

		standard_model->setRowCount(0);

	}
}

GPlatesQtWidgets::AgeModelManagerDialog::AgeModelManagerDialog(
		GPlatesAppLogic::ApplicationState &app_state,
		QWidget *parent_):
	GPlatesDialog(
		parent_,
		Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
	d_age_model_collection(app_state.get_age_model_collection()),
	d_standard_model(new QStandardItemModel(this))
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
GPlatesQtWidgets::AgeModelManagerDialog::handle_combo_box_current_index_changed()
{

}

void
GPlatesQtWidgets::AgeModelManagerDialog::setup_widgets()
{
	line_edit_collection->setText(d_age_model_collection.get_filename());

	add_model_identifiers_to_combo_box(d_age_model_collection,combo_active_model);

	fill_table_model(d_age_model_collection,d_standard_model);

	table_age_models->setModel(d_standard_model);
	table_age_models->verticalHeader()->setVisible(false);
}

void
GPlatesQtWidgets::AgeModelManagerDialog::setup_connections()
{
	QObject::connect(button_import,SIGNAL(clicked()),this,SLOT(handle_import()));
	QObject::connect(combo_active_model,SIGNAL(currentIndexChanged(QString)),this,SLOT(handle_combo_box_current_index_changed()));
}


