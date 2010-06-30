/* $Id$ */

/**
 * \file
 * Contains the definition of the class ElidedLabel.
 *
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
	 * truncated with an ellipsis (...) at the end. When the text is truncated and
	 * the user moves the cursor over the widget, a tooltip that contains the full
	 * text is displayed over the widget.
	 */
	class ElidedLabel :
			public QWidget
	{
	public:

		ElidedLabel(
				QWidget *parent_ = NULL);

		ElidedLabel(
				const QString &text_,
				QWidget *parent_ = NULL);

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
		enterEvent(
				QEvent *event_);

		virtual
		void
		paintEvent(
				QPaintEvent *event_);

	private:

		/**
		 * The tooltip that is displayed over the ElidedLabel when the text is
		 * truncated and the user moves the cursor over the ElidedLabel.
		 *
		 * We use a custom widget instead of a QToolTip because there are issues with
		 * tooltip alignment and flickering.
		 *
		 * This is a Singleton, but the utils/Singleton class is not used because Qt
		 * likes to manage its own memory.
		 */
		class ElidedLabelToolTip :
				public QDialog
		{
		public:

			static
			void
			showToolTip(
					const QString &text,
					const QPoint &global_pos,
					const QFont &text_font);

		protected:

			virtual
			void
			leaveEvent(
					QEvent *event_);

		private:

			static
			ElidedLabelToolTip &
			instance();

			void
			do_show(
					const QString &text,
					const QPoint &global_pos,
					const QFont &text_font);

			ElidedLabelToolTip();

			QLabel *d_internal_label;
		};

		void
		init();

		void
		update_substring_widths_map();

		void
		update_internal_label();

		QLabel *d_internal_label;

		/**
		 * The current full text.
		 */
		QString d_text;

		/**
		 * Typedef for a map from widths (in pixels) of substrings of the current @a text
		 * (plus any ellipsis at the end) to the length of the substring in characters.
		 */
		typedef std::map<int, int> substring_width_map_type;

		// Invariant: if d_text is not the empty string, contains at least one entry.
		substring_width_map_type d_substring_widths;

		/**
		 * If true, @a d_substring_widths needs to be regenerated from @a d_text.
		 */
		bool d_substring_widths_needs_updating;

		/**
		 * Whether the current text display is truncated.
		 */
		bool d_is_truncated;

		static const QString ELLIPSIS;
	};
}

#endif  // GPLATES_QTWIDGETS_ELIDEDLABEL_H
