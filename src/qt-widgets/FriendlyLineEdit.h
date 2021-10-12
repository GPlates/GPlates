/* $Id$ */

/**
 * \file
 * Contains the definition of the class FriendlyLineEdit.
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
 
#ifndef GPLATES_QTWIDGETS_FRIENDLYLINEEDIT_H
#define GPLATES_QTWIDGETS_FRIENDLYLINEEDIT_H

#include <boost/function.hpp>
#include <QWidget>
#include <QLineEdit>
#include <QString>
#include <QValidator>
#include <QSizePolicy>


namespace GPlatesQtWidgets
{
	namespace FriendlyLineEditInternals
	{
		class InternalLineEdit :
				public QLineEdit
		{
			Q_OBJECT

		public:

			InternalLineEdit(
					const QString &message_on_empty_string,
					const boost::function<void (QFocusEvent *)> &parent_focus_in_event_function,
					const boost::function<void (QFocusEvent *)> &parent_focus_out_event_function,
					QWidget *parent_ = NULL);

			// text is not virtual but we'll override it anyway.
			QString
			text() const;

			// setText is not virtual but we'll override it anyway.
			void
			setText(
					const QString &);

		protected:

			virtual
			void
			focusInEvent(
					QFocusEvent *event);

			virtual
			void
			focusOutEvent(
					QFocusEvent *event);

		private:

			void
			handle_focus_in();

			void
			handle_focus_out();

			QString d_message_on_empty_string;
			boost::function<void (QFocusEvent *)> d_parent_focus_in_event_function;
			boost::function<void (QFocusEvent *)> d_parent_focus_out_event_function;

			QPalette d_default_palette;
			QPalette d_empty_string_palette;

			QFont d_default_font;
			QFont d_empty_string_font;

			bool d_is_empty_string;
		};
	}

	/**
	 * FriendlyLineEdit wraps around a QLineEdit and displays a custom string in
	 * the line edit when the logical contents of the line edit is the empty
	 * string; this custom string is displayed in grey and italics.
	 */
	class FriendlyLineEdit :
			public QWidget
	{
		Q_OBJECT

	public:

		/**
		 * Constructs a FriendlyLineEdit.
		 *
		 * The @a message_on_empty_string is displayed in the internal line edit
		 * when the logical value of the line edit is the empty string.
		 */
		FriendlyLineEdit(
				const QString &contents = QString(),
				const QString &message_on_empty_string = QString(),
				QWidget *parent_ = NULL);

		// Using Qt naming conventions here.

		QString
		text() const;

		void
		setText(
				const QString& text);

		bool
		isReadOnly() const;

		void
		setReadOnly(
				bool read_only);

		void
		setValidator(
				const QValidator *v);

		const QValidator *
		validator() const;

		void
		setAlignment(
				Qt::Alignment flag);

		Qt::Alignment
		alignment() const;

		void
		setLineEditSizePolicy(
				QSizePolicy policy);

		QSizePolicy
		lineEditSizePolicy() const;

	signals:

		// Using Qt naming conventions here.

		void
		editingFinished();

		void
		textEdited(
				const QString &text_);

	protected:

		virtual
		void
		handle_text_edited(
				const QString &text_)
		{
			// Default implementation: does nothing.
		}

	private slots:

		void
		handle_internal_line_edit_editing_finished()
		{
			emit editingFinished();
		}

		void
		handle_internal_line_edit_text_edited(
				const QString &text_)
		{
			emit textEdited(text_);
			handle_text_edited(text_);
		}

	private:

		/**
		 * The line edit that we wrap around. Memory managed by Qt.
		 */
		FriendlyLineEditInternals::InternalLineEdit *d_line_edit;
	};
}

#endif  // GPLATES_QTWIDGETS_FRIENDLYLINEEDIT_H
