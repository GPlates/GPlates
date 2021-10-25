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
#include <QDateTime>
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
	class ViewState;

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

		/**
		 * Information about a session such as time created, description and loaded files.
		 *
		 * Avoids exposing internal @a Session objects.
		 */
		class SessionInfo
		{
		public:

			explicit
			SessionInfo(
					Session::non_null_ptr_type session);

			/**
			 * Textual description - see "Session::get_description()".
			 */
			QString
			get_description() const
			{
				return d_session->get_description();
			}

			/**
			 * The time when the session was saved - see "Session::get_time()".
			 */
			const QDateTime &
			get_time() const
			{
				return d_session->get_time();
			}

			/**
			 * Which files were active when the session was saved - see "Session::get_loaded_files()".
			 */
			QList<QString>
			get_loaded_files() const
			{
				return d_session->get_loaded_files();
			}

			/**
			 * It is possible to have an 'empty' session without any files - see "Session::is_empty()".
			 */
			bool
			is_empty() const
			{
				return d_session->is_empty();
			}

		private:
			Session::non_null_ptr_type d_session;

			Session::non_null_ptr_type
			get_session() const
			{
				return d_session;
			}

			friend class SessionManagement;
		};


		/**
		 * Information about an internal session.
		 *
		 * Currently all information is in base class @a SessionInfo information such as
		 * time created, description and loaded files.
		 *
		 * Avoids exposing internal @a InternalSession objects.
		 */
		class InternalSessionInfo :
				public SessionInfo
		{
		public:

			explicit
			InternalSessionInfo(
					InternalSession::non_null_ptr_type internal_session);

			/**
			 * Returns unique sorted lists of all (absolute) file paths of transcribed files that
			 * currently exist and are currently missing.
			 *
			 * See "InternalSession::get_file_paths()" for more details.
			 */
			void
			get_file_paths(
					QStringList &existing_absolute_file_paths,
					QStringList &missing_absolute_file_paths) const
			{
				d_internal_session->get_file_paths(existing_absolute_file_paths, missing_absolute_file_paths);
			}

			/**
			 * Specify whether to remap missing file paths to existing file paths.
			 *
			 * See "InternalSession::set_remapped_file_paths()" for more details.
			 */
			void
			set_remapped_file_paths(
					boost::optional< QMap<QString/*missing*/, QString/*existing*/> > file_path_remapping)
			{
				d_internal_session->set_remapped_file_paths(file_path_remapping);
			}

		private:
			InternalSession::non_null_ptr_type d_internal_session;

			InternalSession::non_null_ptr_type
			get_internal_session() const
			{
				return d_internal_session;
			}

			friend class SessionManagement;
		};


		/**
		 * Information about a project session such as project filename and existence of
		 * absolute file paths in project (versus file paths relative to the project file if
		 * the project file has moved location).
		 *
		 * Also includes @a SessionInfo information such as time created, description and loaded files.
		 *
		 * Avoids exposing internal @a ProjectSession objects.
		 */
		class ProjectInfo :
				public SessionInfo
		{
		public:

			explicit
			ProjectInfo(
					ProjectSession::non_null_ptr_type project_session);

			/**
			 * Returns the project filename - see "ProjectSession::get_project_filename()".
			 */
			QString
			get_project_filename() const
			{
				return d_project_session->get_project_filename();
			}

			/**
			 * Returns the number of file paths of transcribed files.
			 */
			int
			get_num_file_paths() const
			{
				return d_project_session->get_num_file_paths();
			}

			/**
			 * Returns true if the project file being loaded has moved from where it was saved.
			 *
			 * See "ProjectSession::has_project_file_moved()" for more details.
			 */
			bool
			has_project_file_moved() const
			{
				return d_project_session->has_project_file_moved();
			}

			/**
			 * Returns unique sorted lists of all absolute file paths of transcribed files that
			 * currently exist and are currently missing.
			 *
			 * See "ProjectSession::get_absolute_file_paths()" for more details.
			 */
			void
			get_absolute_file_paths(
					QStringList &existing_absolute_file_paths,
					QStringList &missing_absolute_file_paths) const
			{
				d_project_session->get_absolute_file_paths(existing_absolute_file_paths, missing_absolute_file_paths);
			}

			/**
			 * Returns unique sorted lists of all relative file paths of transcribed files that
			 * currently exist and are currently missing.
			 *
			 * See "ProjectSession::get_relative_file_paths()" for more details.
			 */
			void
			get_relative_file_paths(
					QStringList &existing_relative_file_paths,
					QStringList &missing_relative_file_paths) const
			{
				d_project_session->get_relative_file_paths(existing_relative_file_paths, missing_relative_file_paths);
			}

			/**
			 * Specify whether to use file paths that are relative to the project file when loading data files.
			 *
			 * By default uses the absolute file paths transcribed into the project.
			 *
			 * See "ProjectSession::set_load_relative_file_paths()" for more details.
			 */
			void
			set_load_relative_file_paths(
					bool load_relative_file_paths = true)
			{
				d_project_session->set_load_relative_file_paths(load_relative_file_paths);
			}

			/**
			 * Specify whether to remap missing file paths to existing file paths.
			 *
			 * See "ProjectSession::set_remapped_file_paths()" for more details.
			 */
			void
			set_remapped_file_paths(
					boost::optional< QMap<QString/*missing*/, QString/*existing*/> > file_path_remapping)
			{
				d_project_session->set_remapped_file_paths(file_path_remapping);
			}

			/**
			 * Compare the current session state with the last saved or restored project session state
			 * to see if the session state has changed.
			 *
			 * See "ProjectSession::has_session_state_changed()" for more details.
			 */
			bool
			has_session_state_changed() const
			{
				return d_project_session->has_session_state_changed();
			}

		private:
			ProjectSession::non_null_ptr_type d_project_session;

			ProjectSession::non_null_ptr_type
			get_project_session() const
			{
				return d_project_session;
			}

			friend class SessionManagement;
		};


		//! Constructor.
		explicit
		SessionManagement(
				GPlatesAppLogic::ApplicationState &app_state,
				GPlatesPresentation::ViewState &view_state);

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
		 * Returns a list of all session information objects that are currently in persistent storage.
		 * This is used by the GPlatesGui::SessionMenu to generate a menu with one menu item per session.
		 */
		QList<InternalSessionInfo>
		get_recent_session_list();

	public Q_SLOTS:

		/**
		 * Clear the current session so there's no files loaded and
		 * no auto-created or user-created layers left.
		 *
		 * If @a save_current_session is true then the current session is saved first before clearing.
		 */
		void
		clear_session(
				bool save_current_session);


		/**
		 * Retrieves the session information from the most recent session (default),
		 * or specified session slot, from user preference storage.
		 *
		 * The default value retrieves the most recent session "slot" in the user's
		 * history; higher numbers dig further into the past. Attempting to
		 * retrieve a "session slot" which does not exist returns none - the menu
		 * should match the correct number of slots anyway.
		 */
		boost::optional<InternalSessionInfo>
		get_previous_session_info(
				int session_slot = 0);

		/**
		 * Loads the specified session from user preference storage.
		 *
		 * If @a save_current_session is true then the current session is saved first before loading.
		 *
		 * Any files that were not loaded (either they don't exist or the load failed) get reported
		 * in the read errors dialog.
		 *
		 * This can throw all of the exceptions that FeatureCollectionFileIO can.
		 * It can also throw Scribe exceptions if the unserialization of the session failed.
		 */
		void
		load_previous_session(
				const InternalSessionInfo &session,
				bool save_current_session);


		/**
		 * Save information about which files are currently loaded to persistent storage and
		 * the entire application state.
		 *
		 * Also removes any unnamed files.
		 *
		 * Returns true if the current session was successfully saved.
		 * Returns false if the serialization of the session failed (eg, a Scribe exception).
		 */
		bool
		save_session();


		/**
		 * Returns the project information if the current session is a project session, otherwise returns none.
		 *
		 * The current session is a project if @a load_project or @a save_project has been called.
		 * However calls to either @a load_previous_session or @a clear_session will cause the
		 * current session to no longer be a project.
		 */
		boost::optional<ProjectInfo>
		is_current_session_a_project() const;


		/**
		 * Returns true if the current session is a project session and it has unsaved session state
		 * changes since it was last saved or restored.
		 *
		 * NOTE: The unsaved changes do *not* include unsaved feature collections.
		 * Only includes unsaved session state changes (eg, changes to layer settings).
		 */
		bool
		is_current_session_a_project_with_unsaved_changes() const;


		/**
		 * Retrieves the project information from the specified project file.
		 */
		ProjectInfo
		get_project_info(
				const QString &project_filename);

		/**
		 * Loads a project session from the specified project (similar to @a load_previous_session
		 * but not loading from the recent sessions list).
		 *
		 * If @a save_current_session is true then the current session is saved first before loading.
		 *
		 * This can throw all of the exceptions that FeatureCollectionFileIO can.
		 * It can also throw Scribe exceptions if the unserialization of the session failed.
		 *
		 * Any files that were not loaded (either they don't exist or the load failed) get reported
		 * in the read errors dialog.
		 */
		void
		load_project(
				const ProjectInfo &project,
				bool save_current_session);


		/**
		 * Saves the current session state to the specified project file.
		 *
		 * NOTE: Unlike @a save_session this will throw a Scribe exception
		 * (instead of returning false) if the serialization of the session failed.
		 */
		void
		save_project(
				const QString &project_filename);


		/**
		 * GPlates is closing and we are to remember the current loaded file set
		 * (if that is what the user wants us to do in this situation according to user preferences).
		 */
		void
		close_event_hook();

		void
		debug_session_state();

	Q_SIGNALS:

		/**
		 * Emitted when we write a new session list to persistent storage, so
		 * that menus can be updated.
		 */
		void
		session_list_updated();

		/**
		 * Emitted when a project filename has changed.
		 *
		 * @a project_filename is boost::none when the current session no longer corresponds to a
		 * project. This happens when either an internal session is loaded (@a load_previous_session)
		 * or the current session is cleared (@a clear_session).
		 */
		void
		changed_project_filename(
				boost::optional<QString> project_filename);

	private:

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
		 * Sets the current project (or unsets it).
		 *
		 * Also updates whether the current session is a project or not and
		 * emits @a changed_project_filename signal if project filename changed.
		 */
		void
		set_project(
				boost::optional<ProjectInfo> project);

		/**
		 * Clear the current session state so there's no files loaded and
		 * no auto-created or user-created layers left.
		 */
		void
		clear_session_state(
				bool preserve_current_view_time);

		/**
		 * Load files (and re-link Layer relationships) corresponding to the stored session.
		 *
		 * If @a save_current_session is true then the current session is saved first before loading.
		 *
		 * This can throw all of the exceptions that FeatureCollectionFileIO can.
		 *
		 * Any files that were not loaded (either they don't exist or the load failed) get reported
		 * in the read errors dialog.
		 */
		void
		load_session_state(
				const SessionInfo &session_to_load,
				bool save_current_session);

		/**
		 * Save information about which files are currently loaded to persistent storage and
		 * the entire application state.
		 *
		 * Also removes any unnamed files.
		 *
		 * Returns the current session if it was successfully saved.
		 * Returns boost::none if the serialization of the session failed (eg, a Scribe exception).
		 * Returns a valid session (ie, not boost::none) even if the current session is empty
		 * (no files loaded) - this is useful when need to get the initial session state at application
		 * startup (before files are loaded) in order to use it to clear session state later on.
		 */
		boost::optional<InternalSession::non_null_ptr_type>
		save_session_state();

		/**
		 * Save the list of sessions to persistent storage.
		 */
		void
		store_recent_session_list(
				const QList<InternalSessionInfo> &session_list);

		/**
		 * Guarded pointer back to ApplicationState so we can interact with the rest
		 * of GPlates. Since ApplicationState is a QObject, we don't have to worry
		 * about a dangling pointer (even though ApplicationState should never be
		 * destroyed before we are).
		 */
		QPointer<GPlatesAppLogic::ApplicationState> d_app_state_ptr;

		/**
		 * Guarded pointer back to ViewState so we can interact with the rest
		 * of GPlates. Since ViewState is a QObject, we don't have to worry
		 * about a dangling pointer (even though ViewState should never be
		 * destroyed before we are).
		 */
		QPointer<GPlatesPresentation::ViewState> d_view_state_ptr;

		/**
		 * The session state that represents GPlates at application startup (with no files loaded).
		 *
		 * This is used to clear the session state.
		 * It is boost::none if we failed to save the session state at application startup.
		 */
		boost::optional<InternalSession::non_null_ptr_to_const_type> d_clear_session_state;

		/**
		 * The currently loaded project (if any).
		 *
		 * This is boost::none if an internal session is currently loaded or the session has been cleared.
		 */
		boost::optional<ProjectInfo> d_project;
	};
}

#endif // GPLATES_PRESENTATION_SESSIONMANAGEMENT_H
