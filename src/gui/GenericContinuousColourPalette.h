/* $Id$ */

/**
 * @file 
 * Contains the definition of the GenericContinuousColourPalette class.
 *
 * Most recent change:
 *   $Date$
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

#ifndef GPLATES_GUI_GENERICCONTINUOUSCOLOURPALETTE_H
#define GPLATES_GUI_GENERICCONTINUOUSCOLOURPALETTE_H

#include "ColourPalette.h"
#include "maths/Real.h"
#include <map>

namespace GPlatesGui
{
	
	/**
	 * GenericContinuousColourPalette is a continuous colour palette that
	 * linearly interpolates between colours specified for certain control
	 * points.
	 *
	 * Colours can be specified for a series of adjacent ranges. Suppose the
	 * ranges are [p_0, p_1) , [p_1, p_2) , ... , [p_{n-1}, p_m]. (Note that the
	 * ranges are half-open except for the last range.) If the value lies in one
	 * of those ranges, the colour returned is linearly interpolated between the
	 * colour defined at the beginning of that range and the colour defined at the
	 * end of that range. The colour defined at the end of the range [p_0, p_1)
	 * can be different from the colour defined at the start of the range
	 * [p_1, p_2) to provide compatibility with CPT files. Values that lie before
	 * the first range are treated as if they are negative infinity, and values
	 * that lie after the last range are treated as if they are positive infinity.
	 */
	class GenericContinuousColourPalette :
		public ColourPalette<GPlatesMaths::Real>
	{
	public:

		/**
		 * Constructs an instance of GenericContinuousColourPalette
		 * @param control_points A mapping of control values to their assigned colours.
		 */
		explicit
		GenericContinuousColourPalette(
				std::map<GPlatesMaths::Real, Colour> control_points);

		/**
		 * Retrieves a Colour based on the @a value given.
		 *
		 * If the value is a control value, the colour associated with it will be
		 * returned. If the value is in between two control values, the colour
		 * returned will be linearly interpolated between the colours associated
		 * with those two points. If the value is before the first control value or
		 * after the last control value, the colour returned will be the colour
		 * associated with the first and last control value respectively.
		 */
		virtual
		boost::optional<Colour>
		get_colour(
			const GPlatesMaths::Real &value) const;

	private:
		std::map<GPlatesMaths::Real, Colour> d_control_points;

	};
}

#endif  /* GPLATES_GUI_GENERICCONTINUOUSCOLOURPALETTE_H */
