/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2009, 2010, 2011 The University of Sydney, Australia
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

#include <vector>
#include <boost/function.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QUrl>

#include "app-logic/FeatureCollectionFileState.h"

#include "file-io/FeatureCollectionFileFormat.h"
#include "file-io/FeatureCollectionFileFormatConfiguration.h"
#include "file-io/FeatureCollectionFileFormatRegistry.h"
#include "file-io/File.h"

#include "model/FeatureCollectionHandle.h"

#include "qt-widgets/OpenFileDialog.h"
#include "qt-widgets/SaveFileDialog.h"


namespace GPlatesAppLogic
{
	class ApplicationState;
	class FeatureCollectionFileIO;
}

namespace GPlatesGui
{
	class FeatureFocus;
	class UnsavedChangesTracker;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	class FilesNotLoadedWarningDialog;
	class GpgimVersionWarningDialog;
	class ManageFeatureCollectionsDialog;
	// Forward declaration of ViewportWindow and MFCD to avoid spaghetti.
	// Yes, this is ViewportWindow, not the "View State"; we need
	// this to pop dialogs up from, and maybe some progress bars.
	class ViewportWindow;
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

		/**
		 * Filename extension for project files.
		 */
		static const QString PROJECT_FILENAME_EXTENSION;


		explicit
		FileIOFeedback(
				GPlatesAppLogic::ApplicationState &app_state_,
				GPlatesPresentation::ViewState &view_state_,
				GPlatesQtWidgets::ViewportWindow &viewport_window_,
				FeatureFocus &feature_focus_,
				QObject *parent_ = NULL);

		virtual
		~FileIOFeedback()
		{  }


		/**
		 * Opens the specified project file and restores to a previously saved GPlates session,
		 * handling any exceptions thrown by popping up appropriate error dialogs.
		 *
		 * See the slot open_project() for the version which pops up a project selection dialog.
		 */
		void
		open_project(
				const QString &project_filename);

		/**
		 * Opens the specified files, handling any exceptions thrown by popping up
		 * appropriate error dialogs.
		 *
		 * See the slot open_files() for the version which pops up a file selection dialog.
		 *
		 * Each file is read using the default file configuration options for its file format
		 * as currently set at GPlatesFileIO::FeatureCollectionFileFormat::Registry.
		 */
		void
		open_files(
				const QStringList &filenames);

		/**
		 * Reloads the file given by FileState file_reference @a file and handles any
		 * exceptions thrown by popping up appropriate error dialogs.
		 */
		void
		reload_file(
				GPlatesAppLogic::FeatureCollectionFileState::file_reference file);


		/**
		 * Saves the current GPlates session state to the specified project file,
		 * handling any exceptions thrown by popping up appropriate error dialogs.
		 *
		 * Returns false if there are unsaved changes or there was an error generating the project file.
		 *
		 * See the slot save_project() for the version which pops up a project selection dialog.
		 */
		bool
		save_project(
				const QString &project_filename);


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
		 * the file has not been named yet. This could be handled better.
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
		 * Save a file, given by FileState file_reference @a file.
		 * Pops up simple dialogs if there are problems, and returns false.
		 */
		bool
		save_file(
				GPlatesAppLogic::FeatureCollectionFileState::file_reference file);


		/**
		 * Save the specified files as though the 'save in place' button was used.
		 *
		 * If @a include_unnamed_files, we'll also try to save files that don't have names yet,
		 * which will mean popping up save dialogs.
		 *
		 * If @a only_unsaved_changes is true then only files with unsaved changes will be saved.
		 */
		bool
		save_files(
				const std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> &files,
				bool include_unnamed_files,
				bool only_unsaved_changes);


		/**
		 * Save all files as though the 'save in place' button was used.
		 *
		 * If @a include_unnamed_files, we'll also try to save files that don't have names yet,
		 * which will mean popping up save dialogs.
		 *
		 * If @a only_unsaved_changes is true then only files with unsaved changes will be saved.
		 */
		bool
		save_all(
				bool include_unnamed_files,
				bool only_unsaved_changes);


		/**
		 * Creates, and saves, a file named @a filename and saves @a feature_collection to the file,
		 * handling any exceptions thrown by popping up appropriate error dialogs (and returning false).
		 *
		 * This method is useful when you want to save a feature collection that was not
		 * originally loaded from a file.
		 *
		 * NOTE: This should not be used for a file with an empty filename since it cannot be saved
		 * to the file system - use 'FeatureCollectionFileIO::create_empty_file()' for that instead.
		 */
		boost::optional<GPlatesAppLogic::FeatureCollectionFileState::file_reference>
		create_file(
				const GPlatesFileIO::File::non_null_ptr_type &file);


		/**
		 * Returns those URLs that are project files.
		 *
		 * This is useful for drag'n'drop functionality.
		 */
		QStringList
		extract_project_filenames_from_file_urls(
				const QList<QUrl> &urls);


		/**
		 * Returns those URLs that are filenames with extensions registered as feature collection file formats.
		 *
		 * This is useful for drag'n'drop functionality.
		 */
		QStringList
		extract_feature_collection_filenames_from_file_urls(
				const QList<QUrl> &urls);

	public Q_SLOTS:

