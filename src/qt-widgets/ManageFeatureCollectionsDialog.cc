/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 The University of Sydney, Australia
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

#include <boost/foreach.hpp>
#include <QCoreApplication>
#include <QTableWidget>
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QDebug>

#include "app-logic/FeatureCollectionFileState.h"
#include "app-logic/FeatureCollectionFileIO.h"
#include "file-io/FileInfo.h"
#include "file-io/ExternalProgram.h"
#include "file-io/FeatureCollectionFileFormat.h"
#include "file-io/ErrorOpeningFileForWritingException.h"
#include "file-io/ErrorOpeningPipeFromGzipException.h"
#include "file-io/ErrorOpeningPipeToGzipException.h"
#include "file-io/FileFormatNotSupportedException.h"
#include "file-io/OgrException.h"
#include "global/InvalidFeatureCollectionException.h"
#include "global/UnexpectedEmptyFeatureCollectionException.h"
#include "gui/FileIOFeedback.h"
#include "model/FeatureCollectionHandle.h"
#include "ManageFeatureCollectionsActionWidget.h"
#include "ManageFeatureCollectionsStateWidget.h"

#include "ManageFeatureCollectionsDialog.h"


namespace
{
	// The colours to be used for row backgrounds and icon colours.
	const QColor bg_colour_normal(Qt::white);
	const QColor bg_colour_unsaved("#FFA699"); // Red raised to same lightness as below.
	const QColor bg_colour_new_feature_collection("#FFD799");	// Ubuntu (8.04) Light Orange Text Highlight

	namespace ColumnNames
	{
		/**
		 * These should match the columns set up in the designer.
		 */
		enum ColumnName
		{
			FILENAME, FORMAT, LAYER_TYPES, ACTIONS
		};
	}
	
	/**
	 * Returns a text string that identifies the file format for a file.
	 * Used to set the 'FORMAT' column in the table.
	 */
	const QString &
	get_format_for_file(
			const QFileInfo &qfileinfo)
	{
		static const QString format_line(QObject::tr("PLATES4 line"));
		static const QString format_rotation(QObject::tr("PLATES4 rotation"));
		static const QString format_shapefile(QObject::tr("ESRI shapefile"));
		static const QString format_gpml(QObject::tr("GPlates Markup Language"));
		static const QString format_gpml_gz(QObject::tr("Compressed GPML"));
		static const QString format_gmt(QObject::tr("GMT xy"));
		static const QString format_gmap(QObject::tr("GMAP VGP"));
		static const QString format_unknown(QObject::tr(""));
		
		switch ( GPlatesFileIO::get_feature_collection_file_format(qfileinfo) )
		{
		case GPlatesFileIO::FeatureCollectionFileFormat::PLATES4_LINE:
			return format_line;

		case GPlatesFileIO::FeatureCollectionFileFormat::PLATES4_ROTATION:
			return format_rotation;

		case GPlatesFileIO::FeatureCollectionFileFormat::SHAPEFILE:
			return format_shapefile;

		case GPlatesFileIO::FeatureCollectionFileFormat::GPML:
			return format_gpml;

		case GPlatesFileIO::FeatureCollectionFileFormat::GPML_GZ:
			return format_gpml_gz;

		case GPlatesFileIO::FeatureCollectionFileFormat::GMT:
			return format_gmt;
			
		case GPlatesFileIO::FeatureCollectionFileFormat::GMAP:
			return format_gmap;

		case GPlatesFileIO::FeatureCollectionFileFormat::UNKNOWN:
		default:
			return format_unknown;
		}
	}


	GPlatesQtWidgets::ManageFeatureCollectionsStateWidget *
	get_state_widget(
			QTableWidget *qtable_widget,
			int row)
	{
		return dynamic_cast<GPlatesQtWidgets::ManageFeatureCollectionsStateWidget *>(
				qtable_widget->cellWidget(row, ColumnNames::LAYER_TYPES));
	}


	GPlatesQtWidgets::ManageFeatureCollectionsActionWidget *
	get_action_widget(
			QTableWidget *qtable_widget,
			int row)
	{
		return dynamic_cast<GPlatesQtWidgets::ManageFeatureCollectionsActionWidget *>(
				qtable_widget->cellWidget(row, ColumnNames::ACTIONS));
	}


