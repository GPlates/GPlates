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

#include <QModelIndex>
#include "FeaturePropertyTableDelegate.h"

#include "qt-widgets/EditTimePeriodWidget.h"

GPlatesGui::FeaturePropertyTableDelegate::FeaturePropertyTableDelegate(
		QObject *parent_):
	QItemDelegate(parent_)
{  }


QWidget *
GPlatesGui::FeaturePropertyTableDelegate::createEditor(
		QWidget *parent_,
		const QStyleOptionViewItem &option,
		const QModelIndex &index) const
{
#if 0
	GPlatesQtWidgets::EditTimePeriodWidget *editor = new GPlatesQtWidgets::EditTimePeriodWidget(
			0, true, 0, true, parent_);
	return editor;
#else
	return NULL;
#endif
}


void
GPlatesGui::FeaturePropertyTableDelegate::setEditorData(
		QWidget *editor,
		const QModelIndex &index) const
{
	QVariant qv = index.model()->data(index, Qt::EditRole);
	if (qv.canConvert(QVariant::List)) {
		QList<QVariant> list = qv.toList();
		QVariant begin = list.first();
		QVariant end = list.last();
		
#if 0
		GPlatesQtWidgets::EditTimePeriodWidget *edit_time_period_widget =
				static_cast<GPlatesQtWidgets::EditTimePeriodWidget *>(editor);
		edit_time_period_widget->set_time_of_appearance(begin.toInt());
		edit_time_period_widget->set_time_of_appearance(end.toInt());
		if (begin.toString() == "http://gplates.org/times/distantPast") {
			edit_time_period_widget->set_distant_past(true);
		} else {
			edit_time_period_widget->set_distant_past(false);
		}
		if (end.toString() == "http://gplates.org/times/distantFuture") {
			edit_time_period_widget->set_distant_future(true);
		} else {
			edit_time_period_widget->set_distant_future(false);
		}
#endif
		// FIXME: Handle bizzare data with multiple distantFuture/Past s.
	}
}


void
GPlatesGui::FeaturePropertyTableDelegate::setModelData(
		QWidget *editor,
		QAbstractItemModel *model,
		const QModelIndex &index) const
{
	// If the target cell is a QVariant::List, we can assume an EditTimePeriodWidget was used
	// to perform the editing.
	// FIXME: We really need a better way of handling this.
	QVariant qv = index.model()->data(index, Qt::EditRole);
	if (qv.canConvert(QVariant::List)) {
#if 0
		GPlatesQtWidgets::EditTimePeriodWidget *edit_time_period_widget =
				static_cast<GPlatesQtWidgets::EditTimePeriodWidget *>(editor);
		// FIXME: Obtain time period value through EditTimePeriodWidget's getter methods.
		edit_time_period_widget->time_of_appearance();
		edit_time_period_widget->time_of_disappearance();
#endif
		// FIXME: Pack this up and send it to the FeaturePropertyTableModel as a QVariant.
	}
}


void
GPlatesGui::FeaturePropertyTableDelegate::updateEditorGeometry(
		QWidget *editor,
		const QStyleOptionViewItem &option,
		const QModelIndex &index) const
{
	editor->setGeometry(option.rect);
}




