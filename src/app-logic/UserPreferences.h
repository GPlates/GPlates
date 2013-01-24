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

#ifndef GPLATES_APP_LOGIC_USERPREFERENCES_H
#define GPLATES_APP_LOGIC_USERPREFERENCES_H

#include <boost/noncopyable.hpp>
#include <QObject>
#include <QVariant>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QMap>

#include "utils/ConfigInterface.h"


namespace GPlatesUtils
{
	class ConfigBundle;
}


namespace GPlatesAppLogic
{
	/**
	 * Handles User Preference and internal GPlates state storage via QSettings backend.
	 *
	 * A few handy guidelines:-
	 *
	 *  - Keys are set using a hierarchy with a unix-like '/' path delimiter.
	 *    There is NO initial '/' as the first character.
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
	 *    Some default values are magically sourced from the system, e.g. proxy url.
	 *
	 *  - If we are running multiple GPlates versions simultaneously and the user wishes
	 *    to keep profile data for the old version around, it gets 'sandboxed' into
	 *    a path like "version/old/GPlates 0.9.10/" - calls to get_value and set_value
	 *    should seamlessly map to this location if on the old version. (NOT FULLY IMPLEMENTED)
	 */
	class UserPreferences :
			public GPlatesUtils::ConfigInterface
	{
		Q_OBJECT

	public:

		typedef QMap<QString, QVariant> KeyValueMap;

		explicit
		UserPreferences(
				QObject *_parent);

		virtual
		~UserPreferences();


		/**
		 * This should be your primary point of access for user preferences.
		 *
		 * Falls back to default value if not set.
		 */
		virtual
		QVariant
		get_value(
				const QString &key) const;

		/**
		 * Indicates if this key has been overriden from the defaults by the user
		 * (or potentially, by GPlates) and set in the user's platform's 'registry'.
		 *
		 * A key can exist and can return a value without having been 'set'.
		 */
		virtual
		bool
		has_been_set(
				const QString &key) const;

		/**
		 * Fetches default value directly - only useful for user preferences dialog.
		 */
		virtual
		QVariant
		get_default_value(
				const QString &key) const;

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
		virtual
		bool
		exists(
				const QString &key) const;

		/**
		 * Tests the existence of a compiled-in default key/value.
		 */
		virtual
		bool
		default_exists(
				const QString &key) const;

		/**
		 * Sets new user value, overriding any compiled-in defaults.
		 */
		virtual
		void
		set_value(
				const QString &key,
				const QVariant &value);

		/**
		 * Clears any user-set value, reverting to a default value if one exists.
		 * 
		 * If the key supplied is being used as a 'directory' (a common prefix of other
		 * keys) but there is no actual value set for it, nothing will happen.
		 *
		 * The implementation is slightly hackish due to how QSettings works, but
		 * it is included for sake of interface completeness and compatibility with
		 * ConfigBundle / ConfigInterface.
		 */
		virtual
		void
		clear_value(
				const QString &key);


		/**
		 * Clears any user-set value for all keys with the given prefix, reverting to
		 * a default value if one exists.
		 * 
		 * If the key supplied is being used as a 'directory' (a common prefix of other
		 * keys) then all those keys will be removed.
		 */
		virtual
		void
		clear_prefix(
				const QString &prefix);

		/**
		 * Lists all keys, including sub-keys, from the given prefix.
		 * Defaults to everything from the root ("").
		 *
		 * This will include key names from the defaults even if no explicitly-set
		 * value has been assigned by the user.
		 *
		 * For example, in the key structure below:-
		 *    parameters/plateid1/name
		 *    parameters/plateid1/type
		 *    parameters/fromage/name
		 *    parameters/fromage/type
		 *    parameters/toage/name
		 *    parameters/toage/type
		 *    colouring/style
		 *    colouring/mode
		 *    callbacks_ok
		 *
		 * Calling subkeys() will return the entire list of keys:- 
		 *    parameters/plateid1/name, parameters/plateid1/type, parameters/fromage/name,
		 *    parameters/fromage/type, parameters/toage/name, parameters/toage/type,
		 *    colouring/style, colouring/mode, callbacks_ok.
		 *
		 * Calling subkeys("parameters") will return only a subset:-
		 *    plateid1/name, plateid1/type, fromage/name, fromage/type, toage/name, toage/type.
		 */
		virtual
		QStringList
		subkeys(
				const QString &prefix = "") const;


		/**
		 * Lists all "root entries", or entries available for a given prefix.
		 * This is somewhat analagous to asking for a directory listing, although it would
		 * be a mistake to assume a ConfigBundle behaves identically to a file hierarchy.
		 *
		 * Essentially, it returns a list of possible prefixes for keys up to the first '/'
		 * character. This might be less important when we are dealing with proper UserPreferences,
		 * since in that case we know exactly what keys we wish to access and their full name.
		 * However, in the case of ConfigBundle it is entirely possible we are dealing with
		 * some user-set values, and might wish to know what groups of keys are available.
		 *
		 * For example, in the key structure below:-
		 *    parameters/plateid1/name
		 *    parameters/plateid1/type
		 *    parameters/fromage/name
		 *    parameters/fromage/type
		 *    parameters/toage/name
		 *    parameters/toage/type
		 *    colouring/style
		 *    colouring/mode
		 *    callbacks_ok
		 *
		 * Calling root_entries() will return (parameters, colouring, callbacks_ok),
		 * Calling root_entries("parameters") will return (plateid1, fromage, toage).
		 *
		 * Defaults to everything from the root ("").
		 *
		 * This will include key names from the defaults even if no explicitly-set
		 * value has been assigned by the user.
		 */
		virtual
		QStringList
		root_entries(
				const QString &prefix = "") const;


		/**
		 * Given a @a prefix to a set of keys, extract all those keys and
		 * values into a GPlatesUtils::ConfigBundle.
		 *
		 * This is intended to make working with groups of related sub-keys as
		 * a single "object" easier - for example, python colouring configuration.
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
		 * The ConfigBundle returned will be parented to the UserPreferences system,
		 * and cleaned up when GPlates exits. If you want it to have a shorter lifespan,
		 * use QObject::setParent().
		 *
		 * FIXME: Defaults handling?
		 */
		GPlatesUtils::ConfigBundle *
		extract_keyvalues_as_configbundle(
				const QString &prefix);
		

		/**
		 * Given a @a prefix in the key-value store, and a GPlatesUtils::ConfigBundle,
		 * set all the given keys in one pass.
		 *
		 * This is intended to make working with groups of related sub-keys as
		 * a single "object" easier - for example, python colouring configuration.
		 *
		 * All key names should have the prefix stripped - they will be
		 * "relative pathnames" from the given root. All pre-existing keys for that
		 * prefix are cleared before setting the new values.
		 */
		void
		insert_keyvalues_from_configbundle(
				const QString &prefix,
				const GPlatesUtils::ConfigBundle &bundle);


		
		
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
		virtual
		KeyValueMap
		get_keyvalues_as_map(
				const QString &prefix) const;
		

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
		virtual
		void
		set_keyvalues_from_map(
				const QString &prefix,
				const KeyValueMap &keyvalues);


	public Q_SLOTS:

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


	private:

		/**
		 * Configures preference keys for multiple-GPlates-version support.
		 */
		void
		initialise_versioning();

		/**
		 * Stores executable path of current application in user settings.                                                                    
		 */
		void
		store_executable_path();

		/**
		 * If this string ! .isNull(), all settings operations will be performed
		 * on a 'subdirectory' of the keystore - this is so that we can support
		 * simultaneous use of different gplates versions with different settings.
		 */
		QString d_key_root;


		/**
		 * Our default settings, loaded from a compiled-in resource file.
		 * (and includes a few 'magic' values generated at runtime)
		 *
		 * Why mutable? Because QSettings::beginGroup() and friends aren't declared const,
		 * but we want to use that in our subkeys() const method.
		 *
		 * Why bother with const correctness at all on UserPreferences, an app-logic object?
		 * because we share an interface with ConfigBundle, which might have a legitimate
		 * reason to want const methods.
		 */
		mutable QSettings d_defaults;
	};
}

#endif // GPLATES_APP_LOGIC_USERPREFERENCES_H
