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

#include "SelectionWidget.h"

#include "QtWidgetUtils.h"


GPlatesQtWidgets::SelectionWidget::SelectionWidget(
		DisplayWidget display_widget,
		QWidget *parent_) :
	QWidget(parent_),
	d_listwidget(NULL),
	d_combobox(NULL)
{
	if (display_widget == Q_LIST_WIDGET)
	{
		d_listwidget = new InternalListWidget(this);
		QtWidgetUtils::add_widget_to_placeholder(d_listwidget, this);

		QObject::connect(
				d_listwidget,
				SIGNAL(itemActivated(QListWidgetItem *)),
				this,
				SLOT(handle_listwidget_item_activated(QListWidgetItem *)));
		QObject::connect(
				d_listwidget,
				SIGNAL(currentRowChanged(int)),
				this,
				SLOT(handle_listwidget_current_row_changed(int)));
	}
	else if (display_widget == Q_COMBO_BOX)
	{
		d_combobox = new QComboBox(this);
		QtWidgetUtils::add_widget_to_placeholder(d_combobox, this);

		QObject::connect(
				d_combobox,
				SIGNAL(currentIndexChanged(int)),
				this,
				SLOT(handle_combobox_current_index_changed(int)));
	}
}


void
GPlatesQtWidgets::SelectionWidget::clear()
{
	if (d_listwidget)
	{
		d_listwidget->clear();
	}
	else // d_combobox
	{
		d_combobox->clear();
	}
}


int
GPlatesQtWidgets::SelectionWidget::get_count() const
{
	if (d_listwidget)
	{
		return d_listwidget->count();
	}
	else // d_combobox
	{
		return d_combobox->count();
	}
}


int
GPlatesQtWidgets::SelectionWidget::get_current_index() const
{
	if (d_listwidget)
	{
		return d_listwidget->currentRow();
	}
	else // d_combobox
	{
		return d_combobox->currentIndex();
	}
}


void
GPlatesQtWidgets::SelectionWidget::set_current_index(
		int index)
{
	if (d_listwidget)
	{
		d_listwidget->setCurrentRow(index);
	}
	else // d_combobox
	{
		d_combobox->setCurrentIndex(index);
	}
}


int
GPlatesQtWidgets::SelectionWidget::find_text(
		const QString &text,
		Qt::MatchFlags flags)
{
	if (d_listwidget)
	{
		QList<QListWidgetItem *> matched_items = d_listwidget->findItems(text, flags);

		// There should be exactly one match.
		if (matched_items.count() == 1)
		{
			return d_listwidget->indexFromItem(matched_items.front()).row();
		}
		else
		{
			return -1;
		}
	}
	else // d_combobox
	{
		return d_combobox->findText(text, flags);
	}
}


void
GPlatesQtWidgets::SelectionWidget::focusInEvent(
		QFocusEvent *ev)
{
	if (d_listwidget)
	{
		d_listwidget->setFocus();
	}
	else // d_combobox
	{
		d_combobox->setFocus();
	}
}


void
GPlatesQtWidgets::SelectionWidget::handle_listwidget_item_activated(
		QListWidgetItem *item)
{
	// d_listwidget must be non-NULL here.
	int index = d_listwidget->indexFromItem(item).row();
	if (index != -1)
	{
		emit item_activated(index);
	}
}


void
GPlatesQtWidgets::SelectionWidget::handle_listwidget_current_row_changed(
		int current_row)
{
	emit current_index_changed(current_row);
}


void
GPlatesQtWidgets::SelectionWidget::handle_combobox_current_index_changed(
		int index)
{
	emit current_index_changed(index);
}

