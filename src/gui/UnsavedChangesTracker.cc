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

#include <algorithm>
#include <boost/foreach.hpp>
#include <QDialogButtonBox>
#include <QtGlobal>
#include <QDebug>

#include "UnsavedChangesTracker.h"

#include "Dialogs.h"
#include "FileIOFeedback.h"
#include "TrinketArea.h"

#include "file-io/File.h"
#include "file-io/FileInfo.h"

#include "global/GPlatesAssert.h"
#include "global/AssertionFailureException.h"

#include "model/FeatureCollectionHandle.h"
#include "model/WeakReferenceCallback.h"

#include "presentation/SessionManagement.h"

#include "qt-widgets/ViewportWindow.h"
#include "qt-widgets/UnsavedChangesWarningDialog.h"
#include "qt-widgets/ManageFeatureCollectionsDialog.h"

namespace
{
	/**
	 * Callback to receive notifications of changes to feature collections.
	 */
	class UnsavedChangesCallback :
			public GPlatesModel::WeakReferenceCallback<GPlatesModel::FeatureCollectionHandle>
	{
	public:
		explicit
		UnsavedChangesCallback(
				GPlatesGui::UnsavedChangesTracker &tracker):
			d_tracker_ptr(&tracker)
		{  }


		void
		publisher_modified(
				const weak_reference_type &,
				const modified_event_type &)
		{
			// The situation has changed, let everyone know.
			d_tracker_ptr->handle_model_has_changed();
		}

		void
		publisher_deactivated(
				const weak_reference_type &,
				const deactivated_event_type &)
		{  }

		void
		publisher_reactivated(
				const weak_reference_type &,
				const reactivated_event_type &)
		{  }

		void
		publisher_about_to_be_destroyed(
				const weak_reference_type &,
				const about_to_be_destroyed_event_type &)
		{  }

	private:

		/**
		 * Pointer to the Unsaved Changes Tracker, so we can actually do stuff to the rest
		 * of GPlates.
		 */
		GPlatesGui::UnsavedChangesTracker *d_tracker_ptr;
	};


	/**
	 * Returns true if user decides it's OK to discard the unsaved changes.
	 */
	GPlatesGui::UnsavedChangesTracker::UnsavedChangesResult
	get_unsaved_changes_result(
			GPlatesQtWidgets::UnsavedChangesWarningDialog *warning_dialog_ptr,
			GPlatesQtWidgets::UnsavedChangesWarningDialog::ActionRequested action_requested,
			QStringList unsaved_feature_collection_filenames,
			bool has_unsaved_project_changes)
	{
		// See if have no unsaved changes.
		if (unsaved_feature_collection_filenames.isEmpty() &&
			!has_unsaved_project_changes)
		{
			// All saved, all good.
			return GPlatesGui::UnsavedChangesTracker::NO_UNSAVED_CHANGES;
		}

		// We have unsaved changes, ask user whether to discard them or not.
		//
		// Exec the dialog and find which QDialogButtonBox::StandardButton was clicked.
		warning_dialog_ptr->set_action_requested(
				action_requested,
				unsaved_feature_collection_filenames,
				has_unsaved_project_changes);

		switch (warning_dialog_ptr->exec())
		{
		case QDialogButtonBox::Discard:
			// The unsaved changes will be discarded.
			return GPlatesGui::UnsavedChangesTracker::DISCARD_UNSAVED_CHANGES;

		default:
		case QDialogButtonBox::Abort:
			// Do not discard the unsaved changes.
			return GPlatesGui::UnsavedChangesTracker::DONT_DISCARD_UNSAVED_CHANGES;
		}
	}
}




GPlatesGui::UnsavedChangesTracker::UnsavedChangesTracker(
		GPlatesQtWidgets::ViewportWindow &viewport_window_,
		GPlatesAppLogic::FeatureCollectionFileState &file_state_,
		GPlatesAppLogic::FeatureCollectionFileIO &feature_collection_file_io_,
		GPlatesPresentation::SessionManagement &session_management_,
		QObject *parent_):
	QObject(parent_),
	d_viewport_window_ptr(&viewport_window_),
	d_warning_dialog_ptr(new GPlatesQtWidgets::UnsavedChangesWarningDialog(d_viewport_window_ptr)),
	d_file_state_ptr(&file_state_),
	d_feature_collection_file_io_ptr(&feature_collection_file_io_),
	d_session_management_ptr(&session_management_)
{
	setObjectName("UnsavedChangesTracker");
	connect_to_file_state_signals();
}


void
GPlatesGui::UnsavedChangesTracker::init()
{
	// Set up UI connections and things here which don't exist until after
	// ViewportWindow's setupUi() has been called.
}


