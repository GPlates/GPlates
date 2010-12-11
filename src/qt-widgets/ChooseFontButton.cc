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
#include <QFontDialog>
#include <QApplication>

#include "ChooseFontButton.h"


GPlatesQtWidgets::ChooseFontButton::ChooseFontButton(
		QWidget *parent_) :
	QToolButton(parent_)
{
	QObject::connect(
			this,
			SIGNAL(clicked()),
			this,
			SLOT(handle_clicked()));
}


void
GPlatesQtWidgets::ChooseFontButton::set_font(
		const QFont &font_)
{
	d_font = font_;
	
	// Show description of font on button.
	static const char *DESCRIPTION_FORMAT = QT_TR_NOOP("%1, %2pt");
	setText(tr(DESCRIPTION_FORMAT).arg(d_font.family()).arg(d_font.pointSizeF()));

	// Set button's font to be that of selected font, but with default font size.
	static const qreal DEFAULT_FONT_SIZE = QApplication::font().pointSizeF();
	QFont modified_font = d_font;
	modified_font.setPointSizeF(DEFAULT_FONT_SIZE);
	setFont(modified_font);
}


void
GPlatesQtWidgets::ChooseFontButton::handle_clicked()
{
	bool ok;
	QFont new_font = QFontDialog::getFont(&ok, d_font, parentWidget());
	if (ok)
	{
		set_font(new_font);
	}
}

