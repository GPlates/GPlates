/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010, 2011 The University of Sydney, Australia
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
#include <boost/noncopyable.hpp>
#include <QDebug>
#include <QFileInfo>
#include <QFile>
#include <QList>

#include "SessionManagement.h"

#include "app-logic/UserPreferences.h"
#include "app-logic/ApplicationState.h"
#include "app-logic/FeatureCollectionFileState.h"
#include "app-logic/FeatureCollectionFileIO.h"

#include "file-io/FileInfo.h"

#include "scribe/ScribeExceptions.h"


GPlatesPresentation::SessionManagement::SessionManagement(
		GPlatesAppLogic::ApplicationState &app_state) :
	QObject(NULL),
	d_app_state_ptr(&app_state)
{  }


void
GPlatesPresentation::SessionManagement::initialise()
{
// For now we just clear the session state by unloading all files and layers.
// When session save/restore gets more involved we can bring this back.
#if 0
	// Saving the current session may generate a serialization error...
	try
	{
		// Create a Session object that matches the current GPlates session.
		// Since we've just started GPlates this represents the clear session state (no files loaded).
		d_clear_session_state = InternalSession::save_session();
	}
	catch (const GPlatesScribe::Exceptions::BaseException &scribe_exception)
	{
		// Log the detailed error message.
		qWarning() << "Unable to generate the clear session state at application startup: " << scribe_exception;
	}
#endif
}


void
GPlatesPresentation::SessionManagement::clear_session()
{
	// Block any signaled calls to 'ApplicationState::reconstruct' until we exit this scope.
	// Blocking calls to 'reconstruct' during this scope prevents multiple calls caused by
	// layer signals, etc, which is unnecessary if we're going to call 'reconstruct' anyway.
	GPlatesAppLogic::ApplicationState::ScopedReconstructGuard scoped_reconstruct_guard(
			*d_app_state_ptr, true/*reconstruct_on_scope_exit*/);

	// Put all layer removals in a single remove layers group.
	// We also start this before unloading files since that can trigger removal of auto-created layers.
	GPlatesAppLogic::ReconstructGraph::AddOrRemoveLayersGroup remove_layers_group(
			d_app_state_ptr->get_reconstruct_graph());
	remove_layers_group.begin_add_or_remove_layers();

	// Unloading all files should remove all auto-created layers but any user-created layers
	// will not be removed so we'll have to remove them explicitly - if we don't then the number
	// of user-created layers increases continuously as we switch between sessions.
	unload_all_files();

	// Copy remaining user-created layers into an array before removing them to avoid iteration issues.
	const std::vector<GPlatesAppLogic::Layer> user_created_layers(
			d_app_state_ptr->get_reconstruct_graph().begin(),
			d_app_state_ptr->get_reconstruct_graph().end());
	BOOST_FOREACH(GPlatesAppLogic::Layer layer, user_created_layers)
	{
		d_app_state_ptr->get_reconstruct_graph().remove_layer(layer);
	}

	// End the remove layers group.
	remove_layers_group.end_add_or_remove_layers();

#if 0
	// To ensure that everything is restored to the way it was at application startup we also
	// load the default clear session state using the Scribe.
	if (d_clear_session_state)
	{
		// Note that we don't surround this with a try catch block because it should always
		// succeed and if it doesn't then it's a program error (as opposed to a corrupt archive
		// stream or archive version issue).
		d_clear_session_state.get()->restore_session();
	}
#endif
}


