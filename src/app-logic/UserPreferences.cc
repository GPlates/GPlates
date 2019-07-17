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

#include <QDebug>
#include <QtAlgorithms>
#include <QCoreApplication>
#include <QSet>
#include <QList>
#include <QSettings>

// For magic defaults:-
#include <QDir>
#include <QDesktopServices>
#include <QUrl>
// Automatic proxy detection needs Qt 4.5, so using a conditional compile (see set_magic_defaults for more info)
#if QT_VERSION >= 0x040500
	#include <QNetworkProxyQuery>
	#include <QNetworkProxyFactory>
	#include <QNetworkProxy>
	#include <QtNetwork>
#endif


#include "UserPreferences.h"

#include "global/Constants.h"
#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "utils/ConfigBundle.h"
#include "utils/ConfigBundleUtils.h"
#include "utils/NetworkUtils.h"
#include "utils/Environment.h"

//initialise static class memebers
bool GPlatesAppLogic::UserPreferences::s_is_defaults_initialised = false;
QSettings GPlatesAppLogic::UserPreferences::s_defaults(
		":/DefaultPreferences.conf", 
		QSettings::IniFormat);

namespace
{
	/**
	 * Sets "magic" default preference values that are derived from system calls.
	 */
	void
	set_magic_defaults(
			QSettings &defaults)
	{
		////////////////////////////////
		// PATHS:-
		////////////////////////////////

		// paths/python_user_script_dir :-
		//
		// Get the platform-specific "application user data" dir. Add "scripts/" to that.
		//   Linux: ~/.local/share/data/GPlates/GPlates/
		//   Windows 7: C:/Users/*/AppData/Local/GPlates/GPlates/
		QDir local_scripts_dir(QDesktopServices::storageLocation(QDesktopServices::DataLocation) + "/scripts/");
		defaults.setValue("paths/python_user_script_dir", QVariant(local_scripts_dir.absolutePath()));

		// paths/python_system_script_dir :-
		//
		// Going to have to #ifdef this per-platform for now, there's no 'nice' way to query this through Qt.
#if defined(Q_OS_LINUX)
		defaults.setValue("paths/python_system_script_dir", "/usr/share/gplates/scripts");

#elif defined(Q_OS_MAC)
		// While in theory the place for this would be in "/Library/Application Data" somewhere, we don't use
		// a .pkg and don't want to "install" stuff - OSX users much prefer drag-n-drop .app bundles. So,
		// the sample scripts resource would probably best be added to the bundle.
		QDir app_scripts_dir(QCoreApplication::applicationDirPath() + "/../Resources/scripts/");
		defaults.setValue("paths/python_system_script_dir", QVariant(app_scripts_dir.absolutePath()));

#elif defined(Q_OS_WIN32)
		// The Windows Installer should drop a scripts/ directory in whatever Program Files area the gplates.exe
		// file lands in.
		QDir progfile_scripts_dir(QCoreApplication::applicationDirPath() + "/scripts/");
		defaults.setValue("paths/python_system_script_dir", QVariant(progfile_scripts_dir.absolutePath()));
#else
		// Er. Look for the current directory?
		defaults.setValue("paths/python_system_script_dir", "scripts/");
#endif

		// paths/default_export_dir :-
		//
		// Get the platform-specific "Documents" dir. 
		//   Linux and OSX: ~/Documents/
		defaults.setValue(
				"paths/default_export_dir", 
				QDir(QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation)).absolutePath());
		defaults.setValue(
					"paths/default_feature_collection_dir",
					QDir::currentPath());
		defaults.setValue(
					"paths/default_project_dir",
					QDir(QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation)).absolutePath());


		////////////////////////////////
		// NET:-
		////////////////////////////////
		
		// net/proxy/url :-
		//
		// Requires Qt 4.5, which is above our minimum, so I'm putting it in as a conditional
		// compilation bonus for those that have it. It's only useful on OSX and Windows anyway,
		// where we're bundling the libraries each time anyway.
		//
		// Take any system-supplied proxy information and try to smoosh it into a 'url'
		// style form that we can store as a string. The user can then override this string
		// if they want to use a different proxy (or if the system does not supply proxy
		// information to applications).
		//
		// We should probably write some Utils code or something to then extract this url
		// string whenever it changes and update the global Qt-wide proxy setting.
		// See also: net/proxy/enabled
		defaults.setValue("net/proxy/url", "");
		
		// GPlates will use information from Qt where it can (OSX Frameworks and Win32 DLLs)
