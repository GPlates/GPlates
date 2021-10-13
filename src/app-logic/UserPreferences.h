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

#ifndef GPLATES_APP_LOGIC_USERPREFERENCES_H
#define GPLATES_APP_LOGIC_USERPREFERENCES_H

#include <boost/noncopyable.hpp>
#include <QObject>
#include <QVariant>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QMap>


namespace GPlatesAppLogic
{
	/**
	 * Handles User Preference and internal GPlates state storage via QSettings backend.
	 *
	 * A few handy guidelines:-
	 *
	 *  - Keys are set using a hierarchy with a unix-like '/' path delimiter.
	 *    There is no initial '/' as the first character.
	 *
	 *  - Treat keys as though they were case-sensitive, because they might be.
	 *
	 *  - Prefer a lowercased naming scheme with underscores to separate words.
	 *
	 *  - Don't put any values in the root, i.e. use "network/proxy" rather than "proxy".
	 *    However, don't use "general" as a root entry, it is reserved for Qt's .ini
	 *    file format support.
	 *
	 *  - Values get stored as a QVariant. Depending on the backend, they may get
	 *    stringified and you might notice the 'type' of them being a QString upon re-load.
	 *    Don't let this bother you - store a QVariant(56) and get it back as the int you
	 *    would expect with .toInt(), Qt does the rest.
	 *
	 *  - Almost all recognised keys should get a default value. If the user hasn't
	 *    picked anything explicitly, we fall back to this. Default values are read from
	 *    the DefaultPreferences.conf file in qt-resources/.
	 *
	 *  - If we are running multiple GPlates versions simultaneously and the user wishes
	 *    to keep profile data for the old version around, it gets 'sandboxed' into
	 *    a path like "version/old/GPlates 0.9.10/" - calls to get_value and set_value
	 *    should seamlessly map to this location if on the old version. (NOT FULLY IMPLEMENTED)
	 */
	class UserPreferences :
			public QObject,
			private boost::noncopyable
	{
		Q_OBJECT

	public:

		typedef QMap<QString, QVariant> KeyValueMap;

		UserPreferences();

		virtual
		~UserPreferences();


		/**
		 * This should be your primary point of access for user preferences.
		 *
		 * Falls back to default value if not set.
		 */
		QVariant
		get_value(
				const QString &key);

		/**
		 * Indicates if this key has been overriden from the defaults by the user
		 * (or potentially, by GPlates) and set in the user's platform's 'registry'.
		 *
		 * A key can exist and can return a value without having been 'set'.
		 */
		bool
		has_been_set(
				const QString &key);

		/**
		 * Fetches default value directly - only useful for user preferences dialog.
		 */
		QVariant
		get_default_value(
				const QString &key);

		/**
		 * Indicates if this key exists in any form, from the user profile or GPlates
		 * compiled-in defaults.
		 *
		 * Note: Keys can exist outside of the compiled-in defaults, i.e. session storage
		 * which has no limit to the number of sub-keys.
		 *
		 * Further note: This only checks if a key/value pair has been set for the given
		 * name. It is possible to have "directories" which have no values associated with
		 * them, use only to sub-divide things. @a exists will return @a false if you
		 * ask about such key-paths.
		 */
		bool
		exists(
				const QString &key);

		/**
		 * Sets new user value, overriding any compiled-in defaults.
		 */
		void
		set_value(
				const QString &key,
				const QVariant &value);

		/**
		 * Clears any user-set value, reverting to compiled-in defaults.
		 */
		void
		clear_value(
				const QString &key);

		/**
		 * Lists all keys, including sub-keys, from the given prefix.
		 * Defaults to everything from the root ("").
		 */
		QStringList
		subkeys(
				const QString &prefix = "");

		/**
		 * Given a @a prefix to a set of keys, slurp all those keys and
		 * values into a QMap<QString, QVariant>.
		 * This is intended to make working with groups of related sub-keys as
		 * a single "object" easier - for example, storing a GPlatesAppLogic::Session.
		 *
		 * All key names will have the prefix stripped - they will be
		 * "relative pathnames" from the given root. It is assumed that
		 * the prefix itself does not have a value stored.
		 *
		 * For example, you could get the keyvalues for prefix "session/recent/1"
		 * and this method would return a map containing keys such as "loaded_files"
		 * and "date" - corresponding to "session/recent/1/loaded_files" and
		 * "session/recent/1/date".
		 *
		 * While there is probably no real application for applying this method
		 * to key/values with defaults and system fallbacks, the returned map
		 * will include default and fallback values even if nothing has been
		 * explicitly set in the "user" scope - returning the same list of keys
		 * that @a subkeys(prefix) would have matched.
		 */
		KeyValueMap
		get_keyvalues_as_map(
				const QString &prefix);
		

		/**
		 * Given a @a prefix in the key-value store, and a map of keyname->value
		 * in a QMap<QString, QVariant>, set all the given keys in one pass.
		 * This is intended to make working with groups of related sub-keys as
		 * a single "object" easier - for example, storing a GPlatesAppLogic::Session.
		 *
		 * All key names should have the prefix stripped - they will be
		 * "relative pathnames" from the given root. All pre-existing keys for that
		 * prefix are cleared before setting the new values.
		 */
		void
		set_keyvalues_from_map(
				const QString &prefix,
				const KeyValueMap &keyvalues);


	public slots:

		/**
		 * Indicates where settings are stored to console.
		 */
		void
		debug_file_locations();

		/**
		 * Writes all keys and values to console.
		 */
		void
		debug_key_values();

	signals:

		void
		key_value_updated(
				QString key);

	private:

		/**
		 * Configures preference keys for multiple-GPlates-version support.
		 */
		void
		initialise_versioning();

		/**
		 * Tests the existence of a compiled-in default key/value.
		 */
		bool
		default_exists(
				const QString &key);

		/**
		 * If this string ! .isNull(), all settings operations will be performed
		 * on a 'subdirectory' of the keystore - this is so that we can support
		 * simultaneous use of different gplates versions with different settings.
		 */
		QString d_key_root;


		/**
		 * Our default settings, loaded from a compiled-in resource file.
		 */
		QSettings d_defaults;
	};
}

#endif // GPLATES_APP_LOGIC_USERPREFERENCES_H
