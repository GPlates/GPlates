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

#ifndef GPLATES_PRESENTATION_SESSIONMANAGEMENT_H
#define GPLATES_PRESENTATION_SESSIONMANAGEMENT_H

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QStringList>

#include "InternalSession.h"
#include "ProjectSession.h"


namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesPresentation
{

	/**
	 * As a first-cut implementation of a Projects system, get GPlates to remember
	 * which files were loaded and the state of the Layers system between sessions,
	 * by storing session data via UserPreferences.
	 *
	 * Note that "saving" a session refers to recording the list of files, layers etc.
	 * that were loaded into memory by GPlates at a particular time; feature data
	 * does not get written to disk.
	 */
	class SessionManagement :
			public QObject,
			private boost::noncopyable
	{
		Q_OBJECT

	public:

		explicit
		SessionManagement(
				GPlatesAppLogic::ApplicationState &app_state);

		virtual
		~SessionManagement()
		{  }


		/**
		 * Initialise the session management once the entire application has started up.
		 *
		 * This currently generates a clear session that represents the state of GPlates at
		 * application startup and is used to clear the session state.
		 *
		 * This should be called after ViewportWindow, ViewState and ApplicationState have initialised.
		 */
		void
		initialise();


		/**
		 * Returns a list of all Session objects that are currently in persistent
		 * storage. This is used by the GPlatesGui::SessionManagement to generate
		 * a menu with one menu item per session.
		 */
		QList<InternalSession::non_null_ptr_to_const_type>
		get_recent_session_list();

	public Q_SLOTS:

		/**
		 * Clear the current session so there's no files loaded and
		 * no auto-created or user-created layers left.
		 */
		void
		clear_session();

		/**
		 * As @a load_session(const Session::non_null_ptr_to_const_type &session_to_load),
		 * but automatically picks the most recent session from user preference storage to load.
		 *
		 * The default value loads the most recent session "slot" in the user's
		 * history; higher numbers dig further into the past. Attempting to
		 * load a "session slot" which does not exist does nothing - the menu
		 * should match the correct number of slots anyway.
		 *
		 * Returns a list of feature collection files that were not loaded.
		 */
		QStringList
		load_previous_session(
				int session_slot_to_load = 0);


		/**
		 * Loads a session, same as @a load_previous_session, but from a project file instead of
		 * the recent sessions list.
		 *
		 * This can throw all of the exceptions that FeatureCollectionFileIO can.
		 *
		 * Returns a list of feature collection files that were not loaded.
		 */
		QStringList
		load_project_session(
				const QString &project_filename);

		/**
		 * Clear out all loaded files (in preparation for loading some new session)
		 */
		void
		unload_all_files();

		/**
		 * Clear out all feature collections which do not correspond to a file on disk,
		 * i.e. New Feature Collections or those with an empty filename.
		 *
		 * This is called in situations where a session is about to be saved but an
		 * Unsaved Changes dialog might be triggered. If the user wishes to discard their
		 * unnamed temporary feature collections, we should first unload them from the
		 * model to trigger the appropriate auto-created-layer removal, so that the
		 * logical state of the ReconstructionGraph @em matches the state we would be
		 * re-loading from a stored session.
		 */
		void
		unload_all_unnamed_files();

		/**
		 * GPlates is closing and we are to remember the current loaded file set
		 * (if that is what the user wants us to do in this situation).
		 */
		void
		close_event_hook();

		/**
		 * Save information about which files are currently loaded to persistent storage and
		 * the entire application state.
		 *
		 * Returns the current session if it was successfully saved.
		 * Returns boost::none if the serialization of the session failed (eg, a Scribe exception).
		 * Returns a valid session (ie, not boost::none) even if the current session is empty
		 * (no files loaded) - this is useful when need to get the initial session state at application
		 * startup (before files are loaded) in order to use it to clear session state later on.
		 */
		boost::optional<InternalSession::non_null_ptr_to_const_type>
		save_session();

		/**
		 * Saves the current session state to the specified project file.
		 *
		 * NOTE: Unlike @a save_session this will throw a Scribe exception (instead of returning
		 * boost::none) if the serialization of the session failed.
		 */
		ProjectSession::non_null_ptr_to_const_type
		save_session_to_project(
				const QString &project_filename);

		void
		debug_session_state();

	Q_SIGNALS:

		/**
		 * Emitted when we write a new session list to persistent storage, so
		 * that menus can be updated.
		 */
		void
		session_list_updated();

	private:

		/**
		 * Load files (and re-link Layer relationships) corresponding to the
		 * stored session.
		 *
		 * This can throw all of the exceptions that FeatureCollectionFileIO can.
		 *
		 * Returns a list of feature collection files that were not loaded (either they don't exist
		 * or the load failed - the latter not applying to deprecated sessions).
		 */
		QStringList
		load_session(
				const Session::non_null_ptr_to_const_type &session_to_load);

		void
		store_recent_session_list(
				const QList<InternalSession::non_null_ptr_to_const_type> &session_list);

		/**
		 * Guarded pointer back to ApplicationState so we can interact with the rest
		 * of GPlates. Since ApplicationState is a QObject, we don't have to worry
		 * about a dangling pointer (even though ApplicationState should never be
		 * destroyed before we are)
		 */
		QPointer<GPlatesAppLogic::ApplicationState> d_app_state_ptr;

// For now we just clear the session state by unloading all files and layers.
// When session save/restore gets more involved we can bring this back.
#if 0
		/**
		 * The session state that represents GPlates at application startup (with no files loaded).
		 *
		 * This is used to clear the session state.
		 * It is boost::none if we failed to save the session state at application startup.
		 */
		boost::optional<InternalSession::non_null_ptr_to_const_type> d_clear_session_state;
#endif
	};
}

#endif // GPLATES_PRESENTATION_SESSIONMANAGEMENT_H
