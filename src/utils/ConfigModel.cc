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

#include <QBrush>
#include <QColor>
#include <QIcon>
#include <QDebug>

#include "ConfigModel.h"

#include "ConfigInterface.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"


namespace
{
	/**
	 * Initialises the SchemaEntry list with a single basic entry per key found
	 * in the ConfigBundle/UserPreferences, so that we can use it as an 'index'.
	 */
	void
	initialise_basic_schema(
			GPlatesUtils::ConfigModel::SchemaType &schema,
			GPlatesUtils::ConfigInterface &config)
	{
		QStringList keys = config.subkeys();
		Q_FOREACH(QString key, keys) {
			GPlatesUtils::ConfigModel::SchemaEntry entry;
			entry.key = key;
			entry.label = key;
			
			schema.append(entry);
		}
	}
}



GPlatesUtils::ConfigModel::ConfigModel(
		ConfigInterface &_config,
		QObject *_parent):
	QAbstractTableModel(_parent),
	d_config_ptr(&_config),
	d_default_foreground(QBrush(Qt::black)),
	d_default_background(QBrush(Qt::white)),
	d_user_overriding_default_icon(QIcon(":/gnome_emblem_default_16.png")),
	d_user_no_default_icon(QIcon(":/gnome_emblem_default_yellow_16.png")),
	d_default_value_icon(QIcon(":/blank_16.png"))
{
	// No user-supplied schema for now, just always make our own.
	initialise_basic_schema(d_schema, _config);

	// Signals and slots to keep everything in sync with everything.
	connect(d_config_ptr, SIGNAL(key_value_updated(QString)),
			this, SLOT(react_key_value_updated(QString)));
}


GPlatesUtils::ConfigModel::~ConfigModel()
{  }


QVariant
GPlatesUtils::ConfigModel::data(
		const QModelIndex &idx,
		int role) const
{
	if ( ! idx.isValid()) {
		// An invalid index - we cannot report data for this.
		return QVariant();
	} else if (idx.row() < 0 || idx.row() >= d_schema.size()) {
		// The index is valid, but refers to an out-of-bounds row - we cannot report data for this.
		return QVariant();
	}

	// Depending on what role was asked for by the view, and what column, we might have
	// quite a few alternatives to choose between:-
	switch (idx.column())
	{
	case COLUMN_NAME:
			// The name is simple enough, as it is constant and non-editable.
			return get_name_data_for_role(d_schema.at(idx.row()), role);
	
	case COLUMN_VALUE:
			// The value is a little harder, since it is editable and might have other attributes.
			return get_value_data_for_role(d_schema.at(idx.row()), role);
	
	default:
			// The index is valid, but refers to an out-of-bounds column - we cannot report data for this.
			return QVariant();
	}
}


QVariant
GPlatesUtils::ConfigModel::get_name_data_for_role(
		const SchemaEntry &entry,
		int role) const
{
	// Present the name of a particular key in various ways for the QTableView.
	switch (role)
	{
	case Qt::DisplayRole:
			return entry.label;
	
	case Qt::DecorationRole:
			// Use a small icon in front of the name to indicate whether a value has been
			// explicitly set by the user or not (and whether there is a default backing it).
			if (d_config_ptr->has_been_set(entry.key)) {
				if (d_config_ptr->default_exists(entry.key)) {
					return d_user_overriding_default_icon;
				} else {
					return d_user_no_default_icon;
				}
			} else {
				return d_default_value_icon;
			}

	case Qt::ForegroundRole:
			return d_default_foreground;
			
	case Qt::BackgroundRole:
			return d_default_background;

	default:
			return QVariant();
	}
}


QVariant
GPlatesUtils::ConfigModel::get_value_data_for_role(
		const SchemaEntry &entry,
		int role) const
{
	// Present the value of a particular key in various ways for the QTableView.
	switch (role)
	{
	case Qt::DisplayRole:
			return d_config_ptr->get_value(entry.key);

	case Qt::EditRole:
			return d_config_ptr->get_value(entry.key);
	
	case Qt::ForegroundRole:
			return d_default_foreground;
			
	case Qt::BackgroundRole:
			return d_default_background;

	default:
			return QVariant();
	}
}


QVariant
GPlatesUtils::ConfigModel::headerData(
		int section,
		Qt::Orientation orientation,
		int role) const
{
	// We are only concerned with the horizontal header.
	if (orientation != Qt::Horizontal) {
		return QVariant();
	}
	
	// We are also only interested in a couple of roles for this basic header.
	if (role == Qt::DisplayRole) {
		switch (section)
		{
		case COLUMN_NAME:
				return tr("Name");
		
		case COLUMN_VALUE:
				return tr("Value");
		
		default:
				return QVariant();
		}

	} else if (role == Qt::TextAlignmentRole) {
		return Qt::AlignLeft;
		
	} else {
		return QVariant();
	}
}


bool
GPlatesUtils::ConfigModel::setData(
		const QModelIndex &idx,
		const QVariant &value,
		int role)
{
	// Can't edit for invalid indexes or roles.
	if ( ! idx.isValid() || role != Qt::EditRole) {
		return false;
	}
	// Can't edit the key names, either.
	if (idx.column() != COLUMN_VALUE) {
		return false;
	}
	
	QString key = d_schema.at(idx.row()).key;
	qDebug() << "ConfigModel: Setting" << key << "=" << value;
	d_config_ptr->set_value(key, value);
	
	return true;
}




Qt::ItemFlags
GPlatesUtils::ConfigModel::flags(
		const QModelIndex &idx) const
{
	if ( ! idx.isValid()) {
		// An invalid index - we cannot report data for this.
		return Qt::NoItemFlags;
	}
	
	// While the name can never be edited, the 'value' column is user-editabe.
	if (idx.column() == COLUMN_VALUE) {
		return Qt::ItemIsEnabled | Qt::ItemIsEditable;
	} else {
		return Qt::ItemIsEnabled;
	}
}
		

int
GPlatesUtils::ConfigModel::rowCount(
		const QModelIndex &parent_idx) const
{
	return d_schema.size();
}


void
GPlatesUtils::ConfigModel::react_key_value_updated(
		QString key)
{
	// Ah, the ConfigInterface's key value got changed somewhere by someone.
	// Are we following this key? If so, figure out the indexes.
	for (int i = 0; i < d_schema.size(); ++i) {
		if (d_schema.at(i).key == key) {
			qDebug() << "ConfigModel: Oh, the key" << key << "got changed. It's on our row" << i <<", so I'll update that.";
			
			QModelIndex idx_top_left = index(i, 0);
			QModelIndex idx_bottom_right = index(i, 1);

			// Update our Views.
			emit dataChanged(idx_top_left, idx_bottom_right);
			return;
		}
	}
}


