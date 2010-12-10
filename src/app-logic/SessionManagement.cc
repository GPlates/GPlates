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

#include <boost/foreach.hpp>
#include <QDebug>
#include <QFileInfo>
#include <QList>

#include "SessionManagement.h"

#include "UserPreferences.h"
#include "ApplicationState.h"
#include "FeatureCollectionFileState.h"
#include "FeatureCollectionFileIO.h"
#include "file-io/FileInfo.h"


namespace
{
	QList<QFileInfo>
	loaded_file_info(
			GPlatesAppLogic::FeatureCollectionFileState &file_state)
	{
		QList<QFileInfo> files;
		const std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> loaded_files =
				file_state.get_loaded_files();
		BOOST_FOREACH(
				const GPlatesAppLogic::FeatureCollectionFileState::file_reference &file_ref,
				loaded_files)
		{
			const GPlatesFileIO::FileInfo &file_info = file_ref.get_file().get_file_info();
			files << file_info.get_qfileinfo();
		}
		return files;
	}
}


GPlatesAppLogic::SessionManagement::SessionManagement(
		GPlatesAppLogic::ApplicationState &app_state):
	d_app_state_ptr(&app_state)
{
}


void
GPlatesAppLogic::SessionManagement::load_session(
		const GPlatesAppLogic::Session &session_to_load)
{
	GPlatesAppLogic::FeatureCollectionFileIO &file_io = d_app_state_ptr->get_feature_collection_file_io();

	if (session_to_load.is_empty()) {
		// How did we get here? Menu shouldn't contain empty listings.
		return;
	}

	Session original_session = new_session_from_current_state();
	if (original_session == session_to_load) {
		// User is attempting to re-load the session that they are already in.
		// This is equivalent to hitting 'Reload' on all their loaded files.
		// We do *not* save the current session beforehand in this case.
		// NOTE: "Reloading" layer relationships may prove complicated.
		
		// For now, maybe just clear and load everything?
		unload_all_files();
		
		file_io.load_files(QStringList::fromSet(session_to_load.loaded_files()));

	} else {
		// User is attempting to load a new session. Should we replace the old one?
		// For now, the answer is yes - always unload the original files first.
		// However, before we do that, save the current session.
		save_session();
		unload_all_files();
		
		file_io.load_files(QStringList::fromSet(session_to_load.loaded_files()));
	}
	// Thinking out loud:-
	// save current session first, if any - may require a prompt up in a gui level
	// asking user if they want to overwrite current, or append loaded session onto current.
	// of course if they want to re-load the session they're already in, don't save first.
}


QList<GPlatesAppLogic::Session>
GPlatesAppLogic::SessionManagement::get_recent_session_list()
{
	QList<Session> session_list;

	// Sessions are stored as an "array" in the Qt style, so first read the 'size' of that array.
	GPlatesAppLogic::UserPreferences &prefs = d_app_state_ptr->get_user_preferences();
	int sessions_size = prefs.get_value("session/recent/size").toInt();
 	if (sessions_size <= 0) {
		// Nothing to load.
		return session_list;
	}
	
	// Pull the recent sessions out of the user preferences storage.
	// They are 1-indexed.
	for (int i = 1; i <= sessions_size; ++i) {
		// Session number i is stored in a 'directory' named i.
		QString session_path = QString("session/recent/%1").arg(i);
 		if (prefs.exists(session_path + "/loaded_files")) {
			GPlatesAppLogic::UserPreferences::KeyValueMap map = prefs.get_keyvalues_as_map(
					session_path);
			session_list << Session::unserialise_from_prefs_map(map);
		}
	}
	
	return session_list;
}


void
GPlatesAppLogic::SessionManagement::load_previous_session(
		int session_slot_to_load)
{
	QList<GPlatesAppLogic::Session> sessions = get_recent_session_list();
	if (session_slot_to_load >= sessions.size() || session_slot_to_load < 0) {
		// Nothing to load.
		return;
	}

	// Load it, potentially saving the previous session.
	load_session(sessions.at(session_slot_to_load));
}


void
GPlatesAppLogic::SessionManagement::unload_all_files()
{
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
GPlatesAppLogic::SessionManagement::close_event_hook()
{
	// if user wants to auto-save at end (default), save.
	GPlatesAppLogic::UserPreferences &prefs = d_app_state_ptr->get_user_preferences();
	if (prefs.get_value("session/auto_save_on_quit").toBool()) {
		save_session();
	}
}


void
GPlatesAppLogic::SessionManagement::save_session()
{
	// Create a Session object that matches what we have currently loaded.
	Session current = new_session_from_current_state();
	if (current.is_empty()) {
		// We don't save empty sessions.
		return;
	}

	// In order to save this current session, we must first check the existing
	// session list to see where it belongs.
	QList<GPlatesAppLogic::Session> session_list = get_recent_session_list();

	int existing_entry = session_list.indexOf(current);
	if (existing_entry >= 0) {
		// Matching session already in storage, we should remove that one before
		// we put the current one onto the top (head) of the list.
		session_list.removeAt(existing_entry);
	}
	
	// No duplicate entry on the session list now, we can put the current one
	// at the head of the list. This will have the appropriate effect if we
	// are "bumping" the old session entry to the top.
	session_list.prepend(current);

	// Store the modified list to persistent storage, cropping it to the max size
	// as necessary.
	store_recent_session_list(session_list);
}


void
GPlatesAppLogic::SessionManagement::debug_session_state()
{
	Session current = new_session_from_current_state();
	qDebug() << "Current session:" << current.description();
	Q_FOREACH(const QString &fi, current.loaded_files()) {
		qDebug() << fi;
	}
	
	qDebug() << "Recent sessions:-";
	QList<GPlatesAppLogic::Session> sessions = get_recent_session_list();
	Q_FOREACH(const Session &recent, sessions) {
		qDebug() << recent.description();
	}
}


void
GPlatesAppLogic::SessionManagement::store_recent_session_list(
		const QList<GPlatesAppLogic::Session> &session_list)
{
	GPlatesAppLogic::UserPreferences &prefs = d_app_state_ptr->get_user_preferences();

	// We need to store the size of the list in a special 'size' key.
	int sessions_size = session_list.size();
	// And crop the list to prevent it getting huge.
	int sessions_max_size = prefs.get_value("session/recent/max_size").toInt();
	if (sessions_size > sessions_max_size) {
		sessions_size = sessions_max_size;
	}
	prefs.set_value("session/recent/size", sessions_size);
	
	// Push the recent sessions into the user preferences storage.
	// They are 1-indexed.
	for (int i = 1; i <= sessions_size; ++i) {
		// Session number i is stored in a 'directory' named i.
		const Session &session = session_list.at(i-1);
		QString session_path = QString("session/recent/%1").arg(i);

		prefs.set_keyvalues_from_map(session_path, session.serialise_to_prefs_map());
	}

	// Ensure menu is updated.
	emit session_list_updated();
}


GPlatesAppLogic::Session
GPlatesAppLogic::SessionManagement::new_session_from_current_state()
{
	QDateTime time = QDateTime::currentDateTime();

	QList<QFileInfo> files = loaded_file_info(d_app_state_ptr->get_feature_collection_file_state());
	QSet<QString> filenames;
	Q_FOREACH(const QFileInfo &fi, files) {
		filenames.insert(fi.absoluteFilePath());
	}
	
	// Create and return the new Session. It's a lightweight class, all the members
	// are Qt pimpl-idiom stuff, passing it by value isn't a bad thing.
	return Session(time, filenames);
}



