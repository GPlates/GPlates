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

#include "Session.h"

#include "app-logic/UserPreferences.h"


namespace GPlatesPresentation
{
	/**
	 * An internal session of GPlates (saved to a text archive string in the user preferences store).
	 */
	class InternalSession :
			public Session
	{
	public:

		// Convenience typedefs for a shared pointer to a @a InternalSession.
		typedef GPlatesUtils::non_null_intrusive_ptr<InternalSession> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const InternalSession> non_null_ptr_to_const_type;


		/**
		 * Create a @a InternalSession object, from the specified session state, that can be used to restore a session.
		 *
		 * NOTE: This doesn't actually restore the session. For that you need to call @a restore_session.
		 *
		 * The session state is stored in a sub-section of the user preferences (key-value map).
		 *
		 * @a is_deprecated_session is for deprecated sessions before the Scribe system was implemented.
		 */
		static
		non_null_ptr_to_const_type
		create_restore_session(
				const GPlatesAppLogic::UserPreferences::KeyValueMap &session_state,
				bool is_deprecated_session);


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
		non_null_ptr_to_const_type
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
		 * Returns a list of feature collection files that were not loaded
		 * (either they don't exist or the load failed).
		 */
		virtual
		QStringList
		restore_session() const;


		/**
		 * Return the key-value map associated with this session for easy insertion into the user preferences storage.
		 */
		const GPlatesAppLogic::UserPreferences::KeyValueMap &
		get_session_key_value_map() const
		{
			return d_session_key_value_map;
		}

	private:

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
		 * Deprecated sessions are those before the Scribe system was implemented.
		 *
		 * Deprecated sessions have a version number (from 0 to 3 inclusive).
		 * Non-deprecated sessions do not need a version number (versioning is handled implicitly
		 * by the Scribe system).
		 */
		boost::optional<int> d_deprecated_version;


		/**
		 * Construct a new @a InternalSession object.
		 */
		InternalSession(
				const GPlatesAppLogic::UserPreferences::KeyValueMap &session_key_value_map_,
				const QDateTime &time_,
				const QStringList &files_,
				boost::optional<int> deprecated_version = boost::none);

	};
}

#endif // GPLATES_PRESENTATION_INTERNALSESSION_H