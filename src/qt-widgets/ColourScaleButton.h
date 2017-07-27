/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2016 The University of Sydney, Australia
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

#ifndef GPLATES_QT_WIDGETS_COLOURSCALEBUTTON_H
#define GPLATES_QT_WIDGETS_COLOURSCALEBUTTON_H

#include <boost/optional.hpp>
#include <QPixmap>
#include <QToolButton>
#include <QString>

#include "gui/ColourScaleGenerator.h"
#include "gui/RasterColourPalette.h"


namespace GPlatesQtWidgets
{
	/**
	 * ColourScaleButton displays a colour scale image (without annotations) in a QToolButton.
	 *
	 * A QToolButton is used since it seems to respect our size hints (which QPushButton doesn't) and
	 * it's more suitable for images (versus text for QPushButton).
	 */
	class ColourScaleButton :
			public QToolButton
	{
		Q_OBJECT
		
	public:

		explicit
		ColourScaleButton(
				QWidget *parent_ = NULL);

		/**
		 * Causes this widget to render scales for the given @a colour_palette.
		 * Returns whether this widget is able to render scales for the given @a colour_palette.
		 *
		 * Specify @a use_log_scale to distribute the display of the colour scale uniformly in log space.
		 * The 'double' value is only used if the min/max range of colour scale includes zero
		 * (ie 'max_value >= 0' and 'min_value <= 0') in which case the value should be positive and
		 * non-zero (ie, '> 0.0'), otherwise it can be set to any dummy value (like 0.0).
		 * This is because, in log space, zero cannot be reached but we can get near to zero.
		 * The positive range is at least from 'log(max_value)' to 'log(max_value) - use_log_scale_value'.
		 * The negative range is at least from 'log(-min_value)' to 'log(-min_value) - use_log_scale_value'.
		 * If 'abs(max_value)' is larger than 'abs(min_value)' then the positive range will be larger
		 * to compensate (and vice versa for negative range).
		 */
		bool
		populate(
				const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &colour_palette,
				boost::optional<double> use_log_scale = boost::none);

	protected:

		virtual
		void
		paintEvent(
				QPaintEvent *ev);

		virtual
		void
		resizeEvent(
				QResizeEvent *ev);

		virtual
		void
		enterEvent(
				QEvent *ev);

		virtual
		void
		leaveEvent(
				QEvent *ev);

		virtual
		QSize
		sizeHint() const;

		virtual
		QSize
		minimumSizeHint() const;

	private Q_SLOTS:

		void
		handle_pressed();

		void
		handle_released();

	private:

		/**
		 * Returns true if we were able to extract the right info out of @a d_curr_colour_palette.
		 */
		bool
		regenerate_contents();


		// Pixmap size.
		static const int MINIMUM_PIXMAP_WIDTH = 15;
		static const int MINIMUM_PIXMAP_HEIGHT = 40;

		// Button size (including border size of 1).
		static const int MINIMUM_WIDTH = MINIMUM_PIXMAP_WIDTH + 2;
		static const int MINIMUM_HEIGHT = MINIMUM_PIXMAP_HEIGHT + 2;

		GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type d_curr_colour_palette;
		boost::optional<double> d_use_log_scale;

		QPixmap d_colour_scale_pixmap;
		QPixmap d_disabled_colour_scale_pixmap;

		bool d_mouse_inside_button;
		bool d_mouse_pressed;
	};
}

#endif // GPLATES_QT_WIDGETS_COLOURSCALEBUTTON_H
