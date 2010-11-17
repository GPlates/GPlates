/* $Id$ */
 
/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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
#include <fstream>
#include <iostream>
#include <QFileDialog>
#include <QDebug>
#include <QMessageBox>
#include <sstream>
#include <string>

#include "CreateFeatureIdListDialog.h"

#include "gui/FeatureFocus.h"


GPlatesQtWidgets::CreateFeatureIdListDialog::CreateFeatureIdListDialog(
		GPlatesPresentation::ViewState & view_state,
		QWidget *parent_ )
	:QDialog(
			parent_, 
			Qt::CustomizeWindowHint | 
			Qt::WindowTitleHint | 
			Qt::WindowSystemMenuHint | 
			Qt::MSWindowsFixedSizeDialogHint),
	d_model(new CreateFeatureIdListModel()),
	d_view_state(view_state)
{
	setupUi(this);
	
	listView->setModel(d_model.get());

	QObject::connect(pushButton_add, SIGNAL(clicked()), this, SLOT(handle_add()));
	QObject::connect(pushButton_remove, SIGNAL(clicked()), this, SLOT(handle_remove()));
	QObject::connect(pushButton_save_file, SIGNAL(clicked()), this, SLOT(handle_save()));
	QObject::connect(pushButton_open_file, SIGNAL(clicked()), this, SLOT(handle_open()));

	QObject::connect(
			listView->selectionModel(),
			SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
			this,
			SLOT(handle_selection_change(const QItemSelection &, const QItemSelection &)));

}

void
GPlatesQtWidgets::CreateFeatureIdListDialog::handle_add()
{
	GPlatesGui::FeatureFocus& focus = d_view_state.get_feature_focus();
	if(focus.is_valid())
	{
		const GPlatesModel::FeatureId& feature_id = focus.focused_feature()->feature_id();
		d_model->add(
			GPlatesUtils::make_qstring_from_icu_string(feature_id.get()));
	}
	
	return;
}

void
GPlatesQtWidgets::CreateFeatureIdListDialog::handle_remove()
{
	if(d_current_selection.isValid())
	{
		d_model->remove(d_current_selection);
	}
}

void
GPlatesQtWidgets::CreateFeatureIdListDialog::handle_open()
{
	QString filters = tr("All files (*)" );
	
	QString filename = 		
		QFileDialog::getOpenFileName(
				this,
				tr("Open Files"), 
				QDir::currentPath(), 
				filters);
	
	if(filename.size() == 0)
	{
		return;
	}

	d_model->clear();
	std::string line;
	std::ifstream input_file;
	try{
		input_file.open(filename.toStdString().c_str());
		if (input_file.is_open())
		{
			while (! input_file.eof() )
			{
				std::getline(input_file,line);
				d_model->add(line.c_str());
			}
		}
	}catch (...)
	{
		input_file.close();
	}
	input_file.close();
}


void
GPlatesQtWidgets::CreateFeatureIdListDialog::handle_save()
{
	static const QString filters = tr("All files (*)" );
	
	QString filename = 		
		QFileDialog::getSaveFileName(
				this,
				tr("Save Files"), 
				QDir::currentPath(), 
				filters);
	
	std::ofstream output_file;
	try{
		output_file.open(filename.toStdString().c_str());
		if (output_file.is_open())
		{
			const QStringList& ids = d_model->feature_id_list();
			QStringList::ConstIterator it = ids.constBegin();
			QStringList::ConstIterator it_end = ids.constEnd();
			for(; it != it_end; it++)
			{
				output_file << it->toStdString() << std::endl;
			}
		}
	}catch (...)
	{
		output_file.close();
	}
	output_file.close();
}


void
GPlatesQtWidgets::CreateFeatureIdListDialog::handle_selection_change(
		const QItemSelection &selected,
		const QItemSelection &deselected)
{
	if(selected.count()>0)
	{
		d_current_selection = selected.indexes().first();
	}
	else
	{
		d_current_selection = QModelIndex();
	}
}


