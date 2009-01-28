/* $Id$ */

/**
 * \file 
 * Contains parameter values used when created @a RenderedGeometry objects.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#ifndef GPLATES_VIEWOPERATIONS_RENDEREDGEOMETRYPARAMETERS_H
#define GPLATES_VIEWOPERATIONS_RENDEREDGEOMETRYPARAMETERS_H

#include "gui/Colour.h"


namespace GPlatesViewOperations
{
	/**
	 * Parameters that specify how to draw geometry for the different rendered layers.
	 */
	namespace RenderedLayerParameters
	{
		/**
		 * Default point size hint used by most (or all) layers.
		 */
		const float DEFAULT_POINT_SIZE_HINT = 4.0f;
		const float DEFUALT_LINE_WIDTH_HINT = 1.5f;

		//! Point size for reconstruction layer.
		const float RECONSTRUCTION_POINT_SIZE_HINT = DEFAULT_POINT_SIZE_HINT;

		//! Line width for reconstruction layer.
		const float RECONSTRUCTION_LINE_WIDTH_HINT = 1.5f;

		//! Point size for reconstruction layer.
		const float DIGITISATION_POINT_SIZE_HINT = DEFAULT_POINT_SIZE_HINT;

		//! Line width for reconstruction layer.
		const float DIGITISATION_LINE_WIDTH_HINT = 2.0f;

		//! Point size for reconstruction layer.
		const float GEOMETRY_FOCUS_POINT_SIZE_HINT = DEFAULT_POINT_SIZE_HINT;

		//! Line width for reconstruction layer.
		const float GEOMETRY_FOCUS_LINE_WIDTH_HINT = 2.5f;

		//! Point size for reconstruction layer.
		const float POLE_MANIPULATION_POINT_SIZE_HINT = DEFAULT_POINT_SIZE_HINT;

		//! Line width for reconstruction layer.
		const float POLE_MANIPULATION_LINE_WIDTH_HINT = 1.5f;
	}

	/**
	 * Parameters that specify how geometry operations should draw geometry.
	 */
	namespace GeometryOperationParameters
	{
		/**
		 * Width of lines to render.
		 */
		const float LINE_WIDTH_HINT = 1.5f;

		/**
		 * Regular size of point to render at each point/vertex.
		 * Used when it is not desired to have the point/vertex stick out.
		 */
		const float REGULAR_POINT_SIZE_HINT = 2.0f;

		/**
		 * Large size of point to render at each point/vertex.
		 * Used to make point/vertex more visible or to emphasise it.
		 */
		const float LARGE_POINT_SIZE_HINT = 4.0f;
		const float EXTRA_LARGE_POINT_SIZE_HINT = 7.0f;

		/**
		 * Colour to use for rendering those parts of geometry that are in focus.
		 *
		 * NOTE: We cannot use:
		 *     const GPlatesGui::Colour FOCUS_COLOUR = GPlatesGui::Colour::WHITE;
		 * because both 'FOCUS_COLOUR' and 'WHITE' are global variables and we
		 * cannot be guaranteed that WHITE is constructed before FOCUS_COLOUR.
		 * Solution is to make FOCUS_COLOUR a reference and use 'static' to avoid
		 * multiple definitions linker error.
		 */
		static const GPlatesGui::Colour &FOCUS_COLOUR = GPlatesGui::Colour::WHITE;

		/**
		 * Colour to use for rendering those parts of geometry that are not in focus.
		 *
		 * NOTE: We cannot use:
		 *     const GPlatesGui::Colour NOT_IN_FOCUS_COLOUR = GPlatesGui::Colour::GREY;
		 * because both 'NOT_IN_FOCUS_COLOUR ' and 'GREY' are global variables and we
		 * cannot be guaranteed that GREY is constructed before NOT_IN_FOCUS_COLOUR .
		 * Solution is to make NOT_IN_FOCUS_COLOUR a reference and use 'static' to avoid
		 * multiple definitions linker error.
		 */
		static const GPlatesGui::Colour &NOT_IN_FOCUS_COLOUR = GPlatesGui::Colour::GREY;
	}
}

#endif // GPLATES_VIEWOPERATIONS_RENDEREDGEOMETRYPARAMETERS_H