bool
GPlatesGui::UnsavedChangesTracker::has_unsaved_feature_collections()
{
	// Taking the brute force approach for now, later should delegate to an applogic class which
	// could be smarter about the whole deal.

	// Iterate over our internal sequence.
	BOOST_FOREACH(const LoadedFile &loaded_file, d_loaded_files)
	{
		GPlatesModel::FeatureCollectionHandle::const_weak_ref feature_collection_ref =
				loaded_file.d_file_reference.get_file().get_feature_collection();

		if (feature_collection_ref.is_valid() && feature_collection_ref->contains_unsaved_changes()) {
			return true;
		}
	}

	return false;
}


QStringList
GPlatesGui::UnsavedChangesTracker::list_unsaved_feature_collection_filenames()
{
	QStringList filenames;

	// Iterate over our internal sequence.
	BOOST_FOREACH(const LoadedFile &loaded_file, d_loaded_files)
	{
		GPlatesModel::FeatureCollectionHandle::const_weak_ref feature_collection_ref =
				loaded_file.d_file_reference.get_file().get_feature_collection();
		const GPlatesFileIO::FileInfo &file_info =
				loaded_file.d_file_reference.get_file().get_file_info();
		const QFileInfo &qfileinfo = file_info.get_qfileinfo();

		if (feature_collection_ref.is_valid() && feature_collection_ref->contains_unsaved_changes()) {
			if ( ! qfileinfo.fileName().isEmpty()) {
				filenames << qfileinfo.fileName();
			} else {
				filenames << "New Feature Collection";
			}
		}
	}

	return filenames;
}


GPlatesGui::UnsavedChangesTracker::UnsavedChangesResult
GPlatesGui::UnsavedChangesTracker::close_event_hook()
{
	// If any unsaved changes then they will either be discarded when GPlates quits, or we won't quit GPlates.
	return get_unsaved_changes_result(
			d_warning_dialog_ptr,
			GPlatesQtWidgets::UnsavedChangesWarningDialog::CLOSE_GPLATES,
			list_unsaved_feature_collection_filenames(),
			d_session_management_ptr->is_current_session_a_project_with_unsaved_changes()/*has_unsaved_project_changes*/);
}


GPlatesGui::UnsavedChangesTracker::UnsavedChangesResult
GPlatesGui::UnsavedChangesTracker::clear_session_event_hook()
{
	// If any unsaved changes then they will either be discarded when the session is cleared, or
	// we won't clear the session.
	return get_unsaved_changes_result(
			d_warning_dialog_ptr,
			GPlatesQtWidgets::UnsavedChangesWarningDialog::CLEAR_SESSION,
			list_unsaved_feature_collection_filenames(),
			d_session_management_ptr->is_current_session_a_project_with_unsaved_changes()/*has_unsaved_project_changes*/);
}


GPlatesGui::UnsavedChangesTracker::UnsavedChangesResult
GPlatesGui::UnsavedChangesTracker::load_previous_session_event_hook()
{
	// If any unsaved changes then they will either be discarded (and then a previous session loaded), or
	// we won't load a previous session.
	return get_unsaved_changes_result(
			d_warning_dialog_ptr,
			GPlatesQtWidgets::UnsavedChangesWarningDialog::LOAD_PREVIOUS_SESSION,
			list_unsaved_feature_collection_filenames(),
			d_session_management_ptr->is_current_session_a_project_with_unsaved_changes()/*has_unsaved_project_changes*/);
}


GPlatesGui::UnsavedChangesTracker::UnsavedChangesResult
GPlatesGui::UnsavedChangesTracker::load_project_event_hook()
{
	// If any unsaved changes then they will either be discarded (and then a project loaded), or
	// we won't load a project.
	return get_unsaved_changes_result(
			d_warning_dialog_ptr,
			GPlatesQtWidgets::UnsavedChangesWarningDialog::LOAD_PROJECT,
			list_unsaved_feature_collection_filenames(),
			d_session_management_ptr->is_current_session_a_project_with_unsaved_changes()/*has_unsaved_project_changes*/);
}


