/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_FEATUREPROPERTYTABLEDELEGATE_H
#define GPLATES_GUI_FEATUREPROPERTYTABLEDELEGATE_H

#include <QItemDelegate>


namespace GPlatesGui
{
	/**
	 * THIS CLASS IS INCOMPLETE AND NOT CURRENTLY USED!
	 * 
	 * This class is used by Qt to edit entries in a QTableView.
	 * It creates a widget as appropriate and populates it with data from our
	 * FeaturePropertyTableModel.
	 * 
	 * It uses Qt's model/view framework, not to be confused with GPlates' own model/view,
	 * to provide multi-column data to a view. See the following urls for more information:
	 * http://trac.gplates.org/wiki/QtModelView
	 * http://doc.trolltech.com/4.3/model-view-delegate.html
	 */
	class FeaturePropertyTableDelegate:
			public QItemDelegate
	{
		Q_OBJECT
		
	public:

		explicit
		FeaturePropertyTableDelegate(
				QObject *parent_ = NULL);
		
		QWidget *
		createEditor(
				QWidget *parent_,
				const QStyleOptionViewItem &option,
				const QModelIndex &index) const;
		
		void
		setEditorData(
				QWidget *editor,
				const QModelIndex &index) const;
		
		void
		setModelData(
				QWidget *editor,
				QAbstractItemModel *model,
				const QModelIndex &index) const;
		
		void
		updateEditorGeometry(
				QWidget *editor,
				const QStyleOptionViewItem &option,
				const QModelIndex &index) const;
		
	};
}

#endif // GPLATES_GUI_FEATUREPROPERTYTABLEDELEGATE_H
