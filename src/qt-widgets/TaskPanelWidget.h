/* $Id$ */

/**
 * @file
 *
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
 
#ifndef GPLATES_QTWIDGETS_TASKPANELWIDGET_H
#define GPLATES_QTWIDGETS_TASKPANELWIDGET_H

#include <QWidget>
#include <QString>


namespace GPlatesQtWidgets
{
	/**
	 * TaskPanelWidget is the abstract base class of widgets that are displayed in the TaskPanel.
	 */
	class TaskPanelWidget :
			public QWidget
	{
		Q_OBJECT

	public:

		TaskPanelWidget(
				QWidget *parent_ = NULL) :
			QWidget(parent_)
		{  }

		virtual
		void
		handle_activation() = 0;

		/**
		 * The text of the TaskPanel's clear action when this widget is activated.
		 * This string should have already been internationalised.
		 *
		 * If this widget does not support a clear action, return the empty string;
		 * this causes the clear action to be hidden.
		 */
		virtual
		QString
		get_clear_action_text() const
		{
			return QString();
		}

		/**
		 * Whether the TaskPanel's clear action, if visible, is enabled.
		 */
		virtual
		bool
		clear_action_enabled() const
		{
			return false;
		}

		/**
		 * Handle the TaskPanel's clear action being triggered.
		 */
		virtual
		void
		handle_clear_action_triggered()
		{
			// Default implementation does nothing.
		}

	Q_SIGNALS:

		void
		clear_action_enabled_changed(
				bool enabled);

	protected:

		void
		emit_clear_action_enabled_changed(
				bool enabled)
		{
			Q_EMIT clear_action_enabled_changed(enabled);
		}
	};
}

#endif  // GPLATES_QTWIDGETS_TASKPANELWIDGET_H