QStringList
GPlatesPresentation::SessionManagement::load_session(
		const Session::non_null_ptr_to_const_type &session_to_load)
{
	// Block any signaled calls to 'ApplicationState::reconstruct' until we exit this scope.
	// This prevents multiple calls to 'reconstruct' caused by layer signals, etc.
	GPlatesAppLogic::ApplicationState::ScopedReconstructGuard scoped_reconstruct_guard(
			*d_app_state_ptr, true/*reconstruct_on_scope_exit*/);

	//
	// Note that we reload the session even if it contains the same set of files.
	// This is because the user may have changed layer settings or connections but then
	// wants to essentially undo that by reloading the original settings/connections in the
	// saved session state.
	//

	//
	// User is attempting to load a new session. Should we replace the old one?
	// For now, the answer is yes - always unload the original files first and clear the session state.
	// However, before we do that, save the current session.
	//
	// UPDATE: Now restoring a session essentially overwrites the previous session anyway.
	// It's conceivable that two sessions could be merged but this would be difficult especially
	// if both sessions had some files in common but the layers connected to them (from different
	// sessions) had different state resulting in a conflict (unless the files were duplicated to
	// effectively make both sessions have independent files and layers).
	//

	// Save the current session.
	// We get boost::none if there was a Scribe exception whilst saving the session.
	const boost::optional<InternalSession::non_null_ptr_to_const_type> current_session = save_session();

	// Clear the current session so there's no files loaded and
	// no auto-created or user-created layers left.
	clear_session();

	QStringList feature_collection_files_not_loaded;

	// Load the requested session.
	try
	{
		feature_collection_files_not_loaded = session_to_load->restore_session();
	}
	catch (const GPlatesScribe::Exceptions::BaseException &/*scribe_exception*/)
	{
		// We failed to restore the session...

		// Clear the session since it could be partially restored.
		clear_session();

		// If the current session was successfully saved before we tried to load a session then
		// attempt to restore that session, otherwise clear the session and re-throw.
		// This reverts everything back to the way it was.
		if (current_session)
		{
			try
			{
				current_session.get()->restore_session();
			}
			catch (const GPlatesScribe::Exceptions::BaseException &/*scribe_exception*/)
			{
				// Clear the session since it could be partially restored.
				clear_session();

				// Re-throw the exception so the error can get reported in the GUI.
				throw;
			}
		}

		// Re-throw the exception so the error can get reported in the GUI.
		// We do this even if we managed to restore things back to the way they were.
		throw;
	}

	return feature_collection_files_not_loaded;
}


