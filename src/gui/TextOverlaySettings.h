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

#ifndef GPLATES_GUI_TEXTOVERLAYSETTINGS_H
#define GPLATES_GUI_TEXTOVERLAYSETTINGS_H

#include <boost/operators.hpp>
#include <QString>
#include <QFont>

#include "Colour.h"


namespace GPlatesGui
{
	class TextOverlaySettings :
			public boost::equality_comparable<TextOverlaySettings>
	{
	public:

		enum Anchor
		{
			TOP_LEFT,
			TOP_RIGHT,
			BOTTOM_LEFT,
			BOTTOM_RIGHT
		};

		/**
		 * Constructs a TextOverlaySettings with default values.
		 */
		explicit
		TextOverlaySettings();

		const QString &
		get_text() const
		{
			return d_text;
		}

		void
		set_text(
				const QString &text)
		{
			d_text = text;
		}

		int
		get_decimal_places() const
		{
			return d_decimal_places;
		}

		void
		set_decimal_places(
				int dp)
		{
			d_decimal_places = dp;
		}

		const QFont &
		get_font() const
		{
			return d_font;
		}

		void
		set_font(
				const QFont &font)
		{
			d_font = font;
		}

		const GPlatesGui::Colour &
		get_colour() const
		{
			return d_colour;
		}

		void
		set_colour(
				const GPlatesGui::Colour &colour)
		{
			d_colour = colour;
		}

		Anchor
		get_anchor() const
		{
			return d_anchor;
		}

		void
		set_anchor(
				Anchor anchor)
		{
			d_anchor = anchor;
		}

		int
		get_x_offset() const
		{
			return d_x_offset;
		}

		void
		set_x_offset(
				int x_offset)
		{
			d_x_offset = x_offset;
		}

		int
		get_y_offset() const
		{
			return d_y_offset;
		}

		void
		set_y_offset(
				int y_offset)
		{
			d_y_offset = y_offset;
		}

		bool
		is_enabled() const
		{
			return d_is_enabled;
		}

		void
		set_enabled(
				bool enabled)
		{
			d_is_enabled = enabled;
		}

		bool
		has_shadow() const
		{
			return d_has_shadow;
		}

		void
		set_shadow(
				bool shadow)
		{
			d_has_shadow = shadow;
		}

		static const char *DEFAULT_TEXT;
		static const int DEFAULT_DECIMAL_PLACES;
		static const GPlatesGui::Colour DEFAULT_COLOUR;
		static const Anchor DEFAULT_ANCHOR;
		static const int DEFAULT_X_OFFSET;
		static const int DEFAULT_Y_OFFSET;
		static const bool DEFAULT_IS_ENABLED;
		static const bool DEFAULT_HAS_SHADOW;

	private:

		QString d_text;
		int d_decimal_places;
		QFont d_font;
		GPlatesGui::Colour d_colour;
		Anchor d_anchor;
		int d_x_offset;
		int d_y_offset;
		bool d_is_enabled;
		bool d_has_shadow;

		friend
		bool
		operator==(
				const TextOverlaySettings &lhs,
				const TextOverlaySettings &rhs)
		{
			return lhs.d_colour == rhs.d_colour;
		}
	};

}

#endif  // GPLATES_GUI_TEXTOVERLAYSETTINGS_H
