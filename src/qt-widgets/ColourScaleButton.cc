/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2016 The University of Sydney, Australia
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

#include <cmath>
#include <utility>
#include <boost/optional.hpp>
#include <boost/numeric/conversion/converter.hpp>
#include <boost/foreach.hpp>
#include <QBrush>
#include <QDebug>
#include <QFontMetrics>
#include <QLocale>
#include <QPainter>
#include <QPalette>
#include <QResizeEvent>

#include "ColourScaleButton.h"

#include "gui/Colour.h"


GPlatesQtWidgets::ColourScaleButton::ColourScaleButton(
		QWidget *parent_) :
	QToolButton(parent_),
	d_curr_colour_palette(GPlatesGui::RasterColourPalette::create()),
	d_mouse_inside_button(false),
	d_mouse_pressed(false)
{
	QObject::connect(
			this, SIGNAL(pressed()),
			this, SLOT(handle_pressed()));
	QObject::connect(
			this, SIGNAL(released()),
			this, SLOT(handle_released()));
}


bool
GPlatesQtWidgets::ColourScaleButton::populate(
		const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &colour_palette,
		boost::optional<double> use_log_scale)
{
	d_curr_colour_palette = colour_palette;
	d_use_log_scale = use_log_scale;

	return regenerate_contents();
}


void
GPlatesQtWidgets::ColourScaleButton::paintEvent(
		QPaintEvent *ev)
{
	QPainter painter(this);

	// Paint the background.
	QPalette this_palette = palette();
	painter.fillRect(
			0,
			0,
			width(),
			height(),
			QBrush(this_palette.color(QPalette::Window)));

	// Draw the colour scale.
	painter.drawPixmap(
			1,
			1,
			isEnabled() ? d_colour_scale_pixmap : d_disabled_colour_scale_pixmap);

	// If the mouse is inside the button then draw a semi-transparent fill in the highlight colour
	// over the top of the button. This is needed, in addition to a border highlight, to make the
	// highlight more obvious to the user.
	if (d_mouse_inside_button)
	{
		QColor highlight_fill_colour = this_palette.color(QPalette::Highlight);

		// If mouse is also pressed (inside button) then make the highlight darker (more opaque).
		if (d_mouse_pressed)
		{
			highlight_fill_colour.setAlpha(128);
		}
		else
		{
			highlight_fill_colour.setAlpha(64);
		}

		painter.fillRect(
				1,
				1,
				d_colour_scale_pixmap.width(),
				d_colour_scale_pixmap.height(),
				QBrush(highlight_fill_colour));
	}

	//
	// Draw a border around the colour scale.
	//

	// Use highlight colour if mouse cursor is inside the button.
	QColor pen_colour = d_mouse_inside_button
			? this_palette.color(QPalette::Highlight)
			: QColor(Qt::gray);

	QPen border_pen(pen_colour);
	border_pen.setWidth(1);
	painter.setPen(border_pen);

	// The regular 1-pixel border just outside pixmap (ie, doesn't overwrite pixmap).
	painter.drawRect(
			0,
			0,
			d_colour_scale_pixmap.width() + 1,
			d_colour_scale_pixmap.height() + 1);

	// If mouse pressed (inside button) then extend the border by one pixel *into* the pixmap (ie, overwrites pixmap).
	if (d_mouse_pressed)
	{
		painter.drawRect(
				1,
				1,
				d_colour_scale_pixmap.width() - 1,
				d_colour_scale_pixmap.height() - 1);
	}
}


void
GPlatesQtWidgets::ColourScaleButton::resizeEvent(
		QResizeEvent *ev)
{
	if (ev->oldSize() != size())
	{
		regenerate_contents();
	}

	QToolButton::resizeEvent(ev);
}


void
GPlatesQtWidgets::ColourScaleButton::enterEvent(
		QEvent *ev)
{
	d_mouse_inside_button = true;

	// Need to re-draw so we can highlight button.
	// Seems this is needed for Mac, but not Windows or Ubuntu (for Qt 4.8).
	update();

	QToolButton::enterEvent(ev);
}


void
GPlatesQtWidgets::ColourScaleButton::leaveEvent(
		QEvent *ev)
{
	d_mouse_inside_button = false;

	// Need to re-draw so we can unhighlight button.
	// Seems this is needed for Mac, but not Windows or Ubuntu (for Qt 4.8).
	update();

	QToolButton::leaveEvent(ev);
}


QSize
GPlatesQtWidgets::ColourScaleButton::sizeHint() const
{
	return QSize(MINIMUM_WIDTH, MINIMUM_HEIGHT);
}


QSize
GPlatesQtWidgets::ColourScaleButton::minimumSizeHint() const
{
	return QSize(MINIMUM_WIDTH, MINIMUM_HEIGHT);
}


void
GPlatesQtWidgets::ColourScaleButton::handle_pressed()
{
	d_mouse_pressed = true;
	update(); // Need to re-draw highlight
}


void
GPlatesQtWidgets::ColourScaleButton::handle_released()
{
	d_mouse_pressed = false;
	update(); // Need to re-draw highlight
}


bool
GPlatesQtWidgets::ColourScaleButton::regenerate_contents()
{
	// Need a one pixel border around the pixmap when drawing button.
	int pixmap_width = width() - 2;
	int pixmap_height = height() - 2;

	if (!GPlatesGui::ColourScale::generate(
			d_curr_colour_palette,
			d_colour_scale_pixmap,
			d_disabled_colour_scale_pixmap,
			pixmap_width,
			pixmap_height,
			d_use_log_scale))
	{
		return false;
	}

	update();

	return true;
}
