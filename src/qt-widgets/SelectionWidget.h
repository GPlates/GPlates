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
 
#ifndef GPLATES_QTWIDGETS_SELECTIONWIDGET_H
#define GPLATES_QTWIDGETS_SELECTIONWIDGET_H

#include <boost/optional.hpp>
#include <QWidget>
#include <QListWidget>
#include <QListWidgetItem>
#include <QComboBox>
#include <QFocusEvent>

#include "utils/TypeTraits.h"


namespace GPlatesQtWidgets
{
	/**
	 * SelectionWidget is a widget that unifies QListWidget and QComboBox,
	 * providing the user with a mechanism to make one choice out of a possible
	 * many. SelectionWidget allows code designed to be used with, say, a list
	 * widget, to be easily adapted to work with a combo box, and vice versa.
	 */
	class SelectionWidget :
			public QWidget
	{
		Q_OBJECT

	public:

		enum DisplayWidget
		{
			Q_LIST_WIDGET,
			Q_COMBO_BOX
		};

		/**
		 * Creates a SelectionWidget that encapsulates either a QListWidget or a
		 * QComboBox, depending on the value of @a display_widget.
		 */
		SelectionWidget(
				DisplayWidget display_widget,
				QWidget *parent_ = NULL);

		/**
		 * Appends an item to the selections available. The item is displayed as
		 * @a display_text and is associated with @a user_data.
		 *
		 * The type @a T must be a type that can be stored in a QVariant. If it is a
		 * custom type, @a T must have been registered using Q_DECLARE_METATYPE
		 * defined in the <QMetaType> header.
		 */
		template<typename T>
		void
		add_item(
				const QString &display_text,
				typename GPlatesUtils::TypeTraits<T>::argument_type user_data)
		{
			QVariant qv;
			qv.setValue(user_data);

			if (d_listwidget)
			{
				QListWidgetItem *new_item = new QListWidgetItem(display_text);
				new_item->setData(Qt::UserRole, qv);

				d_listwidget->addItem(new_item);
			}
			else // d_combobox
			{
				d_combobox->addItem(display_text, qv);
			}
		}

		/**
		 * Removes all items.
		 */
		void
		clear();

		/**
		 * Returns the number of items.
		 */
		int
		get_count() const;

		/**
		 * Returns the index of the currently selected item. Returns -1 if there is no
		 * selection.
		 */
		int
		get_current_index() const;

		/**
		 * Sets the selected index to be @a index.
		 */
		void
		set_current_index(
				int index);

		/**
		 * Returns the data at the given index. Returns boost::none if the data is not
		 * of type @a T or if the index is invalid.
		 */
		template<typename T>
		boost::optional<T>
		get_data(
				int index) const
		{
			QVariant qv;

			if (d_listwidget)
			{
				QListWidgetItem *item = d_listwidget->itemFromIndex(
						d_listwidget->model()->index(index, 0));
				if (!item)
				{
					return boost::none;
				}

				qv = item->data(Qt::UserRole);
			}
			else // d_combobox
			{
				if (index == -1)
				{
					return boost::none;
				}

				qv = d_combobox->itemData(index);
			}

			if (qv.canConvert<T>())
			{
				return qv.value<T>();
			}
			else
			{
				return boost::none;
			}
		}

		/**
		 * Returns the index of the item containing the given @a text. Returns -1 if
		 * @a text is not found.
		 */
		int
		find_text(
				const QString &text,
				Qt::MatchFlags flags = Qt::MatchExactly | Qt::MatchCaseSensitive);

	signals:

		/**
		 * Emitted when the user clicks or double clicks on an item (depending on
		 * system configuration) and when the user presses the activation key.
		 *
		 * This signal is only emitted if the display widget is QListWidget.
		 */
		void
		item_activated(
				int index);

		/**
		 * Emitted when the current index changes either through user interaction or
		 * programmatically. If there is no current item, @a index is -1.
		 */
		void
		current_index_changed(
				int index);

	protected:

		virtual
		void
		focusInEvent(
				QFocusEvent *ev);

	private slots:

		void
		handle_listwidget_item_activated(
				QListWidgetItem *item);

		void
		handle_listwidget_current_row_changed(
				int current_row);

		void
		handle_combobox_current_index_changed(
				int index);

	private:

		class InternalListWidget :
				public QListWidget
		{
		public:

			InternalListWidget(
					QWidget *parent_) :
				QListWidget(parent_)
			{  }

			// Exposing some protected functions as public.

			QModelIndex
			indexFromItem(
					QListWidgetItem *item_) const
			{
				return QListWidget::indexFromItem(item_);
			}

			QListWidgetItem *
			itemFromIndex(
					const QModelIndex &index_) const
			{
				return QListWidget::itemFromIndex(index_);
			}
		};

		// Precisely one of d_listwidget and d_combobox is non-NULL.
		InternalListWidget *d_listwidget;
		QComboBox *d_combobox;
	};
}


#endif	// GPLATES_QTWIDGETS_SELECTIONWIDGET_H
