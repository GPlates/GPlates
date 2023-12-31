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

#include <set>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QHeaderView>
#include <QMessageBox>
#include <QMimeData>
#include <QPushButton>
#include <QTableWidget>

#include "ManageFeatureCollectionsDialog.h"

#include "ManageFeatureCollectionsActionWidget.h"
#include "ManageFeatureCollectionsEditConfigurations.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/FeatureCollectionFileIO.h"
#include "app-logic/FeatureCollectionFileState.h"
#include "app-logic/ReconstructGraph.h"

#include "file-io/ErrorOpeningFileForWritingException.h"
#include "file-io/ErrorOpeningPipeFromGzipException.h"
#include "file-io/ErrorOpeningPipeToGzipException.h"
#include "file-io/FeatureCollectionFileFormat.h"
#include "file-io/FeatureCollectionFileFormatConfiguration.h"
#include "file-io/FeatureCollectionFileFormatRegistry.h"
#include "file-io/FileFormatNotSupportedException.h"
#include "file-io/FileInfo.h"
#include "file-io/OgrException.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/InvalidFeatureCollectionException.h"
#include "global/UnexpectedEmptyFeatureCollectionException.h"

#include "gui/FileIOFeedback.h"

#include "model/FeatureCollectionHandle.h"

#include "presentation/ViewState.h"

