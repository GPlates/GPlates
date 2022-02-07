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
#ifndef GPLATES_WIDGETS_CREATEFEATUREIDLISTMODEL_H
#define GPLATES_WIDGETS_CREATEFEATUREIDLISTMODEL_H

#include <QObject>
#include <QDialog>

#include <iostream>
#include <QAbstractItemModel>

namespace GPlatesQtWidgets
{

	class CreateFeatureIdListModel : 
		public QAbstractItemModel
	{
	public:
		virtual 
		QModelIndex 
		index(
				int row, 
				int column,
                const QModelIndex &parent = QModelIndex()) const;

		virtual
		QVariant 
		headerData(
				int section, 
				Qt::Orientation orientation,
                int role = Qt::DisplayRole) const;
    
		virtual 
		QModelIndex 
		parent(
				const QModelIndex &child) const ;

		virtual 
		int 
		rowCount(
				const QModelIndex &parent = QModelIndex()) const ;

		virtual 
		int 
		columnCount(
				const QModelIndex &parent = QModelIndex()) const;

		Qt::ItemFlags
		flags(
				const QModelIndex &idx) const;

		virtual 
		QVariant 
		data(
				const QModelIndex &index, 
				int role = Qt::DisplayRole) const;

		void
		add(
				const QString& feature_id);

		void
		remove(
				QModelIndex& index);

		void
		clear();

		inline
		const QStringList&
		feature_id_list()
		{
			return d_feature_id_list;
		}

	private:
		QStringList d_feature_id_list;
	};
}
#endif //GPLATES_WIDGETS_CREATEFEATUREIDLISTMODEL_H


