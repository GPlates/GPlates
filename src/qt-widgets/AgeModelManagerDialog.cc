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
#include "boost/unordered_set.hpp"

#include <QStandardItemModel>

#include "AgeModelManagerDialog.h"
#include "app-logic/AgeModelCollection.h"
#include "app-logic/ApplicationState.h"
#include "file-io/AgeModelReader.h"
#include "presentation/ViewState.h"
#include "qt-widgets/OpenFileDialog.h"

namespace
{
	void
	add_model_identifiers_to_combo_box(
			const GPlatesAppLogic::AgeModelCollection &age_model_collection,
			QComboBox *combo_box)
	{
		combo_box->clear();
		BOOST_FOREACH(const GPlatesAppLogic::AgeModel model,age_model_collection.get_age_models())
		{
			combo_box->addItem(model.d_identifier);
		}
	}

	void
	add_row_to_standard_model(
			QStandardItemModel *standard_model,
			const QString &chron,
			const GPlatesAppLogic::age_model_container_type &models)
	{
		int row = standard_model->rowCount();
		standard_model->insertRow(row);

		// Add the chron string to the first column.
		standard_model->setData(standard_model->index(row,GPlatesQtWidgets::AgeModelManagerDialog::CHRON_COLUMN),chron);


		// Check each model for the chron key - if it's there, add the model's time.

		int column = GPlatesQtWidgets::AgeModelManagerDialog::NUM_FIXED_COLUMNS;
		BOOST_FOREACH(GPlatesAppLogic::AgeModel age_model, models)
		{
			GPlatesAppLogic::age_model_map_type::const_iterator it = age_model.d_model.find(chron);
#if 0
			GPlatesAppLogic::age_model_map_type::const_iterator it =
					std::find_if(age_model.d_model.begin(),age_model.d_model.end(),
								boost::bind(&GPlatesAppLogic::age_model_pair_type::first, _1) == chron);
#endif
			if (it != age_model.d_model.end())
			{
				standard_model->setData(standard_model->index(row,column),it->second);
			}
			++column;
		}
	}

	void
	fill_table_model(
			const GPlatesAppLogic::AgeModelCollection &age_model_collection,
			QStandardItemModel *standard_model)
	{

		standard_model->setColumnCount(0);
		standard_model->setRowCount(0);

		standard_model->setHorizontalHeaderItem(GPlatesQtWidgets::AgeModelManagerDialog::CHRON_COLUMN,new QStandardItem(QObject::tr("Chron")));

		int column = GPlatesQtWidgets::AgeModelManagerDialog::NUM_FIXED_COLUMNS;
		BOOST_FOREACH(const GPlatesAppLogic::AgeModel model, age_model_collection.get_age_models())
		{
			standard_model->setHorizontalHeaderItem(column,new QStandardItem(model.d_identifier));
			++column;
		}

#if 0
		// Each model might only define times for a subset of all possible chrons; find the set of all
		// chrons used in all the models.
		// TODO: this will be ordered alphabetically, but we don't want this - our final table output order currently
		// follows the ordering of this set. For example 2ny will get placed after 2An.1ny.
		std::set<QString> unique_chrons;
		BOOST_FOREACH(const GPlatesAppLogic::AgeModel model, age_model_collection.get_age_models())
		{
			BOOST_FOREACH(const GPlatesAppLogic::age_model_pair_type pair, model.d_model)
			{
				unique_chrons.insert(pair.first);
			}
		}

		// For each unique chron, add a row to the table containing the ages (where they exist) for each model.
		BOOST_FOREACH(QString chron, unique_chrons)
		{
			qDebug() << "Chron: " << chron;
			add_row_to_standard_model(standard_model,chron,age_model_collection.get_age_models());
		}
#endif
		BOOST_FOREACH(QString chron,age_model_collection.get_ordered_chrons())
		{
			add_row_to_standard_model(standard_model,chron,age_model_collection.get_age_models());
		}
	}

	void
	highlight_cell(
			int row,
			int column,
			QStandardItemModel *standard_model)
	{
			standard_model->setData(standard_model->index(row,column),Qt::yellow,Qt::BackgroundColorRole);
	}

	void
	highlight_column(
			int column,
			QStandardItemModel *standard_model)
	{
		int rows = standard_model->rowCount();
		for (int row = 0; row < rows ; ++row)
		{
			highlight_cell(row,column,standard_model);
		}
	}

	void
	highlight_selected_age_model(
			QComboBox *combo_box,
			QStandardItemModel *standard_model)
	{
		int index = combo_box->currentIndex();
		int column = GPlatesQtWidgets::AgeModelManagerDialog::NUM_FIXED_COLUMNS + index;
		highlight_column(column,standard_model);
	}
}

GPlatesQtWidgets::AgeModelManagerDialog::AgeModelManagerDialog(
		GPlatesPresentation::ViewState &view_state,
		QWidget *parent_):
	GPlatesDialog(
		parent_,
		Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
	d_age_model_collection(view_state.get_application_state().get_age_model_collection()),
	d_standard_model(new QStandardItemModel(this)),
	d_application_state(view_state.get_application_state()),
	d_open_file_dialog(new OpenFileDialog(
						   this,
						   "Select age model file",
						   "Age model file (*.dat)",
						   view_state))
{
	setupUi(this);

	setup_widgets();

	setup_connections();
}

GPlatesQtWidgets::AgeModelManagerDialog::~AgeModelManagerDialog()
{

}

void
GPlatesQtWidgets::AgeModelManagerDialog::handle_import()
{
	QString filename = d_open_file_dialog->get_open_file_name();
	qDebug() << "Filename: " << filename;
	if (filename.isEmpty())
	{
		return;
	}

	qDebug() << "Handle import";
	line_edit_collection->setText(filename);

	try{
		GPlatesFileIO::AgeModelReader::read_file(filename,d_age_model_collection);
	}
	catch(std::exception &exception)
	{
		qWarning() << "Error reading age model file " << filename << ": " << exception.what();
	}
	catch(...)
	{
		qWarning() << "Unknown error reading age model file " << filename;
	}

	update_dialog();
}

void
GPlatesQtWidgets::AgeModelManagerDialog::handle_combo_box_current_index_changed()
{
	fill_table_model(d_age_model_collection,d_standard_model);
	highlight_selected_age_model(combo_active_model,d_standard_model);
	d_age_model_collection.set_active_age_model(combo_active_model->currentIndex());
}

void
GPlatesQtWidgets::AgeModelManagerDialog::setup_widgets()
{
	line_edit_collection->setText(d_age_model_collection.get_filename());

	add_model_identifiers_to_combo_box(d_age_model_collection,combo_active_model);

	fill_table_model(d_age_model_collection,d_standard_model);
	highlight_selected_age_model(combo_active_model,d_standard_model);

	table_age_models->setModel(d_standard_model);
	table_age_models->verticalHeader()->setVisible(false);
}

void
GPlatesQtWidgets::AgeModelManagerDialog::setup_connections()
{
	QObject::connect(button_import,SIGNAL(clicked()),this,SLOT(handle_import()));
	QObject::connect(combo_active_model,SIGNAL(currentIndexChanged(QString)),this,SLOT(handle_combo_box_current_index_changed()));
}

void
GPlatesQtWidgets::AgeModelManagerDialog::update_dialog()
{
	line_edit_collection->setText(d_age_model_collection.get_filename());
	add_model_identifiers_to_combo_box(d_age_model_collection,combo_active_model);
	fill_table_model(d_age_model_collection,d_standard_model);
	highlight_selected_age_model(combo_active_model,d_standard_model);
}