#include "utils/Profile.h"


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
			FILENAME, FORMAT, ACTIONS
		};
	}
	
	/**
	 * Returns the file format for a file if it was identified, otherwise returns boost::none.
	 */
	boost::optional<GPlatesFileIO::FeatureCollectionFileFormat::Format>
	get_format_for_file(
			GPlatesAppLogic::FeatureCollectionFileState::file_reference file,
			const GPlatesFileIO::FeatureCollectionFileFormat::Registry &file_format_registry)
	{
		// Determine the file format from the filename.
		const boost::optional<GPlatesFileIO::FeatureCollectionFileFormat::Format> file_format =
				file_format_registry.get_file_format(
						file.get_file().get_file_info().get_qfileinfo());

		// Note that we don't throw a 'FileFormatNotSupportedException' exception here since
		// it's possible for generic XML data to now be loaded into GPlates and it seems the
		// filename extension could be anything.

		return file_format;
	}
	
	/**
	 * Returns the file format for a file.
	 * Used to set the 'FORMAT' column in the table.
	 */
	QString
	get_format_description_for_file(
			boost::optional<GPlatesFileIO::FeatureCollectionFileFormat::Format> file_format,
			const GPlatesFileIO::FeatureCollectionFileFormat::Registry &file_format_registry)
	{
		if (!file_format)
		{
			// Could not find a read or write format for the specified file so return an
			// empty format string to indicate an unknown file format.
			return QObject::tr("");
		}

		return QObject::tr(
				file_format_registry.get_short_description(file_format.get())
						.toLatin1().constData());
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
	 * Returns a list of selected files (rows) by returning the action widget in each selected row.
	 *
	 * Note that only rows with non-null action widgets are included (there shouldn't be any null widgets though).
	 */
	std::vector<GPlatesQtWidgets::ManageFeatureCollectionsActionWidget *>
	get_selected_action_widgets(
			QTableWidget *qtable_widget)
	{
		std::vector<GPlatesQtWidgets::ManageFeatureCollectionsActionWidget *> selected_files;

		// Iterate over the selected files.
		const QList<QTableWidgetSelectionRange> selected_ranges = qtable_widget->selectedRanges();
		const int num_selected_ranges = selected_ranges.count();
		for (int range_index = 0; range_index < num_selected_ranges; ++range_index)
		{
			const QTableWidgetSelectionRange &selection_range = selected_ranges.at(range_index);

			// Iterate over the rows in the current selection range.
			for (int selected_row = selection_range.topRow();
				selected_row <= selection_range.bottomRow();
				++selected_row)
			{
				// Get the file_reference corresponding to this table row.
				GPlatesQtWidgets::ManageFeatureCollectionsActionWidget *action_widget =
						get_action_widget(qtable_widget, selected_row);
				if (!action_widget)
				{
					continue;
				}

				selected_files.push_back(action_widget);
			}
		}

		return selected_files;
	}


	/**
	 * Returns a list of selected files (rows) by returning the file reference in each selected row.
	 */
	std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference>
	get_selected_files(
			QTableWidget *qtable_widget)
	{
		std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> selected_files;

		std::vector<GPlatesQtWidgets::ManageFeatureCollectionsActionWidget *> selected_action_widgets =
				get_selected_action_widgets(qtable_widget);
		for (unsigned int n = 0; n < selected_action_widgets.size(); ++n)
		{
			selected_files.push_back(selected_action_widgets[n]->get_file_reference());
		}

		return selected_files;
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
		GPlatesAppLogic::ReconstructGraph &reconstruct_graph,
		GPlatesPresentation::ViewState &view_state, 
		QWidget *parent_):
	GPlatesDialog(parent_, Qt::Window),
	d_file_format_registry(view_state.get_application_state().get_feature_collection_file_format_registry()),
	d_file_state(file_state),
	d_feature_collection_file_io(&feature_collection_file_io),
	d_gui_file_io_feedback_ptr(&gui_file_io_feedback),
	d_reconstruct_graph(reconstruct_graph),
	d_view_state(view_state)
{
	setupUi(this);

	// Focus on the list of feature collections so that any selected files (rows) are more visible
	// (the row highlights are darker, and hence more noticeable, when the QTableWidget has focus).
	setFocusProxy(table_feature_collections);

	// Try to adjust column widths.
	QHeaderView *header = table_feature_collections->horizontalHeader();
	header->setSectionResizeMode(ColumnNames::FILENAME, QHeaderView::Stretch);
	header->resizeSection(ColumnNames::FORMAT, 128);
	header->resizeSection(ColumnNames::ACTIONS, 212);

	// Enforce minimum row height for the Actions widget's sake.
	QHeaderView *sider = table_feature_collections->verticalHeader();
	sider->setSectionResizeMode(QHeaderView::Fixed);
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

	// Gray out all buttons that have to do with selected feature collections (initially no collections selected).
	selection_buttons->setEnabled(false);

	// Set up slots for Open File and Save All
	QObject::connect(button_open_file, SIGNAL(clicked()), d_gui_file_io_feedback_ptr, SLOT(open_files()));
	QObject::connect(button_save_all_changes, SIGNAL(clicked()), this, SLOT(save_all_named_changes()));

	// Set up slots for Save Selected, Unload Selected, Reload Selected and Clear Selection.
	QObject::connect(button_save_selected, SIGNAL(clicked()), this, SLOT(save_selected()));
	QObject::connect(button_reload_selected, SIGNAL(clicked()), this, SLOT(reload_selected()));
	QObject::connect(button_unload_selected, SIGNAL(clicked()), this, SLOT(unload_selected()));
	QObject::connect(button_clear_selection, SIGNAL(clicked()), this, SLOT(clear_selection()));

	QObject::connect(
			table_feature_collections, SIGNAL(itemSelectionChanged()),
			this, SLOT(handle_selection_changed()));
	QObject::connect(
			table_feature_collections->horizontalHeader(), SIGNAL(sectionClicked(int)),
			this, SLOT(header_section_clicked(int)));

	// Set up slots for file load/unload notifications.
	connect_to_file_state_signals();

	// The close button has the initial focus.
	button_close->setFocus();
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::register_edit_configuration(
		GPlatesFileIO::FeatureCollectionFileFormat::Format file_format,
		const ManageFeatureCollections::EditConfiguration::shared_ptr_type &edit_configuration_ptr)
{
	d_edit_configurations[file_format] = edit_configuration_ptr;
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::edit_configuration(
		ManageFeatureCollectionsActionWidget *action_widget_ptr)
{
	GPlatesAppLogic::FeatureCollectionFileState::file_reference file = action_widget_ptr->get_file_reference();

	boost::optional<GPlatesFileIO::FeatureCollectionFileFormat::Format> file_format_opt =
			get_format_for_file(file, d_file_format_registry);

	// The edit configuration button (in the action widget) should only be enabled we have
	// identified a file format for the file.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			file_format_opt,
			GPLATES_ASSERTION_SOURCE);
	const GPlatesFileIO::FeatureCollectionFileFormat::Format file_format = file_format_opt.get();

	// The edit configuration button (in the action widget) should only be enabled we have an
	// edit configuration (ability to edit the file configuration).
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_edit_configurations.find(file_format) != d_edit_configurations.end(),
			GPLATES_ASSERTION_SOURCE);

	// Get the file configuration from the file if it has one otherwise use the default configuration
	// associated with its file format.
	boost::optional<GPlatesFileIO::FeatureCollectionFileFormat::Configuration::shared_ptr_to_const_type>
			file_configuration = file.get_file().get_file_configuration();
	if (!file_configuration)
	{
		file_configuration = d_file_format_registry.get_default_configuration(file_format);
	}

	// If there's no default configuration then return early without editing.
	if (!file_configuration)
	{
		qWarning() << "ERROR: Unable to edit file configuration because the file format has no default configuration.";
		return;
	}

	// The user can now edit the file configuration.
	file_configuration =
			d_edit_configurations[file_format]->edit_configuration(
					file.get_file(),
					file_configuration.get(),
					parentWidget());

	// Store the (potentially) updated file configuration back in the file.
	// NOTE: This will trigger a signal that will call our 'handle_file_state_file_info_changed' method.
	file.set_file_info(file.get_file().get_file_info(), file_configuration);
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
	// Block any calls to 'ApplicationState::reconstruct()' because we're going to
	// call it at the end of this method.
	GPlatesAppLogic::ApplicationState::ScopedReconstructGuard scoped_reconstruct_guard(
			d_view_state.get_application_state());

	GPlatesAppLogic::FeatureCollectionFileState::file_reference file_it =
			action_widget_ptr->get_file_reference();
	
	d_gui_file_io_feedback_ptr->reload_file(file_it);

	// Make sure 'ApplicationState::reconstruct()' gets called when all scopes exit.
	scoped_reconstruct_guard.call_reconstruct_on_scope_exit();
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::unload_file(
		ManageFeatureCollectionsActionWidget *action_widget_ptr)
{
	// Block any calls to 'ApplicationState::reconstruct()' because we're going to
	// call it at the end of this method.
	GPlatesAppLogic::ApplicationState::ScopedReconstructGuard scoped_reconstruct_guard(
			d_view_state.get_application_state());

	GPlatesAppLogic::FeatureCollectionFileState::file_reference file_it =
			action_widget_ptr->get_file_reference();
	
	d_feature_collection_file_io->unload_file(file_it);

	// Make sure 'ApplicationState::reconstruct()' gets called when all scopes exit.
	scoped_reconstruct_guard.call_reconstruct_on_scope_exit();
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
		add_row(file_ref, false/*should_highlight_unsaved_changes*/);
	}

	// Highlight unsaved changes all in one go instead of individually for each file.
	highlight_unsaved_changes();
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
	// Find the existing row for the specified file.
	const int row = find_row(file_ref);
	if (row == table_feature_collections->rowCount())
	{
		// We should assert here but printing a warning instead.
		qWarning() << "Internal Error: Unable to find renamed file in ManageFeatureCollectionsDialog.";
		return;
	}

	// Row text needs to be updated to reflect a new filename and a new default file configuration
	// if the file's format needs a file configuration.
	update_row(row, file_ref);
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::header_section_clicked(
		int section_index)
{
	// We only sort by filename or file format.
	if (section_index != ColumnNames::FILENAME &&
		section_index != ColumnNames::FORMAT)
	{
		// Make sure the sort indicator remains drawn on the previously sorted column, if any,
		// otherwise it'll be drawn on the currently selected column (the column we're ignoring).
		if (d_column_sort)
		{
			table_feature_collections->horizontalHeader()->setSortIndicator(
					d_column_sort->column_index, d_column_sort->sort_order);
		}

		return;
	}

	if (d_column_sort)
	{
		// If the column is the same as before then turn off sorting.
		if (d_column_sort->column_index == section_index)
		{
			// Turn off sorting.
			d_column_sort = boost::none;
			table_feature_collections->horizontalHeader()->setSortIndicatorShown(false);

			//
			// Sort by original file order (the order in which files were loaded).
			//
			// This is most easily done by clearing all rows and adding them in the original order.

			// First get a list of all selected files, if any.
			const std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> selected_files =
					get_selected_files(table_feature_collections);
			const std::set<GPlatesAppLogic::FeatureCollectionFileState::file_reference> selected_files_set(
					selected_files.begin(), selected_files.end());

			clear_rows();

			const std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> file_references =
					d_file_state.get_loaded_files();
			BOOST_FOREACH(
					const GPlatesAppLogic::FeatureCollectionFileState::file_reference &file_ref,
					file_references)
			{
				add_row(file_ref, false/*should_highlight_unsaved_changes*/);

				// If the current file was previously selected then add it to the selection.
				if (selected_files_set.find(file_ref) != selected_files_set.end())
				{
					// Select the row just added.
					table_feature_collections->setRangeSelected(
							QTableWidgetSelectionRange(
									table_feature_collections->rowCount() - 1,
									0,
									table_feature_collections->rowCount() - 1,
									table_feature_collections->columnCount() - 1),
							true);
				}
			}

			// Highlight unsaved changes all in one go instead of individually for each file.
			highlight_unsaved_changes();

			return;
		}
	}
	else // Sorting is currently disabled, so enable it...
	{
		d_column_sort = ColumnSort();
	}

	// Set the column to sort and the sort order (currently sort order is always ascending anyway).
	d_column_sort->column_index = section_index;
	d_column_sort->sort_order = Qt::AscendingOrder;

	// Sort the table.
	table_feature_collections->sortItems(d_column_sort->column_index, d_column_sort->sort_order);

	// Draw the sort indicator.
	table_feature_collections->horizontalHeader()->setSortIndicator(
			d_column_sort->column_index, d_column_sort->sort_order);
	table_feature_collections->horizontalHeader()->setSortIndicatorShown(true);
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::handle_selection_changed()
{
	const QList<QTableWidgetSelectionRange> selected_ranges = table_feature_collections->selectedRanges();

	// Enable/disable all buttons related to selections based on whether any files/rows are selected.
	selection_buttons->setEnabled(!selected_ranges.isEmpty());
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
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::clear_rows()
{
	table_feature_collections->clearContents();	// Do not clear the header items as well.
	table_feature_collections->setRowCount(0);	// Do remove the newly blanked rows.
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::add_row(
		GPlatesAppLogic::FeatureCollectionFileState::file_reference file,
		bool should_highlight_unsaved_changes)
{
	// Add blank row.
	const int row = table_feature_collections->rowCount();
	table_feature_collections->insertRow(row);

	// Set the actions item so that we can mark the cell as non-selectable.
	// This way the row colour show through selected rows (files) from behind the action buttons.
	// Otherwise the row colour is not visible for selected rows.
	QTableWidgetItem *actions_item = new QTableWidgetItem("");
	actions_item->setFlags(Qt::ItemIsEnabled); // Not selectable.
	table_feature_collections->setItem(row, ColumnNames::ACTIONS, actions_item);

	// Add action buttons widget.
	ManageFeatureCollectionsActionWidget *action_widget_ptr =
			new ManageFeatureCollectionsActionWidget(
					*this,
					file,
					this);
	table_feature_collections->setCellWidget(row, ColumnNames::ACTIONS, action_widget_ptr);

	update_row(row, file, should_highlight_unsaved_changes);
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::update_row(
		int row,
		GPlatesAppLogic::FeatureCollectionFileState::file_reference file,
		bool should_highlight_unsaved_changes)
{
	// Obtain information from the FileInfo
	const GPlatesFileIO::FileInfo &file_info = file.get_file().get_file_info();

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

	// Determine the file format of the file if possible.
	boost::optional<GPlatesFileIO::FeatureCollectionFileFormat::Format> file_format =
			get_format_for_file(file, d_file_format_registry);

	QString format_str = get_format_description_for_file(file_format, d_file_format_registry);
	QString filename_str = file_info.get_qfileinfo().fileName();
	QString filepath_str = QDir::toNativeSeparators(file_info.get_qfileinfo().path());
	
	// Set the filename item.
	QTableWidgetItem *filename_item = new QTableWidgetItem(display_name);
	filename_item->setToolTip(tr("Location: %1").arg(filepath_str));
	filename_item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
	table_feature_collections->setItem(row, ColumnNames::FILENAME, filename_item);

	// Set the file format item.
	QTableWidgetItem *format_item = new QTableWidgetItem(format_str);
	format_item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
	table_feature_collections->setItem(row, ColumnNames::FORMAT, format_item);
	
	// Update the action buttons widget.
	ManageFeatureCollectionsActionWidget *action_widget = get_action_widget(table_feature_collections, row);
	if (action_widget)
	{
		// If we have an edit configuration for the current file format then we can enable the
		// edit configuration button in the action widget.
		const bool enable_edit_configuration =
				file_format &&
					(d_edit_configurations.find(file_format.get()) != d_edit_configurations.end());

		action_widget->update(d_file_format_registry, file_info, file_format, enable_edit_configuration);
	}

	// This might be false if many rows are being added in which case the unsaved changes
	// will be highlighted by the caller once *all* rows have been added.
	if (should_highlight_unsaved_changes)
	{
		highlight_unsaved_changes();
	}

	// Sort the table if sorting has been enabled (if the user clicked on the filename or format header).
	if (d_column_sort)
	{
		table_feature_collections->sortItems(d_column_sort->column_index, d_column_sort->sort_order);
	}
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
GPlatesQtWidgets::ManageFeatureCollectionsDialog::save_all_named_changes()
{
	save_all(false/*include_unnamed_files*/, true/*only_unsaved_changes*/);
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::save_all(
		bool include_unnamed_files,
		bool only_unsaved_changes)
{
	// Instant feedback is nice for such a long operation with no progress bars (yet).
	QString button_text_normal = button_save_all_changes->text();
	button_save_all_changes->setText(tr("Saving..."));
	button_save_all_changes->setEnabled(false);
	// Attempting to make the GUI actually update...
	button_save_all_changes->update();
	QCoreApplication::processEvents();

	// Save all, with feedback.
	d_gui_file_io_feedback_ptr->save_all(include_unnamed_files, only_unsaved_changes);

	// Update each row.
	highlight_unsaved_changes();

	// Re-enable button.
	button_save_all_changes->setText(button_text_normal);
	button_save_all_changes->setEnabled(true);
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::save_selected()
{
	// Instant feedback is nice for such a long operation with no progress bars (yet).
	QString button_text_normal = button_save_selected->text();
	button_save_selected->setText(tr("Saving..."));
	button_save_selected->setEnabled(false);
	// Attempting to make the GUI actually update...
	button_save_selected->update();
	QCoreApplication::processEvents();

	// Get a list of all selected files.
	std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> selected_files =
			get_selected_files(table_feature_collections);

	// Save selected files, with feedback.
	d_gui_file_io_feedback_ptr->save_files(
			selected_files,
			false/*include_unnamed_files*/,
			// We're saving whether the files have unsaved changes or not...
			false/*only_unsaved_changes*/);

	// Update each row.
	highlight_unsaved_changes();

	// Re-enable button.
	button_save_selected->setText(button_text_normal);
	button_save_selected->setEnabled(true);

	// Set the focus back to the table widget so the user can see the selected highlights more easily.
	table_feature_collections->setFocus();
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::reload_selected()
{
	// Block any calls to 'ApplicationState::reconstruct()' because we're going to
	// call it at the end of this method.
	GPlatesAppLogic::ApplicationState::ScopedReconstructGuard scoped_reconstruct_guard(
			d_view_state.get_application_state());

	// Reloading files can, under certain circumstances, trigger layer additions.
	// This is only when the type of features in the reloaded files has changed such that
	// new types of layers are required.
	// As an optimisation, put all layer additions in a single add layers group.
	GPlatesAppLogic::ReconstructGraph::AddOrRemoveLayersGroup add_layers_group(d_reconstruct_graph);
	add_layers_group.begin_add_or_remove_layers();

	// Get a list of all selected files.
	const std::vector<ManageFeatureCollectionsActionWidget *> selected_files =
			get_selected_action_widgets(table_feature_collections);

	// Now that we've collected all the selected files we can safely unload them
	// without causing problem with row indices due to row removal.
	for (unsigned int n = 0; n < selected_files.size(); ++n)
	{
		ManageFeatureCollectionsActionWidget *selected_file = selected_files[n];

		GPlatesAppLogic::FeatureCollectionFileState::file_reference file_ref =
				selected_file->get_file_reference();

		// Skip files that don't exist yet (new files that the user has not yet assigned filenames).
		if (!GPlatesFileIO::file_exists(file_ref.get_file().get_file_info()))
		{
			continue;
		}

		reload_file(selected_file);
	}

	// Set the focus back to the table widget so the user can see the selected highlights more easily.
	table_feature_collections->setFocus();

	add_layers_group.end_add_or_remove_layers();

	// Make sure 'ApplicationState::reconstruct()' gets called when all scopes exit.
	scoped_reconstruct_guard.call_reconstruct_on_scope_exit();
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::unload_selected()
{
	// Block any calls to 'ApplicationState::reconstruct()' because we're going to
	// call it at the end of this method.
	GPlatesAppLogic::ApplicationState::ScopedReconstructGuard scoped_reconstruct_guard(
			d_view_state.get_application_state());

	// Unloading files can trigger layer removals.
	// As an optimisation, put all layer removals in a single remove layers group.
	GPlatesAppLogic::ReconstructGraph::AddOrRemoveLayersGroup remove_layers_group(d_reconstruct_graph);
	remove_layers_group.begin_add_or_remove_layers();

	// First get a list of all selected files before we unload any of them.
	const std::vector<ManageFeatureCollectionsActionWidget *> selected_files =
			get_selected_action_widgets(table_feature_collections);

	// Now that we've collected all the selected files we can safely unload them
	// without causing problem with row indices due to row removal.
	for (unsigned int n = 0; n < selected_files.size(); ++n)
	{
		ManageFeatureCollectionsActionWidget *selected_file = selected_files[n];

		// Note that we unload a file even if the user has not yet assigned a filename and hence
		// doesn't exist of disk yet.
		unload_file(selected_file);
	}

	remove_layers_group.end_add_or_remove_layers();

	// Make sure 'ApplicationState::reconstruct()' gets called when all scopes exit.
	scoped_reconstruct_guard.call_reconstruct_on_scope_exit();
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::clear_selection()
{
	// Deselect the entire table.
	table_feature_collections->setRangeSelected(
			QTableWidgetSelectionRange(
					0,
					0,
					table_feature_collections->rowCount() - 1,
					table_feature_collections->columnCount() - 1),
			false);
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
	for (int row = 0; row < table_feature_collections->rowCount(); ++row)
	{
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
	if (ev->mimeData()->hasUrls())
	{
		const QStringList feature_collection_filenames =
				d_gui_file_io_feedback_ptr->extract_feature_collection_filenames_from_file_urls(
						ev->mimeData()->urls());
		if (!feature_collection_filenames.isEmpty())
		{
			ev->acceptProposedAction();
			return;
		}
	}

	ev->ignore();
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::dropEvent(
		QDropEvent *ev)
{
	if (ev->mimeData()->hasUrls())
	{
		const QStringList feature_collection_filenames =
				d_gui_file_io_feedback_ptr->extract_feature_collection_filenames_from_file_urls(
						ev->mimeData()->urls());
		if (!feature_collection_filenames.isEmpty())
		{
			ev->acceptProposedAction();
			d_gui_file_io_feedback_ptr->open_files(feature_collection_filenames);
			return;
		}
	}

	ev->ignore();
}
