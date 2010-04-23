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

#include <vector>
#include <boost/optional.hpp>

#include "Colour.h"
#include "ColourPalette.h"

#include "maths/Real.h"


namespace GPlatesGui
{
	/**
	 * A colour slice specifies a gradient of colour between two real values.
	 */
	class ColourSlice
	{
	public:

		ColourSlice(
				GPlatesMaths::Real lower_value_,
				GPlatesMaths::Real upper_value_,
				boost::optional<Colour> lower_colour_,
				boost::optional<Colour> upper_colour_);

		bool
		can_handle(
				GPlatesMaths::Real value) const;

		boost::optional<Colour>
		get_colour(
				GPlatesMaths::Real value) const;

		GPlatesMaths::Real
		lower_value() const
		{
			return d_lower_value;
		}

		GPlatesMaths::Real
		upper_value() const
		{
			return d_upper_value;
		}

		boost::optional<Colour>
		lower_colour() const
		{
			return d_lower_colour;
		}

		boost::optional<Colour>
		upper_colour() const
		{
			return d_upper_colour;
		}

	private:

		GPlatesMaths::Real d_lower_value, d_upper_value;
		boost::optional<Colour> d_lower_colour, d_upper_colour;
		
	};
	
	/**
	 * GenericContinuousColourPalette is a continuous colour palette that
	 * linearly interpolates between colours specified for certain control
	 * points.
	 *
	 * The primary design consideration for this class is that it should act as a
	 * data structure for the in-memory representation of a "regular" CPT file.
	 *
	 * A description of a "regular" CPT file can be found at
	 * http://gmt.soest.hawaii.edu/gmt/doc/gmt/html/GMT_Docs/node69.html 
	 */
	class GenericContinuousColourPalette :
			public ColourPalette<GPlatesMaths::Real>
	{
	public:

		/**
		 * Add a colour slice. Colour slices must be added in order of increasing
		 * value other behaviour is undefined.
		 */
		void
		add_colour_slice(
				const ColourSlice &colour_slice)
		{
			d_colour_slices.push_back(colour_slice);
		}

		void
		set_background_colour(
				const Colour &colour)
		{
			d_background_colour = colour;
		}

		void
		set_foreground_colour(
				const Colour &colour)
		{
			d_foreground_colour = colour;
		}

		void
		set_nan_colour(
				const Colour &colour)
		{
			d_nan_colour = colour;
		}

		/**
		 * Retrieves a Colour based on the @a value given.
		 */
		virtual
		boost::optional<Colour>
		get_colour(
			const GPlatesMaths::Real &value) const;

	private:

		std::vector<ColourSlice> d_colour_slices;

		boost::optional<Colour> d_background_colour;

		boost::optional<Colour> d_foreground_colour;

		boost::optional<Colour> d_nan_colour;

	};
}

#endif  /* GPLATES_GUI_GENERICCONTINUOUSCOLOURPALETTE_H */
