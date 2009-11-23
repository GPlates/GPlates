/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2006, 2007, 2008 The University of Sydney, Australia
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

#include <boost/function.hpp>
#include <QObject>
#include <QString>

#include "ManageFeatureCollectionsDialogUi.h"
#include "SaveFileDialog.h"
#include "app-logic/FeatureCollectionFileState.h"
#include "file-io/FeatureCollectionFileFormat.h"


namespace GPlatesAppLogic
{
	class FeatureCollectionFileIO;
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
				QWidget *parent_ = NULL);
		
		/**
		 * Updates the contents of the table to match the current FeatureCollectionFileState.
		 * This clears the table completely, and adds a new row for each loaded file.
		 */
		void
		update();
		
		
		/**
		 * Initiates editing of the file configuration. 
		 */
		void
		edit_configuration(
				ManageFeatureCollectionsActionWidget *action_widget_ptr);

		/**
		 * Opens the specified files and keeps track of them internally.
		 */
		void
		open_files(
				const QStringList &filenames);

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
		 * removed from the active reconstructable files list, effectively
		 * 'showing' or 'hiding' that feature data.
		 */
		void
		set_reconstructable_state_for_file(
				ManageFeatureCollectionsStateWidget *state_widget_ptr,
				bool activate);

		/**
		 * Causes the file referenced by the state widget to be added or
		 * removed from the active reconstruction files list, meaning
		 * GPlates will use or ignore any reconstruction tree present.
		 */
		void
		set_reconstruction_state_for_file(
				ManageFeatureCollectionsStateWidget *state_widget_ptr,
				bool activate);
	
	public slots:
	
		void
		open_files();
		
		void
		save_all()
		{  }	// FIXME: This should invoke a call on AppState or ViewportWindow etc.

	private slots:
		// NOTE: all signals/slots should use namespace scope for all arguments
		//       otherwise differences between signals and slots will cause Qt
		//       to not be able to connect them at runtime.

		void
		end_add_feature_collections(
				GPlatesAppLogic::FeatureCollectionFileState &file_state,
				GPlatesAppLogic::FeatureCollectionFileState::file_iterator new_files_begin,
				GPlatesAppLogic::FeatureCollectionFileState::file_iterator new_files_end);

		void
		begin_remove_feature_collection(
				GPlatesAppLogic::FeatureCollectionFileState &file_state,
				GPlatesAppLogic::FeatureCollectionFileState::file_iterator file);

		void
		reconstructable_file_activation(
				GPlatesAppLogic::FeatureCollectionFileState &file_state,
				GPlatesAppLogic::FeatureCollectionFileState::file_iterator file,
				bool activation);

		void
		reconstruction_file_activation(
				GPlatesAppLogic::FeatureCollectionFileState &file_state,
				GPlatesAppLogic::FeatureCollectionFileState::file_iterator file,
				bool activation);

		void
		workflow_file_activation(
				GPlatesAppLogic::FeatureCollectionFileState &file_state,
				GPlatesAppLogic::FeatureCollectionFileState::file_iterator file,
				const GPlatesAppLogic::FeatureCollectionFileState::workflow_tag_type &workflow_tag,
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
				GPlatesAppLogic::FeatureCollectionFileState::file_iterator file_it);
		
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
				GPlatesAppLogic::FeatureCollectionFileState::file_iterator file_it);

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
		 * Get format for writing to feature collection file.
		 */
		GPlatesFileIO::FeatureCollectionWriteFormat::Format
		get_feature_collection_write_format(
				const GPlatesFileIO::FileInfo& file_info);



		/**
		 * Reloads specified file and handles any exceptions thrown while doing so.
		 */
		void
		reload_file(
				GPlatesAppLogic::FeatureCollectionFileState::file_iterator &file_it);

		/**
		 * Saves specified feature collection into a file described by @a file_info.
		 */
		void
		save_file(
				const GPlatesFileIO::FileInfo &file_info,
				const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection,
				GPlatesFileIO::FeatureCollectionWriteFormat::Format feature_collection_write_format);

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
		 * Holds the path information from the last file opened using the Open File dialog.
		 * Note that file save dialogs infer their directory based on the path of the file
		 * being saved.
		 */
		QString d_open_file_path;
		
		/**
		 * Displays save file as dialog box.
		 */
		boost::shared_ptr<SaveFileDialog> d_save_file_as_dialog_ptr;

		/**
		 * Displays save file copy dialog box.
		 */
		boost::shared_ptr<SaveFileDialog> d_save_file_copy_dialog_ptr;
		
		/**
		 * Controls whether Save File dialogs include a Compressed GPML option.
		 */
		bool d_gzip_available;

		/**
		 * Connect to signals from a @a FeatureCollectionFileState object.
		 */
		void
		connect_to_file_state_signals();

		/**
		 * Allows calling multiple functions that throw the same types of exceptions and
		 * handles those exceptions in one place.
		 */
		void
		try_catch_file_load(
				boost::function<void ()> file_load_func);
	};
}

#endif  // GPLATES_GUI_MANAGEFEATURECOLLECTIONSDIALOG_H
