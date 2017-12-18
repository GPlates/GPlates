/* $Id$ */

/**
 * @file 
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2017 Geological Survey of Norway
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

#include "VelocityLegendOverlaySettings.h"


const GPlatesGui::Colour
GPlatesGui::VelocityLegendOverlaySettings::DEFAULT_SCALE_TEXT_COLOUR = GPlatesGui::Colour::get_white();

const GPlatesGui::Colour
GPlatesGui::VelocityLegendOverlaySettings::DEFAULT_ARROW_COLOUR = GPlatesGui::Colour::get_white();

// A tasteful semi-transparent blue.
const GPlatesGui::Colour
GPlatesGui::VelocityLegendOverlaySettings::DEFAULT_BACKGROUND_COLOUR = GPlatesGui::Colour(0,0,1,0.35f);

const GPlatesGui::VelocityLegendOverlaySettings::Anchor
GPlatesGui::VelocityLegendOverlaySettings::DEFAULT_ANCHOR = GPlatesGui::VelocityLegendOverlaySettings::TOP_LEFT;

const int
GPlatesGui::VelocityLegendOverlaySettings::DEFAULT_X_OFFSET = 20;

const int
GPlatesGui::VelocityLegendOverlaySettings::DEFAULT_Y_OFFSET = 20;

const int
GPlatesGui::VelocityLegendOverlaySettings::DEFAULT_ARROW_LENGTH = 100;

const int
GPlatesGui::VelocityLegendOverlaySettings::DEFAULT_ARROW_ANGLE = 0;

const double
GPlatesGui::VelocityLegendOverlaySettings::DEFAULT_ARROW_SCALE = 2; // cm/yr

const double
GPlatesGui::VelocityLegendOverlaySettings::DEFAULT_BACKGROUND_OPACITY = 0.5;

const bool
GPlatesGui::VelocityLegendOverlaySettings::DEFAULT_IS_ENABLED = false;

const bool
GPlatesGui::VelocityLegendOverlaySettings::DEFAULT_BACKGROUND_ENABLED = true;



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


GPlatesGui::VelocityLegendOverlaySettings::VelocityLegendOverlaySettings() :
	d_scale_text_font(get_default_font()),
	d_scale_text_colour(DEFAULT_SCALE_TEXT_COLOUR),
	d_arrow_colour(DEFAULT_ARROW_COLOUR),
	d_background_colour(DEFAULT_BACKGROUND_COLOUR),
	d_anchor(DEFAULT_ANCHOR),
	d_x_offset(DEFAULT_X_OFFSET),
	d_y_offset(DEFAULT_Y_OFFSET),
	d_arrow_length(DEFAULT_ARROW_LENGTH),
	d_arrow_angle(DEFAULT_ARROW_ANGLE),
	d_scale(DEFAULT_ARROW_SCALE),
	d_background_opacity(DEFAULT_BACKGROUND_OPACITY),
	d_is_enabled(DEFAULT_IS_ENABLED),
	d_background_enabled(DEFAULT_BACKGROUND_ENABLED)
{  }

