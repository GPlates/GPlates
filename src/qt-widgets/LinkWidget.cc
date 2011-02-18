/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#include <QEvent>
#include <QHBoxLayout>
#include <QMouseEvent>

#include "LinkWidget.h"


namespace
{
	const char *LINK_TEMPLATE = QT_TR_NOOP(
		"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
		"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
		"p { white-space: pre-wrap; }\n"
		"</style></head><body>\n"
		"<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">"
		"<a href=\"the_link\"><span style=\"text-decoration: none;\">%1</span></a></p></body></html>");
}


GPlatesQtWidgets::LinkWidget::LinkWidget(
		const QString &link_text,
		QWidget *parent_)
{
	init();
	set_link_text(link_text);
}


GPlatesQtWidgets::LinkWidget::LinkWidget(
		QWidget *parent_)
{
	init();
}


void
GPlatesQtWidgets::LinkWidget::set_link_text(
		const QString &link_text)
{
	d_link_text = link_text;
	update_internal_label();
}


bool
GPlatesQtWidgets::LinkWidget::event(
		QEvent *ev)
{
	if (ev->type() == QEvent::EnabledChange)
	{
		update_internal_label();
	}
	else if (ev->type() == QEvent::MouseButtonPress &&
			d_internal_label->geometry().contains(static_cast<QMouseEvent *>(ev)->pos()))
	{
		// This is necessary otherwise when this LinkWidget is embedded inside the
		// VisualLayerWidget, that tends to steal the mouse press for the drag.
		return true;
	}

	return QWidget::event(ev);
}


void
GPlatesQtWidgets::LinkWidget::handle_link_activated()
{
	emit link_activated();
}


void
GPlatesQtWidgets::LinkWidget::init()
{
	d_internal_label = new QLabel(this);
	QHBoxLayout *this_layout = new QHBoxLayout(this);
	this_layout->setContentsMargins(0, 0, 0, 0);
	this_layout->setSpacing(0);
	this_layout->addWidget(d_internal_label);
	this_layout->insertStretch(-1);
	d_internal_label->setCursor(QCursor(Qt::PointingHandCursor));

	QObject::connect(
			d_internal_label,
			SIGNAL(linkActivated(const QString &)),
			this,
			SLOT(handle_link_activated()));
}


void
GPlatesQtWidgets::LinkWidget::update_internal_label()
{
	if (isEnabled())
	{
		d_internal_label->setText(tr(LINK_TEMPLATE).arg(d_link_text));
	}
	else
	{
		d_internal_label->setText(d_link_text);
	}
}