#if QT_VERSION >= 0x040500

		QUrl gplates_url("http://www.gplates.org");

		// The following code enclosed in the "#if defined(Q_WS_MAC)" preprocessor directive provides 
		// a workaround for a bug on MacOS. The bug causes GPlates fails to launch under certain circumstance. 
		// On MacOS, when the network interface appears active but in fact
		// the computer does not have a valid network connection, the QNetworkProxyFactory::systemProxyForQuery() 
		// function refuses to return and waits for a working network connection indefinitely. 
		// In order to workaround the bug, we need to check the network availability
		// on MacOS before calling QNetworkProxyFactory::systemProxyForQuery().
		// The bug was reported by sabin.zahirovic@sydney.edu.au and verified independently by 
		// michael.chin@sydney.edu.au.
		// To reproduce the bug on MacOS:
		// Step 1: make sure the network interface is active
		// Step 2: enable "Auto Proxy Discovery"
		// Step 3: make the network unavailable. There are several ways to complete this step. 
		// 	 	   One simple way is to mess up your DNS setting. Or when practical, unplug the network cable 
		// 		   on your Wi-Fi router.
		// Without this workaround, GPlates will fail to launch and hang indefinitely during startup.
		// With this workaround, GPlates will wait the "network timeout" and decide there is really
		// no valid network connection, skip "proxy querying" and launch normally. 
		// Although the startup will be slower than normal, GPlates will launch successfully eventually. 
#if defined(Q_WS_MAC)
		QNetworkAccessManager nam;
		QNetworkRequest req(gplates_url);
		QNetworkReply* reply = nam.get(req);
		QEventLoop loop;
		QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
		loop.exec();	
		if (reply->error() == QNetworkReply::NoError){//network is good, go ahead
#endif		
			QNetworkProxyQuery network_proxy_query(gplates_url);
			QList<QNetworkProxy> network_proxy_list = QNetworkProxyFactory::systemProxyForQuery(network_proxy_query);
		
			if (network_proxy_list.size() > 0) {
				QUrl system_proxy_url = GPlatesUtils::NetworkUtils::get_url_for_proxy(network_proxy_list.first());
				if (system_proxy_url.isValid() && ! system_proxy_url.host().isEmpty() && system_proxy_url.port() > 0) {
					defaults.setValue("net/proxy/url", system_proxy_url);
				}
			}
#if defined(Q_WS_MAC)
		}else{//network no good, skip network proxy query and print a warning message.
			qWarning() << "No available network has been detected! Will not query network proxy.";
		}
#endif
#endif
		
		// GPlates will override that default with the "http_proxy" environment variable if it is set.
		QString environment_proxy_url = GPlatesUtils::getenv("http_proxy");
		if ( ! environment_proxy_url.isEmpty()) {
			defaults.setValue("net/proxy/url", environment_proxy_url);
		}
		
		// net/proxy/enabled :-
		//
		// If we've found a suitable proxy from the system or environment, enable it by default.
		defaults.setValue("net/proxy/enabled", ! defaults.value("net/proxy/url").toString().isEmpty());

		defaults.setValue("net/server/port", 9777);
		defaults.setValue("net/server/local", true);
		
	}
}


GPlatesAppLogic::UserPreferences::UserPreferences(
		QObject *_parent):
	GPlatesUtils::ConfigInterface(_parent)
{
	// Initialise names used to identify our settings and paths in the OS.
	// DO NOT CHANGE THESE VALUES without due consideration to the breaking of previously used
	// QDesktopServices paths and preference settings.
	QCoreApplication::setOrganizationName("GPlates");
	QCoreApplication::setOrganizationDomain("gplates.org");
	QCoreApplication::setApplicationName("GPlates");
	
	initialise_versioning();
	store_executable_path();
	
	// Set some default values that cannot be hard-coded, but are instead generated
	// at runtime.
	if( !s_is_defaults_initialised )//ensure the "s_defaults" will not be initialised more than once
	{
		set_magic_defaults(s_defaults);
		s_is_defaults_initialised = true;
	}
}


