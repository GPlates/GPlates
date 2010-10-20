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
}


GPlatesAppLogic::UserPreferences::UserPreferences():
	d_defaults(":/DefaultPreferences.conf", QSettings::IniFormat)
{
	// Initialise names used to identify our settings in the OS preference system.
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
		return QVariant();
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
	
	// and merge them together to get the full list of possible keys.
	keys.unite(keys_default);

	// TODO: Filter out all the /_description keys we get from the defaults.
	
	// And for presentation purposes it would be nice to get that sorted.
	QStringList list = keys.toList();
	qSort(list);
	return list;
}



void
GPlatesAppLogic::UserPreferences::debug_file_locations()
{
	QSettings settings_user_app;
	QSettings settings_user_org(QCoreApplication::organizationName());
	QSettings settings_system_app(QSettings::SystemScope, QCoreApplication::organizationName(), 
			QCoreApplication::applicationName());
	QSettings settings_system_org(QSettings::SystemScope, QCoreApplication::organizationName());

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
