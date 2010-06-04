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


namespace GPlatesGui 
{
	class Colour;

	namespace ColourSpectrum
	{
		/**
		 * Retrieves the colour along the colour spectrum at the given @a position.
		 * The entire spectrum is covered in the range of @a position values from
		 * 0.0 to 1.0.
		 *
		 * If @a position lies outside of [0.0, 1.0], it is clamped to the nearest
		 * value in that range.
		 */
		GPlatesGui::Colour
		get_colour_at(
				double position);
	}
}

#endif  /* GPLATES_GUI_COLOURSPECTRUM_H */
