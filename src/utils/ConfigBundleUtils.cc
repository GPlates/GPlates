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

#include <QRegExp>
#include <QHeaderView>

#include "ConfigBundleUtils.h"

#include "ConfigInterface.h"
#include "ConfigModel.h"

#include "qt-widgets/PreferencesDialog.h"


GPlatesQtWidgets::ConfigTableView *
GPlatesUtils::link_config_interface_to_table(
		ConfigInterface &config,
		QWidget *parent)
{
	// We allocate the memory for this new table widget, and give it the parent supplied
	// by the caller so that Qt will handle cleanup of said memory.
	GPlatesQtWidgets::ConfigTableView *tableview_ptr = new GPlatesQtWidgets::ConfigTableView(parent);
		
	// We also create a ConfigModel to act as the intermediary between ConfigBundle and
	// the table, and parent that to the table view widget so that it also gets cleaned
	// up when appropriate.
	ConfigModel *config_model_ptr = new ConfigModel(config, tableview_ptr);
	
	// Tell the table to use the model we created.
	tableview_ptr->setModel(config_model_ptr);
	
	// Set some sensible defaults for the QTableView.
	tableview_ptr->verticalHeader()->hide();
	tableview_ptr->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
	tableview_ptr->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
	tableview_ptr->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

	return tableview_ptr;
}




QString
GPlatesUtils::sanitise_key(
		const QString &key_with_slashes)
{
	QString sane = key_with_slashes;
	sane.replace('/', '_');
	return sane;
}


QStringList
GPlatesUtils::match_prefix(
		const QStringList &keys,
		const QString &prefix)
{
	QRegExp rx("^" + QRegExp::escape(prefix) + "/*");
	return keys.filter(rx);
}


void
GPlatesUtils::strip_prefix(
		QStringList &keys,
		const QString &prefix)
{
	QRegExp rx("^" + QRegExp::escape(prefix) + "/*");
	keys.replaceInStrings(rx, "");
}


void
GPlatesUtils::strip_all_except_root(
		QStringList &keys)
{
	QRegExp rx("/*$");
	keys.replaceInStrings(rx, "");
}


QString
GPlatesUtils::compose_keyname(
		const QString &prefix,
		const QString &subkey)
{
	if (prefix.isEmpty()) {
		return subkey;
	} else if (prefix.endsWith('/')) {
		// You shouldn't have a trailing slash in your key names / prefixes but just in case...
		return QString("%1%2").arg(prefix, subkey);
	} else {
		return QString("%1/%2").arg(prefix, subkey);
	}
}

