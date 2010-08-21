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

#include <QApplication>
#include <QDesktopWidget>
#include <QHBoxLayout>
#include <QRect>
#include <QSize>

#include "QtWidgetUtils.h"


void
GPlatesQtWidgets::QtWidgetUtils::add_widget_to_placeholder(
		QWidget *widget,
		QWidget *placeholder)
{
	QHBoxLayout *layout = new QHBoxLayout(placeholder);
	layout->addWidget(widget);
	layout->setContentsMargins(0, 0, 0, 0);
}


void
GPlatesQtWidgets::QtWidgetUtils::reposition_to_side_of_parent(
		QDialog *dialog)
{
	QWidget *par = dialog->parentWidget();
	if (par)
	{
		QRect frame_geometry = dialog->frameGeometry();
		int new_x = par->pos().x() + par->frameGeometry().width();
		int new_y = par->pos().y() + (par->frameGeometry().height() - frame_geometry.height()) / 2;

		// Ensure the dialog is not off-screen.
		QDesktopWidget *desktop = QApplication::desktop();
		QRect screen_geometry = desktop->screenGeometry(par);
		if (new_x + frame_geometry.width() > screen_geometry.right())
		{
			new_x = screen_geometry.right() - frame_geometry.width();
		}
		if (new_y + frame_geometry.height() > screen_geometry.bottom())
		{
			new_y = screen_geometry.bottom() - frame_geometry.height();
		}

		dialog->move(new_x, new_y);
	}
}

