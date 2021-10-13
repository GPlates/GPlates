/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#ifndef GPLATES_UTILS_CONFIGBUNDLE_H
#define GPLATES_UTILS_CONFIGBUNDLE_H

#include <boost/noncopyable.hpp>
#include <QObject>
#include <QPointer>
#include <QVariant>
#include <QString>
#include <QStringList>
#include <QMap>

#include "ConfigInterface.h"


namespace GPlatesUtils
{
	/**
	 * A small, portable collection of key-value pairs that can be used independently
	 * of GPlates' UserPreferences system; changes to keys will emit appropriate
	 * update signals but the 'bundle' of key-value pairs may be transient and not
	 * persistently saved to a preferences file somewhere.
	 *
	 * A few handy guidelines (cribbed from GPlatesAppLogic::UserPreferences):-
	 *
	 *  - Keys are set using a hierarchy with a unix-like '/' path delimiter.
	 *    There is NO initial '/' as the first character.
	 *
	 *  - Treat keys as though they were case-sensitive, because they might be.
	 *
	 *  - Prefer a lowercased naming scheme with underscores to separate words.
	 *
	 *  - Values get stored as a QVariant. Depending on the backend, they may get
	 *    stringified and you might notice the 'type' of them being a QString upon re-load.
	 *    Don't let this bother you - store a QVariant(56) and get it back as the int you
	 *    would expect with .toInt(), Qt does the rest.
	 *
	 *  - There is a 'defaults' system for this ConfigBundle, but it is optional as to
	 *    whether you want to use it, unlike UserPreferences. Explicitly 'set' values always
	 *    shadow a default value, and conversely if no 'user' value is set for a key then
	 *    get_value() will return the default value. The defaults may have an effect on
	 *    the presentation of UI elements, so the user knows when they've changed something.
	 */
	class ConfigBundle :
			public ConfigInterface
	{
		Q_OBJECT

	public:

		/**
		 * Constructor for an empty ConfigBundle.
		 * 
		 * Note that since we use signals and slots, this needs to be a QObject,
		 * and so should (ideally) be created with a QObject parent that can manage
		 * the memory.
		 */
		explicit
		ConfigBundle(
				QObject *_parent);

		virtual
		~ConfigBundle();


		/**
		 * This should be your primary point of access for key values.
		 *
		 * If the key does not exist, you will get a "null" QVariant. In most cases
		 * this will be fine to use and will convert to 0, 0.0, or "" as appropriate.
		 */
		virtual
		QVariant
		get_value(
				const QString &key) const;

		/**
		 * Indicates if this key has been overriden from the defaults by the user
		 * (or potentially, by GPlates) and set in the config bundle.
		 *
		 * A key can exist and can return a value without having been 'set'.
		 */
		virtual
		bool
		has_been_set(
				const QString &key) const;

		/**
		 * Fetches default value directly - only useful for user interactions.
		 */
		virtual
		QVariant
		get_default_value(
				const QString &key) const;

		/**
		 * Indicates if this key exists in any form, from this config bundle or some sort
		 * of defaults provided from another bundle linked to this one.
		 *
		 * Note: This only checks if a key/value pair has been set for the given
		 * name. It is possible to have "directories" which have no values associated with
		 * them, use only to sub-divide things. @a exists will return @a false if you
		 * ask about such key-paths.
		 */
		virtual
		bool
		exists(
				const QString &key) const;

		/**
		 * Tests the existence of an assigned default key/value.
		 */
		virtual
		bool
		default_exists(
				const QString &key) const;


		/**
		 * Sets new user value, overriding any default that may or may not exist for that key.
		 */
		virtual
		void
		set_value(
				const QString &key,
				const QVariant &value);


		/**
		 * Sets new default value, which may be shadowed by a 'user set' key.
		 *
		 * Only applicable for ConfigBundle.
		 */
		virtual
		void
		set_default_value(
				const QString &key,
				const QVariant &value);


		/**
		 * Clears any user-set value, reverting to a default value if one exists.
		 * 
		 * If the key supplied is being used as a 'directory' (a common prefix of other
		 * keys) but there is no actual value set for it, nothing will happen.
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
		 * Given a @a prefix to a set of keys, slurp all those keys and
		 * values into a QMap<QString, QVariant>.
		 * This is intended to make working with groups of related sub-keys as
		 * a single "object" easier - for example, storing a GPlatesUtils::Session.
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
		 * a single "object" easier - for example, storing a GPlatesUtils::Session.
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

	private:

		/**
		 * Our insternal storage for the config.
		 */
		KeyValueMap d_map;

		/**
		 * Some defaults to fall back to - optional.
		 */
		KeyValueMap d_defaults;
	};
}

#endif // GPLATES_UTILS_CONFIGBUNDLE_H
