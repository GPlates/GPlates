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

#ifndef GPLATES_GUI_COLOURSCALEGENERATOR_H
#define GPLATES_GUI_COLOURSCALEGENERATOR_H

#include <utility>
#include <vector>
#include <boost/optional.hpp>

#include <QPixmap>
#include <QSize>
#include <QString>

#include "RasterColourPalette.h"


namespace GPlatesGui
{
	namespace ColourScale
	{
		typedef std::pair<int, QString> annotation_type;
		typedef std::vector<annotation_type> annotations_seq_type;

		/**
		 * Contains a *reference* to the caller's sequence of annotations to write to,
		 * and the annotation height to be used.
		 */
		struct Annotations
		{
			Annotations(
					annotations_seq_type &annotations_,
					int annotation_height_) :
				annotations(annotations_),
				annotation_height(annotation_height_)
			{  }

			annotations_seq_type &annotations;
			int annotation_height;
		};


		/**
		 * Generate a pixmap from a colour palette.
		 *
		 * The disabled pixmap is a checkerboard image.
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
		 *
		 * If annotations are desired then pass your empty annotation sequence to @a annotations,
		 * and also specify the annotation height in it.
		 */
		bool
		generate(
				const RasterColourPalette::non_null_ptr_to_const_type &colour_palette,
				QPixmap &colour_scale_pixmap,
				QPixmap &disabled_colour_scale_pixmap,
				int pixmap_width,
				int pixmap_height,
				boost::optional<double> use_log_scale = boost::none,
				boost::optional<Annotations> annotations = boost::none);
	}
}

#endif // GPLATES_GUI_COLOURSCALEGENERATOR_H
