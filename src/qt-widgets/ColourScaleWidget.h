/* $Id$ */

/**
 * \file
 * Contains the definition of the class ElidedLabel.
 *
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
 
#ifndef GPLATES_QTWIDGETS_COLOURSCALEWIDGET_H
#define GPLATES_QTWIDGETS_COLOURSCALEWIDGET_H

#include <vector>
#include <utility>
#include <QWidget>
#include <QPixmap>
#include <QString>

#include "gui/RasterColourPalette.h"


namespace GPlatesQtWidgets
{
	/**
	 * ColourScaleWidget displays an annotated colour scale on screen.
	 */
	class ColourScaleWidget :
			public QWidget
	{
	public:

		typedef std::pair<int, QString> annotation_type;
		typedef std::vector<annotation_type> annotations_seq_type;

		/**
		 * Distance from left border of widget to the colour scale.
		 */
		static const int LEFT_MARGIN = 6;

		/**
		 * Width of the colour scale.
		 */
		static const int COLOUR_SCALE_WIDTH = 32;

		/**
		 * Distance from colour scale to annotation text.
		 */
		static const int INTERNAL_SPACING = 5;

		/**
		 * Grid size of transparent checkerboard pattern.
		 */
		static const int CHECKERBOARD_GRID_SIZE = 8;

		/**
		 * Minimum spacing in pixels between each line of annotation.
		 */
		static const int ANNOTATION_LINE_SPACING = 5;

		/**
		 * Length of tick marks that accompany annotations.
		 */
		static const int TICK_LENGTH = 2;

		explicit
		ColourScaleWidget(
				QWidget *parent_ = NULL);

		/**
		 * Causes this widget to render scales for the given @a colour_palette.
		 * Returns whether this widget is able to render scales for the given @a colour_palette.
		 */
		bool
		populate(
				const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &colour_palette);

	protected:

		virtual
		void
		paintEvent(
				QPaintEvent *ev);

		virtual
		void
		resizeEvent(
				QResizeEvent *ev);

	private:

		/**
		 * Returns true if we were able to extract the right info out of @a d_curr_colour_palette.
		 */
		bool
		regenerate_contents();

		static const int MINIMUM_HEIGHT = 200;

		GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type d_curr_colour_palette;

		QPixmap d_colour_scale_pixmap;
		QPixmap d_disabled_colour_scale_pixmap;
		annotations_seq_type d_annotations;
	};
}

#endif  // GPLATES_QTWIDGETS_COLOURSCALEWIDGET_H
