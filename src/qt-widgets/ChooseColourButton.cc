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

#include <QString>
#include <QIcon>

#include "ChooseColourButton.h"

#include "QtWidgetUtils.h"


GPlatesQtWidgets::ChooseColourButton::ChooseColourButton(
		QWidget *parent_) :
	QToolButton(parent_),
	d_colour(GPlatesGui::Colour::get_white())
{
	QObject::connect(
			this,
			SIGNAL(clicked()),
			this,
			SLOT(handle_clicked()));
}


void
GPlatesQtWidgets::ChooseColourButton::set_colour(
		const GPlatesGui::Colour &colour)
{
	d_colour = colour;
	
	// Set tooltip to display R, G and B of colour.
	GPlatesGui::rgba8_t rgba = GPlatesGui::Colour::to_rgba8(colour);
	static const char *TOOLTIP_TEMPLATE = QT_TR_NOOP("(%1, %2, %3)");
	QString tooltip = QObject::tr(TOOLTIP_TEMPLATE).arg(rgba.red).arg(rgba.green).arg(rgba.blue);
	setToolTip(tooltip);

	// Create an icon to display the colour.
	QPixmap pixmap(iconSize());
	pixmap.fill(colour);
	setIcon(QIcon(pixmap));
}


void
GPlatesQtWidgets::ChooseColourButton::handle_clicked()
{
	boost::optional<GPlatesGui::Colour> new_colour =
		QtWidgetUtils::get_colour_with_alpha(d_colour, parentWidget());
	if (new_colour)
	{
		set_colour(*new_colour);
	}
}

