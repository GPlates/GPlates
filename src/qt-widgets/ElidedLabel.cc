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
#include <QDebug>

#include "ElidedLabel.h"

#include "QtWidgetUtils.h"


// There is a special character for the ellipsis, but for now, let's just use three periods
// so we don't run into complications where the font doesn't have an ellipsis.
const QString
GPlatesQtWidgets::ElidedLabel::ELLIPSIS = "...";


GPlatesQtWidgets::ElidedLabel::ElidedLabel(
		QWidget *parent_) :
	QWidget(parent_)
{
	init();
}


GPlatesQtWidgets::ElidedLabel::ElidedLabel(
		const QString &text_,
		QWidget *parent_) :
	QWidget(parent_)
{
	init();

	setText(text_);
}


void
GPlatesQtWidgets::ElidedLabel::init()
{
	// Initialisation common to both constructors.
	d_internal_label = new QLabel(this);
	QtWidgetUtils::add_widget_to_placeholder(d_internal_label, this);
	d_substring_widths_needs_updating = false;
	d_is_truncated = false;
}


void
GPlatesQtWidgets::ElidedLabel::setText(
		const QString &text_)
{
	// Remember the text.
	d_text = text_;

	// Give the full text to the internal label for now so that the sizeHint (well,
	// at least the height component) is calculated correctly.
	d_internal_label->setText(text_);

	// Mark @a d_substring_widths as needing updating.
	d_substring_widths_needs_updating = true;
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
	d_internal_label->setFrameStyle(style_);
}


int
GPlatesQtWidgets::ElidedLabel::frameStyle() const
{
	return d_internal_label->frameStyle();
}


void
GPlatesQtWidgets::ElidedLabel::resizeEvent(
		QResizeEvent *event_)
{
	update_internal_label();
}


void
GPlatesQtWidgets::ElidedLabel::enterEvent(
		QEvent *event_)
{
	if (d_is_truncated)
	{
		// The tooltip should have the same font family and size as our label.
		QFontInfo font_info(d_internal_label->font());
		QFont tool_tip_font(
				font_info.family(),
				font_info.pointSize(),
				font_info.weight(),
				font_info.italic());

		// The tool tip should be positioned on top of our label.
		QPoint tool_tip_pos = mapToGlobal(d_internal_label->pos());

		// Move the tool tip towards bottom-right if the label has a frame.
		if (d_internal_label->frameShape() != QFrame::NoFrame)
		{
			int frame_width = d_internal_label->frameWidth();
			tool_tip_pos += QPoint(frame_width, frame_width);
		}

		ElidedLabelToolTip::showToolTip(d_text, tool_tip_pos, tool_tip_font);
	}
}


void
GPlatesQtWidgets::ElidedLabel::paintEvent(
		QPaintEvent *event_)
{
	if (d_substring_widths_needs_updating)
	{
		d_substring_widths_needs_updating = false;

		update_substring_widths_map();
		update_internal_label();
	}

	QWidget::paintEvent(event_);
}

void
GPlatesQtWidgets::ElidedLabel::update_substring_widths_map()
{
	substring_width_map_type substring_widths;

	if (d_text.length())
	{
		QFontMetrics font_metrics = fontMetrics();

		// Calculate the width of the full string.
		int full_width = font_metrics.width(d_text);

		// Go through each of the substrings, and calculate the width of that substring
		// with the ellipsis at the end. Ignore any that are wider (once the ellipsis
		// has been added) than the width of the full string.
		// Note that the loop does not consider the empty string or the full string as
		// substrings.
		for (int i = 1; i != d_text.length(); ++i)
		{
			QString substring = d_text.left(i);
			substring.append(ELLIPSIS);

			int substring_width = font_metrics.width(substring);
			if (substring_width < full_width)
			{
				substring_widths.insert(std::make_pair(substring_width, i));
			}
		}

		// Now add the length of the full string, without any ellipsis.
		substring_widths.insert(std::make_pair(full_width, d_text.length()));
	}

	std::swap(d_substring_widths, substring_widths);
}


void
GPlatesQtWidgets::ElidedLabel::update_internal_label()
{
	// Handle empty string case.
	if (d_text.isEmpty())
	{
		d_internal_label->setText(QString());
		return;
	}

	substring_width_map_type::const_iterator iter = d_substring_widths.upper_bound(width());

	// iter points to entry after the one that we want.
	// We can always decrement iter because there is always at least one entry in
	// the map, unless iter points to the first element.
	if (iter == d_substring_widths.begin())
	{
		// This means the width isn't enough to even display one character, plus
		// ellipsis. So let's just display the empty string.
		d_internal_label->setText(QString());
		d_is_truncated = true;
	}
	else
	{
		--iter;
		int num_chars = iter->second;

		// Put the ellipsis after the substring if the substring is not the full text.
		if (num_chars == d_text.length())
		{
			d_internal_label->setText(d_text);
			d_is_truncated = false;
		}
		else
		{
			d_internal_label->setText(d_text.left(num_chars) + ELLIPSIS);
			d_is_truncated = true;
		}
	}
}


GPlatesQtWidgets::ElidedLabel::ElidedLabelToolTip::ElidedLabelToolTip() :
	QDialog(NULL, Qt::Popup),
	d_internal_label(new QLabel(this))
{
	// Put the internal label into a frame.
	QFrame *label_frame = new QFrame(this);
	label_frame->setFrameStyle(QFrame::Box | QFrame::Plain);
	QtWidgetUtils::add_widget_to_placeholder(d_internal_label, label_frame);

	// Put the frame into this widget.
	QtWidgetUtils::add_widget_to_placeholder(label_frame, this);
}


void
GPlatesQtWidgets::ElidedLabel::ElidedLabelToolTip::showToolTip(
		const QString &text,
		const QPoint &global_pos,
		const QFont &text_font)
{
	instance().do_show(text, global_pos, text_font);
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
		const QPoint &global_pos,
		const QFont &text_font)
{
	d_internal_label->setText(text);

	// Shift 1px towards top-left because of 1px frame.
	move(global_pos - QPoint(1, 1));

	// This roundabout way of setting the font is a hack to make sure the size of
	// tool tip is recalculated.
	setFont(QFont());
	setFont(text_font);

	show();
}
