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

		/**
		 * Default line width hint used by most (or all) layers.
		 */
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
	namespace FocusedFeatureParameters
	{
		/////////////
		// Colours //
		/////////////

		/**
		 * Colour to use for rendering the actual focus geometry clicked by user.
		 * Since there can be multiple geometry properties associated with a single feature
		 * only one of them (the clicked geometry) gets rendered in this colour.
		 */
		const GPlatesGui::Colour CLICKED_GEOMETRY_OF_FOCUSED_FEATURE_COLOUR =
				GPlatesGui::Colour::get_white();

		/**
		 * Colour to use for rendering the geometries of focused feature that the user
		 * did not click on.
		 * When the user clicks on a geometry it focuses the feature that the geometry
		 * belongs to. If there are other geometries associated with that feature then
		 * they will get rendered in this colour.
		 */
		const GPlatesGui::Colour NON_CLICKED_GEOMETRY_OF_FOCUSED_FEATURE_COLOUR =
				GPlatesGui::Colour::get_grey();
	}

	/**
	 * Parameters that specify how geometry operations should draw geometry.
	 */
	namespace GeometryOperationParameters
	{
		/////////////////
		// Line widths //
		/////////////////

		/**
		 * Width of lines to render in the most general case.
		 */
		const float LINE_WIDTH_HINT = 1.5f;

		/**
		 * Width of lines for rendering those parts of geometry that need highlighting
		 * to indicate, to the user, that an operation is possible.
		 */
		const float HIGHLIGHT_LINE_WIDTH_HINT = 2.0f;

		/////////////////
		// Point sizes //
		/////////////////

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

		/**
		 * Extra large size of point to render at each point/vertex.
		 * Used to make point/vertex even more visible or to emphasise it even more.
		 */
		const float EXTRA_LARGE_POINT_SIZE_HINT = 6.0f;

		/////////////
		// Colours //
		/////////////

		/**
		 * Colour to use for rendering those parts of geometry that are in focus.
		 */
		const GPlatesGui::Colour FOCUS_COLOUR = GPlatesGui::Colour::get_white();

		/**
		 * Colour to use for rendering those parts of geometry that are not in focus.
		 */
		const GPlatesGui::Colour NOT_IN_FOCUS_COLOUR = GPlatesGui::Colour::get_grey();

		/**
		 * Colour to use for rendering those parts of geometry that need highlighting
		 * to indicate, to the user, that an operation is possible.
		 */
		const GPlatesGui::Colour HIGHLIGHT_COLOUR = GPlatesGui::Colour::get_yellow();

		/**
		 * Colour to use for rendering those parts of geometry that can be deleted.
		 */
		const GPlatesGui::Colour DELETE_COLOUR = GPlatesGui::Colour::get_red();
	}
}

#endif // GPLATES_VIEWOPERATIONS_RENDEREDGEOMETRYPARAMETERS_H
