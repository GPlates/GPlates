/* $Id$ */

/**
 * \file 
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

#include <QFontMetrics>
#include <QToolTip>
#include <QFrame>
#include <QFontInfo>
#include <QLayout>
#include <QHelpEvent>

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
	d_internal_label = new InternalLabel(this);
	QtWidgetUtils::add_widget_to_placeholder(d_internal_label, d_internal_label_frame);

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

	bool is_elided = (elided_text != d_text);
	d_internal_label->setToolTip(is_elided ? d_text : QString());
}


GPlatesQtWidgets::ElidedLabel::InternalLabel::InternalLabel(
		QWidget *parent_) :
	QLabel(parent_)
{  }


bool
GPlatesQtWidgets::ElidedLabel::InternalLabel::event(
		QEvent *ev)
{
	if (ev->type() == QEvent::ToolTip)
	{
		// Always show tooltip anchored to top left (but of course it might not appear
		// exactly at the top left due to platform-dependent offsets).
		QPoint help_pos(0, 0);
		QHelpEvent *help_ev = new QHelpEvent(QEvent::ToolTip, help_pos, mapToGlobal(help_pos));
		bool result = QLabel::event(help_ev);
		delete help_ev;

		return result;
	}
	else
	{
		return QLabel::event(ev);
	}
}

