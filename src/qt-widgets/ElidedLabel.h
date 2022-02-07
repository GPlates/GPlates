/* $Id$ */

/**
 * \file
 * Contains the definition of the class ElidedLabel.
 *
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2010, 2011 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_ELIDEDLABEL_H
#define GPLATES_QTWIDGETS_ELIDEDLABEL_H

#include <map>
#include <QWidget>
#include <QLabel>
#include <QString>
#include <QResizeEvent>
#include <QDialog>
#include <QFont>


namespace GPlatesQtWidgets
{
	/**
	 * ElidedLabel is a widget that can display a piece of text, much like a QLabel.
	 * However, when the text is wider than the width of the widget, the text is
	 * elided with an ellipsis (...) at the end. When the text is elided and
	 * the user moves the cursor over the widget, a tooltip that contains the full
	 * text is displayed over the widget.
	 */
	class ElidedLabel :
			public QWidget
	{
	public:

		explicit
		ElidedLabel(
				Qt::TextElideMode mode,
				QWidget *parent_ = NULL);

		explicit
		ElidedLabel(
				const QString &text_,
				Qt::TextElideMode mode,
				QWidget *parent_ = NULL);

		void
		set_text_elide_mode(
				Qt::TextElideMode mode);

		Qt::TextElideMode
		get_text_elide_mode() const;

		// Using Qt naming conventions here.

		void
		setText(
				const QString &text_);

		QString
		text() const;

		void
		setFrameStyle(
				int style_);

		int
		frameStyle() const;

	protected:

		virtual
		void
		resizeEvent(
				QResizeEvent *event_);

		virtual
		void
		paintEvent(
				QPaintEvent *event_);

	private:

		/**
		 * The internal label used to display the elided text.
		 */
		class InternalLabel :
				public QLabel
		{
		public:

			explicit
			InternalLabel(
					QWidget *parent_ = NULL);

		protected:

			virtual
			bool
			event(
					QEvent *ev);
		};

		// Initialisation common to both constructors.
		void
		init();

		void
		update_internal_label();

		QFrame *d_internal_label_frame;
		InternalLabel *d_internal_label;

		/**
		 * The current full text.
		 */
		QString d_text;

		/**
		 * Where the ellipsis should appear when eliding text.
		 */
		Qt::TextElideMode d_mode;

		bool d_internal_label_needs_updating;
	};
}

#endif  // GPLATES_QTWIDGETS_ELIDEDLABEL_H
