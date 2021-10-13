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

#include <QDebug>
#include <QtAlgorithms>
#include <QCoreApplication>
#include <QSet>
#include <QSettings>
#include <QDir>
#include <QDesktopServices>

#include "UserPreferences.h"

#include "global/Constants.h"
#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"


namespace
{
	/**
	 * Necessary when dealing with generated key names.
	 */
	QString
	sanitise_key(
			const QString &key_with_slashes)
	{
		QString sane = key_with_slashes;
		sane.replace('/', '_');
		return sane;
	}

	/**
	 * Returns "magic" default preference values that are derived from system calls.
	 * Returns null QVariant if no such magic key exists.
	 */
	QVariant
	get_magic_default_value(
			const QString &key)
	{
		// Yes, an if/else chain is a slightly ugly way to do this, but I want to ensure that
		// the system calls get invoked *each time* the preference value is asked for, so that
		// in the case of e.g. user changing their Location and therefore their proxy details
		// in the middle of a GPlates session, GPlates still works seamlessly. Maybe that's overkill.
		// FIXME: In fact, yeah, it's probably overkill. Make a note to change this to a member-data
		// QSettings object that does not correspond to a file or resource, that we can load these
		// magic key/values into and then merge in with all the other keys. Maybe around the same
		// time we do the UI for the preferences.

		if (key == "paths/python_user_script_dir") {
			// Get the platform-specific "application user data" dir. Add "scripts/" to that.
			//   Linux: ~/.local/share/data/GPlates/GPlates/
			//   Windows 7: C:/Users/*/AppData/Local/GPlates/GPlates/
			QDir local_scripts_dir(QDesktopServices::storageLocation(QDesktopServices::DataLocation) + "/scripts/");
			return QVariant(local_scripts_dir.absolutePath());

		} else if (key == "paths/python_system_script_dir") {
			// Going to have to #ifdef this per-platform for now, there's no 'nice' way to query this through Qt.
#if defined(Q_OS_LINUX)
			return "/usr/share/gplates/scripts";

#elif defined(Q_OS_MAC)
			// While in theory the place for this would be in "/Library/Application Data" somewhere, we don't use
			// a .pkg and don't want to "install" stuff - OSX users much prefer drag-n-drop .app bundles. So,
			// the sample scripts resource would probably best be added to the bundle.
			QDir app_scripts_dir(QCoreApplication::applicationDirPath() + "/../Resources/scripts/");
			return QVariant(app_scripts_dir.absolutePath());

#elif defined(Q_OS_WIN32)
			// The Windows Installer should drop a scripts/ directory in whatever Program Files area the gplates.exe
			// file lands in.
			QDir progfile_scripts_dir(QCoreApplication::applicationDirPath() + "/scripts/");
			return QVariant(progfile_scripts_dir.absolutePath());
#else
			return "scripts/";
#endif

		} else {
			// No such magic key exists.
			return QVariant();
		}
	}
}


GPlatesAppLogic::UserPreferences::UserPreferences():
	d_defaults(":/DefaultPreferences.conf", QSettings::IniFormat)
{
	// Initialise names used to identify our settings and paths in the OS.
	// DO NOT CHANGE THESE VALUES without due consideration to the breaking of previously used
	// QDesktopServices paths and preference settings.
	QCoreApplication::setOrganizationName("GPlates");
	QCoreApplication::setOrganizationDomain("gplates.org");
	QCoreApplication::setApplicationName("GPlates");
	
	initialise_versioning();
}


GPlatesAppLogic::UserPreferences::~UserPreferences()
{
	// Ensure everything is flushed to persistent storage before we quit.
	QSettings settings;
	settings.sync();
}


QVariant
GPlatesAppLogic::UserPreferences::get_value(
		const QString &key)
{
	QSettings settings;
	if ( ! d_key_root.isNull()) {
		settings.beginGroup(d_key_root);
	}
	
	if (settings.contains(key)) {
		return settings.value(key);
	} else {
		return get_default_value(key);
	}
}

bool
GPlatesAppLogic::UserPreferences::has_been_set(
		const QString &key)
{
	QSettings settings;
	if ( ! d_key_root.isNull()) {
		settings.beginGroup(d_key_root);
	}
	
	return settings.contains(key);
}

QVariant
GPlatesAppLogic::UserPreferences::get_default_value(
		const QString &key)
{
	if (default_exists(key)) {
		return d_defaults.value(key);
	} else {
		return get_magic_default_value(key);
	}
}

bool
GPlatesAppLogic::UserPreferences::exists(
		const QString &key)
{
	QSettings settings;
	if ( ! d_key_root.isNull()) {
		settings.beginGroup(d_key_root);
	}
	
	return settings.contains(key) || default_exists(key);
}

