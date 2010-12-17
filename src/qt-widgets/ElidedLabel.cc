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
#include <QMouseEvent>
#include <QDebug>

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
#if 0
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
				height(),
				width());
#endif
		ElidedLabelToolTip::showToolTip(d_full_text, this);
	}
}


GPlatesQtWidgets::ElidedLabel::ElidedLabelToolTip::ElidedLabelToolTip() :
	QDialog(NULL, Qt::Popup),
	d_internal_label_frame(new QFrame(this)),
	d_internal_label(new QLabel(this)),
	d_master_label(NULL),
	d_inside_do_show(false)
{
	// Put the internal label into a frame.
	d_internal_label_frame->setFrameStyle(QFrame::Box | QFrame::Plain);
	QtWidgetUtils::add_widget_to_placeholder(d_internal_label, d_internal_label_frame);

	// Put the frame into this widget.
	QtWidgetUtils::add_widget_to_placeholder(d_internal_label_frame, this);

	setMouseTracking(true);
	d_internal_label->setMouseTracking(true);
	d_internal_label->installEventFilter(this);
}


void
GPlatesQtWidgets::ElidedLabel::ElidedLabelToolTip::showToolTip(
		const QString &text,
		QLabel *master_label)
{
	// FIXME: Temporarily disabling tooltips on Windows as two computers so far lose mouse/key
	// focus for all applications when a tooltip in the Layers dialog pops up.
	// CTRL+ALT+DEL is the only way to break the freeze after which GPlates can continue to be used.
#if !defined(WIN32)
	instance().do_show(text, master_label);
#endif
}


void
GPlatesQtWidgets::ElidedLabel::ElidedLabelToolTip::hideToolTip()
{
	// FIXME: Temporarily disabling tooltips on Windows as two computers so far lose mouse/key
	// focus for all applications when a tooltip in the Layers dialog pops up.
	// CTRL+ALT+DEL is the only way to break the freeze after which GPlates can continue to be used.
#if !defined(WIN32)
	instance().do_hide();
#endif
}


void
GPlatesQtWidgets::ElidedLabel::ElidedLabelToolTip::leaveEvent(
		QEvent *event_)
{
	do_hide();
}


void
GPlatesQtWidgets::ElidedLabel::ElidedLabelToolTip::mouseMoveEvent(
		QMouseEvent *event_)
{
	QPointF event_pos = event_->pos();
	if (event_pos.x() < 0 || event_pos.x() > d_master_label->width() ||
			event_pos.y() < 0 || event_pos.y() > height())
	{
		do_hide();
	}
}


bool
GPlatesQtWidgets::ElidedLabel::ElidedLabelToolTip::eventFilter(
		QObject *object_,
		QEvent *event_)
{
	if (event_->type() == QEvent::MouseMove)
	{
		QMouseEvent *mouse_event = static_cast<QMouseEvent *>(event_);
		if (mouse_event->pos().x() > d_master_label->width())
		{
			do_hide();
			return true;
		}
	}
	return false;
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
		QLabel *master_label)
{
	if (d_inside_do_show)
	{
		return;
	}

	if (isVisible() && d_master_label == master_label)
	{
		return;
	}
	d_master_label = master_label;

	// On MacOS, we're getting infinite loops. What's happening is that the hide()
	// call below causes the tooltip to disappear, which sometimes causes the
	// enter event of ElidedLabel to get triggered, which then calls this function,
	// and then bad things happen.
	d_inside_do_show = true;

	d_internal_label->setText(text);

	// Shift towards top-left because of frame.
	int frame_width = d_internal_label_frame->frameWidth();
	QPoint global_pos = master_label->mapToGlobal(QPoint(0, 0));
	move(global_pos - QPoint(frame_width, frame_width));

	// The tool tip should have the exact same font as the master label.
	QFontInfo font_info(master_label->font());
	QFont text_font(
			font_info.family(),
			font_info.pointSize(),
			font_info.weight(),
			font_info.italic());

	// This little song and dance is necessary to make sure the tool tip is resized
	// correctly when the user moves the mouse from one ElidedLabel to another.
	setFont(QFont());
	setFont(text_font);
	hide();
	show();
	QSize new_size = layout()->sizeHint();
	new_size.setHeight(master_label->height() + frame_width * 2);
	resize(new_size);

	grabMouse();

	d_inside_do_show = false;
}


void
GPlatesQtWidgets::ElidedLabel::ElidedLabelToolTip::do_hide()
{
	releaseMouse();
	hide();
}