QList<GPlatesPresentation::InternalSession::non_null_ptr_to_const_type>
GPlatesPresentation::SessionManagement::get_recent_session_list()
{
	QList<InternalSession::non_null_ptr_to_const_type> session_list;

	// Sessions are stored as an "array" in the Qt style, so first read the 'size' of that array.
	GPlatesAppLogic::UserPreferences &prefs = d_app_state_ptr->get_user_preferences();
	const int deprecated_sessions_size = prefs.get_value("session/recent/size").toInt();
	const int sessions_size = prefs.get_value("session/recent/sessions/size").toInt();
 	if (deprecated_sessions_size + sessions_size <= 0)
	{
		// Nothing to load.
		return session_list;
	}
	
	const int sessions_max_size = prefs.get_value("session/recent/max_size").toInt();

	// Pull the recent sessions out of the user preferences storage.
	// They are 1-indexed.
	for (int i = 1; i <= sessions_size; ++i)
	{
		// Session number i is stored in a 'directory' named i.
		QString session_path = QString("session/recent/sessions/%1").arg(i);
		// The "loaded_files" key exists for all deprecated session versions and
		// the "serialized_session_state" key exists for all non-deprecated versions
		// so we use these to test for the existence of a session.
 		if (prefs.exists(session_path + "/serialized_session_state") ||
 			prefs.exists(session_path + "/loaded_files"))
		{
			const GPlatesAppLogic::UserPreferences::KeyValueMap session_state =
					prefs.get_keyvalues_as_map(session_path);

			// Deprecated sessions don't have the "serialized_session_state" key.
			const bool is_deprecated_session = !prefs.exists(session_path + "/serialized_session_state");

			// Note that we add the current session to the list even if GPlates cannot restore it
			// (because, for example, it has been created by a future incompatible version of GPlates)
			// in which case it will just fail to load if the user selects it.
			// This is because all versions of GPlates share the same logical session list and anytime
			// one version of GPlates saves a session then it should appear in the list regardless of
			// whether other versions of GPlates can read it or not.
			session_list << InternalSession::create_restore_session(session_state, is_deprecated_session);
		}
	}

	// Now go through the deprecated sessions list and merge any deprecated sessions that are more recent.
	// They are 1-indexed.
	for (int i = 1; i <= deprecated_sessions_size; ++i)
	{
		// Session number i is stored in a 'directory' named i.
		QString deprecated_session_path = QString("session/recent/%1").arg(i);
		// The "loaded_files" key exists for all deprecated session versions so it's safe to
		// use it to test for the existence of a session.
 		if (!prefs.exists(deprecated_session_path + "/loaded_files"))
		{
			continue;
		}

		// Note that if the deprecated session gets inserted into the session list but can no longer
		// be restored then it will just fail to load if the user selects it.
		// This is because all versions of GPlates share the same logical session list and anytime
		// one version of GPlates saves a session then it should appear in the list regardless of
		// whether other versions of GPlates can read it or not.
		const GPlatesAppLogic::UserPreferences::KeyValueMap deprecated_session_state =
				prefs.get_keyvalues_as_map(deprecated_session_path);
		InternalSession::non_null_ptr_to_const_type deprecated_session =
				InternalSession::create_restore_session(deprecated_session_state, true/*is_deprecated_session*/);

		// Search for a session, if any, that matches the deprecated session (has same loaded files).
		// If there's a match and the deprecated session is more recent then remove the session
		// (the deprecated session will later get inserted at the right location).
		bool matches_existing_session_but_is_older = false;
		for (int session_index = 0; session_index < session_list.size(); ++session_index)
		{
			if (deprecated_session->has_same_loaded_files_as(*session_list[session_index]))
			{
				// Matching session already in storage.
				// If deprecated session is more recent, then remove the session already in storage.
				if (session_list[session_index]->get_time() < deprecated_session->get_time())
				{
					session_list.removeAt(session_index);
				}
				else
				{
					matches_existing_session_but_is_older = true;
				}

				break;
			}
		}

		if (matches_existing_session_but_is_older)
		{
			continue;
		}

		// See if the current deprecated session is more recent than any sessions in the list.
		//
		// Note that we traverse the session list from most recent to least recent.
		bool inserted_deprecated_session = false;
		for (int session_index = 0; session_index < session_list.size(); ++session_index)
		{
			// If deprecated session is more recent, then insert it into the session list.
			if (session_list[session_index]->get_time() < deprecated_session->get_time())
			{
				session_list.insert(session_index, deprecated_session);
				inserted_deprecated_session = true;

				// Make sure the list does not exceed the maximum number of session entries.
				if (session_list.size() > sessions_max_size)
				{
					// Remove the least recent session entry.
					session_list.removeLast();
				}

				break;
			}
		}

		if (inserted_deprecated_session)
		{
			continue;
		}

		// The deprecated session does not match an existing session (same loaded files) and
		// is not more recent than any existing sessions.
		// So just append it to the end of the list (least recent) if there's room.
		if (session_list.size() < sessions_max_size)
		{
			session_list.append(deprecated_session);
		}
	}
	
	return session_list;
}


QStringList
GPlatesPresentation::SessionManagement::load_previous_session(
		int session_slot_to_load)
{
	QList<InternalSession::non_null_ptr_to_const_type> sessions = get_recent_session_list();
	if (session_slot_to_load >= sessions.size() || session_slot_to_load < 0) {
		// Nothing to load.
		return QStringList();
	}

	const Session::non_null_ptr_to_const_type session_to_load = sessions.at(session_slot_to_load);

	// Load it, potentially saving the previous session.
	return load_session(session_to_load);
}


QStringList
GPlatesPresentation::SessionManagement::load_project_session(
		const QString &project_filename)
{
	// Create a Session object that can be used to restore the session from the project file.
	const Session::non_null_ptr_to_const_type project_session =
			ProjectSession::create_restore_session(project_filename);

	return load_session(project_session);
}


