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

#ifndef GPLATES_UTILS_CONFIGBUNDLEUTILS_H
#define GPLATES_UTILS_CONFIGBUNDLEUTILS_H

#include <QString>
#include <QStringList>
#include <QWidget>
#include <QTableView>


namespace GPlatesUtils
{
	class ConfigInterface;

	/**
	 * Given a ConfigBundle (or UserPreferences) and parent widget, create a QTableView that
	 * is linked to the ConfigBundle; changes in one will be reflected in the other.
	 *
	 * @param bundle - the bundle of key/value pairs.
	 * @param parent - a QWidget to serve as the parent for the returned QTableView widget.
	 *                 This is to ensure memory will be cleaned up appropriately; it is up to
	 *                 you to insert the widget into a layout somewhere.
	 */
	QTableView *
	link_config_interface_to_table(
			ConfigInterface &config,
			QWidget *parent);
	

	////////////////////////////////////////////////////////////////////////
	// What follows are a bunch of small utility functions used by
	// UserPreferences and ConfigBundle, formerly in the anon namespace
	// but potentially useful to those who wish to do advanced manipulation
	// of key names.
	////////////////////////////////////////////////////////////////////////
	
	/**
	 * Necessary when dealing with generated key names.
	 * Replaces any slash ('/') characters with underscores ('_').
	 *
	 * (^_^)v
	 */
	QString
	sanitise_key(
			const QString &key_with_slashes);


	/**
	 * Returns a new list of only those key names that match a given prefix.
	 * The key names are otherwise unchanged.
	 */
	QStringList
	match_prefix(
			const QStringList &keys,
			const QString &prefix);


	/**
	 * Modifies a list of key names to strip off a given prefix.
	 */
	void
	strip_prefix(
			QStringList &keys,
			const QString &prefix);


	/**
	 * Modifies a list of key names to strip off everything past the first
	 * '/' character, if any.
	 */
	void
	strip_all_except_root(
			QStringList &keys);


	/**
	 * Intelligently concatenates a prefix with a (part of a) key name,
	 * inserting a '/' only if appropriate.
	 */
	QString
	compose_keyname(
			const QString &prefix,
			const QString &subkey);

}

#endif // GPLATES_UTILS_CONFIGBUNDLEUTILS_H