void
GPlatesAppLogic::UserPreferences::set_value(
		const QString &key,
		const QVariant &value)
{
	QSettings settings;
	if ( ! d_key_root.isNull()) {
		settings.beginGroup(d_key_root);
	}

	QVariant orig_value = settings.value(key);
	settings.setValue(key, value);
	if (orig_value != value) {
		emit key_value_updated(key);
	}
}

void
GPlatesAppLogic::UserPreferences::clear_value(
		const QString &key)
{
	QSettings settings;
	if ( ! d_key_root.isNull()) {
		settings.beginGroup(d_key_root);
	}
	
	settings.remove(key);
	emit key_value_updated(key);
}

QStringList
GPlatesAppLogic::UserPreferences::subkeys(
		const QString &prefix)
{
	QSettings settings;
	if ( ! d_key_root.isNull()) {
		settings.beginGroup(d_key_root);
	}

	// Take the explicitly-set (or visible from the OS) keys,
	settings.beginGroup(prefix);
	QSet<QString> keys = settings.allKeys().toSet();
	settings.endGroup();
	
	// and the compiled-in default keys,
	d_defaults.beginGroup(prefix);
	QSet<QString> keys_default = d_defaults.allKeys().toSet();
	d_defaults.endGroup();

	// FIXME: Include magic default keys in a better way, see @a get_magic_default_value() above.
	keys_default.insert("paths/python_user_script_dir");
	keys_default.insert("paths/python_system_script_dir");

	// and merge them together to get the full list of possible keys.
	keys.unite(keys_default);

	// TODO: Filter out all the /_description keys we get from the defaults.
	
	// And for presentation purposes it would be nice to get that sorted.
	QStringList list = keys.toList();
	qSort(list);
	return list;
}


GPlatesAppLogic::UserPreferences::KeyValueMap
GPlatesAppLogic::UserPreferences::get_keyvalues_as_map(
		const QString &prefix)
{
	QStringList keys = subkeys(prefix);
	KeyValueMap map;
	
	Q_FOREACH(QString subkey, keys) {
		QString fullkey = QString("%1/%2").arg(prefix, subkey);
		map.insert(subkey, get_value(fullkey));
	}
	return map;	// Is a Qt container, uses pimpl idiom, is fine to return by value.
}


void
GPlatesAppLogic::UserPreferences::set_keyvalues_from_map(
		const QString &prefix,
		const GPlatesAppLogic::UserPreferences::KeyValueMap &keyvalues)
{
	clear_value(prefix);
	Q_FOREACH(QString subkey, keyvalues.keys()) {
		QString fullkey = QString("%1/%2").arg(prefix, subkey);
		set_value(fullkey, keyvalues.value(subkey));
	}
}


void
GPlatesAppLogic::UserPreferences::debug_file_locations()
{
	// The default location:-
	QSettings settings_user_app;
	// It is necessary to pull these names out of the default settings as our
	// organization name is "gplates.org" on OSX and "GPlates" everywhere else,
	// to be consistent with the platform conventions.
	QString org_name = settings_user_app.organizationName();
	QString app_name = settings_user_app.applicationName();
	
	// Tweaked locations for global-organization settings & operating-system-wide settings.
	QSettings settings_user_org(org_name);
	QSettings settings_system_app(QSettings::SystemScope, org_name, app_name);
	QSettings settings_system_org(QSettings::SystemScope, org_name);

	qDebug() << "UserPreferences file locations:-";
	qDebug() << "User/App:" << settings_user_app.fileName();
	qDebug() << "User/Org:" << settings_user_org.fileName();
	qDebug() << "System/App:" << settings_system_app.fileName();
	qDebug() << "System/Org:" << settings_system_org.fileName();
	qDebug() << "GPlates Defaults:" << d_defaults.fileName();
}

void
GPlatesAppLogic::UserPreferences::debug_key_values()
{
	QStringList keys = subkeys();

	qDebug() << "UserPreferences key values:-";
	Q_FOREACH(QString key, keys) {
		QVariant value = get_value(key);
		const char *overridden = has_been_set(key) ? "U" : " ";
		const char *has_default = default_exists(key) ? "D" : " ";
		qDebug() << overridden << has_default << key << "=" << value;
	}
}


void
GPlatesAppLogic::UserPreferences::initialise_versioning()
{
	QSettings raw_settings;
	
	// Record the most recent version of GPlates that has been run on the user's machine.
	// FIXME: Ideally this should not overwrite if existing version >= current version,
	// and also trigger version upgrade or version sandbox as appropriate.
	raw_settings.setValue("version/current", GPlatesGlobal::VersionString);
}


bool
GPlatesAppLogic::UserPreferences::default_exists(
		const QString &key)
{
	return d_defaults.contains(key);
}
