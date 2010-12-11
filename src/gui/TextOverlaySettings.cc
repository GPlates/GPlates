/* $Id$ */

/**
 * @file 
 *
 * Most recent change:
 *   $Date$
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
#include <QFontInfo>

#include "TextOverlaySettings.h"


const char *
GPlatesGui::TextOverlaySettings::DEFAULT_TEXT = "%f Ma";

const GPlatesGui::Colour
GPlatesGui::TextOverlaySettings::DEFAULT_COLOUR = GPlatesGui::Colour::get_white();

const GPlatesGui::TextOverlaySettings::Anchor
GPlatesGui::TextOverlaySettings::DEFAULT_ANCHOR = GPlatesGui::TextOverlaySettings::TOP_LEFT;

const int
GPlatesGui::TextOverlaySettings::DEFAULT_X_OFFSET = 20;

const int
GPlatesGui::TextOverlaySettings::DEFAULT_Y_OFFSET = 20;

const bool
GPlatesGui::TextOverlaySettings::DEFAULT_IS_ENABLED = false;

const bool
GPlatesGui::TextOverlaySettings::DEFAULT_HAS_SHADOW = true;


namespace
{
	QFont
	get_default_font()
	{
		static const qreal SCALE = 1.5;

		QFont result = QApplication::font();
		result.setPointSize(static_cast<int>(QFontInfo(result).pointSizeF() * SCALE));

		return result;
	}
}


GPlatesGui::TextOverlaySettings::TextOverlaySettings() :
	d_text(DEFAULT_TEXT),
	d_font(get_default_font()),
	d_colour(DEFAULT_COLOUR),
	d_anchor(DEFAULT_ANCHOR),
	d_x_offset(DEFAULT_X_OFFSET),
	d_y_offset(DEFAULT_Y_OFFSET),
	d_is_enabled(DEFAULT_IS_ENABLED),
	d_has_shadow(DEFAULT_HAS_SHADOW)
{  }

