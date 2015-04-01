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
 
#ifndef GPLATES_GUI_UNSAVEDCHANGESTRACKER_H
#define GPLATES_GUI_UNSAVEDCHANGESTRACKER_H

#include <vector>
#include <QObject>
#include <QWidget>
#include <QStringList>

#include "app-logic/FeatureCollectionFileState.h"
#include "app-logic/FeatureCollectionFileIO.h"
#include "model/FeatureCollectionHandle.h"


namespace GPlatesGui
{
	class FileIOFeedback;
}

namespace GPlatesQtWidgets
{
	// Forward declaration of ViewportWindow to avoid spaghetti.
	// Yes, this is ViewportWindow, not the "View State"; we need
	// this to add status icons to, hook close events, etc.
	class ViewportWindow;
	// The dialog we use to notify users on close.
	class UnsavedChangesWarningDialog;
	// A temporary surrogate for some applogic type thing for saving, though it
	// is certainly nice to have this pop up when saving (especially when we
	// get progress bars).
	// Oh, we also need it so we can trigger ManageFeatureCollections::highlight_rows().
	class ManageFeatureCollectionsDialog;
}

namespace GPlatesGui
{
	/**
	 * This GUI class tracks changes to the saved/unsaved state of loaded files,
	 * and updates the GUI appropriately. It also ensures the user is warned
	 * about quitting GPlates while there are unsaved files.
	 * 
	 * Actual details of the saved/unsaved state should naturally be handled
	 * by the app-logic tier. The point of this class is to draw together all the
	 * various parts of the interface that show the user the 'unsaved' warnings.
	 */
	class UnsavedChangesTracker: 
			public QObject
	{
		Q_OBJECT
		
	public:
		explicit
		UnsavedChangesTracker(
				GPlatesQtWidgets::ViewportWindow &viewport_window_,
				GPlatesAppLogic::FeatureCollectionFileState &file_state_,
				GPlatesAppLogic::FeatureCollectionFileIO &feature_collection_file_io_,
				QObject *parent_ = NULL);

		virtual
		~UnsavedChangesTracker()
		{  }


		/**
		 * Connects buttons, adds menus, etc. This step must be done @em after
		 * ViewportWindow::setupUi() has been called, and therefore cannot
		 * be done in UnsavedChangesTracker's constructor.
		 */
		void
		init();


		/**
		 * True when there are any feature collections containing anything unsaved.
		 * 
		 * This could delegate to an app-state level thing later on.
		 */
		bool
		has_unsaved_changes();


		/**
		 * List of file names with unsaved changes, for listing in the
		 * UnsavedChangesWarningDialog.
		 */
		QStringList
		list_unsaved_filenames();


		/**
		 * Hook called when ViewportWindow is closing. Returns true if closing
		 * is ok, false if the user wants to abort.
		 */
		bool
		close_event_hook();


		/**
		 * Called when the user wants to clear the session. Returns true if clearing
		 * the existing session is ok, false if the user does not want to clear the session.
		 */
		bool
		clear_session_event_hook();

	
	public Q_SLOTS:

		/**
		 * Slot called after some changes have been made to the Model.
		 * Update the GUI in all the relevant places.
		 */
		void
		handle_model_has_changed();


	private Q_SLOTS:

		void
		handle_file_state_files_added(
				GPlatesAppLogic::FeatureCollectionFileState &file_state,
				const std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> &new_files);

		void
		handle_file_state_file_about_to_be_removed(
				GPlatesAppLogic::FeatureCollectionFileState &file_state,
				GPlatesAppLogic::FeatureCollectionFileState::file_reference file);

	private:

		/**
		 * Quick method to get at the ViewportWindow from inside this class.
		 */
		GPlatesQtWidgets::ViewportWindow &
		viewport_window()
		{
			return *d_viewport_window_ptr;
		}
		
		/**
		 * Returns the ManageFeatureCollectionsDialog via ViewportWindow.
		 */
		GPlatesQtWidgets::ManageFeatureCollectionsDialog &
		manage_feature_collections_dialog();

		/**
		 * Sneaky method to find the FileIOFeedback via
		 * ViewportWindow and the Qt object tree. Means we don't have
		 * to pass yet more things in through the constructor.
		 */
		GPlatesGui::FileIOFeedback &
		file_io_feedback();


		/**
		 * Makes the signal/slot connections to FileState so we can maintain
		 * a bunch of weakrefs to loaded files and watch them for changes.
		 */
		void
		connect_to_file_state_signals();
	

		/**
		 * Keeps track of a loaded file and its feature collection.
		 *
		 * This is done because @a FeatureCollectionFileState now tells us when a file is
		 * about to be removed so we shouldn't then iterate over its loaded files whilst
		 * in the middle of its signal because it'll list all files including the one about
		 * to be removed.
		 */
		class LoadedFile
		{
		public:
			LoadedFile(
					const GPlatesAppLogic::FeatureCollectionFileState::file_reference &file_reference) :
				d_file_reference(file_reference),
				d_callback_feature_collection(file_reference.get_file().get_feature_collection())
			{  }

			GPlatesAppLogic::FeatureCollectionFileState::file_reference d_file_reference;
			GPlatesModel::FeatureCollectionHandle::weak_ref d_callback_feature_collection;
		};
		typedef std::vector<LoadedFile> loaded_files_container_type;
	
	

		/**
		 * Pointer to the main window to update with changes.
		 */
		GPlatesQtWidgets::ViewportWindow *d_viewport_window_ptr;

		/**
		 * Pointer to the dialog we use to notify users on close.
		 * This dialog is parented to ViewportWindow so Qt takes care of the cleanup.
		 */
		GPlatesQtWidgets::UnsavedChangesWarningDialog *d_warning_dialog_ptr;

		/**
		 * The loaded feature collection files.
		 */
		GPlatesAppLogic::FeatureCollectionFileState *d_file_state_ptr;

		/**
		 * Handles loading/unloading of feature collections.
		 */
		GPlatesAppLogic::FeatureCollectionFileIO *d_feature_collection_file_io_ptr;


		/**
		 * We maintain a list of weak-refs to the currently loaded FeatureCollections.
		 * Why? We need to add a callback to each of them, so that we can be notified
		 * of updates to those FeatureCollections. Thus, our special callback-enhanced
		 * weakrefs need to persist.
		 *
		 * Can't use a std::set because I need to add the callback to the weakref after
		 * it's been copied into the set (callbacks don't get copied for reasons which
		 * are obvious if you think about it), and the STL set won't let you modify
		 * an item in the set because of course then it's a @em different thing.
		 * Sometimes I think the STL is too damn clever for its own good.
		 */
		loaded_files_container_type d_loaded_files;
	};
}


#endif	// GPLATES_GUI_UNSAVEDCHANGESTRACKER_H
