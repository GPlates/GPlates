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
#include <QDialogButtonBox>
#include <QtGlobal>
#include <QDebug>

#include "UnsavedChangesTracker.h"

#include "file-io/FileInfo.h"
#include "global/GPlatesAssert.h"
#include "global/AssertionFailureException.h"
#include "gui/FileIOFeedback.h"
#include "gui/TrinketArea.h"
#include "qt-widgets/ViewportWindow.h"
#include "qt-widgets/UnsavedChangesWarningDialog.h"
#include "qt-widgets/ManageFeatureCollectionsDialog.h"
#include "model/FeatureCollectionHandle.h"
#include "model/WeakReferenceCallback.h"


namespace
{
	/**
	 * Callback to receive notifications of changes to feature collections.
	 */
	class UnsavedChangesCallback :
			public GPlatesModel::WeakReferenceCallback<GPlatesModel::FeatureCollectionHandle>
	{
	public:

		typedef GPlatesModel::WeakReferencePublisherModifiedEvent<GPlatesModel::FeatureCollectionHandle> modified_event_type;
		typedef GPlatesModel::WeakReferencePublisherDeactivatedEvent<GPlatesModel::FeatureCollectionHandle> deactivated_event_type;
		typedef GPlatesModel::WeakReferencePublisherReactivatedEvent<GPlatesModel::FeatureCollectionHandle> reactivated_event_type;
		typedef GPlatesModel::WeakReferencePublisherAboutToBeDestroyedEvent<GPlatesModel::FeatureCollectionHandle> about_to_be_destroyed_event_type;


		explicit
		UnsavedChangesCallback(
				GPlatesGui::UnsavedChangesTracker &tracker):
			d_tracker_ptr(&tracker)
		{  }


		void
		publisher_modified(
				const modified_event_type &)
		{
			// The situation has changed, let everyone know.
			d_tracker_ptr->handle_model_has_changed();
		}

		void
		publisher_deactivated(
				const deactivated_event_type &)
		{  }

		void
		publisher_reactivated(
				const reactivated_event_type &)
		{  }

		void
		publisher_about_to_be_destroyed(
				const about_to_be_destroyed_event_type &)
		{  }

	private:

		/**
		 * Pointer to the Unsaved Changes Tracker, so we can actually do stuff to the rest
		 * of GPlates.
		 */
		GPlatesGui::UnsavedChangesTracker *d_tracker_ptr;
	};


}




GPlatesGui::UnsavedChangesTracker::UnsavedChangesTracker(
		GPlatesQtWidgets::ViewportWindow &viewport_window_,
		GPlatesAppLogic::FeatureCollectionFileState &file_state_,
		GPlatesAppLogic::FeatureCollectionFileIO &feature_collection_file_io_,
		QObject *parent_):
	QObject(parent_),
	d_viewport_window_ptr(&viewport_window_),
	d_warning_dialog_ptr(new GPlatesQtWidgets::UnsavedChangesWarningDialog(d_viewport_window_ptr)),
	d_file_state_ptr(&file_state_),
	d_feature_collection_file_io_ptr(&feature_collection_file_io_)
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
GPlatesGui::UnsavedChangesTracker::has_unsaved_changes()
{
	// Taking the brute force approach for now, later should delegate to an applogic class which
	// could be smarter about the whole deal.
	GPlatesAppLogic::FeatureCollectionFileState::file_iterator_range it_range =
			d_file_state_ptr->get_loaded_files();
	GPlatesAppLogic::FeatureCollectionFileState::file_iterator it = it_range.begin;
	GPlatesAppLogic::FeatureCollectionFileState::file_iterator end = it_range.end;
	
	for (; it != end; ++it) {
		GPlatesModel::FeatureCollectionHandle::const_weak_ref feature_collection_ref = it->get_const_feature_collection();

		if (feature_collection_ref.is_valid() && feature_collection_ref->contains_unsaved_changes()) {
			return true;
		}
	}

	return false;
}