		/**
		 * Opens an Open File dialog allowing the user to select zero or more files,
		 * then opens them.
		 *
		 * Each file is read using the default file configuration options for its file format
		 * as currently set at GPlatesFileIO::FeatureCollectionFileFormat::Registry.
		 */
		void
		open_files();


		/**
		 * Clears the current session.
		 *
		 * This essentially clears GPlates to the state it was at application startup.
		 *
		 * Returns true on success.
		 */
		bool
		clear_session();


		/**
		 * Opens the set of files from the user's previous session.
		 * This delegates to SessionManagement, but catches any exceptions the
		 * file-io code might throw.
		 *
		 * The default value loads the most recent session "slot" in the user's
		 * history; higher numbers dig further into the past. Attempting to
		 * load a "session slot" which does not exist does nothing - the menu
		 * should match the correct number of slots anyway.
		 *
		 * If necessary, the Unsaved Changes warning dialog will be shown, so the
		 * user can save or cancel the operation.
		 */
		void
		open_previous_session(
				int session_slot_to_load = 0);


		/**
		 * Opens an Open Project dialog allowing the user to select a project file to restore
		 * to a previously saved GPlates session.
		 */
		void
		open_project();


		/**
		 * Opens an Save Project dialog allowing the user to select a project file to save
		 * the current GPlates session state to.
		 *
		 * Returns false if there are unsaved changes or there was an error generating the project file.
		 */
		bool
		save_project();


	private:

		/**
		 * Saves the feature collection in @a file_ref to the filename in @a file_ref.
		 * Pops up simple dialogs if there are problems, and returns false.
		 *
		 * This is called by the other @a save_file_* methods above.
		 * NOTE: @a clear_unsaved_changes can be set to false when this method is used by
		 * @a save_file_copy - that is the original file has not been saved and so it still
		 * has unsaved changes.
		 */
		bool
		save_file(
				GPlatesFileIO::File::Reference &file_ref,
				bool clear_unsaved_changes = true);


		/**
		 * Allows calling multiple functions that throw the same types of exceptions and
		 * handles those exceptions in one place.
		 */
		void
		try_catch_file_or_session_load_with_feedback(
				boost::function<void ()> file_or_session_load_func,
				boost::optional<QString> filename = boost::none);


		/**
		 * Quick method to get at the ViewportWindow from inside this class.
		 */
		GPlatesQtWidgets::ViewportWindow &
		viewport_window()
		{
			return *d_viewport_window_ptr;
		}


		/**
		 * Quick method to get at the ApplicationState from inside this class.
		 */
		GPlatesAppLogic::ApplicationState &
		app_state();


		/**
		 * Quick method to get at the ViewState from inside this class.
		 */
		GPlatesPresentation::ViewState &
		view_state();

		
		/**
		 * Returns the ManageFeatureCollectionsDialog via ViewportWindow.
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
		 * ApplicationState for getting access to important file-loading stuff.
		 */
		GPlatesAppLogic::ApplicationState *d_app_state_ptr;

		/**
		 * ViewState for getting access to session management.
		 */
		GPlatesPresentation::ViewState *d_view_state_ptr;

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
		 * The registry of file formats.
		 */
		GPlatesFileIO::FeatureCollectionFileFormat::Registry *d_file_format_registry_ptr;

		/**
		 * Stores the notion of which feature has the focus.
		 */
		FeatureFocus &d_feature_focus;
		
		

		/**
		 * The save file as dialog box.
		 */
		GPlatesQtWidgets::SaveFileDialog d_save_file_as_dialog;

		/**
		 * The save file copy dialog box.
		 */
		GPlatesQtWidgets::SaveFileDialog d_save_file_copy_dialog;

		/**
		 * The save project dialog box.
		 */
		GPlatesQtWidgets::SaveFileDialog d_save_project_dialog;

		/**
		 * The open files dialog box.
		 */
		GPlatesQtWidgets::OpenFileDialog d_open_files_dialog;

		/**
		 * The open project dialog box.
		 */
		GPlatesQtWidgets::OpenFileDialog d_open_project_dialog;

		/**
		 * Pointer to the dialog we use to notify users of files that were not loaded during
		 * session/project restore.
		 */
		GPlatesQtWidgets::FilesNotLoadedWarningDialog *d_files_not_loaded_warning_dialog_ptr;

		/**
		 * Pointer to the dialog we use to notify users of files with different GPGIM versions .
		 * This dialog is parented to ViewportWindow so Qt takes care of the cleanup.
		 */
		GPlatesQtWidgets::GpgimVersionWarningDialog *d_gpgim_version_warning_dialog_ptr;
	};


	/**
	 * Used to collect the files loaded during the lifetime of a @a CollectLoadFilesScope object.
	 *
	 * This class cannot be nested since Qt meta objects features are not supported for nested classes.
	 */
	class CollectLoadedFilesScope :
			public QObject
	{
		Q_OBJECT

	public:

		explicit
		CollectLoadedFilesScope(
				GPlatesAppLogic::FeatureCollectionFileState *feature_collection_file_state);

		//! Get the files loaded during the lifetime of 'this' object.
		const std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> &
		get_loaded_files() const;

	private Q_SLOTS:

		void
		handle_file_state_files_added(
				GPlatesAppLogic::FeatureCollectionFileState &file_state,
				const std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> &new_files);

	private:

		std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> d_loaded_files;

	};
}


#endif	// GPLATES_GUI_FILEIOFEEDBACK_H
