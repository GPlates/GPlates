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
 
#ifndef GPLATES_GUI_MANAGEFEATURECOLLECTIONSDIALOG_H
#define GPLATES_GUI_MANAGEFEATURECOLLECTIONSDIALOG_H

#include <map>
#include <vector>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <Qt>
#include <QObject>
#include <QPointer>
#include <QString>

#include "ui_ManageFeatureCollectionsDialogUi.h"

#include "GPlatesDialog.h"
#include "SaveFileDialog.h"

#include "app-logic/FeatureCollectionFileState.h"

#include "file-io/FeatureCollectionFileFormat.h"


namespace GPlatesAppLogic
{
	class FeatureCollectionFileIO;
	class ReconstructGraph;
}

namespace GPlatesFileIO
{
	namespace FeatureCollectionFileFormat
	{
		class Registry;
	}
}

namespace GPlatesGui
{
	class FileIOFeedback;
}

namespace GPlatesPresentation
{
	class ViewState;
}


namespace GPlatesQtWidgets
{
	class ManageFeatureCollectionsActionWidget;
	
	namespace ManageFeatureCollections
	{
		class EditConfiguration;
	}

	class ManageFeatureCollectionsDialog:
			public GPlatesDialog, 
			protected Ui_ManageFeatureCollectionsDialog
	{
		Q_OBJECT
		
	public:

		explicit
		ManageFeatureCollectionsDialog(
				GPlatesAppLogic::FeatureCollectionFileState &file_state,
				GPlatesAppLogic::FeatureCollectionFileIO &feature_collection_file_io,
				GPlatesGui::FileIOFeedback &gui_file_io_feedback,
				GPlatesAppLogic::ReconstructGraph &reconstruct_graph,
				GPlatesPresentation::ViewState& d_view_state,
				QWidget *parent_ = NULL);
		
		/**
		 * Registers an edit configuration for the specified file format.
		 *
		 * When the 'edit configuration' button is pressed the edit configuration GUI will be triggered.
		 * Also the 'edit configuration' button is only enabled for registered file formats.
		 */
		void
		register_edit_configuration(
				GPlatesFileIO::FeatureCollectionFileFormat::Format file_format,
				const boost::shared_ptr<ManageFeatureCollections::EditConfiguration> &edit_configuration_ptr);

		/**
		 * Initiates editing of the file configuration. 
		 */
		void
		edit_configuration(
				ManageFeatureCollectionsActionWidget *action_widget_ptr);

		/**
		 * Causes the file referenced by the action widget to be saved with its
		 * current name. No user interaction is necessary unless an exception
		 * occurs.
		 */
		void
		save_file(
				ManageFeatureCollectionsActionWidget *action_widget_ptr);

		/**
		 * Causes the file referenced by the action widget to be saved with a
		 * new name. A file save dialog is popped up to request the new name
		 * from the user. The file info and appropriate table row will be updated.
		 * Exceptions will be handled with a small error dialog.
		 */
		void
		save_file_as(
				ManageFeatureCollectionsActionWidget *action_widget_ptr);

		/**
		 * Causes a copy of the file referenced by the action widget to be saved
		 * using different name. A file save dialog is popped up to request the new
		 * name from the user. The file info and appropriate table row will be updated.
		 * Exceptions will be handled with a small error dialog.
		 */
		void
		save_file_copy(
				ManageFeatureCollectionsActionWidget *action_widget_ptr);

		/**
		 * Unloads and re-loads the file referenced by the action widget.
		 */
		void
		reload_file(
				ManageFeatureCollectionsActionWidget *action_widget_ptr);

		/**
		 * Causes the file referenced by the action widget to be unloaded,
		 * and removed from the table. No user interaction is necessary.
		 */
		void
		unload_file(
				ManageFeatureCollectionsActionWidget *action_widget_ptr);
	
	public Q_SLOTS:

		/**
		 * Recolours table rows' background colours based on saved/unsaved state.
		 *
		 * If the only thing that has changed is this saved/unsaved state, this is much
		 * cheaper to call and will not result in the scroll pane jumping to the top.
		 */
		void
		highlight_unsaved_changes();

		
		/**
		 * Saves-in-place all files with unsaved changes, except for those which
		 * have not yet been given filenames.
		 */
		void
		save_all_named_changes();

	private Q_SLOTS:
		// NOTE: all signals/slots should use namespace scope for all arguments
		//       otherwise differences between signals and slots will cause Qt
		//       to not be able to connect them at runtime.

		void
		handle_file_state_files_added(
				GPlatesAppLogic::FeatureCollectionFileState &file_state,
				const std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> &new_files);

		void
		handle_file_state_file_about_to_be_removed(
				GPlatesAppLogic::FeatureCollectionFileState &file_state,
				GPlatesAppLogic::FeatureCollectionFileState::file_reference file);

		void
		handle_file_state_file_info_changed(
				GPlatesAppLogic::FeatureCollectionFileState &file_state,
				GPlatesAppLogic::FeatureCollectionFileState::file_reference file);

		void
		header_section_clicked(
				int section_index);

		void
		handle_selection_changed();

		void
		save_selected();

		void
		reload_selected();

		void
		unload_selected();

		void
		clear_selection();

	protected:
	
		/**
		 * Deletes items and completely removes all rows from the table.
		 */
		void
		clear_rows();

		/**
		 * Adds a row to the table, creating a ManageFeatureCollectionsActionWidget to
		 * store the FileInfo and the buttons used to interact with the file.
		 *
		 * @a should_highlight_unsaved_changes might be false if you're adding many rows
		 * and you want to highlight unsaved changes only once at the end.
		 */
		void
		add_row(
				GPlatesAppLogic::FeatureCollectionFileState::file_reference file_it,
				bool should_highlight_unsaved_changes = true);

		/**
		 * Updates the specified row in the table to a new filename (FileInfo) and default
		 * file configuration if one is required for the file's format.
		 *
		 * @a should_highlight_unsaved_changes might be false if you're updatin many rows
		 * and you want to highlight unsaved changes only once at the end.
		 */
		void
		update_row(
				int row,
				GPlatesAppLogic::FeatureCollectionFileState::file_reference file,
				bool should_highlight_unsaved_changes = true);

		/**
		 * Locates the current row of the table used by the given action widget.
		 * Will return table_feature_collections->rowCount() if not found.
		 */
		int
		find_row(
				ManageFeatureCollectionsActionWidget *action_widget_ptr);
		
		/**
		 * Locates the current row of the table used by the given file info iterator.
		 * Will return table_feature_collections->rowCount() if not found.
		 */
		int
		find_row(
				GPlatesAppLogic::FeatureCollectionFileState::file_reference file_it);

		/**
		 * Removes the row indicated by the given action widget.
		 */
		void
		remove_row(
				ManageFeatureCollectionsActionWidget *action_widget_ptr);

		/**
		 * Removes the row indicated indexed by @a row.
		 */
		void
		remove_row(
				int row);


		/**
		 * Goes through each loaded file and saves-in-place.
		 *
		 * If @a only_unsaved_changes is true then only files with unsaved changes will be saved.
		 *
		 * Will prompt for filenames on unnamed collections if @a include_unnamed_files.
		 */
		void
		save_all(
				bool include_unnamed_files,
				bool only_unsaved_changes);

		/**
		 * Recolours a single row's background based on saved/unsaved state.
		 */
		void
		set_row_background_colour(
				int row);

		/**
		 * Reimplementation of drag/drop events so we can handle users dragging files onto
		 * Manage Feature Collections Dialog.
		 */
		void
		dragEnterEvent(
				QDragEnterEvent *ev);

		/**
		 * Reimplementation of drag/drop events so we can handle users dragging files onto
		 * Manage Feature Collections Dialog.
		 */
		void
		dropEvent(
				QDropEvent *ev);

	private:
		//! Typedef for a mapping of file formats to registered edit configurations.
		typedef std::map<
				GPlatesFileIO::FeatureCollectionFileFormat::Format,
				boost::shared_ptr<ManageFeatureCollections::EditConfiguration> >
						edit_configuration_map_type;

		//! Identifies which column (if any) is sorted and whether it's sorted ascending or descending.
		struct ColumnSort
		{
			ColumnSort() :
				column_index(0),
				sort_order(Qt::AscendingOrder)
			{  }

			int column_index;
			Qt::SortOrder sort_order;
		};


		/**
		 * Registry of file formats.
		 */
		const GPlatesFileIO::FeatureCollectionFileFormat::Registry &d_file_format_registry;

		/**
		 * The loaded feature collection files.
		 */
		GPlatesAppLogic::FeatureCollectionFileState &d_file_state;

		/**
		 * Handles loading/unloading of feature collections.
		 */
		GPlatesAppLogic::FeatureCollectionFileIO *d_feature_collection_file_io;

		/**
		 * GUI wrapper around saving/loading to handle feedback dialogs, progress bars, etc.
		 *
		 * Guarded pointer, ViewportWindow/Qt owns FileIOFeedback.
		 */
		QPointer<GPlatesGui::FileIOFeedback> d_gui_file_io_feedback_ptr;

		/**
		 * As an optimisation, group a sequence of file unloads into a single remove layers group.
		 *
		 * File unloading can trigger layer removals.
		 */
		GPlatesAppLogic::ReconstructGraph &d_reconstruct_graph;

		GPlatesPresentation::ViewState& d_view_state;

		/**
		 * The registered edit configurations (mapped to file formats).
		 */
		edit_configuration_map_type d_edit_configurations;

		/**
		 * The column (and sort order) currently used for sorting, if sorting enabled.
		 */
		boost::optional<ColumnSort> d_column_sort;


		/**
		 * Connect to signals from a @a FeatureCollectionFileState object.
		 */
		void
		connect_to_file_state_signals();

	};
}

#endif  // GPLATES_GUI_MANAGEFEATURECOLLECTIONSDIALOG_H
