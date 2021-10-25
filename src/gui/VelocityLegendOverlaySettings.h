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

#ifndef GPLATES_GUI_VELOCITYLEGENDOVERLAYSETTINGS_H
#define GPLATES_GUI_VELOCITYLEGENDOVERLAYSETTINGS_H

#include <boost/weak_ptr.hpp>

#include <QString>
#include <QFont>

#include "presentation/VisualLayers.h"

#include "Colour.h"


namespace GPlatesGui
{
	class VelocityLegendOverlaySettings
	{
	public:

		enum Anchor
		{
			TOP_LEFT,
			TOP_RIGHT,
			BOTTOM_LEFT,
			BOTTOM_RIGHT
		};

		enum ArrowLengthType
		{
			/***
			 * DYNAMIC_ARROW_LENGTH: The velocity scale (cm/yr) is fixed by the user, and the
			 * legend's arrow length (and surrounding box, if enabled) changes in response to zoom etc in order
			 * to maintain the fixed scale.
			 */
			DYNAMIC_ARROW_LENGTH,

			/***
			 * FIXED_ARROW_LENGTH: the arrow length is (approximately) fixed by the user, and the velocity scale (cm/yr)
			 *  changes in response to zoom etc to maintain the fixed arrow length.
			 * The velocity scale does not change freely however: it is restricted to multiples of 2, 5, 10, 20 etc cm / yr
			 *
			 */
			MAXIMUM_ARROW_LENGTH
		};

		/**
		 * Constructs a VelocityLegendOverlaySettings with default values.
		 */
		explicit
		VelocityLegendOverlaySettings();

		const QFont &
		get_scale_text_font() const
		{
			return d_scale_text_font;
		}

		void
		set_scale_text_font(
				const QFont &font)
		{
			d_scale_text_font = font;
		}

		const GPlatesGui::Colour &
		get_scale_text_colour() const
		{
			return d_scale_text_colour;
		}

		void
		set_scale_text_colour(
				const GPlatesGui::Colour &colour)
		{
			d_scale_text_colour = colour;
		}

		const GPlatesGui::Colour &
		get_arrow_colour() const
		{
			return d_arrow_colour;
		}

		void
		set_arrow_colour(
				const GPlatesGui::Colour &colour)
		{
			d_arrow_colour = colour;
		}

		const GPlatesGui::Colour &
		get_background_colour() const
		{
			return d_background_colour;
		}

		void
		set_background_colour(
				const GPlatesGui::Colour &colour)
		{
			d_background_colour = colour;
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

		int
		get_arrow_length() const
		{
			return d_arrow_length;
		}

		void
		set_arrow_length(
				int length)
		{
			d_arrow_length = length;
		}

		int
		get_arrow_angle() const
		{
			return d_arrow_angle;
		}

		void
		set_arrow_angle(
				int angle)
		{
			d_arrow_angle = angle;
		}

		double
		get_arrow_scale() const
		{
			return d_scale;
		}

		void
		set_arrow_scale(
				double scale)
		{
			d_scale = scale;
		}

		double
		get_background_opacity() const
		{
			return d_background_opacity;
		}

		void
		set_background_opacity(
				double opacity)
		{
			d_background_opacity = opacity;
		}

		bool
		background_enabled() const
		{
			return d_background_enabled;
		}

		void
		set_background_enabled(
				bool enabled)
		{
			d_background_enabled = enabled;
		}

		ArrowLengthType
		get_arrow_length_type() const
		{
			return d_arrow_length_type;
		}

		void
		set_arrow_length_type(
				ArrowLengthType type)
		{
			d_arrow_length_type = type;
		}

		boost::weak_ptr<GPlatesPresentation::VisualLayer>
		get_selected_velocity_layer() const
		{
			return d_selected_velocity_layer;
		}

		void
		set_selected_velocity_layer(
				boost::weak_ptr<GPlatesPresentation::VisualLayer> layer)
		{
			d_selected_velocity_layer = layer;
		}

		static const GPlatesGui::Colour DEFAULT_SCALE_TEXT_COLOUR;
		static const GPlatesGui::Colour DEFAULT_ARROW_COLOUR;
		static const GPlatesGui::Colour DEFAULT_BACKGROUND_COLOUR;
		static const Anchor DEFAULT_ANCHOR;
		static const int DEFAULT_X_OFFSET;
		static const int DEFAULT_Y_OFFSET;
		static const int DEFAULT_ARROW_LENGTH;
		static const int DEFAULT_ARROW_ANGLE;
		static const double DEFAULT_ARROW_SCALE;
		static const double DEFAULT_BACKGROUND_OPACITY;
		static const bool DEFAULT_IS_ENABLED;
		static const bool DEFAULT_BACKGROUND_ENABLED;

	private:


		QFont d_scale_text_font;
		GPlatesGui::Colour d_scale_text_colour;
		GPlatesGui::Colour d_arrow_colour;
		GPlatesGui::Colour d_background_colour;
		Anchor d_anchor;
		int d_x_offset;
		int d_y_offset;

		/**
		 * @brief d_arrow_length - in pixels
		 */
		int d_arrow_length;

		/**
		 * @brief d_arrow_angle- angle of velocity arrow. Zero angle is horizontal, to the right, and
		 * angle is measured clockwise.
		 */
		int d_arrow_angle;

		/**
		 * @brief d_scale  Velocity scale (cm / yr) provided by user.
		 */
		double d_scale;

		double d_background_opacity;
		bool d_is_enabled;
		bool d_background_enabled;
		ArrowLengthType d_arrow_length_type;

		/**
		 * @brief d_selected_velocity_layer - the velocity layer selected in the UI's combo-box.
		 */
		boost::weak_ptr<GPlatesPresentation::VisualLayer> d_selected_velocity_layer;
	};

}

#endif  // GPLATES_GUI_VELOCITYLEGENDOVERLAYSETTINGS_H
