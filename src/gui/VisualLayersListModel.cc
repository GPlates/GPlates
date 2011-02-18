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

#include <boost/foreach.hpp>
#include <QMimeData>
#include <QByteArray>
#include <QDataStream>

#include "VisualLayersListModel.h"

#include "VisualLayersProxy.h"


const QString
GPlatesGui::VisualLayersListModel::VISUAL_LAYERS_MIME_TYPE = "application/gplates.visuallayers.index";


GPlatesGui::VisualLayersListModel::VisualLayersListModel(
		VisualLayersProxy &visual_layers,
		QObject *parent_) :
	QAbstractListModel(parent_),
	d_visual_layers(visual_layers)
{
	make_signal_slot_connections();
}


void
GPlatesGui::VisualLayersListModel::make_signal_slot_connections()
{
	// Connect to signals from VisualLayers.
	QObject::connect(
			&d_visual_layers,
			SIGNAL(layer_order_changed(size_t, size_t)),
			this,
			SLOT(handle_visual_layers_order_changed(size_t, size_t)));
	QObject::connect(
			&d_visual_layers,
			SIGNAL(layer_about_to_be_added(size_t)),
			this,
			SLOT(handle_visual_layer_about_to_be_added(size_t)));
	QObject::connect(
			&d_visual_layers,
			SIGNAL(layer_added(size_t)),
			this,
			SLOT(handle_visual_layer_added(size_t)));
	QObject::connect(
			&d_visual_layers,
			SIGNAL(layer_about_to_be_removed(size_t)),
			this,
			SLOT(handle_visual_layer_about_to_be_removed(size_t)));
	QObject::connect(
			&d_visual_layers,
			SIGNAL(layer_removed(size_t)),
			this,
			SLOT(handle_visual_layer_removed(size_t)));
	QObject::connect(
			&d_visual_layers,
			SIGNAL(layer_modified(size_t)),
			this,
			SLOT(handle_visual_layer_modified(size_t)));
}


Qt::ItemFlags
GPlatesGui::VisualLayersListModel::flags(
		const QModelIndex &index_) const
{
	return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsDropEnabled;
}


QVariant
GPlatesGui::VisualLayersListModel::data(
		const QModelIndex &index_,
		int role) const
{
	if (!index_.isValid() || role != Qt::DisplayRole)
	{
		return QVariant();
	}

	const int row = index_.row();
	if (0 <= row && static_cast<size_t>(row) < d_visual_layers.size())
	{
		return QVariant(d_visual_layers.child_layer_index_at(row));
	}
	else
	{
		return QVariant();
	}
}


int
GPlatesGui::VisualLayersListModel::rowCount(
		const QModelIndex &parent_) const
{
	return d_visual_layers.size();
}


Qt::DropActions
GPlatesGui::VisualLayersListModel::supportedDropActions() const
{
	return Qt::MoveAction;
}


QStringList
GPlatesGui::VisualLayersListModel::mimeTypes() const
{
	return QStringList(VISUAL_LAYERS_MIME_TYPE);
}

#if 0
QMimeData *
GPlatesGui::VisualLayersListModel::mimeData(
		const QModelIndexList &indices) const
{
	QMimeData *mime_data = new QMimeData(); // Memory managed by Qt.
	QByteArray encoded_data;
	QDataStream stream(&encoded_data, QIODevice::WriteOnly);

	// Store the first valid index as an encoded integer.
	BOOST_FOREACH(QModelIndex i, indices)
	{
		if (i.isValid())
		{
			stream << i.row();
			break;
		}
	}

	if (encoded_data.size())
	{
		mime_data->setData(VISUAL_LAYERS_MIME_TYPE, encoded_data);
	}

	return mime_data;
}
#endif

bool
GPlatesGui::VisualLayersListModel::dropMimeData(
		const QMimeData *mime_data,
		Qt::DropAction action,
		int row,
		int column,
		const QModelIndex &parent_)
{
	if (action == Qt::IgnoreAction)
	{
		return true;
	}

	if (!mime_data->hasFormat(VISUAL_LAYERS_MIME_TYPE))
	{
		return false;
	}

	if (column > 0)
	{
		return false;
	}

	// Read from the mime object for the source row.
	QByteArray encoded_data = mime_data->data(VISUAL_LAYERS_MIME_TYPE);
	QDataStream stream(&encoded_data, QIODevice::ReadOnly);
	int from_row;
	stream >> from_row;

	// Sanity check.
	if (from_row < 0 || static_cast<std::size_t>(from_row) >= d_visual_layers.size())
	{
		return false;
	}

	// Work out the destination row.
	int to_row;
	if (row != -1)
	{
		to_row = row;
	}
	else if (parent_.isValid())
	{
		// Dropped occurred on an item.
		to_row = parent_.row();
	}
	else
	{
		// Treat a drop on the blank area after the last item in the list view as a
		// drop on the last item itself.
		to_row = d_visual_layers.size() - 1;
	}

	// Sanity check.
	if (to_row < 0)
	{
		to_row = 0;
	}
	else if (static_cast<std::size_t>(to_row) >= d_visual_layers.size())
	{
		to_row = d_visual_layers.size() - 1;
	}

	d_visual_layers.move_layer(from_row, to_row);

	return true;
}


void
GPlatesGui::VisualLayersListModel::handle_visual_layers_order_changed(
		size_t first_row,
		size_t last_row)
{
	emit dataChanged(index(first_row, 0), index(last_row, 0));
}


void
GPlatesGui::VisualLayersListModel::handle_visual_layer_about_to_be_added(
		size_t row)
{
	beginInsertRows(QModelIndex(), row, row);
}


void
GPlatesGui::VisualLayersListModel::handle_visual_layer_added(
		size_t row)
{
	endInsertRows();

	// Need to refresh all visual layers after visual layer added, to make sure
	// widgets for adding new connections get refreshed.
	for (size_t i = 0; i != d_visual_layers.size(); ++i)
	{
		handle_visual_layer_modified(i);
	}
}


void
GPlatesGui::VisualLayersListModel::handle_visual_layer_about_to_be_removed(
		size_t row)
{
	beginRemoveRows(QModelIndex(), row, row);
}


void
GPlatesGui::VisualLayersListModel::handle_visual_layer_removed(
		size_t row)
{
	endRemoveRows();

	// Need to refresh all visual layers after visual layer removed, to make sure
	// widgets for adding new connections get refreshed.
	for (size_t i = 0; i != d_visual_layers.size(); ++i)
	{
		handle_visual_layer_modified(i);
	}
}


void
GPlatesGui::VisualLayersListModel::handle_visual_layer_modified(
		size_t row)
{
	QModelIndex model_index = index(row, 0);
	emit dataChanged(model_index, model_index);
}

