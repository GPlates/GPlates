/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2006, 2007 The University of Sydney, Australia
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

#include <QTableWidget>
#include <QHeaderView>
#include <QFileDialog>
#include "ApplicationState.h"
#include "file-io/FileInfo.h"
#include "ViewportWindow.h"
#include "ManageFeatureCollectionsActionWidget.h"
#include "ManageFeatureCollectionsDialog.h"

namespace
{
	namespace ColumnNames
	{
		/**
		 * These should match the columns set up in the designer.
		 */
		enum ColumnName
		{
			FILENAME, FORMAT, IN_USE, ACTIONS
		};
	}
}


GPlatesQtWidgets::ManageFeatureCollectionsDialog::ManageFeatureCollectionsDialog(
		ViewportWindow &viewport_window,
		QWidget *parent_):
	QDialog(parent_),
	d_viewport_window_ptr(&viewport_window)
{
	setupUi(this);
	
	// Try to adjust column widths.
	QHeaderView *header = table_feature_collections->horizontalHeader();
	header->setResizeMode(ColumnNames::FILENAME, QHeaderView::Stretch);
	header->resizeSection(ColumnNames::FORMAT, 128);
	header->resizeSection(ColumnNames::IN_USE, 64);
	header->resizeSection(ColumnNames::ACTIONS, 146);

	// Enforce minimum row height for the Actions widget's sake.
	QHeaderView *sider = table_feature_collections->verticalHeader();
	sider->setResizeMode(QHeaderView::Fixed);
	sider->setDefaultSectionSize(34);
	
	// Set up slots for Open File and Save All
	QObject::connect(button_open_file, SIGNAL(clicked()), this, SLOT(open_file()));
	QObject::connect(button_save_all, SIGNAL(clicked()), this, SLOT(save_all()));
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::update()
{
	GPlatesAppState::ApplicationState *state = GPlatesAppState::ApplicationState::instance();
	
	GPlatesAppState::ApplicationState::file_info_iterator it = state->files_begin();
	GPlatesAppState::ApplicationState::file_info_iterator end = state->files_end();
	
	clear_rows();
	for (; it != end; ++it) {
		add_row(it);
	}
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::unload_file(
		ManageFeatureCollectionsActionWidget *action_widget_ptr)
{
	GPlatesAppState::ApplicationState::file_info_iterator file_it =
			action_widget_ptr->get_file_info_iterator();
	
	// BIG FIXME: Unloading files should be handled by ApplicationState, which should
	// then update all ViewportWindows' ViewState's active files list. However, we
	// don't have that built yet, so the current method of unloading files is to call
	// ViewportWindow and let it do the ApplicationState call.
	d_viewport_window_ptr->deactivate_loaded_file(file_it);
	d_viewport_window_ptr->reconstruct();
	
	remove_row(action_widget_ptr);
}



void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::open_file()
{
	static const QString filters = tr(
			"All loadable files (*.dat *.pla *.rot *.shp);;"
			"PLATES4 line (*.dat *.pla);;"
			"PLATES4 rotation (*.rot);;"
			"ESRI shapefile (*.shp);;"
			"All files (*)" );
	
	QStringList filenames = QFileDialog::getOpenFileNames(d_viewport_window_ptr,
			tr("Open Files"), "", filters);
	
	if ( ! filenames.isEmpty() ) {
		d_viewport_window_ptr->load_files(filenames);
		d_viewport_window_ptr->reconstruct();
	}
}



void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::clear_rows()
{
	table_feature_collections->clearContents();	// Do not clear the header items as well.
	table_feature_collections->setRowCount(0);	// Do remove the newly blanked rows.
}


GPlatesQtWidgets::ManageFeatureCollectionsActionWidget *
GPlatesQtWidgets::ManageFeatureCollectionsDialog::add_row(
		GPlatesAppState::ApplicationState::file_info_iterator file_it)
{
	static const QString in_use_str(tr("Yes"));
	static const QString not_in_use_str(tr(""));

	// Obtain information from the FileInfo
	const QFileInfo &qfileinfo = file_it->get_qfileinfo();
	
	QString filename_str = qfileinfo.fileName();
	QString filepath_str = qfileinfo.path();
	QString format_str = "";		// FIXME: FileInfo not handling file format atm!
	bool in_use = d_viewport_window_ptr->is_file_active(file_it);
	
	// Add blank row.
	int row = table_feature_collections->rowCount();
	table_feature_collections->insertRow(row);
	
	// Add filename item.
	QTableWidgetItem *filename_item = new QTableWidgetItem(filename_str);
	filename_item->setToolTip(tr("Location: %1").arg(filepath_str));
	filename_item->setFlags(Qt::ItemIsEnabled);
	table_feature_collections->setItem(row, ColumnNames::FILENAME, filename_item);

	// Add file format item.
	QTableWidgetItem *format_item = new QTableWidgetItem(format_str);
	format_item->setFlags(Qt::ItemIsEnabled);
	table_feature_collections->setItem(row, ColumnNames::FORMAT, format_item);

	// Add in use status.
	QTableWidgetItem *in_use_item = new QTableWidgetItem();
	if (in_use)	{
		in_use_item->setText(in_use_str);
	} else {
		in_use_item->setText(not_in_use_str);
	}
	in_use_item->setFlags(Qt::ItemIsEnabled);
	in_use_item->setTextAlignment(Qt::AlignCenter);
	table_feature_collections->setItem(row, ColumnNames::IN_USE, in_use_item);
	
	// Add action buttons widget.
	ManageFeatureCollectionsActionWidget *action_widget_ptr =
			new ManageFeatureCollectionsActionWidget(*this, file_it, this);
	table_feature_collections->setCellWidget(row, ColumnNames::ACTIONS, action_widget_ptr);
	
	return action_widget_ptr;
}


int
GPlatesQtWidgets::ManageFeatureCollectionsDialog::find_row(
		ManageFeatureCollectionsActionWidget *action_widget_ptr)
{
	int row = 0;
	int end = table_feature_collections->rowCount();
	for (; row < end; ++row) {
		if (table_feature_collections->cellWidget(row, ColumnNames::ACTIONS) == action_widget_ptr) {
			return row;
		}
	}
	return end;
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::remove_row(
		ManageFeatureCollectionsActionWidget *action_widget_ptr)
{
	int row = find_row(action_widget_ptr);
	if (row < table_feature_collections->rowCount()) {
		table_feature_collections->removeRow(row);
	}
}

