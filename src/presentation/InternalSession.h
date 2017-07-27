/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#ifndef GPLATES_PRESENTATION_INTERNALSESSION_H
#define GPLATES_PRESENTATION_INTERNALSESSION_H

#include <boost/optional.hpp>
#include <QString>
#include <QStringList>

#include "Session.h"

#include "app-logic/UserPreferences.h"


namespace GPlatesPresentation
{
	/**
	 * An internal session of GPlates (saved to the user preferences store).
	 */
	class InternalSession :
			public Session
	{
	public:

		// Convenience typedefs for a shared pointer to a @a InternalSession.
		typedef GPlatesUtils::non_null_intrusive_ptr<InternalSession> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const InternalSession> non_null_ptr_to_const_type;


		/**
		 * Returns true if there are keys in @a session_state that are recognised as session keys.
		 *
		 * This should return true before @a create_restore_session is called.
		 */
		static
		bool
		has_valid_session_keys(
				const GPlatesAppLogic::UserPreferences::KeyValueMap &session_state);


		/**
		 * Create a @a InternalSession object, from the specified session state, that can be used to restore a session.
		 *
		 * Note this doesn't actually restore the session. For that you need to call @a restore_session.
		 *
		 * The session state is stored in a sub-section of the user preferences (key-value map).
		 *
		 * NOTE: A call to @a has_valid_session_keys should have returned true before calling this method.
		 */
		static
		non_null_ptr_type
		create_restore_session(
				const GPlatesAppLogic::UserPreferences::KeyValueMap &session_state);


		/**
		 * Saves the current session and returns it in a @a InternalSession object.
		 *
		 * The singleton @a Application is used to obtain the session state since it contains
		 * the entire state of GPlates.
		 *
		 * NOTE: Throws an exception derived from GPlatesScribe::Exceptions::ExceptionBase
		 * if there was an error during serialization of the session state.
		 */
		static
		non_null_ptr_type
		save_session();


		/**
		 * Restores the session state, contained within, to GPlates.
		 *
		 * Throws @a UnsupportedVersion exception if session was created from a
		 * version of GPlates that is either too old or too new.
		 *
		 * NOTE: Throws an exception derived from GPlatesScribe::Exceptions::ExceptionBase
		 * if there was an error during serialization of the session state.
		 *
		 * Any files that were not loaded (either they don't exist or the load failed) get reported
		 * in the read errors dialog.
		 */
		virtual
		void
		restore_session() const;


		/**
		 * Return the key-value map associated with this session for easy insertion into the user preferences storage.
		 */
		const GPlatesAppLogic::UserPreferences::KeyValueMap &
		get_session_key_value_map() const
		{
			return d_session_key_value_map;
		}


		/**
		 * Returns unique sorted lists of all (absolute) file paths of transcribed files that
		 * currently exist and are currently missing.
		 *
		 * These are file paths transcribed via the TranscribeUtils::FilePath API when the session was saved.
		 *
		 * Note: These are the file paths transcribed into the session when it was saved, and the
		 * files might no longer exist.
		 */
		void
		get_file_paths(
				QStringList &existing_file_paths,
				QStringList &missing_file_paths) const;


		/**
		 * Specify whether to remap missing file paths to existing file paths.
		 *
		 * This is used to rename missing files to existing files when a loaded session references
		 * files that no longer exist (see @a get_file_paths).
		 */
		void
		set_remapped_file_paths(
				boost::optional< QMap<QString/*missing*/, QString/*existing*/> > file_path_remapping);


	private:

		/**
		 * An enumeration that determines what format a session was saved in.
		 */
		enum SessionFormat
		{
			// GPlates 1.4 and prior versions did not use the Scribe system.
			//
			// This format can be loaded by GPlates version 1.5 and above, but GPlates 1.4 cannot
			// load sessions saved by GPlates 1.5 and above.
			GPLATES_1_4_OR_BEFORE_FORMAT,

			// First use of the Scribe system was in GPlates 1.5.
			//
			// But it was not transcribed in such a way that GPlates 1.5 could load sessions saved
			// by future GPlates versions, hence the need for the 'CURRENT_FORMAT' format which
			// enables GPlates 1.5 to load sessions saved by versions above GPlates 1.5.
			GPLATES_1_5_FORMAT,

			// The current format also uses the Scribe system.
			//
			// This format should be able to load sessions saved by future GPlates versions - it won't
			// be able to load all future state of course but should be able to load what it knows.
			// If all goes well we shouldn't need a new format in the future (a single transcription
			// should be backwards compatible, and forwards compatible to an extent).
			CURRENT_FORMAT,

			// An unknown or invalid format.
			UNKNOWN_FORMAT
		};

		//! Session state key for session 'metadata'.
		static const QString CURRENT_FORMAT_SESSION_METADATA_KEY;

		//! Session state key for session 'data'.
		static const QString CURRENT_FORMAT_SESSION_DATA_KEY;

		//! Session state key for GPlates 1.5 session state.
		static const QString GPLATES_1_5_FORMAT_SESSION_STATE_KEY;


		/*
		 * The entire session state including all key/value pairs stored in the session state map.
		 *
		 * Note that this could be the session state generated by a future version of GPlates.
		 * This data member's job is simply to store this state without knowing the specific
		 * key names and what they represent. This enables the session management code to keep
		 * track of the most recent sessions (by date/time) regardless of what version of GPlates
		 * generated each session.
		 */
		GPlatesAppLogic::UserPreferences::KeyValueMap d_session_key_value_map;

		/**
		 * A unique sorted list of all transcribed filenames (transcribed via the TranscribeUtils::FilePath API).
		 */
		QStringList d_all_file_paths;

		/**
		 * Whether to remap missing file paths to existing file paths.
		 */
		boost::optional< QMap<QString/*missing*/, QString/*existing*/> > d_file_path_remapping;


		/**
		 * Determines the session format from the session state (key/value map).
		 */
		static
		SessionFormat
		get_session_format(
				const GPlatesAppLogic::UserPreferences::KeyValueMap &session_state);


		/**
		 * Construct a new @a InternalSession object.
		 */
		InternalSession(
				const GPlatesAppLogic::UserPreferences::KeyValueMap &session_key_value_map_,
				const QDateTime &time_,
				const QStringList &filenames_,
				const QStringList &all_file_paths_);
	};
}

#endif // GPLATES_PRESENTATION_INTERNALSESSION_H
