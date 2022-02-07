/* $Id$ */

/**
 * @file 
 * Contains the definition of the SingleColourScheme class.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
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

namespace GPlatesModel
{
	class FeatureHandle;
}

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
					const GPlatesAppLogic::ReconstructionGeometry &reconstruction_geometry) const;

			boost::optional<Colour>
			get_colour(
					const GPlatesModel::FeatureHandle& feature_ptr) const
			{
				return d_colour;
			}

			boost::optional<Colour>
			get_colour() const;

		private:
			boost::optional<Colour> d_colour;

			static
			const Colour
			DEFAULT_COLOUR;
	};


	inline
	ColourScheme::non_null_ptr_type
	make_single_colour_scheme(
			const Colour &colour)
	{
		return new SingleColourScheme(colour);
	}
}

#endif  /* GPLATES_GUI_SINGLECOLOURSCHEME_H */
