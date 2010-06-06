/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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
 
#ifndef GPLATES_GUI_FILEIOFEEDBACK_H
#define GPLATES_GUI_FILEIOFEEDBACK_H

#include <QObject>
#include <QString>
#include <QList>
#include <QUrl>
#include <boost/function.hpp>

#include "app-logic/FeatureCollectionFileState.h"
#include "file-io/FeatureCollectionFileFormat.h"
#include "model/FeatureCollectionHandle.h"
#include "qt-widgets/SaveFileDialog.h"


namespace GPlatesAppLogic
{
	class FeatureCollectionFileIO;
}

namespace GPlatesGui
{
	class UnsavedChangesTracker;
}


namespace GPlatesQtWidgets
{
	// Forward declaration of ViewportWindow and MFCD to avoid spaghetti.
	// Yes, this is ViewportWindow, not the "View State"; we need
	// this to pop dialogs up from, and maybe some progress bars.
	class ViewportWindow;
	class ManageFeatureCollectionsDialog;
}


namespace GPlatesGui
{
	/**
	 * This GUI class is responsible for wrapping the saving and loading app-logic
	 * operations with a thin layer of GUI feedback for users - in particular, calls
	 * to the save methods of this class will prompt the user for filenames
	 * appropriately, and will provide feedback if there are serious errors while
	 * saving. It has been refactored out of the ManageFeatureCollectionsDialog.
	 * 
	 * In the future, jetpacks etc, it should also be a means for the GUI to get
	 * progress updates during saving and loading, enabling progress bars to be
	 * displayed.
	 */
	class FileIOFeedback: 
			public QObject
	{
		Q_OBJECT
		
	public:
	
		explicit
		FileIOFeedback(
				GPlatesQtWidgets::ViewportWindow &viewport_window_,
				GPlatesAppLogic::FeatureCollectionFileState &file_state_,
				GPlatesAppLogic::FeatureCollectionFileIO &feature_collection_file_io_,
				QObject *parent_ = NULL);

		virtual
		~FileIOFeedback()
		{  }


		/**
		 * Opens the specified files, handling any exceptions thrown by popping up
		 * appropriate error dialogs.
		 *
		 * See the slot open_files() for the version which pops up a file selection dialog.
		 */
		void
		open_files(
				const QStringList &filenames);

		/**
		 * As @a open_files(QStringList), but for a list of QUrl. For drag-and-drop
		 * functionality.
		 */
		void
		open_urls(
				const QList<QUrl> &urls);

		/**
		 * Reloads the file given by FileState file_reference @a file and handles any
		 * exceptions thrown by popping up appropriate error dialogs.
		 */
		void
		reload_file(
				GPlatesAppLogic::FeatureCollectionFileState::file_reference &file_it);


		/**
		 * Save a file, given by FileState file_reference @a file. If the file has not
		 * yet been named, save using a Save As dialog with @a save_file_as(); if the
		 * file does have a name, save with as little user intervention as possible using
		 * @a save_file_in_place().
		 */
		bool
		save_file_as_appropriate(
				GPlatesAppLogic::FeatureCollectionFileState::file_reference file);

		/**
		 * Save a file, given by FileState file_reference @a file. Does not prompt the
		 * user with any kind of Save As dialog, it assumes the file is to be saved
		 * with the current name and settings and the only feedback desired is progress
		 * bars and/or error dialogs.
		 *
		 * NOTE: This will likely result in a "Unable to save files of that type" error if
		 * the file has not been named yet. This could be handled better. See also
		 * GMT header problem in @a get_feature_collection_write_format().
		 */
		bool
		save_file_in_place(
				GPlatesAppLogic::FeatureCollectionFileState::file_reference file);

		/**
		 * Save a file, given by FileState file_reference @a file, and prompt the
		 * user with a Save As dialog to let them specify a new name for the loaded file.
		 * The new name will be associated with the loaded file afterwards.
		 */
		bool
		save_file_as(
				GPlatesAppLogic::FeatureCollectionFileState::file_reference file);

		/**
		 * Save a file, given by FileState file_reference @a file, and prompt the
		 * user with a Save a Copy dialog to let them specify a new name for the loaded file.
		 * The loaded file retains its original filename.
		 */
		bool
		save_file_copy(
				GPlatesAppLogic::FeatureCollectionFileState::file_reference file);


		/**
		 * Saves specified feature collection into a file described by @a file_info.
		 * Pops up simple dialogs if there are problems, and returns false.
		 *
		 * This is called by the other @a save_file_* methods above.
		 */
		bool
		save_file(
				const GPlatesFileIO::FileInfo &file_info,
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection,
				GPlatesFileIO::FeatureCollectionWriteFormat::Format feature_collection_write_format);


		/**
		 * Save all files as though the 'save in place' button was used.
		 * If @a include_unnamed_files, we'll also try to save files that don't have names yet,
		 * which will mean popping up save dialogs.
		 */
		bool
		save_all(
				bool include_unnamed_files);

	public slots:

		/**
		 * Opens an Open File dialog allowing the user to select zero or more files,
		 * then opens them.
		 */
		void
		open_files();


	private:

		/**
		 * Get format for writing to feature collection file.
		 */
		GPlatesFileIO::FeatureCollectionWriteFormat::Format
		get_feature_collection_write_format(
				const GPlatesFileIO::FileInfo& file_info);

		/**
		 * Allows calling multiple functions that throw the same types of exceptions and
		 * handles those exceptions in one place.
		 */
		void
		try_catch_file_load_with_feedback(
				boost::function<void ()> file_load_func);


		/**
		 * Quick method to get at the ViewportWindow from inside this class.
		 */
		GPlatesQtWidgets::ViewportWindow &
		viewport_window()
		{
			return *d_viewport_window_ptr;
		}

		
		/**
		 * Sneaky method to find the ManageFeatureCollectionsDialog via
		 * ViewportWindow and the Qt object tree. Means we don't have
		 * to pass yet more things in through the constructor.
		 */
		GPlatesQtWidgets::ManageFeatureCollectionsDialog &
		manage_feature_collections_dialog();


		/**
		 * Sneaky method to find the UnsavedChangesTracker via
		 * ViewportWindow and the Qt object tree. Means we don't have
		 * to pass yet more things in through the constructor.
		 */
		GPlatesGui::UnsavedChangesTracker &
		unsaved_changes_tracker();



		/**
		 * Pointer to the main window, to pop up error dialogs from etc.
		 */
		GPlatesQtWidgets::ViewportWindow *d_viewport_window_ptr;

		/**
		 * The loaded feature collection files.
		 */
		GPlatesAppLogic::FeatureCollectionFileState *d_file_state_ptr;

		/**
		 * Handles loading/unloading of feature collections.
		 */
		GPlatesAppLogic::FeatureCollectionFileIO *d_feature_collection_file_io_ptr;
		
		

		/**
		 * The save file as dialog box.
		 *
		 * FIXME: I don't believe shared_ptr is the Right Thing To Do here,
		 * since we are already parenting to Qt. - JC
		 */
		boost::shared_ptr<GPlatesQtWidgets::SaveFileDialog> d_save_file_as_dialog_ptr;

		/**
		 * The save file copy dialog box.
		 *
		 * FIXME: I don't believe shared_ptr is the Right Thing To Do here,
		 * since we are already parenting to Qt. - JC
		 */
		boost::shared_ptr<GPlatesQtWidgets::SaveFileDialog> d_save_file_copy_dialog_ptr;
		
		/**
		 * Controls whether Save File dialogs include a Compressed GPML option.
		 */
		bool d_gzip_available;

		/**
		 * Holds the path information from the last file opened using the Open File dialog.
		 * Note that file save dialogs infer their directory based on the path of the file
		 * being saved.
		 */
		QString d_open_file_path;

	};
}


#endif	// GPLATES_GUI_FILEIOFEEDBACK_H
