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
#include <boost/optional.hpp>
#include <QWidget>
#include <QPixmap>
#include <QString>
#include <QList>
#include <QAction>

#include "SaveFileDialog.h"

#include "gui/ColourScaleGenerator.h"
#include "gui/RasterColourPalette.h"


namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	// Forward declaration.
	class ViewportWindow;

	/**
	 * ColourScaleWidget displays an annotated colour scale on screen.
	 */
	class ColourScaleWidget :
			public QWidget
	{
	public:

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
		 * Minimum spacing in pixels between each line of annotation.
		 */
		static const int ANNOTATION_LINE_SPACING = 5;

		/**
		 * Length of tick marks that accompany annotations.
		 */
		static const int TICK_LENGTH = 2;


		explicit
		ColourScaleWidget(
				GPlatesPresentation::ViewState &view_state,
				ViewportWindow *viewport_window,
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
		contextMenuEvent(
				QContextMenuEvent *ev);

	private:

		/**
		 * Returns true if we were able to extract the right info out of @a d_curr_colour_palette.
		 */
		bool
		regenerate_contents();

		static const int MINIMUM_HEIGHT = 200;

		ViewportWindow *d_viewport_window;

		GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type d_curr_colour_palette;

		QPixmap d_colour_scale_pixmap;
		QPixmap d_disabled_colour_scale_pixmap;
		GPlatesGui::ColourScale::annotations_seq_type d_annotations;
		boost::optional<double> d_use_log_scale;
		QList<QAction *> d_right_click_actions;
		SaveFileDialog d_save_file_dialog;
	};
}

#endif  // GPLATES_QTWIDGETS_COLOURSCALEWIDGET_H
