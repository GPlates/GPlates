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

	// TEMP: Just assume exactly one (or zero I guess) session.
	GPlatesAppLogic::UserPreferences &prefs = d_app_state_ptr->get_user_preferences();
	if (prefs.get_value("session/recent/size").toInt() <= 0) {
		// Nothing to load.
		return session_list;
	}
	
	// Pull the most recent session out of the user preferences storage.	
	GPlatesAppLogic::UserPreferences::KeyValueMap map = prefs.get_keyvalues_as_map(
			"session/recent/1");
	session_list << Session::unserialise_from_prefs_map(map);
	
	return session_list;
}


void
GPlatesAppLogic::SessionManagement::load_previous_session()
{

	// TEMP: Just load exactly one session.
	GPlatesAppLogic::UserPreferences &prefs = d_app_state_ptr->get_user_preferences();
	if (prefs.get_value("session/recent/size").toInt() <= 0) {
		// Nothing to load.
		return;
	}

	// Pull the most recent session out of the user preferences storage.	
	GPlatesAppLogic::UserPreferences::KeyValueMap map = prefs.get_keyvalues_as_map(
			"session/recent/1");
	Session previous = Session::unserialise_from_prefs_map(map);
	
	// Load it, potentially saving the previous session.
	load_session(previous);
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
	Session current = new_session_from_current_state();
	if (current.is_empty()) {
		// We don't save empty sessions.
		return;
	}

	// TEMP: Just save exactly one session.
	GPlatesAppLogic::UserPreferences &prefs = d_app_state_ptr->get_user_preferences();
	prefs.set_value("session/recent/size", 1);
	prefs.set_keyvalues_from_map("session/recent/1", current.serialise_to_prefs_map());

	// Thinking out loud:-
	// return if session empty
	// serialise current state to some struct
	// identify matching sessions already in storage
	// bump that session to top or unshift new session
	// crop session list
}


void
GPlatesAppLogic::SessionManagement::debug_session_state()
{
	Session current = new_session_from_current_state();
	qDebug() << "Current session:" << current.description();
	Q_FOREACH(const QString &fi, current.loaded_files()) {
		qDebug() << fi;
	}
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



