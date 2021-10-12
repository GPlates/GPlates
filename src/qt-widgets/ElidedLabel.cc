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

#include <QFontMetrics>
#include <QToolTip>
#include <QFrame>
#include <QFontInfo>
#include <QLayout>

#include "ElidedLabel.h"

#include "QtWidgetUtils.h"


GPlatesQtWidgets::ElidedLabel::ElidedLabel(
		Qt::TextElideMode mode,
		QWidget *parent_) :
	QWidget(parent_),
	d_mode(mode)
{
	init();
}


GPlatesQtWidgets::ElidedLabel::ElidedLabel(
		const QString &text_,
		Qt::TextElideMode mode,
		QWidget *parent_) :
	QWidget(parent_),
	d_mode(mode)
{
	init();

	setText(text_);
}


void
GPlatesQtWidgets::ElidedLabel::init()
{
	// Create the frame around the internal label and add it to this widget.
	d_internal_label_frame = new QFrame(this);
	QtWidgetUtils::add_widget_to_placeholder(d_internal_label_frame, this);

	// Create the internal label and add it to the frame.
	d_internal_label = new InternalLabel(d_text, d_is_elided, this);
	QtWidgetUtils::add_widget_to_placeholder(d_internal_label, d_internal_label_frame);

	d_is_elided = false;
	d_internal_label_needs_updating = false;
}


void
GPlatesQtWidgets::ElidedLabel::set_text_elide_mode(
		Qt::TextElideMode mode)
{
	d_mode = mode;
}


Qt::TextElideMode
GPlatesQtWidgets::ElidedLabel::get_text_elide_mode() const
{
	return d_mode;
}


void
GPlatesQtWidgets::ElidedLabel::setText(
		const QString &text_)
{
	d_text = text_;
	
	// We will calculate the elided text only upon painting, but for now, we give
	// the full text to the internal label so that the sizeHint (well, at least
	// the height component) is calculated correctly.
	d_internal_label->setText(text_);

	d_internal_label_needs_updating = true;
}


QString
GPlatesQtWidgets::ElidedLabel::text() const
{
	return d_text;
}


void
GPlatesQtWidgets::ElidedLabel::setFrameStyle(
		int style_)
{
	d_internal_label_frame->setFrameStyle(style_);
}


int
GPlatesQtWidgets::ElidedLabel::frameStyle() const
{
	return d_internal_label_frame->frameStyle();
}


void
GPlatesQtWidgets::ElidedLabel::resizeEvent(
		QResizeEvent *event_)
{
	d_internal_label_needs_updating = true;
}


void
GPlatesQtWidgets::ElidedLabel::paintEvent(
		QPaintEvent *event_)
{
	if (d_internal_label_needs_updating)
	{
		d_internal_label_needs_updating = false;
		update_internal_label();
	}

	QWidget::paintEvent(event_);
}


void
GPlatesQtWidgets::ElidedLabel::update_internal_label()
{
	// Calculate the elided string and show it in the internal label.
	QFontMetrics font_metrics(font());
	QString elided_text = font_metrics.elidedText(d_text, d_mode, d_internal_label->width());
	d_internal_label->setText(elided_text);
	d_is_elided = (elided_text != d_text);
}


GPlatesQtWidgets::ElidedLabel::InternalLabel::InternalLabel(
		const QString &full_text,
		const bool &is_elided,
		QWidget *parent_) :
	QLabel(parent_),
	d_full_text(full_text),
	d_is_elided(is_elided)
{
}


void
GPlatesQtWidgets::ElidedLabel::InternalLabel::enterEvent(
		QEvent *event_)
{
	// Only show a tool tip if currently elided.
	if (d_is_elided)
	{
		// The tool tip should have the exact same font as 'this'.
		QFontInfo font_info(font());
		QFont tool_tip_font(
				font_info.family(),
				font_info.pointSize(),
				font_info.weight(),
				font_info.italic());

		// The tool tip should be positioned on top of 'this'.
		QPoint tool_tip_pos = mapToGlobal(QPoint(0, 0));

		ElidedLabelToolTip::showToolTip(
				d_full_text,
				tool_tip_font,
				tool_tip_pos,
				height());
	}
}


GPlatesQtWidgets::ElidedLabel::ElidedLabelToolTip::ElidedLabelToolTip() :
	QDialog(NULL, Qt::Popup),
	d_internal_label_frame(new QFrame(this)),
	d_internal_label(new QLabel(this))
{
	// Put the internal label into a frame.
	d_internal_label_frame->setFrameStyle(QFrame::Box | QFrame::Plain);
	QtWidgetUtils::add_widget_to_placeholder(d_internal_label, d_internal_label_frame);

	// Put the frame into this widget.
	QtWidgetUtils::add_widget_to_placeholder(d_internal_label_frame, this);
}


void
GPlatesQtWidgets::ElidedLabel::ElidedLabelToolTip::showToolTip(
		const QString &text,
		const QFont &text_font,
		const QPoint &global_pos,
		int label_height)
{
	instance().do_show(text, text_font, global_pos, label_height);
}


void
GPlatesQtWidgets::ElidedLabel::ElidedLabelToolTip::hideToolTip()
{
	instance().do_hide();
}


void
GPlatesQtWidgets::ElidedLabel::ElidedLabelToolTip::leaveEvent(
		QEvent *event_)
{
	hide();
}


GPlatesQtWidgets::ElidedLabel::ElidedLabelToolTip &
GPlatesQtWidgets::ElidedLabel::ElidedLabelToolTip::instance()
{
	static ElidedLabelToolTip *instance = new ElidedLabelToolTip();
	return *instance;
}


void
GPlatesQtWidgets::ElidedLabel::ElidedLabelToolTip::do_show(
		const QString &text,
		const QFont &text_font,
		const QPoint &global_pos,
		int label_height)
{
	d_internal_label->setText(text);

	// Shift towards top-left because of frame.
	int frame_width = d_internal_label_frame->frameWidth();
	move(global_pos - QPoint(frame_width, frame_width));

	// This little song and dance is necessary to make sure the tool tip is resized
	// correctly when the user moves the mouse from one ElidedLabel to another.
	setFont(QFont());
	setFont(text_font);
	hide();
	show();
	QSize new_size = layout()->sizeHint();
	new_size.setHeight(label_height + frame_width * 2);
	resize(new_size);
}


void
GPlatesQtWidgets::ElidedLabel::ElidedLabelToolTip::do_hide()
{
	hide();
}

