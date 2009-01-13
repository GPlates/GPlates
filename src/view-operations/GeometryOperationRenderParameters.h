/* $Id$ */

/**
 * \file 
 * Parameters that specify how geometry operations should draw geometry.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#ifndef GPLATES_VIEWOPERATIONS_GEOMETRYOPERATIONRENDERPARAMETERS_H
#define GPLATES_VIEWOPERATIONS_GEOMETRYOPERATIONRENDERPARAMETERS_H

#include "gui/Colour.h"


namespace GPlatesViewOperations
{
	/**
	 * Parameters that specify how geometry operations should draw geometry.
	 */
	class GeometryOperationRenderParameters
	{
	public:
		GeometryOperationRenderParameters(
				float line_width_hint = 1.5f,
				float regular_point_size_hint = 2.0f,
				float large_point_size_hint = 4.0f,
				const GPlatesGui::Colour &focus_colour = GPlatesGui::Colour::WHITE,
				const GPlatesGui::Colour &not_in_focus_colour = GPlatesGui::Colour::GREY);

		/**
		 * Width of lines to render.
		 */
		float
		get_line_width_hint() const
		{
			return d_line_width_hint;
		}

		/**
		 * Regular size of point to render at each point/vertex.
		 * Used when it is not desired to have the point/vertex stick out.
		 */
		float
		get_regular_point_size_hint() const
		{
			return d_regular_point_size_hint;
		}

		/**
		 * Large size of point to render at each point/vertex.
		 * Used to make point/vertex more visible or to emphasise it.
		 */
		float
		get_large_point_size_hint() const
		{
			return d_large_point_size_hint;
		}

		/**
		 * Colour to use for rendering those parts of geometry that are in focus.
		 */
		const GPlatesGui::Colour &
		get_focus_colour() const
		{
			return d_focus_colour;
		}

		/**
		 * Colour to use for rendering those parts of geometry that are not in focus.
		 */
		const GPlatesGui::Colour &
		get_not_in_focus_colour() const
		{
			return d_not_in_focus_colour;
		}

	private:
		float d_line_width_hint;
		float d_regular_point_size_hint;
		float d_large_point_size_hint;
		GPlatesGui::Colour d_focus_colour;
		GPlatesGui::Colour d_not_in_focus_colour;
	};
}

#endif // GPLATES_VIEWOPERATIONS_GEOMETRYOPERATIONRENDERPARAMETERS_H