QStringList
GPlatesGui::UnsavedChangesTracker::list_unsaved_filenames()
{
	QStringList filenames;
	GPlatesAppLogic::FeatureCollectionFileState::file_iterator_range it_range =
			d_file_state_ptr->get_loaded_files();
	GPlatesAppLogic::FeatureCollectionFileState::file_iterator it = it_range.begin;
	GPlatesAppLogic::FeatureCollectionFileState::file_iterator end = it_range.end;
	
	for (; it != end; ++it) {
		GPlatesModel::FeatureCollectionHandle::const_weak_ref feature_collection_ref = it->get_const_feature_collection();
		const GPlatesFileIO::FileInfo &file_info = it->get_file_info();
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


bool
GPlatesGui::UnsavedChangesTracker::close_event_hook()
{
	if (has_unsaved_changes()) {
		// Exec the dialog and find which QDialogButtonBox::StandardButton was clicked.
		d_warning_dialog_ptr->set_filename_list(list_unsaved_filenames());
		switch (d_warning_dialog_ptr->exec()) {
		case QDialogButtonBox::SaveAll:
			// Save. And if this save should fail, you MUST NOT CLOSE.
			// Note also that this save must prompt the user for filenames if necessary,
			// unlike the MCFD Save All button.
			return file_io_feedback().save_all(true);

		case QDialogButtonBox::Discard:
			return true;

		default:
		case QDialogButtonBox::Abort:
			return false;

		}
	} else {
		// All saved, all good. Quit already.
		return true;
	}
}


void
GPlatesGui::UnsavedChangesTracker::handle_model_has_changed()
{
	if (has_unsaved_changes()) {
		// Build a tooltip to list the files which need saving.
		QStringList files = list_unsaved_filenames();
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
GPlatesGui::UnsavedChangesTracker::handle_end_add_feature_collections(
		GPlatesAppLogic::FeatureCollectionFileState &/*file_state*/,
		GPlatesAppLogic::FeatureCollectionFileState::file_iterator new_files_begin,
		GPlatesAppLogic::FeatureCollectionFileState::file_iterator new_files_end)
{
	GPlatesAppLogic::FeatureCollectionFileState::file_iterator new_file_it = new_files_begin;
	for ( ; new_file_it != new_files_end; ++new_file_it)
	{
		//qDebug() << "[UnsavedChanges] a new FC has appeared!";

		GPlatesFileIO::File &file = *new_file_it;
		// Insert the weak_ref into our "set" - really a vector, not that it matters much here.
		d_feature_collection_weak_refs.push_back(file.get_feature_collection());
		// Get a reference to the copied weak ref that is actually in the container.
		GPlatesModel::FeatureCollectionHandle::weak_ref &fc_weak_ref = d_feature_collection_weak_refs.back();
		// Attach callback to *that* one, because it will persist.
		fc_weak_ref.attach_callback(new UnsavedChangesCallback(*this));
	}
	
	// Since some new files have appeared, it might be prudent to set colours up appropriately,
	// etc. even though this may not *directly* affect unsaved indicators.
	handle_model_has_changed();
}


void
GPlatesGui::UnsavedChangesTracker::handle_begin_remove_feature_collection(
		GPlatesAppLogic::FeatureCollectionFileState &/*file_state*/,
		GPlatesAppLogic::FeatureCollectionFileState::file_iterator unload_file_it)
{
	//qDebug() << "[UnsavedChanges] a FC is disappearing!";

	// Here's the file that's leaving:
	GPlatesFileIO::File &file = *unload_file_it;
	// Find it in our cache of weakrefs. weakref will be equal if they point to the same FC.
	feature_collection_weak_ref_iterator rm_it = std::find(
			d_feature_collection_weak_refs.begin(), d_feature_collection_weak_refs.end(),
			file.get_feature_collection());
	// Remove it - we no longer care about updates for this one.
	d_feature_collection_weak_refs.erase(rm_it);
}

void
GPlatesGui::UnsavedChangesTracker::handle_end_remove_feature_collection(
		GPlatesAppLogic::FeatureCollectionFileState &/*file_state*/)
{
	//qDebug() << "[UnsavedChanges] a FC has disappeared!";

	// Since that file might have been the sole unsaved file, and all remaining files might
	// be clean, ensure that all saved/unsaved state is reported accurately.
	handle_model_has_changed();
}


void
GPlatesGui::UnsavedChangesTracker::handle_begin_reset_feature_collection(
		GPlatesAppLogic::FeatureCollectionFileState &/*file_state*/,
		GPlatesAppLogic::FeatureCollectionFileState::file_iterator reset_file_it)
{
	//qDebug() << "[UnsavedChanges] a FC is about to be reset!";

	// Here's the file that's leaving:
	GPlatesFileIO::File &file = *reset_file_it;
	// Find it in our cache of weakrefs. weakref will be equal if they point to the same FC.
	feature_collection_weak_ref_iterator rm_it = std::find(
			d_feature_collection_weak_refs.begin(), d_feature_collection_weak_refs.end(),
			file.get_feature_collection());
	// Remove it - we no longer care about updates for this one.
	d_feature_collection_weak_refs.erase(rm_it);
}


void
GPlatesGui::UnsavedChangesTracker::handle_end_reset_feature_collection(
		GPlatesAppLogic::FeatureCollectionFileState &/*file_state*/,
		GPlatesAppLogic::FeatureCollectionFileState::file_iterator reset_file_it)
{
	//qDebug() << "[UnsavedChanges] a FC has been reset!";

	GPlatesFileIO::File &file = *reset_file_it;
	// Insert the weak_ref into our "set" - really a vector, not that it matters much here.
	d_feature_collection_weak_refs.push_back(file.get_feature_collection());
	// Get a reference to the copied weak ref that is actually in the container.
	GPlatesModel::FeatureCollectionHandle::weak_ref &fc_weak_ref = d_feature_collection_weak_refs.back();
	// Attach callback to *that* one, because it will persist.
	fc_weak_ref.attach_callback(new UnsavedChangesCallback(*this));

	// Since that file might have been the sole unsaved file, and all remaining files might
	// be clean, ensure that all saved/unsaved state is reported accurately.
	handle_model_has_changed();
}



GPlatesQtWidgets::ManageFeatureCollectionsDialog &
GPlatesGui::UnsavedChangesTracker::manage_feature_collections_dialog()
{
	// Obtain a pointer to the dialog once, via the ViewportWindow and Qt magic.
	static GPlatesQtWidgets::ManageFeatureCollectionsDialog *dialog_ptr = 
			viewport_window().findChild<GPlatesQtWidgets::ManageFeatureCollectionsDialog *>(
					"ManageFeatureCollectionsDialog");
	// The dialog not existing is a serious error.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			dialog_ptr != NULL,
			GPLATES_ASSERTION_SOURCE);
	return *dialog_ptr;
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
			SIGNAL(end_add_feature_collections(
					GPlatesAppLogic::FeatureCollectionFileState &,
					GPlatesAppLogic::FeatureCollectionFileState::file_iterator,
					GPlatesAppLogic::FeatureCollectionFileState::file_iterator)),
			this,
			SLOT(handle_end_add_feature_collections(
					GPlatesAppLogic::FeatureCollectionFileState &,
					GPlatesAppLogic::FeatureCollectionFileState::file_iterator,
					GPlatesAppLogic::FeatureCollectionFileState::file_iterator)));

	QObject::connect(
			d_file_state_ptr,
			SIGNAL(begin_remove_feature_collection(
					GPlatesAppLogic::FeatureCollectionFileState &,
					GPlatesAppLogic::FeatureCollectionFileState::file_iterator)),
			this,
			SLOT(handle_begin_remove_feature_collection(
					GPlatesAppLogic::FeatureCollectionFileState &,
					GPlatesAppLogic::FeatureCollectionFileState::file_iterator)));

	QObject::connect(
			d_file_state_ptr,
			SIGNAL(end_remove_feature_collection(
					GPlatesAppLogic::FeatureCollectionFileState &)),
			this,
			SLOT(handle_end_remove_feature_collection(
					GPlatesAppLogic::FeatureCollectionFileState &)));

	QObject::connect(
			d_file_state_ptr,
			SIGNAL(begin_reset_feature_collection(
					GPlatesAppLogic::FeatureCollectionFileState &,
					GPlatesAppLogic::FeatureCollectionFileState::file_iterator)),
			this,
			SLOT(handle_begin_reset_feature_collection(
					GPlatesAppLogic::FeatureCollectionFileState &,
					GPlatesAppLogic::FeatureCollectionFileState::file_iterator)));

	QObject::connect(
			d_file_state_ptr,
			SIGNAL(end_reset_feature_collection(
					GPlatesAppLogic::FeatureCollectionFileState &,
					GPlatesAppLogic::FeatureCollectionFileState::file_iterator)),
			this,
			SLOT(handle_end_reset_feature_collection(
					GPlatesAppLogic::FeatureCollectionFileState &,
					GPlatesAppLogic::FeatureCollectionFileState::file_iterator)));

}