void
GPlatesPresentation::SessionManagement::unload_all_files()
{
	// Block any signaled calls to 'ApplicationState::reconstruct' until we exit this scope.
	// Blocking calls to 'reconstruct' during this scope prevents multiple calls caused by
	// layer signals, etc, which is unnecessary if we're going to call 'reconstruct' anyway.
	GPlatesAppLogic::ApplicationState::ScopedReconstructGuard scoped_reconstruct_guard(
			*d_app_state_ptr, true/*reconstruct_on_scope_exit*/);

	GPlatesAppLogic::FeatureCollectionFileState &file_state = d_app_state_ptr->get_feature_collection_file_state();
	GPlatesAppLogic::FeatureCollectionFileIO &file_io = d_app_state_ptr->get_feature_collection_file_io();

	const std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> loaded_files =
			file_state.get_loaded_files();
	BOOST_FOREACH(
			const GPlatesAppLogic::FeatureCollectionFileState::file_reference &file_ref,
			loaded_files)
	{
		file_io.unload_file(file_ref);
	}
}


void
GPlatesPresentation::SessionManagement::unload_all_unnamed_files()
{
	// Block any signaled calls to 'ApplicationState::reconstruct' until we exit this scope.
	// Blocking calls to 'reconstruct' during this scope prevents multiple calls caused by
	// layer signals, etc, which is unnecessary if we're going to call 'reconstruct' anyway.
	GPlatesAppLogic::ApplicationState::ScopedReconstructGuard scoped_reconstruct_guard(
			*d_app_state_ptr, true/*reconstruct_on_scope_exit*/);

	GPlatesAppLogic::FeatureCollectionFileState &file_state = d_app_state_ptr->get_feature_collection_file_state();
	GPlatesAppLogic::FeatureCollectionFileIO &file_io = d_app_state_ptr->get_feature_collection_file_io();

	const std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> loaded_files =
			file_state.get_loaded_files();
	BOOST_FOREACH(
			const GPlatesAppLogic::FeatureCollectionFileState::file_reference &file_ref,
			loaded_files)
	{
		const GPlatesFileIO::FileInfo &file_info = file_ref.get_file().get_file_info();
		if (file_info.get_qfileinfo().absoluteFilePath().isEmpty())
		{
			file_io.unload_file(file_ref);
		}
	}
}


void
GPlatesPresentation::SessionManagement::close_event_hook()
{
	// if user wants to auto-save at end (default), save.
	GPlatesAppLogic::UserPreferences &prefs = d_app_state_ptr->get_user_preferences();
	if (prefs.get_value("session/auto_save_on_quit").toBool()) {
		// Note that we ALWAYS save_session on (normal) exit, to ensure that any old sessions
		// get updated to new versions, to update the timestamp, and to ensure that if a
		// user was only opening GPlates to mess with some Layers state, that it will be
		// preserved.
		save_session();
	}
}


boost::optional<GPlatesPresentation::InternalSession::non_null_ptr_to_const_type>
GPlatesPresentation::SessionManagement::save_session()
{
	// Saving the current session may generate a serialization error...
	try
	{
		// Create a Session object that matches the current GPlates session.
		InternalSession::non_null_ptr_to_const_type current_session = InternalSession::save_session();

		// If session is not empty then save it to the recent session lists.
		// We don't save empty sessions to the recent sessions list.
		if (!current_session->is_empty())
		{
			// In order to save this current session, we must first check the existing
			// session list to see where it belongs.
			QList<InternalSession::non_null_ptr_to_const_type> session_list = get_recent_session_list();

			// Search for a session that matches the current session.
			for (int session_index = 0; session_index < session_list.size(); ++session_index)
			{
				if (current_session->has_same_loaded_files_as(*session_list[session_index]))
				{
					// Matching session already in storage, we should remove that one before
					// we put the current one onto the top (head) of the list.
					session_list.removeAt(session_index);
					break;
				}
			}

			// No duplicate entry on the session list now, we can put the current one
			// at the head of the list. This will have the appropriate effect if we
			// are "bumping" the old session entry to the top.
			session_list.prepend(current_session);

			// Store the modified list to persistent storage, cropping it to the max size
			// as necessary.
			store_recent_session_list(session_list);
		}

		return current_session;
	}
	catch (const GPlatesScribe::Exceptions::BaseException &scribe_exception)
	{
		qWarning() << scribe_exception; // Log the detailed error message.

		return boost::none;
	}
}


