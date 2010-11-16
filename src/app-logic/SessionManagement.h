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

#ifndef GPLATES_APP_LOGIC_SESSIONMANAGEMENT_H
#define GPLATES_APP_LOGIC_SESSIONMANAGEMENT_H

#include <boost/noncopyable.hpp>
#include <QObject>
#include <QPointer>
#include <QList>
#include <QString>
#include <QStringList>

#include "Session.h"


namespace GPlatesAppLogic
{
	class ApplicationState;

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

		SessionManagement(
				GPlatesAppLogic::ApplicationState &app_state);

		virtual
		~SessionManagement()
		{  }


		/**
		 * Load files (and re-link Layer relationships) corresponding to the
		 * stored session.
		 *
		 * This can throw all of the exceptions that FeatureCollectionFileIO can.
		 */
		void
		load_session(
				const GPlatesAppLogic::Session &session_to_load);


		/**
		 * Returns a list of all Session objects that are currently in persistent
		 * storage. This is used by the GPlatesGui::SessionManagement to generate
		 * a menu with one menu item per session.
		 */
		QList<GPlatesAppLogic::Session>
		get_recent_session_list();

	public slots:

		/**
		 * As @a load_session(Session &), but automatically picks the most recent
		 * session from user preference storage to load.
		 */
		void
		load_previous_session();

		/**
		 * Clear out all loaded files (in preparation for loading some new session)
		 */
		void
		unload_all_files();

		/**
		 * GPlates is closing and we are to remember the current loaded file set
		 * (if that is what the user wants us to do in this situation).
		 */
		void
		close_event_hook();

		/**
		 * Save information about which files are currently loaded to persistent
		 * storage.
		 */
		void
		save_session();

		void
		debug_session_state();

	private:

		Session
		new_session_from_current_state();

		/**
		 * Guarded pointer back to ApplicationState so we can interact with the rest
		 * of GPlates. Since ApplicationState is a QObject, we don't have to worry
		 * about a dangling pointer (even though ApplicationState should never be
		 * destroyed before we are)
		 */
		QPointer<GPlatesAppLogic::ApplicationState> d_app_state_ptr;
	};
}

#endif // GPLATES_APP_LOGIC_SESSIONMANAGEMENT_H