GPlatesAppLogic::UserPreferences::~UserPreferences()
{
	// Ensure everything is flushed to persistent storage before we quit.
	QSettings settings;
	settings.sync();
}


QVariant
GPlatesAppLogic::UserPreferences::get_value(
		const QString &key) const
{
	// Note that QSettings destructor calls 'sync()' to:
	//  (1) Write unsaved changes to persistent storage, and
	//  (2) Reload any settings that have been changed in the meantime by another application (GPlates).
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
		const QString &key) const
{
	// Note that QSettings destructor calls 'sync()' to:
	//  (1) Write unsaved changes to persistent storage, and
	//  (2) Reload any settings that have been changed in the meantime by another application (GPlates).
	QSettings settings;
	if ( ! d_key_root.isNull()) {
		settings.beginGroup(d_key_root);
	}
	
	return settings.contains(key);
}

QVariant
GPlatesAppLogic::UserPreferences::get_default_value(
		const QString &key) const
{
	if (default_exists(key)) {
		return s_defaults.value(key);
	} else {
		return QVariant();
	}
}


bool
GPlatesAppLogic::UserPreferences::exists(
		const QString &key) const
{
	// Note that QSettings destructor calls 'sync()' to:
	//  (1) Write unsaved changes to persistent storage, and
	//  (2) Reload any settings that have been changed in the meantime by another application (GPlates).
	QSettings settings;
	if ( ! d_key_root.isNull()) {
		settings.beginGroup(d_key_root);
	}
	
	return settings.contains(key) || default_exists(key);
}


bool
GPlatesAppLogic::UserPreferences::default_exists(
		const QString &key) const
{
	return s_defaults.contains(key);
}


void
GPlatesAppLogic::UserPreferences::set_value(
		const QString &key,
		const QVariant &value)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			key.startsWith('/') == false,
			GPLATES_ASSERTION_SOURCE);

	// Note that QSettings destructor calls 'sync()' to:
	//  (1) Write unsaved changes to persistent storage, and
	//  (2) Reload any settings that have been changed in the meantime by another application (GPlates).
	QSettings settings;
	if ( ! d_key_root.isNull()) {
		settings.beginGroup(d_key_root);
	}

	QVariant orig_value = settings.value(key);
	settings.setValue(key, value);
	if (orig_value != value) {
		Q_EMIT key_value_updated(key);
	}
}


void
GPlatesAppLogic::UserPreferences::clear_value(
		const QString &key)
{
	// Note that QSettings destructor calls 'sync()' to:
	//  (1) Write unsaved changes to persistent storage, and
	//  (2) Reload any settings that have been changed in the meantime by another application (GPlates).
	QSettings settings;
	if ( ! d_key_root.isNull()) {
		settings.beginGroup(d_key_root);
	}
	
	// We need a bit of a hack to remove a single value and only that value safely
	// in UserPreferences (leaving potential 'sub keys' alone).
	// Unlike ConfigBundle, calling settings.remove(key) always removes everything
	// with that prefix, so we have to emulate removing a single key by removing
	// and then silently replacing subkeys.
	KeyValueMap backup = get_keyvalues_as_map(key);
	settings.remove(key);

	Q_FOREACH(QString subkey, backup.keys()) {
		QString fullkey = GPlatesUtils::compose_keyname(key, subkey);
		settings.setValue(fullkey, backup.value(subkey));
	}
	
	Q_EMIT key_value_updated(key);
}


