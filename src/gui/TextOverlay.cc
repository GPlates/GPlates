/* $Id$ */

/**
 * @file 
 *
 * Most recent change:
 *   $Date$
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
#include <QLocale>

#include "TextOverlay.h"

#include "TextOverlaySettings.h"

#include "app-logic/ApplicationState.h"


GPlatesGui::TextOverlay::TextOverlay(
		const GPlatesAppLogic::ApplicationState &application_state,
		const TextRenderer::non_null_ptr_to_const_type &text_renderer) :
	d_application_state(application_state),
	d_text_renderer(text_renderer)
{  }


void
GPlatesGui::TextOverlay::paint(
		const TextOverlaySettings &settings,
		int canvas_width,
		int canvas_height,
		float scale)
{
	if (!settings.is_enabled())
	{
		return;
	}

	// Substitute %f for the current reconstruction time.
	QLocale loc;
	QString time = loc.toString(d_application_state.get_current_reconstruction_time(), 'f', settings.get_decimal_places());
	QString substituted = settings.get_text();
	substituted.replace("%f", time);

	// Scale the x and y offsets.
	float x_offset = settings.get_x_offset() * scale;
	float y_offset = settings.get_y_offset() * scale;

	// Work out position of text.
	QFontMetrics fm(settings.get_font());
	int x;
	if (settings.get_anchor() == GPlatesGui::TextOverlaySettings::TOP_LEFT ||
		settings.get_anchor() == GPlatesGui::TextOverlaySettings::BOTTOM_LEFT)
	{
		x = static_cast<int>(x_offset);
	}
	else // TOP_RIGHT, BOTTOM_RIGHT
	{
		float text_width = fm.width(substituted) * scale;
		x = static_cast<int>(canvas_width - x_offset - text_width);
	}

	int y;
	if (settings.get_anchor() == GPlatesGui::TextOverlaySettings::TOP_LEFT ||
		settings.get_anchor() == GPlatesGui::TextOverlaySettings::TOP_RIGHT)
	{
		float text_height = fm.height() * scale;
		y = static_cast<int>(y_offset + text_height);
	}
	else // BOTTOM_LEFT, BOTTOM_RIGHT
	{
		y = static_cast<int>(canvas_height - y_offset);
	}

	if (settings.has_shadow())
	{
		// The shadow's colour is black, with the alpha value copied across.
		Colour shadow_colour = GPlatesGui::Colour::get_black();
		shadow_colour.alpha() = settings.get_colour().alpha();

		d_text_renderer->render_text(
				x + 1,
				y + 1,
				substituted,
				shadow_colour,
				settings.get_font(),
				scale);
	}

	d_text_renderer->render_text(
			x,
			y,
			substituted,
			settings.get_colour(),
			settings.get_font(),
			scale);
}