void
GPlatesGui::UnsavedChangesTracker::handle_model_has_changed()
{
	if (has_unsaved_feature_collections()) {
		// Build a tooltip to list the files which need saving.
		QStringList files = list_unsaved_feature_collection_filenames();
		QString tip;
		if (files.size() < 10) {
			tip = tr("The following files have unsaved changes:-\n");
			tip.append(files.join("\n"));
		} else {
			tip = tr("There are %1 files with unsaved changes.").arg(files.size());
		}

		// Ensure the Unsaved Changes Trinket is visible if it wasn't before, and
		// update the tooltip.
		viewport_window().trinket_area().unsaved_changes_trinket().setToolTip(tip);
		viewport_window().trinket_area().unsaved_changes_trinket().setVisible(true);
	} else {
		// No unsaved changes at all, ensure our trinket is invisible.
		viewport_window().trinket_area().unsaved_changes_trinket().setToolTip(tr("No unsaved changes."));
		viewport_window().trinket_area().unsaved_changes_trinket().setVisible(false);
	}
	
	// Update the highlighting on the rows of the Manage Feature Collections Dialog
	// (don't update() as that will cause the table to be destroyed and rebuilt).
	manage_feature_collections_dialog().highlight_unsaved_changes();
}



void
GPlatesGui::UnsavedChangesTracker::handle_file_state_files_added(
		GPlatesAppLogic::FeatureCollectionFileState &/*file_state*/,
		const std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> &new_files)
{
	BOOST_FOREACH(
			const GPlatesAppLogic::FeatureCollectionFileState::file_reference &file_ref,
			new_files)
	{
		//qDebug() << "[UnsavedChanges] a new file containing a FC has appeared!";

		// Insert the weak_ref into our "set" - really a vector, not that it matters much here.
		// NOTE: We can simply append without inserting according to the new file indices because
		// the file state guarantees the file indices are sequential and contiguous.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				file_ref.get_file_index() == d_loaded_files.size(),
				GPLATES_ASSERTION_SOURCE);
		d_loaded_files.push_back(LoadedFile(file_ref));
		// Get a reference to the copied weak ref that is actually in the container.
		GPlatesModel::FeatureCollectionHandle::weak_ref &fc_weak_ref =
				d_loaded_files.back().d_callback_feature_collection;
		// Attach callback to *that* one, because it will persist.
		fc_weak_ref.attach_callback(new UnsavedChangesCallback(*this));
	}
	
	// Since some new files have appeared, it might be prudent to set colours up appropriately,
	// etc. even though this may not *directly* affect unsaved indicators.
	handle_model_has_changed();
}


void
GPlatesGui::UnsavedChangesTracker::handle_file_state_file_about_to_be_removed(
		GPlatesAppLogic::FeatureCollectionFileState &/*file_state*/,
		GPlatesAppLogic::FeatureCollectionFileState::file_reference file_about_to_be_removed)
{
	//qDebug() << "[UnsavedChanges] a file containing a FC is about to disappear!";

	// Here's the file index of the file that's leaving:
	const unsigned int file_index = file_about_to_be_removed.get_file_index();
	// Remove it - we no longer care about updates for this one.
	// We can simply erase the file entry in our internal sequence at index 'file_index'.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			file_index < d_loaded_files.size(),
			GPLATES_ASSERTION_SOURCE);
	d_loaded_files.erase(
			d_loaded_files.begin() + file_index);

	// Since that file might have been the sole unsaved file, and all remaining files might
	// be clean, ensure that all saved/unsaved state is reported accurately.
	handle_model_has_changed();
}


GPlatesQtWidgets::ManageFeatureCollectionsDialog &
GPlatesGui::UnsavedChangesTracker::manage_feature_collections_dialog()
{
	return d_viewport_window_ptr->dialogs().manage_feature_collections_dialog();
}


GPlatesGui::FileIOFeedback &
GPlatesGui::UnsavedChangesTracker::file_io_feedback()
{
	// Obtain a pointer to the thing once, via the ViewportWindow and Qt magic.
	static GPlatesGui::FileIOFeedback *feedback_ptr = 
			viewport_window().findChild<GPlatesGui::FileIOFeedback *>(
					"FileIOFeedback");
	// The thing not existing is a serious error.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			feedback_ptr != NULL,
			GPLATES_ASSERTION_SOURCE);
	return *feedback_ptr;
}



void
GPlatesGui::UnsavedChangesTracker::connect_to_file_state_signals()
{
	QObject::connect(
			d_file_state_ptr,
			SIGNAL(file_state_files_added(
					GPlatesAppLogic::FeatureCollectionFileState &,
					const std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> &)),
			this,
			SLOT(handle_file_state_files_added(
					GPlatesAppLogic::FeatureCollectionFileState &,
					const std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> &)));

	QObject::connect(
			d_file_state_ptr,
			SIGNAL(file_state_file_about_to_be_removed(
					GPlatesAppLogic::FeatureCollectionFileState &,
					GPlatesAppLogic::FeatureCollectionFileState::file_reference)),
			this,
			SLOT(handle_file_state_file_about_to_be_removed(
					GPlatesAppLogic::FeatureCollectionFileState &,
					GPlatesAppLogic::FeatureCollectionFileState::file_reference)));
}



