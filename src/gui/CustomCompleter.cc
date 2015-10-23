/* $Id$ */

/**
 * \file
 * $Revision$
 * $Date$
 *
 * Copyright (C) 2015 The University of Sydney, Australia
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

#include <QStringList>
#include <QTreeView>
#include <QHeaderView>
#include <QItemSelectionModel>

#include "CustomCompleter.h"


namespace
{
	enum ModelColumnName
	{
		MODEL_COLUMN_COMPLETION, MODEL_COLUMN_POPUP
	};
}

GPlatesGui::CustomCompleter::CustomCompleter(
		QObject *parent_) :
	QCompleter(parent_)
{  }


void
GPlatesGui::CustomCompleter::set_custom_popup()
{
	// Okay, forget trying to ->setCompletionRole, we'll use a custom QTreeView as the popup styled to
	// look like a single-column table (with the 'completion' column being effectively hidden).
	QTreeView *treeview = new QTreeView;
	this->setPopup(treeview);
	
	// Make the TreeView approximately table-like.
	treeview->setRootIsDecorated(false);
	treeview->setAllColumnsShowFocus(true);
	treeview->setSelectionBehavior(QTreeView::SelectRows);
	treeview->header()->hide();
	treeview->header()->setStretchLastSection(false);
	
	// Hide the zeroeth ("completion data") column. Note: we CANNOT merely use
	//		treeview->header()->setSectionHidden(MODEL_COLUMN_COMPLETION, true);
	// because QCompleter only current()s the zeroeth's column's item, not the entire row,
	// despite our ->setAllColumnsShowFocus(true) earlier. This leads to Interesting behaviour.
	// We need to keep the zeroeth column 'visible', but all our actual data for presentation
	// is in the next column. Making it zero-width allows us to display things the way we want,
	// but also (importantly!) keep keyboard focus behaviour working sanely.
	treeview->header()->setResizeMode(MODEL_COLUMN_COMPLETION, QHeaderView::Fixed);
	treeview->header()->resizeSection(MODEL_COLUMN_COMPLETION, 0);
	treeview->header()->setResizeMode(MODEL_COLUMN_POPUP, QHeaderView::Stretch);
}


QStringList
GPlatesGui::CustomCompleter::splitPath(
		const QString &path) const
{
	QStringList list;
	list << path.trimmed();
	return list;
}


QString
GPlatesGui::CustomCompleter::pathFromIndex(
		const QModelIndex &idx) const
{
	QString str = model()->data(idx, Qt::EditRole).toString().trimmed();
	return str;
}


