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

#ifndef GPLATES_APP_LOGIC_SESSION_H
#define GPLATES_APP_LOGIC_SESSION_H

#include <QCoreApplication>
#include <QSet>
#include <QString>
#include <QDateTime>

#include "UserPreferences.h"

namespace GPlatesAppLogic
{
	/**
	 * Lightweight class to encapsulate one previous session of GPlates,
	 * including what files were loaded at the time and Layers state.
	 */
	class Session
	{
		Q_DECLARE_TR_FUNCTIONS(Session)

	public:

		/**
		 * Construct a new Session object to represent a specific collection
		 * of files that were loaded in GPlates at some time.
		 *
		 * @a _files is a collection of absolute path names, obtained via
		 * QFileInfo::absoluteFilePath().
		 */
		Session(
				const QDateTime &_time,
				const QSet<QString> &_files);

		virtual
		~Session()
		{  }
		
		const QDateTime &
		time() const;
		
		const QSet<QString> &
		loaded_files() const;

		/**
		 * Textual description suitable for menus, e.g.
		 * "5 files on Mon Nov 1, 5:57 PM"
		 */
		QString
		description() const;

		/**
		 * It is possible to have an 'empty' session without any files.
		 */
		bool
		is_empty() const;

		/**
		 * Comparing two Session together should ignore the datestamp and
		 * focus on whether the list of files match; this is so that
		 * GPlates can be a bit smarter about how the Recent Sessions menu
		 * operates w.r.t. people loading/saving prior sessions.
		 */
		bool
		operator==(
				const Session &other) const;

		bool
		operator!=(
				const Session &other) const;


		/**
		 * Convert this Session to a key-value map for easy insertion into
		 * the user preferences storage.
		 */
		GPlatesAppLogic::UserPreferences::KeyValueMap
		serialise_to_prefs_map() const;

		/**
		 * Construct a new Session object from a given key-value map.
		 */
		static
		Session
		unserialise_from_prefs_map(
				const GPlatesAppLogic::UserPreferences::KeyValueMap &map);

	private:
		/*
		 * Avoid putting heavy STL member data here, this class gets passed
		 * around by value.
		 */

		/**
		 * The time when the session was saved; usually the time GPlates
		 * last quit while these files were active.
		 */
		QDateTime d_time;

		/**
		 * Which files were active when the session was saved.
		 */
		QSet<QString> d_loaded_files;
	};
}

#endif // GPLATES_APP_LOGIC_SESSION_H