GPlatesPresentation::ProjectSession::non_null_ptr_to_const_type
GPlatesPresentation::SessionManagement::save_session_to_project(
		const QString &project_filename)
{
	return ProjectSession::save_session(project_filename);
}


void
GPlatesPresentation::SessionManagement::debug_session_state()
{
	// Saving the current session may generate a serialization error...
	try
	{
		// Create a Session object that matches the current GPlates session.
		InternalSession::non_null_ptr_to_const_type current_session = InternalSession::save_session();

		qDebug() << "Current session:" << current_session->get_description();
		Q_FOREACH(const QString &fi, current_session->get_loaded_files())
		{
			qDebug() << fi;
		}
	}
	catch (const GPlatesScribe::Exceptions::BaseException &scribe_exception)
	{
		qWarning() << scribe_exception; // Log the detailed error message.

		// Return early without changing the session list.
		return;
	}
	
	qDebug() << "Recent sessions:-";
	QList<InternalSession::non_null_ptr_to_const_type> sessions = get_recent_session_list();
	Q_FOREACH(const InternalSession::non_null_ptr_to_const_type &recent_session, sessions)
	{
		qDebug() << recent_session->get_description();
	}
}


void
GPlatesPresentation::SessionManagement::store_recent_session_list(
		const QList<InternalSession::non_null_ptr_to_const_type> &session_list)
{
	GPlatesAppLogic::UserPreferences &prefs = d_app_state_ptr->get_user_preferences();

	// We need to store the size of the list in a special 'size' key.
	int sessions_size = session_list.size();
	// And crop the list to prevent it getting huge.
	int sessions_max_size = prefs.get_value("session/recent/max_size").toInt();
	if (sessions_size > sessions_max_size)
	{
		sessions_size = sessions_max_size;
	}

	//
	// Versions prior to the Scribe system (eg, GPlates 1.0 to 1.2) save each session in the session
	// list with *only* the four keys that were used for saving sessions prior to the Scribe system.
	// So if a future version of GPlates adds a new key or changes the name of an existing key then
	// prior versions will lose information when they store the recent session list below.
	// When the Scribe system was introduced this was rectified by storing the entire session state
	// including all key/value pairs in a session entry (this makes storing the recent session list
	// work when sessions, saved by future versions of GPlates, are encountered).
	// Also prior versions prior would attempt to restore sessions created by future versions
	// which would fail - and this has also been rectified.
	// So to avoid problems with GPlates versions prior to this we now store the sessions
	// in a separate area under 'session/recent/sessions/' instead of 'session/recent/'.
	// This enables these prior versions (in 'session/recent/') to work correctly since they are
	// unaware of the presence of future versions (in 'session/recent/sessions/').
	// All versions still use 'session/recent/max_size' and other related parameters - it's just
	// the actual sessions themselves and the number of sessions 'session/recent/sessions/size'
	// that have been moved.
	//
	prefs.set_value("session/recent/sessions/size", sessions_size);
	
	// Push the recent sessions into the user preferences storage.
	// They are 1-indexed.
	for (int i = 1; i <= sessions_size; ++i)
	{
		// Session number i is stored in a 'directory' named i.
		const InternalSession::non_null_ptr_to_const_type &session = session_list.at(i-1);
		QString session_path = QString("session/recent/sessions/%1").arg(i);

		prefs.set_keyvalues_from_map(session_path, session->get_session_key_value_map());
	}

	// Ensure menu is updated.
	Q_EMIT session_list_updated();
}
