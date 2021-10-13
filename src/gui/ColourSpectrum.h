/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2009, 2010 The University of Sydney, Australia
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
#ifndef GPLATES_GUI_COLOURSPECTRUM_H
#define GPLATES_GUI_COLOURSPECTRUM_H
#include <boost/optional.hpp>
#include <vector>

#include "Colour.h"

namespace GPlatesGui 
{
	class Colour;

	class ColourSpectrum
	{
	public:
		explicit
		ColourSpectrum(
				const Colour& upper_colour = Colour::get_white(),
				const Colour& lower_colour = Colour::get_black(),
				const double upper_bound = 1.0,
				const double lower_bound = 0.0); 
		/**
		 * Retrieves the colour along the colour spectrum at the given @a position.
		 * The entire spectrum is covered in the range of @a position values from
		 * 0.0 to 1.0.
		 *
		 * If @a position lies outside of [0.0, 1.0], it is clamped to the nearest
		 * value in that range.
		 */
		boost::optional<GPlatesGui::Colour>
		get_colour_at(double position) const;

	protected:
		Colour d_upper_colour;
		Colour d_lower_colour;
		double d_upper_bound;
		double d_lower_bound;
	};
}

#endif  /* GPLATES_GUI_COLOURSPECTRUM_H */
