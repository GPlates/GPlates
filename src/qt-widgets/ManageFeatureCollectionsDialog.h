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

#ifdef HAVE_PYTHON
// We need to include this _before_ any Qt headers get included because
// of a moc preprocessing problems with a feature called 'slots' in the
// python header file object.h
# include <boost/python.hpp>
#endif

#include <QObject>
#include <QPointer>
#include <QString>

#include "ManageFeatureCollectionsDialogUi.h"
#include "SaveFileDialog.h"
#include "app-logic/FeatureCollectionFileState.h"
#include "file-io/FeatureCollectionFileFormat.h"


namespace GPlatesAppLogic
{
	class FeatureCollectionFileIO;
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
	class ManageFeatureCollectionsStateWidget;
	

	class ManageFeatureCollectionsDialog:
			public QDialog, 
			protected Ui_ManageFeatureCollectionsDialog
	{
		Q_OBJECT
		
	public:
	
		explicit
		ManageFeatureCollectionsDialog(
				GPlatesAppLogic::FeatureCollectionFileState &file_state,
				GPlatesAppLogic::FeatureCollectionFileIO &feature_collection_file_io,
				GPlatesGui::FileIOFeedback &gui_file_io_feedback,
				GPlatesPresentation::ViewState& d_view_state,
				QWidget *parent_ = NULL);
		

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

		/**
		 * Causes the file referenced by the state widget to be added or
		 * removed from the active files list, effectively
		 * 'showing' or 'hiding' that feature data.
		 */
		void
		set_state_for_file(
				ManageFeatureCollectionsStateWidget *state_widget_ptr,
				bool activate);
	
	public slots:
	
		/**
		 * Updates the contents of the table to match the current FeatureCollectionFileState.
		 * This clears the table completely, and adds a new row for each loaded file.
		 * Doing this makes the scroll pane jump to the top of the table, which may not be
		 * desirable - so avoid doing this unless the set of loaded files has @em actually changed.
		 *
		 * It also calls @a highlight_unsaved_changes().
		 */
		void
		update();


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
		save_all_named();

	private slots:
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
		handle_file_state_file_activation_changed(
				GPlatesAppLogic::FeatureCollectionFileState &file_state,
				GPlatesAppLogic::FeatureCollectionFileState::file_reference file,
				bool activation);

	protected:
	
		/**
		 * Deletes items and completely removes all rows from the table.
		 */
		void
		clear_rows();

		/**
		 * Adds a row to the table, creating a ManageFeatureCollectionsActionWidget to
		 * store the FileInfo and the buttons used to interact with the file.
		 */
		void
		add_row(
				GPlatesAppLogic::FeatureCollectionFileState::file_reference file_it);
		
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
		 * Goes through each loaded file and saves-in-place each one that needs saving.
		 * Will prompt for filenames on unnamed collections if @a include_unnamed_files.
		 */
		void
		save_all(
				bool include_unnamed_files);

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

		GPlatesPresentation::ViewState& d_view_state;

		/**
		 * Connect to signals from a @a FeatureCollectionFileState object.
		 */
		void
		connect_to_file_state_signals();

	};
}

#endif  // GPLATES_GUI_MANAGEFEATURECOLLECTIONSDIALOG_H
