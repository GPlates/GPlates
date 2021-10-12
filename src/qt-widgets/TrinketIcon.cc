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

#include "TrinketIcon.h"


GPlatesQtWidgets::TrinketIcon::TrinketIcon(
		const QIcon &icon,
		const QString &tooltip,
		QWidget *parent_):
	QLabel(parent_),
	d_clickable(false)
{
	setIcon(icon);
	setToolTip(tooltip);
}


void
GPlatesQtWidgets::TrinketIcon::setIcon(
		const QIcon &icon)
{
	d_pixmap_normal = icon.pixmap(22, QIcon::Normal, QIcon::On);
	d_pixmap_clicking = icon.pixmap(22, QIcon::Selected, QIcon::On);
	setPixmap(d_pixmap_normal);
}


void
GPlatesQtWidgets::TrinketIcon::mousePressEvent(
		QMouseEvent *ev)
{
	setPixmap(d_pixmap_clicking);
	ev->accept();
}

void
GPlatesQtWidgets::TrinketIcon::mouseMoveEvent(
		QMouseEvent *ev)
{
	// As we are not explicitly asking Qt to turn tracking on, we will only receive this
	// event if a mouse button is down while it occurs.
	if (rect().contains(ev->pos())) {
		setPixmap(d_pixmap_clicking);
	} else {
		setPixmap(d_pixmap_normal);
	}
	ev->accept();
}

void
GPlatesQtWidgets::TrinketIcon::mouseReleaseEvent(
		QMouseEvent *ev)
{
	setPixmap(d_pixmap_normal);
	if (rect().contains(ev->pos())) {
		// If we have a boost::function we can call, do that.
		if (d_clicked_callback) {
			d_clicked_callback();
		}
		// Let's also emit a signal, in case that approach is preferable.
		emit clicked(this, ev);
	}
	ev->accept();
}