void
GPlatesAppLogic::UserPreferences::clear_prefix(
		const QString &prefix)
{
	// Note that QSettings destructor calls 'sync()' to:
	//  (1) Write unsaved changes to persistent storage, and
	//  (2) Reload any settings that have been changed in the meantime by another application (GPlates).
	QSettings settings;
	if ( ! d_key_root.isNull()) {
		settings.beginGroup(d_key_root);
	}
	
	settings.remove(prefix);
	Q_EMIT key_value_updated(prefix);	// FIXME: Might not be doing what we want, may have to emit multiple signals.
}


QStringList
GPlatesAppLogic::UserPreferences::subkeys(
		const QString &prefix) const
{
	// Note that QSettings destructor calls 'sync()' to:
	//  (1) Write unsaved changes to persistent storage, and
	//  (2) Reload any settings that have been changed in the meantime by another application (GPlates).
	QSettings settings;
	if ( ! d_key_root.isNull()) {
		settings.beginGroup(d_key_root);
	}

	// Take the explicitly-set (or visible from the OS) keys,
	settings.beginGroup(prefix);
	QSet<QString> keys = settings.allKeys().toSet();
	settings.endGroup();
	
	// and the compiled-in default keys,
	s_defaults.beginGroup(prefix);
	QSet<QString> keys_default = s_defaults.allKeys().toSet();
	s_defaults.endGroup();

	// and merge them together to get the full list of possible keys.
	keys.unite(keys_default);

	// And for presentation purposes it would be nice to get that sorted.
	QStringList list = keys.toList();
	qSort(list);
	return list;
}


QStringList
GPlatesAppLogic::UserPreferences::root_entries(
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


GPlatesUtils::ConfigBundle *
GPlatesAppLogic::UserPreferences::extract_keyvalues_as_configbundle(
		const QString &prefix)
{
	QStringList keys = subkeys(prefix);
	GPlatesUtils::ConfigBundle *bundle = new GPlatesUtils::ConfigBundle(this);
	
	Q_FOREACH(QString subkey, keys) {
		QString fullkey = GPlatesUtils::compose_keyname(prefix, subkey);
		bundle->set_value(subkey, get_value(fullkey));
	}
	return bundle;	// Memory handled by Qt, object parented to UserPreferences.
}


void
GPlatesAppLogic::UserPreferences::insert_keyvalues_from_configbundle(
		const QString &prefix,
		const GPlatesUtils::ConfigBundle &bundle)
{
	clear_prefix(prefix);
	Q_FOREACH(QString subkey, bundle.subkeys()) {
		QString fullkey = GPlatesUtils::compose_keyname(prefix, subkey);
		set_value(fullkey, bundle.get_value(subkey));
	}
}


GPlatesAppLogic::UserPreferences::KeyValueMap
GPlatesAppLogic::UserPreferences::get_keyvalues_as_map(
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
GPlatesAppLogic::UserPreferences::set_keyvalues_from_map(
		const QString &prefix,
		const GPlatesAppLogic::UserPreferences::KeyValueMap &keyvalues)
{
	clear_prefix(prefix);
	Q_FOREACH(QString subkey, keyvalues.keys()) {
		QString fullkey = GPlatesUtils::compose_keyname(prefix, subkey);
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
	qDebug() << "GPlates Defaults:" << s_defaults.fileName();
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
	// Note that QSettings destructor calls 'sync()' to:
	//  (1) Write unsaved changes to persistent storage, and
	//  (2) Reload any settings that have been changed in the meantime by another application (GPlates).
	QSettings raw_settings;
	
	// Record the most recent version of GPlates that has been run on the user's machine.
	// FIXME: Ideally this should not overwrite if existing version >= current version,
	// and also trigger version upgrade or version sandbox as appropriate...
	// ... which we may not get around to implementing.
	raw_settings.setValue("version/current", GPlatesGlobal::VersionString);
}

void
GPlatesAppLogic::UserPreferences::store_executable_path()
{
	// Note that QSettings destructor calls 'sync()' to:
	//  (1) Write unsaved changes to persistent storage, and
	//  (2) Reload any settings that have been changed in the meantime by another application (GPlates).
	QSettings settings;

	// Record the executable path to this application. 
	settings.setValue("paths/executables/gplates/last_used",QCoreApplication::applicationFilePath());
}
