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

#include <QObject>
#include <QPalette>
#include <QFont>
#include <QFocusEvent>

#include "FriendlyLineEdit.h"

#include "QtWidgetUtils.h"


GPlatesQtWidgets::FriendlyLineEdit::FriendlyLineEdit(
		const QString &contents,	
		const QString &message_on_empty_string,
		QWidget *parent_) :
	QWidget(parent_),
	d_line_edit(
			new FriendlyLineEditInternals::InternalLineEdit(
				contents,
				message_on_empty_string,
				this))
{
	QtWidgetUtils::add_widget_to_placeholder(d_line_edit, this);

	QObject::connect(
			d_line_edit,
			SIGNAL(editingFinished()),
			this,
			SLOT(handle_internal_line_edit_editing_finished()));
}


QString
GPlatesQtWidgets::FriendlyLineEdit::text() const
{
	return d_line_edit->text();
}


void
GPlatesQtWidgets::FriendlyLineEdit::setText(
		const QString &text_)
{
	d_line_edit->setText(text_);
}


bool
GPlatesQtWidgets::FriendlyLineEdit::isReadOnly() const
{
	return d_line_edit->isReadOnly();
}


void
GPlatesQtWidgets::FriendlyLineEdit::setReadOnly(
		bool read_only)
{
	d_line_edit->setReadOnly(read_only);
}


GPlatesQtWidgets::FriendlyLineEditInternals::InternalLineEdit::InternalLineEdit(
		const QString &contents,
		const QString &message_on_empty_string,
		QWidget *parent_) :
	QLineEdit(parent_),
	d_message_on_empty_string(message_on_empty_string),
	d_default_palette(palette()),
	d_empty_string_palette(d_default_palette),
	d_default_font(font()),
	d_empty_string_font(d_default_font),
	d_is_empty_string(contents.isEmpty())
{
	d_empty_string_palette.setColor(
			QPalette::Text, QColor(128, 128, 128));
	d_empty_string_font.setItalic(true);

	// Set initial appearance.
	handle_focus_out();
}


QString
GPlatesQtWidgets::FriendlyLineEditInternals::InternalLineEdit::text() const
{
	// If we are focused, then the contents of the line edit are authoritative.
	// If we are not focused, the line could have the empty string message.
	return (hasFocus() || !d_is_empty_string) ? QLineEdit::text() : QString();
}


void
GPlatesQtWidgets::FriendlyLineEditInternals::InternalLineEdit::setText(
		const QString &text_)
{
	// Pretend that the user typed the text.
	bool has_focus = hasFocus();
	if (!has_focus)
	{
		handle_focus_in();
	}
	QLineEdit::setText(text_);
	if (!has_focus)
	{
		handle_focus_out();
	}
}


void
GPlatesQtWidgets::FriendlyLineEditInternals::InternalLineEdit::focusInEvent(
		QFocusEvent *event_)
{
	QLineEdit::focusInEvent(event_);
	handle_focus_in();
}


void
GPlatesQtWidgets::FriendlyLineEditInternals::InternalLineEdit::focusOutEvent(
		QFocusEvent *event_)
{
	QLineEdit::focusOutEvent(event_);
	handle_focus_out();
}


void
GPlatesQtWidgets::FriendlyLineEditInternals::InternalLineEdit::handle_focus_in()
{
	if (d_is_empty_string)
	{
		QLineEdit::setText(QString());
		setPalette(d_default_palette);
		setFont(d_default_font);
	}
}


void
GPlatesQtWidgets::FriendlyLineEditInternals::InternalLineEdit::handle_focus_out()
{
	if (QLineEdit::text().isEmpty())
	{
		d_is_empty_string = true;
		QLineEdit::setText(d_message_on_empty_string);
		setPalette(d_empty_string_palette);
		setFont(d_empty_string_font);
	}
	else
	{
		d_is_empty_string = false;
	}
}
