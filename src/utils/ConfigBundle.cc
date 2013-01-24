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

#include <QDebug>
#include <QtAlgorithms>
#include <QRegExp>
#include <QSet>

#include "ConfigBundle.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "utils/ConfigBundleUtils.h"


GPlatesUtils::ConfigBundle::ConfigBundle(
		QObject *_parent):
	ConfigInterface(_parent)
{  }


GPlatesUtils::ConfigBundle::~ConfigBundle()
{  }


QVariant
GPlatesUtils::ConfigBundle::get_value(
		const QString &key) const
{
	if (d_map.contains(key)) {
		return d_map.value(key);
	} else {
		return get_default_value(key);
	}
}


bool
GPlatesUtils::ConfigBundle::has_been_set(
		const QString &key) const
{
	return d_map.contains(key);
}


QVariant
GPlatesUtils::ConfigBundle::get_default_value(
		const QString &key) const
{
	if (default_exists(key)) {
		return d_defaults.value(key);
	} else {
		return QVariant();
	}
}


bool
GPlatesUtils::ConfigBundle::exists(
		const QString &key) const
{
	return d_map.contains(key) || default_exists(key);
}

bool
GPlatesUtils::ConfigBundle::default_exists(
		const QString &key) const
{
	return d_defaults.contains(key);
}


void
GPlatesUtils::ConfigBundle::set_value(
		const QString &key,
		const QVariant &value)
{
	QVariant orig_value = d_map.value(key);
	d_map.insert(key, value);
	if (orig_value != value) {
		Q_EMIT key_value_updated(key);
	}
}

void
GPlatesUtils::ConfigBundle::set_default_value(
		const QString &key,
		const QVariant &value)
{
	QVariant orig_visible_value = get_value(key);
	d_defaults.insert(key, value);
	if (orig_visible_value != get_value(key)) {
		Q_EMIT key_value_updated(key);
	}
}


void
GPlatesUtils::ConfigBundle::clear_value(
		const QString &key)
{
	d_map.remove(key);
	Q_EMIT key_value_updated(key);
}

void
GPlatesUtils::ConfigBundle::clear_prefix(
		const QString &prefix)
{
	// Take the explicitly-set keys (which match the prefix),
	QStringList keys_to_remove = GPlatesUtils::match_prefix(d_map.keys(), prefix);
	
	// And remove each one.
	Q_FOREACH(QString key, keys_to_remove) {
		d_map.remove(key);
		Q_EMIT key_value_updated(key);
	}
}


QStringList
GPlatesUtils::ConfigBundle::subkeys(
		const QString &prefix) const
{
	// Take the explicitly-set keys (which match the prefix),
	QSet<QString> keys = GPlatesUtils::match_prefix(d_map.keys(), prefix).toSet();
	
	// and the compiled-in default keys,
	QSet<QString> keys_default = GPlatesUtils::match_prefix(d_defaults.keys(), prefix).toSet();

	// and merge them together to get the full list of possible keys.
	keys.unite(keys_default);

	// If a prefix was requested, the resulting key names should have it removed.
	QStringList list = keys.toList();
	GPlatesUtils::strip_prefix(list, prefix);
	return list;
}


QStringList
GPlatesUtils::ConfigBundle::root_entries(
		const QString &prefix) const
{
	// First get the full 'pathname' keys within that prefix, with the prefix stripped.
	QStringList keys = subkeys(prefix);
	
	// Strip off everything past the first '/', if any.
	GPlatesUtils::strip_all_except_root(keys);
	
	// Push them through a QSet to get rid of duplicates.
	keys = keys.toSet().toList();

	return keys;
}


GPlatesUtils::ConfigBundle::KeyValueMap
GPlatesUtils::ConfigBundle::get_keyvalues_as_map(
		const QString &prefix) const
{
	QStringList keys = subkeys(prefix);
	KeyValueMap map;
	
	Q_FOREACH(QString subkey, keys) {
		QString fullkey = GPlatesUtils::compose_keyname(prefix, subkey);
		map.insert(subkey, get_value(fullkey));
	}
	return map;	// Is a Qt container, uses pimpl idiom, is fine to return by value.
}


void
GPlatesUtils::ConfigBundle::set_keyvalues_from_map(
		const QString &prefix,
		const GPlatesUtils::ConfigBundle::KeyValueMap &keyvalues)
{
	clear_prefix(prefix);
	Q_FOREACH(QString subkey, keyvalues.keys()) {
		QString fullkey = GPlatesUtils::compose_keyname(prefix, subkey);
		set_value(fullkey, keyvalues.value(subkey));
	}
}


