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
#include "CreateFeatureIdListModel.h"

QModelIndex 
GPlatesQtWidgets::CreateFeatureIdListModel::index(
		int row, 
		int column,
        const QModelIndex &_parent) const 

{
	return createIndex(row,column);
}

QVariant 
GPlatesQtWidgets::CreateFeatureIdListModel::headerData(
		int section, 
		Qt::Orientation orientation,
        int role ) const
{
	if(orientation == Qt::Horizontal)
	{
		return "Feature ID";
	}
	return QVariant();
}

QModelIndex 
GPlatesQtWidgets::CreateFeatureIdListModel::parent(
		const QModelIndex &child)  const
{
	return QModelIndex();
}

int 
GPlatesQtWidgets::CreateFeatureIdListModel::rowCount(
		const QModelIndex &_parent)  const
{
	return d_feature_id_list.size();
}

int 
GPlatesQtWidgets::CreateFeatureIdListModel::columnCount(
		const QModelIndex &_parent ) const 
{
	return 1;
}

Qt::ItemFlags
GPlatesQtWidgets::CreateFeatureIdListModel::flags(
		const QModelIndex &idx) const
{
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant 
GPlatesQtWidgets::CreateFeatureIdListModel::data(
		const QModelIndex &_index, 
		int role)  const
{
	
	if(role == Qt::DisplayRole)
	{
		if(_index.row() >= d_feature_id_list.size())
		{
			return QVariant();
		}
		return d_feature_id_list[_index.row()];
	}
	else
	{
		return QVariant();
	}
}

void
GPlatesQtWidgets::CreateFeatureIdListModel::add(
		const QString& feature_id) 
{
	beginInsertRows(QModelIndex(), d_feature_id_list.size(), d_feature_id_list.size()+1);
	
	if(d_feature_id_list.contains(feature_id)==false)
	{
		d_feature_id_list.push_back(feature_id);
	}
	
	endInsertRows();
}

void
GPlatesQtWidgets::CreateFeatureIdListModel::remove(
		QModelIndex& _index) 
{
	beginRemoveRows(QModelIndex(),_index.row(),_index.row());
	
	d_feature_id_list.removeAt(_index.row()+1);
	
	endInsertRows();
}

void
GPlatesQtWidgets::CreateFeatureIdListModel::clear() 
{
	beginRemoveRows(QModelIndex(),0,d_feature_id_list.size());
	
	d_feature_id_list.clear();
	
	endInsertRows();
}