	/**
	 * Convenience function to change the background for all table cells on
	 * a given @a row.
	 */
	void
	set_row_background(
			QTableWidget *qtable_widget,
			int row,
			const QBrush &bg_colour)
	{
		int columns = qtable_widget->columnCount();
		for (int col = 0; col < columns; ++col) {
			QTableWidgetItem *item = qtable_widget->item(row, col);
			if ( ! item) {
				item = new QTableWidgetItem();
				qtable_widget->setItem(row, col, item);
			}
			item->setBackground(bg_colour);
		}
	}
	
	/**
	 * Creates a colour 'swatch' pixmap consisting of the given colour.
	 * Useful for labels.
	 */
	QPixmap
	create_pixmap_from_colour(
			const QColor &colour,
			int size = 16)
	{
		QPixmap pix(size, size);
		pix.fill(colour);
		return pix;
	}

}


GPlatesQtWidgets::ManageFeatureCollectionsDialog::ManageFeatureCollectionsDialog(
		GPlatesAppLogic::FeatureCollectionFileState &file_state,
		GPlatesAppLogic::FeatureCollectionFileIO &feature_collection_file_io,
		GPlatesGui::FileIOFeedback &gui_file_io_feedback,
		QWidget *parent_):
	QDialog(parent_, Qt::Window),
	d_file_state(file_state),
	d_feature_collection_file_io(&feature_collection_file_io),
	d_gui_file_io_feedback_ptr(&gui_file_io_feedback)
{
	setupUi(this);
	
	// Try to adjust column widths.
	QHeaderView *header = table_feature_collections->horizontalHeader();
	header->setResizeMode(ColumnNames::FILENAME, QHeaderView::Stretch);
	header->resizeSection(ColumnNames::FORMAT, 128);
	header->resizeSection(ColumnNames::LAYER_TYPES, 88);
	header->resizeSection(ColumnNames::ACTIONS, 212);

	// Enforce minimum row height for the Actions widget's sake.
	QHeaderView *sider = table_feature_collections->verticalHeader();
	sider->setResizeMode(QHeaderView::Fixed);
	sider->setDefaultSectionSize(34);
	
	// Hide the 'unsaved' information labels by default - these are shown/hidden
	// later as appropriate using highlight_unsaved_changes().
	label_unsaved_changes->hide();
	label_unsaved_changes_swatch->hide();
	label_no_presence_on_disk->hide();
	label_no_presence_on_disk_swatch->hide();
	// Also set a stylish icon for them matching the row colour.
	// These colours are defined in the anonymous namespace section.
	label_unsaved_changes_swatch->setPixmap(create_pixmap_from_colour(bg_colour_unsaved));
	label_no_presence_on_disk_swatch->setPixmap(create_pixmap_from_colour(bg_colour_new_feature_collection));

	// Set up slots for Open File and Save All
	QObject::connect(button_open_file, SIGNAL(clicked()), d_gui_file_io_feedback_ptr, SLOT(open_files()));
	QObject::connect(button_save_all, SIGNAL(clicked()), this, SLOT(save_all_named()));
	
	// Set up slots for file load/unload notifications.
	connect_to_file_state_signals();
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::update()
{
	clear_rows();

	const std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> loaded_files =
			d_file_state.get_loaded_files();
	BOOST_FOREACH(
			const GPlatesAppLogic::FeatureCollectionFileState::file_reference &file_ref,
			loaded_files)
	{
		add_row(file_ref);
	}

	highlight_unsaved_changes();
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::edit_configuration(
		ManageFeatureCollectionsActionWidget *action_widget_ptr)
{
	// The "edit configuration" method only makes sense for shapefiles 
	// (until there is some sort of equivalent requirement for other types of 
	// feature collection), and as such, only shapefiles have the "edit configuration" 
	// icon enabled in the ManageFeatureCollectionsActionWidget. 
	//  
	// For shapefiles, "edit configuration" translates to "re-map shapefile attributes to model properties". 
	// 
	GPlatesAppLogic::FeatureCollectionFileState::file_reference file_ref =
			action_widget_ptr->get_file_reference();

	d_feature_collection_file_io->remap_shapefile_attributes(file_ref);
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::save_file(
		ManageFeatureCollectionsActionWidget *action_widget_ptr)
{
	GPlatesAppLogic::FeatureCollectionFileState::file_reference file =
			action_widget_ptr->get_file_reference();
	d_gui_file_io_feedback_ptr->save_file_in_place(file);
}



void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::save_file_as(
		ManageFeatureCollectionsActionWidget *action_widget_ptr)
{
	GPlatesAppLogic::FeatureCollectionFileState::file_reference file =
			action_widget_ptr->get_file_reference();
	d_gui_file_io_feedback_ptr->save_file_as(file);
}



void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::save_file_copy(
		ManageFeatureCollectionsActionWidget *action_widget_ptr)
{
	GPlatesAppLogic::FeatureCollectionFileState::file_reference file =
			action_widget_ptr->get_file_reference();
	d_gui_file_io_feedback_ptr->save_file_copy(file);
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::reload_file(
		ManageFeatureCollectionsActionWidget *action_widget_ptr)
{
	GPlatesAppLogic::FeatureCollectionFileState::file_reference file_it =
			action_widget_ptr->get_file_reference();
	
	d_gui_file_io_feedback_ptr->reload_file(file_it);
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::unload_file(
		ManageFeatureCollectionsActionWidget *action_widget_ptr)
{
	GPlatesAppLogic::FeatureCollectionFileState::file_reference file_it =
			action_widget_ptr->get_file_reference();
	
	d_feature_collection_file_io->unload_file(file_it);
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::set_state_for_file(
		ManageFeatureCollectionsStateWidget *state_widget_ptr,
		bool activate)
{
	GPlatesAppLogic::FeatureCollectionFileState::file_reference file_ref =
			state_widget_ptr->get_file_reference();

	// File will be activated.
	file_ref.set_file_active(activate);
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::handle_file_state_files_added(
		GPlatesAppLogic::FeatureCollectionFileState &file_state,
		const std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> &new_files)
{
	BOOST_FOREACH(
			const GPlatesAppLogic::FeatureCollectionFileState::file_reference &file_ref,
			new_files)
	{
		add_row(file_ref);
	}
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::handle_file_state_file_about_to_be_removed(
		GPlatesAppLogic::FeatureCollectionFileState &/*file_state*/,
		GPlatesAppLogic::FeatureCollectionFileState::file_reference unload_file_ref)
{
	const int row = find_row(unload_file_ref);

	if (row != table_feature_collections->rowCount())
	{
		remove_row(row);
	}
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::handle_file_state_file_info_changed(
		GPlatesAppLogic::FeatureCollectionFileState &file_state,
		GPlatesAppLogic::FeatureCollectionFileState::file_reference file_ref)
{
	// Row text needs to be updated.
	update();
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::handle_file_state_file_activation_changed(
		GPlatesAppLogic::FeatureCollectionFileState &file_state,
		GPlatesAppLogic::FeatureCollectionFileState::file_reference file_ref,
		bool activation)
{
	const int row = find_row(file_ref);

	if (row != table_feature_collections->rowCount())
	{
		ManageFeatureCollectionsStateWidget *state_widget = get_state_widget(
				table_feature_collections, row);
		if (state_widget)
		{
			state_widget->update_state(activation, true/*enable*/);
		}

		ManageFeatureCollectionsActionWidget *action_widget = get_action_widget(
				table_feature_collections, row);
		if (action_widget)
		{
			action_widget->update_state();
		}
	}
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::connect_to_file_state_signals()
{
	QObject::connect(
			&d_file_state,
			SIGNAL(file_state_files_added(
					GPlatesAppLogic::FeatureCollectionFileState &,
					const std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> &)),
			this,
			SLOT(handle_file_state_files_added(
					GPlatesAppLogic::FeatureCollectionFileState &,
					const std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> &)));

	QObject::connect(
			&d_file_state,
			SIGNAL(file_state_file_about_to_be_removed(
					GPlatesAppLogic::FeatureCollectionFileState &,
					GPlatesAppLogic::FeatureCollectionFileState::file_reference)),
			this,
			SLOT(handle_file_state_file_about_to_be_removed(
					GPlatesAppLogic::FeatureCollectionFileState &,
					GPlatesAppLogic::FeatureCollectionFileState::file_reference)));

	QObject::connect(
			&d_file_state,
			SIGNAL(file_state_file_info_changed(
				GPlatesAppLogic::FeatureCollectionFileState &,
				GPlatesAppLogic::FeatureCollectionFileState::file_reference)),
			this,
			SLOT(handle_file_state_file_info_changed(
					GPlatesAppLogic::FeatureCollectionFileState &,
					GPlatesAppLogic::FeatureCollectionFileState::file_reference)));

	QObject::connect(
			&d_file_state,
			SIGNAL(file_state_file_activation_changed(
				GPlatesAppLogic::FeatureCollectionFileState &,
				GPlatesAppLogic::FeatureCollectionFileState::file_reference,
				bool)),
			this,
			SLOT(handle_file_state_file_activation_changed(
					GPlatesAppLogic::FeatureCollectionFileState &,
					GPlatesAppLogic::FeatureCollectionFileState::file_reference,
					bool)));
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::clear_rows()
{
	table_feature_collections->clearContents();	// Do not clear the header items as well.
	table_feature_collections->setRowCount(0);	// Do remove the newly blanked rows.
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::add_row(
		GPlatesAppLogic::FeatureCollectionFileState::file_reference file)
{
	const GPlatesFileIO::FileInfo &file_info = file.get_file().get_file_info();

	// Obtain information from the FileInfo
	const QFileInfo &qfileinfo = file_info.get_qfileinfo();

	// Some files might not actually exist yet if the user created a new
	// feature collection internally and hasn't saved it to file yet.
	QString display_name;
	if (GPlatesFileIO::file_exists(file_info))
	{
		display_name = file_info.get_display_name(false);
	}
	else
	{
		// The file doesn't exist so give it a filename to indicate this.
		display_name = "New Feature Collection";
	}
	
	QString filename_str = qfileinfo.fileName();
	QString filepath_str = qfileinfo.path();
	QString format_str = get_format_for_file(qfileinfo);
	const bool active = file.is_file_active();

	// Add blank row.
	int row = table_feature_collections->rowCount();
	table_feature_collections->insertRow(row);
	
	// Add filename item.
	QTableWidgetItem *filename_item = new QTableWidgetItem(display_name);
	filename_item->setToolTip(tr("Location: %1").arg(filepath_str));
	filename_item->setFlags(Qt::ItemIsEnabled);
	table_feature_collections->setItem(row, ColumnNames::FILENAME, filename_item);

	// Add file format item.
	QTableWidgetItem *format_item = new QTableWidgetItem(format_str);
	format_item->setFlags(Qt::ItemIsEnabled);
	table_feature_collections->setItem(row, ColumnNames::FORMAT, format_item);

	// Add layer type / in use status.
	ManageFeatureCollectionsStateWidget *state_widget_ptr =
			new ManageFeatureCollectionsStateWidget(*this, file, 
					active, true/*enable*/, this);
	table_feature_collections->setCellWidget(row, ColumnNames::LAYER_TYPES, state_widget_ptr);
	
	// Add action buttons widget.
	ManageFeatureCollectionsActionWidget *action_widget_ptr =
			new ManageFeatureCollectionsActionWidget(*this, file, this);
	table_feature_collections->setCellWidget(row, ColumnNames::ACTIONS, action_widget_ptr);
	
	// Enable the edit_configuration button if we have a shapefile. 
	if (format_str == "ESRI shapefile")
	{
		action_widget_ptr->enable_edit_configuration_button();
	}

	// A bit inefficient, but highlight everything now that we have a new row.
	highlight_unsaved_changes();
}


int
GPlatesQtWidgets::ManageFeatureCollectionsDialog::find_row(
		ManageFeatureCollectionsActionWidget *action_widget_ptr)
{
	int row = 0;
	int end = table_feature_collections->rowCount();
	for (; row < end; ++row) {
		if (get_action_widget(table_feature_collections, row) == action_widget_ptr) {
			return row;
		}
	}
	return end;
}


int
GPlatesQtWidgets::ManageFeatureCollectionsDialog::find_row(
		GPlatesAppLogic::FeatureCollectionFileState::file_reference file_ref)
{
	// Look for the action widget that references 'file_ref'.
	int row = 0;
	int end = table_feature_collections->rowCount();
	for ( ; row < end; ++row)
	{
		ManageFeatureCollectionsActionWidget *action_widget = get_action_widget(
				table_feature_collections, row);
		if (action_widget &&
				action_widget->get_file_reference() == file_ref)
		{
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
	remove_row(row);
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::remove_row(
		int row)
{
	if (row < table_feature_collections->rowCount()) {
		table_feature_collections->removeRow(row);
	}
}




void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::save_all_named()
{
	save_all(false);
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::save_all(
		bool include_unnamed_files)
{
	// Instant feedback is nice for such a long operation with no progress bars (yet).
	QString button_text_normal = button_save_all->text();
	button_save_all->setText(tr("Saving..."));
	button_save_all->setEnabled(false);
	// Attempting to make the GUI actually update...
	button_save_all->update();
	QCoreApplication::processEvents();

	// Save all, with feedback.
	d_gui_file_io_feedback_ptr->save_all(include_unnamed_files);
	
	// Update each row.
	highlight_unsaved_changes();

	// Re-enable button.
	button_save_all->setText(button_text_normal);
	button_save_all->setEnabled(true);
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::highlight_unsaved_changes()
{
	// Assume no unsaved changes; hide the info labels.
	label_unsaved_changes->hide();
	label_unsaved_changes_swatch->hide();
	label_no_presence_on_disk->hide();
	label_no_presence_on_disk_swatch->hide();
	// Change all row background colours to reflect unsavitivity.
	// As a side effect, if there are any rows of that particular colour,
	// the corresponding label will be shown.
	for (int row = 0; row < table_feature_collections->rowCount(); ++row) {
		set_row_background_colour(row);
	}
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::set_row_background_colour(
		int row)
{
	// The colours to be used.
	static const QBrush bg_normal(bg_colour_normal);
	static const QBrush bg_unsaved(bg_colour_unsaved); // Red raised to same lightness as below.
	static const QBrush bg_new_fc(bg_colour_new_feature_collection);	// Ubuntu Light Orange Text Highlight

	// Get the file_reference corresponding to this table row.
	ManageFeatureCollectionsActionWidget *action_widget = get_action_widget(
			table_feature_collections, row);
	if ( ! action_widget) {
		return;
	}
	GPlatesAppLogic::FeatureCollectionFileState::file_reference file_ref =
			action_widget->get_file_reference();

	// Get the FileInfo and Feature Collection associated with that file.
	GPlatesModel::FeatureCollectionHandle::const_weak_ref feature_collection_ref =
			file_ref.get_file().get_feature_collection();
	const GPlatesFileIO::FileInfo &file_info = file_ref.get_file().get_file_info();
	if (feature_collection_ref.is_valid()) {
		bool has_unsaved_changes = feature_collection_ref->contains_unsaved_changes();
		bool has_no_associated_file = ! GPlatesFileIO::file_exists(file_info);

		if (has_no_associated_file) {
			set_row_background(table_feature_collections, row, bg_new_fc);
			label_no_presence_on_disk->show();
			label_no_presence_on_disk_swatch->show();
		} else if (has_unsaved_changes) {
			set_row_background(table_feature_collections, row, bg_unsaved);
			label_unsaved_changes->show();
			label_unsaved_changes_swatch->show();
		} else {
			set_row_background(table_feature_collections, row, bg_normal);
		}
	} else {
		// Something is seriously wrong.
	}
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::dragEnterEvent(
		QDragEnterEvent *ev)
{
	if (ev->mimeData()->hasUrls()) {
		ev->acceptProposedAction();
	} else {
		ev->ignore();
	}
}

void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::dropEvent(
		QDropEvent *ev)
{
	if (ev->mimeData()->hasUrls()) {
		ev->acceptProposedAction();
		d_gui_file_io_feedback_ptr->open_urls(ev->mimeData()->urls());
	} else {
		ev->ignore();
	}
}


