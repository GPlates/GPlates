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
 
#ifndef GPLATES_QTWIDGETS_LINKWIDGET_H
#define GPLATES_QTWIDGETS_LINKWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QString>


namespace GPlatesQtWidgets
{
	/**
	 * LinkWidget wraps around a QLabel and provides a simple interface to have a
	 * clean-looking link that can be placed into the user interface. It also has
	 * special handling for when it is disabled (disabled links in Qt look a bit
	 * weird by default).
	 */
	class LinkWidget :
			public QWidget
	{
		Q_OBJECT

	public:

		/**
		 * Constructs a LinkWidget with the given @a link_text as the text displayed
		 * in the link.
		 */
		explicit
		LinkWidget(
				const QString &link_text,
				QWidget *parent_ = NULL);

		/**
		 * Constructs a blank LinkWidget.
		 */
		explicit
		LinkWidget(
				QWidget *parent_ = NULL);

		void
		set_link_text(
				const QString &link_text);

		const QString &
		get_link_text() const
		{
			return d_link_text;
		}

	signals:

		/**
		 * Emitted when the user clicks on the link.
		 */
		void
		link_activated();

	protected:

		virtual
		bool
		event(
				QEvent *ev);

	private slots:

		void
		handle_link_activated();

	private:

		void
		init();

		void
		update_internal_label();

		QLabel *d_internal_label;
		QString d_link_text;
	};
}

#endif	// GPLATES_QTWIDGETS_LINKWIDGET_H
