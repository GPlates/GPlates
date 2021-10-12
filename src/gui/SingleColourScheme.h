/* $Id$ */

/**
 * @file 
 * Contains the definition of the SingleColourScheme class.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2009 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_SINGLECOLOURSCHEME_H
#define GPLATES_GUI_SINGLECOLOURSCHEME_H

#include "ColourScheme.h"

namespace GPlatesGui 
{
	/**
	 * This class assigns a fixed colour to reconstruction geometries.
	 */
	class SingleColourScheme:
			public ColourScheme
	{
		public:
			SingleColourScheme();

			SingleColourScheme(
					const Colour &colour);

			virtual
			boost::optional<Colour>
			get_colour(
					const GPlatesModel::ReconstructionGeometry &reconstruction_geometry) const;

		private:
			boost::optional<Colour> d_colour;

			static
			const Colour
			DEFAULT_COLOUR;
	};
}

#endif  /* GPLATES_GUI_SINGLECOLOURSCHEME_H */
